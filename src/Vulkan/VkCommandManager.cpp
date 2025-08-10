/* VkCommandManager.cpp - Command manager implementation.
*/

#include "VkCommandManager.hpp"


VkCommandManager::VkCommandManager(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr) :
	m_coreResources(coreResources),
	m_swapchainManager(swapchainMgr),
	m_queueFamilies(coreResources->getQueueFamilyIndices()),
	m_logicalDevice(coreResources->getLogicalDevice()),
	m_swapchainExtent(swapchainMgr->getSwapChainExtent()),
	m_swapchainImages(swapchainMgr->getImages()),
	m_swapchainImgLayouts(swapchainMgr->getImageLayouts()) {
	
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

	bindEvents();

	init();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
};

VkCommandManager::~VkCommandManager() {};


void VkCommandManager::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<VkCommandManager>();

	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_RESET:
				vkDeviceWaitIdle(m_logicalDevice);
				m_sceneReady = false;
				break;

			case POST_INITIALIZATION:
				vkDeviceWaitIdle(m_logicalDevice);
				m_sceneReady = true;
				break;
			}
		}
	);


	m_eventDispatcher->subscribe<RequestEvent::ProcessSecondaryCommandBuffers>(selfIndex,
		[this](const RequestEvent::ProcessSecondaryCommandBuffers &event) {
			m_secondaryCmdBufs = event.buffers;
		}
	);


	m_eventDispatcher->subscribe<InitEvent::OffscreenPipeline>(selfIndex,
		[this](const InitEvent::OffscreenPipeline &event) {
			m_offscreenRenderPass = event.renderPass;
			m_offscreenPipeline = event.pipeline;

			m_offscreenImages = event.offscreenImages;
			m_offscreenFrameBuffers = event.offscreenFrameBuffers;
		}
	);


	m_eventDispatcher->subscribe<InitEvent::PresentPipeline>(selfIndex, 
		[this](const InitEvent::PresentPipeline &event) {
			m_presentPipelineRenderPass = event.renderPass;
		}
	);


	m_eventDispatcher->subscribe<InitEvent::SwapchainManager>(selfIndex,
		[this](const InitEvent::SwapchainManager &event) {
			m_swapchainFramebuffers = m_swapchainManager->getFramebuffers();
		}
	);


	m_eventDispatcher->subscribe<RecreationEvent::Swapchain>(selfIndex, 
		[this](const RecreationEvent::Swapchain &event) {
			m_swapchainImages = m_swapchainManager->getImages();
			m_swapchainImgLayouts = event.imageLayouts;
			m_swapchainFramebuffers = m_swapchainManager->getFramebuffers();
			m_swapchainExtent = m_swapchainManager->getSwapChainExtent();
		}
	);


	m_eventDispatcher->subscribe<RecreationEvent::OffscreenResources>(selfIndex,
		[this](const RecreationEvent::OffscreenResources &event) {
			m_offscreenFrameBuffers = event.framebuffers;
		}
	);
}


