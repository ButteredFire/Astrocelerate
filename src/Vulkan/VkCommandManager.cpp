/* VkCommandManager.cpp - Command manager implementation.
*/

#include "VkCommandManager.hpp"


VkCommandManager::VkCommandManager() {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
};

VkCommandManager::~VkCommandManager() {};


void VkCommandManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::UpdateSessionStatus>(
		[this](const Event::UpdateSessionStatus &event) {
			using namespace Event;

			switch (event.sessionStatus) {
			case UpdateSessionStatus::Status::PREPARE_FOR_RESET:
				m_sceneReady = false;
				break;

			case UpdateSessionStatus::Status::INITIALIZED:
				vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);
				m_sceneReady = true;
				break;
			}
		}
	);


	m_eventDispatcher->subscribe<Event::RequestProcessSecondaryCommandBuffers>(
		[this](const Event::RequestProcessSecondaryCommandBuffers &event) {
			m_secondaryCmdBufCount = event.bufferCount;
			m_pCommandBuffers = event.pBuffers;
		}
	);
}


void VkCommandManager::init() {
	QueueFamilyIndices familyIndices = g_vkContext.Device.queueFamilies;

	m_graphicsCmdPool = createCommandPool(g_vkContext.Device.logicalDevice, familyIndices.graphicsFamily.index.value());
	allocCommandBuffers(m_graphicsCmdPool, m_graphicsCmdBuffers);
	g_vkContext.CommandObjects.graphicsCmdBuffers = m_graphicsCmdBuffers;

	
	if (familyIndices.familyExists(familyIndices.transferFamily)) {
		m_transferCmdPool = createCommandPool(g_vkContext.Device.logicalDevice, familyIndices.transferFamily.index.value());
		allocCommandBuffers(m_transferCmdPool, m_transferCmdBuffers);
		g_vkContext.CommandObjects.transferCmdBuffers = m_transferCmdBuffers;
	}
}


void VkCommandManager::recordRenderingCommandBuffer(VkCommandBuffer& cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) {
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
	LOG_ASSERT(beginCmdBufferResult == VK_SUCCESS, "Failed to start recording command buffer!");


	VkClearValue clearValue{};  // NOTE: we must specify this since the color attachment's load operation is VK_ATTACHMENT_LOAD_OP_CLEAR
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f }; // (0, 0, 0, 1) -> Black
	clearValue.depthStencil = VkClearDepthStencilValue(); // Null for now (if depth stencil is implemented, you must also specify the color attachment load and store operations before specifying the clear value here)

	VkClearValue depthStencilClearValue{};
	depthStencilClearValue.depthStencil.depth = 0.0f;
	depthStencilClearValue.depthStencil.stencil = 0;

	VkClearValue clearValues[] = {
		clearValue,
		depthStencilClearValue
	};



	if (m_sceneReady) {
		// Record all secondary command buffers
		if (m_pCommandBuffers) {
			vkCmdExecuteCommands(cmdBuffer, m_secondaryCmdBufCount, m_pCommandBuffers);
			m_pCommandBuffers = nullptr;
		}

		// OFFSCREEN RENDER PASS
		VkRenderPassBeginInfo offscreenRenderPassBeginInfo{};
		offscreenRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		offscreenRenderPassBeginInfo.renderPass = g_vkContext.OffscreenPipeline.renderPass;
		offscreenRenderPassBeginInfo.framebuffer = g_vkContext.OffscreenResources.framebuffers[currentFrame];
		offscreenRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		offscreenRenderPassBeginInfo.renderArea.extent = g_vkContext.SwapChain.extent;
		offscreenRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(SIZE_OF(clearValues));
		offscreenRenderPassBeginInfo.pClearValues = clearValues;

		/* The final parameter controls how the drawing commands within the render pass will be provided. It can have 2 values: VK_SUBPASS_...
			...CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
			..SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
		*/
		vkCmdBeginRenderPass(cmdBuffer, &offscreenRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.pipeline);

		// Specify viewport and scissor states (since they're dynamic states)
			// Viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(g_vkContext.SwapChain.extent.width);
		viewport.height = static_cast<float>(g_vkContext.SwapChain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		// Scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = g_vkContext.SwapChain.extent;
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);


		// Processes renderables
		m_eventDispatcher->dispatch(Event::UpdateRenderables{
			.commandBuffer = cmdBuffer,
			.descriptorSet = g_vkContext.OffscreenPipeline.perFrameDescriptorSets[currentFrame]
			}, true);


		vkCmdEndRenderPass(cmdBuffer);
	}



	// Transitions swapchain image to the layout COLOR_ATTACHMENT_OPTIMAL before the presentation render pass
	// This is because the image layout is UNDEFINED for the first swapchain image, and PRESENT_SRC_KHR for subsequent ones.
	VkImageMemoryBarrier swapchainImgToPresentBarrier{};
	swapchainImgToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapchainImgToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//swapchainImgToPresentBarrier.oldLayout = g_vkContext.SwapChain.imageLayouts[imageIndex];
	//swapchainImgToPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	swapchainImgToPresentBarrier.image = g_vkContext.SwapChain.images[imageIndex];
	swapchainImgToPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	swapchainImgToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapchainImgToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	VkAccessFlags srcAccessMask;
	VkPipelineStageFlags srcStage;


	if (m_sceneReady) {
		// If the offscreen pass ran, the image is now VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		swapchainImgToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Access from the completed offscreen pass
		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		// If the offscreen pass was skipped, the image is still in its acquired state
		swapchainImgToPresentBarrier.oldLayout = g_vkContext.SwapChain.imageLayouts[imageIndex];
		if (swapchainImgToPresentBarrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
			srcAccessMask = 0;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else { // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // No prior writes this frame, so we will wait at bottom.
		}
	}
	swapchainImgToPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // For the presentation render pass (read and write for GUI)


	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStage,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &swapchainImgToPresentBarrier
	);


	
	// PRESENTATION RENDER PASS
	VkRenderPassBeginInfo presentRenderPassBeginInfo{};
	presentRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	presentRenderPassBeginInfo.renderPass = g_vkContext.PresentPipeline.renderPass;
	presentRenderPassBeginInfo.framebuffer = g_vkContext.SwapChain.imageFrameBuffers[imageIndex];
	presentRenderPassBeginInfo.renderArea.offset = { 0, 0 };
	presentRenderPassBeginInfo.renderArea.extent = g_vkContext.SwapChain.extent;
	presentRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(SIZE_OF(clearValues));
	presentRenderPassBeginInfo.pClearValues = clearValues;


	vkCmdBeginRenderPass(cmdBuffer, &presentRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	m_eventDispatcher->dispatch(Event::UpdateGUI {
		.commandBuffer = cmdBuffer,
		.currentFrame = currentFrame
	}, true);

	vkCmdEndRenderPass(cmdBuffer);


	// Stop recording the command buffer
	VkResult endCmdBufferResult = vkEndCommandBuffer(cmdBuffer);
	if (endCmdBufferResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to record command buffer!");
	}
}


