/* VkCommandManager.cpp - Command manager implementation.
*/

#include "VkCommandManager.hpp"


VkCommandManager::VkCommandManager(VulkanContext& context):
	vkContext(context) {

	garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);
	bufferManager = ServiceLocator::getService<BufferManager>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
};

VkCommandManager::~VkCommandManager() {};


void VkCommandManager::init() {
	QueueFamilyIndices familyIndices = VkDeviceManager::getQueueFamilies(vkContext.physicalDevice, vkContext.vkSurface);
	graphicsCmdPool = createCommandPool(vkContext, vkContext.logicalDevice, familyIndices.graphicsFamily.index.value());
	transferCmdPool = createCommandPool(vkContext, vkContext.logicalDevice, familyIndices.transferFamily.index.value());

	allocCommandBuffers(graphicsCmdPool, graphicsCmdBuffers);
	vkContext.CommandObjects.graphicsCmdBuffers = graphicsCmdBuffers;

	allocCommandBuffers(transferCmdPool, transferCmdBuffers);
	vkContext.CommandObjects.transferCmdBuffers = transferCmdBuffers;
}


void VkCommandManager::recordCommandBuffer(VkCommandBuffer& cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) {
	// Specifies details about how the passed-in command buffer will be used before beginning
	VkCommandBufferBeginInfo bufferBeginInfo{};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	/* Available flags: VK_COMMAND_BUFFER_USAGE_...
	* ...ONE_TIME_SUBMIT_BIT: The command buffer will be re-recorded right after executing it once
	* ...RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass
	* ...SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending execution
	*/
	bufferBeginInfo.flags = 0;

	// pInheritanceInfo is only relevant for secondary command buffers
	// It specifies which state the secondary buffer shold inherit from the primary buffer that is calling it
	bufferBeginInfo.pInheritanceInfo = nullptr;

	// NOTE: vkBeginCommandBuffer will implicitly reset the framebuffer if it has already been recorded before
	VkResult beginCmdBufferResult = vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
	if (beginCmdBufferResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to start recording command buffer!");
	}

	// Starts a render pass to start recording the drawing commands
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkContext.GraphicsPipeline.renderPass;
	renderPassBeginInfo.framebuffer = vkContext.SwapChain.imageFrameBuffers[imageIndex];

	// Defines the render area (from (0, 0) to (extent.width, extent.height))
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = vkContext.SwapChain.extent;

	// Defines the clear values to use (we must specify this since the color attachment's load operation is VK_ATTACHMENT_LOAD_OP_CLEAR)
	VkClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f }; // (0, 0, 0, 1) -> Black;
	clearColor.depthStencil = VkClearDepthStencilValue(); // Null for now (if depth stencil is implemented, you must also specify the color attachment load and store operations before specifying the clear value here)

	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	/* The final parameter controls how the drawing commands within the render pass will be provided. It can have 2 values: VK_SUBPASS_...
	* ...CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
	* ..SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
	*/
	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


	// Commands are now ready to record. Functions that record commands begin with the vkCmd prefix.

	// Binds graphics pipeline
		// The 2nd parameter specifies the pipeline type (in this case, it's the graphics pipeline)
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.GraphicsPipeline.pipeline);

	// Specify viewport and scissor states (since they're dynamic states)
		// Viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(vkContext.SwapChain.extent.width);
	viewport.height = static_cast<float>(vkContext.SwapChain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	// Scissor
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = vkContext.SwapChain.extent;
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// Bind vertex and index buffers, then draw
	/* vkCmdDraw parameters (Vulkan specs):
	* commandBuffer: The command buffer to record the draw commands into
	* vertexCount: The number of vertices to draw
	* instanceCount: The number of instances to draw (useful for instanced rendering)
	* firstVertex: The index of the first vertex to draw. It is used as an offset into the vertex buffer, and defines the lowest value of gl_VertexIndex.
	* firstInstance: The instance ID of the first instance to draw. It is used as an offset for instanced rendering, and defines the lowest value of gl_InstanceIndex.
	*/
	// Vertex buffers
	VkBuffer vertexBuffers[] = {
		bufferManager->getVertexBuffer()
	};
	VkDeviceSize offsets[] = {
		0
	};
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);


	// Index buffer (note: you can only have 1 index buffer)
	VkBuffer indexBuffer = bufferManager->getIndexBuffer();
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	// Draw call
		// Binds descriptor sets
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.GraphicsPipeline.layout, 0, 1, &vkContext.GraphicsPipeline.uniformBufferDescriptorSets[currentFrame], 0, nullptr);

	// Draws vertices based on the index buffer
	auto vertexIndices = bufferManager->getVertexIndexData();
	vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(vertexIndices.size()), 1, 0, 0, 0); // Use vkCmdDrawIndexed instead of vkCmdDraw to draw with the index buffer


	// Transition to the ImGui subpass to record ImGui draw commands
	vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);


	// End the render pass
	vkCmdEndRenderPass(cmdBuffer);

	// Stop recording the command buffer
	VkResult endCmdBufferResult = vkEndCommandBuffer(cmdBuffer);
	if (endCmdBufferResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to record command buffer!");
	}
}