void VkCommandManager::init() {
	QueueFamilyIndices familyIndices = m_queueFamilies;

	m_graphicsCmdPool = CreateCommandPool(m_logicalDevice, familyIndices.graphicsFamily.index.value());
	allocCommandBuffers(m_graphicsCmdPool, m_graphicsCmdBuffers);

	
	if (familyIndices.familyExists(familyIndices.transferFamily)) {
		m_transferCmdPool = CreateCommandPool(m_logicalDevice, familyIndices.transferFamily.index.value());
		allocCommandBuffers(m_transferCmdPool, m_transferCmdBuffers);
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


	// NOTE: we must specify this since the color attachment's load operation is VK_ATTACHMENT_LOAD_OP_CLEAR
	static VkClearValue clearValue{
		.color = { 0.0f, 0.0f, 0.0f, 1.0f },			// (0, 0, 0, 1) -> Black
		//.depthStencil		// Null for now (if depth stencil is implemented, you must also specify the color attachment load and store operations before specifying the clear value here)
	};

	static VkClearValue depthStencilClearValue{
		.color = { 0.0f, 0.0f, 0.0f, 1.0f },
		//.depthStencil
	};

	static VkClearValue clearValues[] = {
		clearValue,
		depthStencilClearValue
	};



	// OFFSCREEN RENDER PASS
	static std::function<void(VkCommandBuffer&, uint32_t, uint32_t)> writeOffscreenCommands = [this](VkCommandBuffer &cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) {
		if (m_coreResources->getAppState() == Application::State::RECREATING_SWAPCHAIN)
			return;

		// Record all secondary command buffers
		if (!m_secondaryCmdBufs.empty()) {
			vkCmdExecuteCommands(cmdBuffer, static_cast<uint32_t>(m_secondaryCmdBufs.size()), m_secondaryCmdBufs.data());
			m_secondaryCmdBufs.clear();
		}

		
		VkRenderPassBeginInfo offscreenRenderPassBeginInfo{};
		offscreenRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		offscreenRenderPassBeginInfo.renderPass = m_offscreenRenderPass;
		offscreenRenderPassBeginInfo.framebuffer = m_offscreenFrameBuffers[currentFrame];
		offscreenRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		offscreenRenderPassBeginInfo.renderArea.extent = m_swapchainExtent;
		offscreenRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(SIZE_OF(clearValues));
		offscreenRenderPassBeginInfo.pClearValues = clearValues;

		/* The final parameter controls how the drawing commands within the render pass will be provided. It can have 2 values: VK_SUBPASS_...
			...CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
			..SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
		*/
		vkCmdBeginRenderPass(cmdBuffer, &offscreenRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipeline);

		// Specify viewport and scissor states (since they're dynamic states)
			// Viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapchainExtent.width);
		viewport.height = static_cast<float>(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		// Scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapchainExtent;
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);


		// Processes renderables
		m_eventDispatcher->dispatch(UpdateEvent::Renderables{
			.renderableType = UpdateEvent::Renderables::Type::MESHES,
			.commandBuffer = cmdBuffer,
			.currentFrame = currentFrame
			}, true);


		vkCmdEndRenderPass(cmdBuffer);
	};



	// PRESENTATION RENDER PASS
	static std::function<void(VkCommandBuffer&, uint32_t, uint32_t)> writePresentCommands = [this](VkCommandBuffer &cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) {
		if (m_coreResources->getAppState() == Application::State::RECREATING_SWAPCHAIN)
			return;

		// Transitions swapchain image to the layout COLOR_ATTACHMENT_OPTIMAL before the presentation render pass
		// This is because the image layout is UNDEFINED for the first swapchain image, and PRESENT_SRC_KHR for subsequent ones.
		VkImageMemoryBarrier swapchainImgToPresentBarrier{};
		swapchainImgToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		swapchainImgToPresentBarrier.srcAccessMask = 0;
		swapchainImgToPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		swapchainImgToPresentBarrier.oldLayout = m_swapchainImgLayouts[imageIndex];
		swapchainImgToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		swapchainImgToPresentBarrier.image = m_swapchainImages[imageIndex];
		swapchainImgToPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		swapchainImgToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchainImgToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		
		VkPipelineStageFlags srcStageMask{};

		if (swapchainImgToPresentBarrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
			swapchainImgToPresentBarrier.srcAccessMask = 0;
			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else if (swapchainImgToPresentBarrier.oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
			swapchainImgToPresentBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // No prior writes this frame, so we will wait at bottom.
		}


		vkCmdPipelineBarrier(
			cmdBuffer,
			srcStageMask,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &swapchainImgToPresentBarrier
		);


		VkRenderPassBeginInfo presentRenderPassBeginInfo{};
		presentRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		presentRenderPassBeginInfo.renderPass = m_presentPipelineRenderPass;
		presentRenderPassBeginInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
		presentRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		presentRenderPassBeginInfo.renderArea.extent = m_swapchainExtent;
		presentRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(SIZE_OF(clearValues));
		presentRenderPassBeginInfo.pClearValues = clearValues;


		vkCmdBeginRenderPass(cmdBuffer, &presentRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		m_eventDispatcher->dispatch(UpdateEvent::Renderables{
			.renderableType = UpdateEvent::Renderables::Type::GUI,
			.commandBuffer = cmdBuffer,
			.currentFrame = currentFrame
			}, true);

		vkCmdEndRenderPass(cmdBuffer);
	};


	if (m_sceneReady)
		writeOffscreenCommands(cmdBuffer, imageIndex, currentFrame);

	writePresentCommands(cmdBuffer, imageIndex, currentFrame);


	// Stop recording the command buffer
	VkResult endCmdBufferResult = vkEndCommandBuffer(cmdBuffer);
	if (endCmdBufferResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to record command buffer!");
	}
}


VkCommandBuffer VkCommandManager::BeginSingleUseCommandBuffer(VkDevice logicalDevice, SingleUseCommandBufferInfo* commandBufInfo) {
	VkCommandBufferAllocateInfo cmdBufAllocInfo{};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.level = commandBufInfo->bufferLevel;
	cmdBufAllocInfo.commandPool = commandBufInfo->commandPool;
	cmdBufAllocInfo.commandBufferCount = 1;
	
	VkCommandBuffer cmdBuffer;
	VkResult bufAllocResult = vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocInfo, &cmdBuffer);
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


void VkCommandManager::EndSingleUseCommandBuffer(VkDevice logicalDevice, SingleUseCommandBufferInfo* commandBufInfo, VkCommandBuffer& cmdBuffer) {
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
			VkSyncManager::WaitForSingleUseFence(logicalDevice, commandBufInfo->fence);
		}
		else {
			vkWaitForFences(logicalDevice, 1, &commandBufInfo->fence, VK_TRUE, UINT64_MAX);
			vkResetFences(logicalDevice, 1, &commandBufInfo->fence);
		}
	}
	else
		vkDeviceWaitIdle(logicalDevice);

	if (commandBufInfo->freeAfterSubmit)
		vkFreeCommandBuffers(logicalDevice, commandBufInfo->commandPool, 1, &cmdBuffer);
}


VkCommandPool VkCommandManager::CreateCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
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
	task.vkHandles = { commandPool };
	task.cleanupFunc = [device, commandPool]() { vkDestroyCommandPool(device, commandPool, nullptr); };

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

	VkResult result = vkAllocateCommandBuffers(m_logicalDevice, &bufferAllocInfo, commandBuffers.data());
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to allocate command buffers!");
	}

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_commandBuffers) };
	task.vkHandles = { commandPool };
	task.cleanupFunc = [this, commandPool, commandBuffers]() { vkFreeCommandBuffers(m_logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data()); };

	m_garbageCollector->createCleanupTask(task);
}