/* VkCommandManager.cpp - Command manager implementation.
*/

#include "VkCommandManager.hpp"


VkCommandManager::VkCommandManager(VulkanContext& context):
	vkContext(context) {

	garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
};

VkCommandManager::~VkCommandManager() {};


void VkCommandManager::init() {
	QueueFamilyIndices familyIndices = vkContext.queueFamilies;
	graphicsCmdPool = createCommandPool(vkContext, vkContext.logicalDevice, familyIndices.graphicsFamily.index.value());
	transferCmdPool = createCommandPool(vkContext, vkContext.logicalDevice, familyIndices.transferFamily.index.value());

	allocCommandBuffers(graphicsCmdPool, graphicsCmdBuffers);
	vkContext.CommandObjects.graphicsCmdBuffers = graphicsCmdBuffers;

	allocCommandBuffers(transferCmdPool, transferCmdBuffers);
	vkContext.CommandObjects.transferCmdBuffers = transferCmdBuffers;
}


void VkCommandManager::recordRenderingCommandBuffer(VkCommandBuffer& cmdBuffer, ComponentArray<RenderableComponent>& renderables, uint32_t imageIndex, uint32_t currentFrame) {
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
			...CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
			..SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
		*/
	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


	// Commands are now ready to record. Functions that record commands begin with the vkCmd prefix.


	// Binds graphics pipeline
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

	// Processes renderables
	const uint32_t MAX_SUBPASSES = vkContext.GraphicsPipeline.subpassCount;
	uint32_t subpassCount = 1;
	for (auto renderable : renderables.getAllComponents()) {
		RenderSystem::processRenderable(vkContext, cmdBuffer, renderable);

		if (subpassCount++ < MAX_SUBPASSES)
			vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
		
	}

	// End the render pass
	vkCmdEndRenderPass(cmdBuffer);

	// Stop recording the command buffer
	VkResult endCmdBufferResult = vkEndCommandBuffer(cmdBuffer);
	if (endCmdBufferResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to record command buffer!");
	}
}


VkCommandBuffer VkCommandManager::beginSingleUseCommandBuffer(VulkanContext& vkContext, SingleUseCommandBufferInfo* commandBufInfo) {
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
	cmdBufBeginInfo.pInheritanceInfo = commandBufInfo->pInheritanceInfo;

	VkResult bufBeginResult = vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo);
	if (bufBeginResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to start recording single-use command buffer!");
	}

	return cmdBuffer;
}


void VkCommandManager::endSingleUseCommandBuffer(VulkanContext& vkContext, SingleUseCommandBufferInfo* commandBufInfo, VkCommandBuffer& cmdBuffer) {
	VkResult bufEndResult = vkEndCommandBuffer(cmdBuffer);
	if (bufEndResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to stop recording single-use command buffer!");
	}


	if (!commandBufInfo->autoSubmit)
		return;


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
		if (commandBufInfo->usingSingleUseFence) {
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
	CommandPoolCreateInfo createInfo{};
	createInfo.logicalDevice = device;
	createInfo.queueFamilyIndex = queueFamilyIndex;
	createInfo.flags = flags;

	if (cmdPoolMappings.find(createInfo) != cmdPoolMappings.end()) {
		Log::print(Log::T_WARNING, __FUNCTION__, "The command pool to be created has creation parameters matching those of another pool. The existing command pool will be used.");
		return cmdPoolMappings[createInfo];
	}


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

	cmdPoolMappings[createInfo] = commandPool;

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