VkCommandBuffer VkCommandManager::beginSingleUseCommandBuffer(VulkanContext& vkContext, SingleUseCommandInfo* commandBufInfo) {
	VkCommandBufferAllocateInfo cmdBufAllocInfo{};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.level = commandBufInfo->bufferLevel;
	cmdBufAllocInfo.commandPool = commandBufInfo->commandPool;
	cmdBufAllocInfo.commandBufferCount = 1;
	
	VkCommandBuffer cmdBuffer;
	VkResult bufAllocResult = vkAllocateCommandBuffers(vkContext.logicalDevice, &cmdBufAllocInfo, &cmdBuffer);
	if (bufAllocResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to allocate single-use command buffer!");
	}

	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = commandBufInfo->bufferUsageFlags;

	VkResult bufBeginResult = vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo);
	if (bufBeginResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to start recording single-use command buffer!");
	}

	return cmdBuffer;
}


void VkCommandManager::endSingleUseCommandBuffer(VulkanContext& vkContext, SingleUseCommandInfo* commandBufInfo, VkCommandBuffer& cmdBuffer) {
	VkResult bufEndResult = vkEndCommandBuffer(cmdBuffer);
	if (bufEndResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to stop recording single-use command buffer!");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	if (commandBufInfo->waitSemaphores.size() > 0) {
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(commandBufInfo->waitSemaphores.size());
		submitInfo.pWaitSemaphores = commandBufInfo->waitSemaphores.data();
		submitInfo.pWaitDstStageMask = &commandBufInfo->waitStageMask;
	}

	if (commandBufInfo->signalSemaphores.size() > 0) {
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(commandBufInfo->signalSemaphores.size());
		submitInfo.pSignalSemaphores = commandBufInfo->signalSemaphores.data();
	}

	VkResult submitResult = vkQueueSubmit(commandBufInfo->queue, 1, &submitInfo, commandBufInfo->fence);
	if (submitResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to submit recorded data from single-use command buffer!");
	}
	
	if (commandBufInfo->fence != VK_NULL_HANDLE) {
		if (commandBufInfo->isSingleUseFence) {
			VkSyncManager::waitForSingleUseFence(vkContext, commandBufInfo->fence);
		}
		else {
			vkWaitForFences(vkContext.logicalDevice, 1, &commandBufInfo->fence, VK_TRUE, UINT64_MAX);
			vkResetFences(vkContext.logicalDevice, 1, &commandBufInfo->fence);
		}
	}
	else
		vkDeviceWaitIdle(vkContext.logicalDevice);

	if (commandBufInfo->freeAfterSubmit)
		vkFreeCommandBuffers(vkContext.logicalDevice, commandBufInfo->commandPool, 1, &cmdBuffer);
}


VkCommandPool VkCommandManager::createCommandPool(VulkanContext& vkContext, VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {

	std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = flags; // Allows command buffers to be re-recorded individually

	// Command buffers are executed by submitting them on a device queue.
	// Each command pool can only allocate command buffers that are submitted on a single type of queue.
	poolCreateInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool commandPool;
	VkResult result = vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create command pool!");
	}

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(commandPool) };
	task.vkObjects = { vkContext.logicalDevice, commandPool };
	task.cleanupFunc = [vkContext, commandPool]() { vkDestroyCommandPool(vkContext.logicalDevice, commandPool, nullptr); };

	garbageCollector->createCleanupTask(task);

	return commandPool;
}


void VkCommandManager::allocCommandBuffers(VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers) {
	commandBuffers.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo bufferAllocInfo{};
	bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	bufferAllocInfo.commandPool = commandPool;

	// Specifies whether the command buffer is primary or secondary
	// BUFFER_LEVEL_PRIMARY: The buffer can be submitted to a queue for execution, but cannot be directly called from other command buffers.
	// BUFFER_LEVEL_SECONDARY: The buffer cannot be submitted directly, but can be called from primary command buffers.
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	bufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	VkResult result = vkAllocateCommandBuffers(vkContext.logicalDevice, &bufferAllocInfo, commandBuffers.data());
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to allocate command buffers!");
	}

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(commandBuffers) };
	task.vkObjects = { vkContext.logicalDevice, commandPool };
	task.cleanupFunc = [this, commandPool, commandBuffers]() { vkFreeCommandBuffers(vkContext.logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data()); };

	garbageCollector->createCleanupTask(task);
}