VkCommandBuffer VkCommandManager::BeginSingleUseCommandBuffer(SingleUseCommandBufferInfo* commandBufInfo) {
	VkCommandBufferAllocateInfo cmdBufAllocInfo{};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.level = commandBufInfo->bufferLevel;
	cmdBufAllocInfo.commandPool = commandBufInfo->commandPool;
	cmdBufAllocInfo.commandBufferCount = 1;
	
	VkCommandBuffer cmdBuffer;
	VkResult bufAllocResult = vkAllocateCommandBuffers(g_vkContext.Device.logicalDevice, &cmdBufAllocInfo, &cmdBuffer);
	if (bufAllocResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to allocate single-use command buffer!");
	}

	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = commandBufInfo->bufferUsageFlags;
	cmdBufBeginInfo.pInheritanceInfo = commandBufInfo->pInheritanceInfo;

	VkResult bufBeginResult = vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo);
	if (bufBeginResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to start recording single-use command buffer!");
	}

	return cmdBuffer;
}


void VkCommandManager::EndSingleUseCommandBuffer(SingleUseCommandBufferInfo* commandBufInfo, VkCommandBuffer& cmdBuffer) {
	VkResult bufEndResult = vkEndCommandBuffer(cmdBuffer);
	if (bufEndResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to stop recording single-use command buffer!");
	}


	if (!commandBufInfo->autoSubmit) {
		std::ostringstream stream;
		stream << static_cast<const void *>(&cmdBuffer);
		
		if (commandBufInfo->usingSingleUseFence && IN_DEBUG_MODE)
			// If fences are accidentally used
			Log::Print(Log::T_WARNING, __FUNCTION__, "Command buffer " + stream.str() + " is not auto-submitted, but uses a single-use fence! Please, depending on your use case, either enable auto-submission or remove the fence.");

		return;
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
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to submit recorded data from single-use command buffer!");
	}
	
	if (commandBufInfo->fence != VK_NULL_HANDLE) {
		if (commandBufInfo->usingSingleUseFence) {
			VkSyncManager::WaitForSingleUseFence(commandBufInfo->fence);
		}
		else {
			vkWaitForFences(g_vkContext.Device.logicalDevice, 1, &commandBufInfo->fence, VK_TRUE, UINT64_MAX);
			vkResetFences(g_vkContext.Device.logicalDevice, 1, &commandBufInfo->fence);
		}
	}
	else
		vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);

	if (commandBufInfo->freeAfterSubmit)
		vkFreeCommandBuffers(g_vkContext.Device.logicalDevice, commandBufInfo->commandPool, 1, &cmdBuffer);
}


VkCommandPool VkCommandManager::createCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
	CommandPoolCreateInfo createInfo{};
	createInfo.logicalDevice = device;
	createInfo.queueFamilyIndex = queueFamilyIndex;
	createInfo.flags = flags;

	if (cmdPoolMappings.find(createInfo) != cmdPoolMappings.end()) {
		//Log::Print(Log::T_WARNING, __FUNCTION__, "The command pool to be created has creation parameters matching those of an existing pool, which will be used instead.");
		return cmdPoolMappings[createInfo];
	}


	std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = flags; // Allows command buffers to be re-recorded individually

	// Command buffers are executed by submitting them on a device queue.
	// Each command pool can only allocate command buffers that are submitted on a single type of queue.
	poolCreateInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool commandPool;
	VkResult result = vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create command pool!");
	}

	cmdPoolMappings[createInfo] = commandPool;

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_commandPool) };
	task.vkHandles = { g_vkContext.Device.logicalDevice, commandPool };
	task.cleanupFunc = [commandPool]() { vkDestroyCommandPool(g_vkContext.Device.logicalDevice, commandPool, nullptr); };

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

	VkResult result = vkAllocateCommandBuffers(g_vkContext.Device.logicalDevice, &bufferAllocInfo, commandBuffers.data());
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to allocate command buffers!");
	}

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_commandBuffers) };
	task.vkHandles = { g_vkContext.Device.logicalDevice, commandPool };
	task.cleanupFunc = [this, commandPool, commandBuffers]() { vkFreeCommandBuffers(g_vkContext.Device.logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data()); };

	m_garbageCollector->createCleanupTask(task);
}