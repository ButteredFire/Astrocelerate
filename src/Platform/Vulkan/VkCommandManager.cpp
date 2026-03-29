/* VkCommandManager.cpp - Command manager implementation.
*/

#include "VkCommandManager.hpp"


VkCommandManager::VkCommandManager(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData) :
	m_renderDeviceCtx(renderDeviceCtx),
	m_windowCtx(windowCtx),
	m_offscreenData(offscreenData) {
	
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);

	init();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
};


void VkCommandManager::init() {
	QueueFamilyIndices familyIndices = m_renderDeviceCtx->queueFamilies;

	m_graphicsCmdPool = VkCommandUtils::CreateCommandPool(m_renderDeviceCtx->logicalDevice, familyIndices.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VkCommandUtils::AllocCommandBuffers(m_renderDeviceCtx->logicalDevice, m_graphicsCmdPool, m_graphicsCmdBuffers);

	
	if (familyIndices.familyExists(familyIndices.transferFamily)) {
		m_transferCmdPool = VkCommandUtils::CreateCommandPool(m_renderDeviceCtx->logicalDevice, familyIndices.transferFamily.index.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VkCommandUtils::AllocCommandBuffers(m_renderDeviceCtx->logicalDevice, m_transferCmdPool, m_transferCmdBuffers);
	}
}


VkCommandManager::RenderBufferData VkCommandManager::beginRenderBuffer(uint32_t currentFrame, uint32_t imageIndex, VkCommandBuffer &buffer) {
	VkCommandBufferBeginInfo bufferBeginInfo{};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = 0;
	bufferBeginInfo.pInheritanceInfo = nullptr;

	VkResult beginCmdBufferResult = vkBeginCommandBuffer(buffer, &bufferBeginInfo);
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

	static std::vector<VkClearValue> clearValues = {
		clearValue,
		depthStencilClearValue
	};


	return RenderBufferData{
		currentFrame,
		imageIndex,
		buffer,
		clearValues
	};
}


void VkCommandManager::executeOffscreenPass(RenderBufferData &renderBuf, const std::vector<VkCommandBuffer> &secondaryBuffers) {
	VkRenderPassBeginInfo offscreenRenderPassBeginInfo{};
	offscreenRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	offscreenRenderPassBeginInfo.renderPass = m_offscreenData->renderPass;
	offscreenRenderPassBeginInfo.framebuffer = m_offscreenData->frameBuffers[renderBuf.currentFrame];
	offscreenRenderPassBeginInfo.renderArea.offset = { 0, 0 };
	offscreenRenderPassBeginInfo.renderArea.extent = m_windowCtx->extent;
	offscreenRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(renderBuf.clearValues.size());
	offscreenRenderPassBeginInfo.pClearValues = renderBuf.clearValues.data();

	/* The final parameter controls how the drawing commands within the render pass will be provided. It can have 2 values: VK_SUBPASS_CONTENTS_...
		...INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
		..SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
		...INLINE_AND_SECONDARY_COMMAND_BUFFERS_KHR (requires VK_KHR_maintenance7 device extension): Allows for hybrid recording (inline commands for the primary buffer; secondary command buffers executed via vkCmdExecuteCommands)
	*/
	vkCmdBeginRenderPass(renderBuf.buffer, &offscreenRenderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	{
		vkCmdExecuteCommands(
			renderBuf.buffer,
			static_cast<uint32_t>(secondaryBuffers.size()),
			secondaryBuffers.data()
		);
	}
	vkCmdEndRenderPass(renderBuf.buffer);
}


void VkCommandManager::executePresentPass(RenderBufferData &renderBuf, const std::vector<VkCommandBuffer> &secondaryBuffers) {
	// Transitions swapchain image to the layout COLOR_ATTACHMENT_OPTIMAL before the presentation render pass
		// This is because the image layout is UNDEFINED for the first swapchain image, and PRESENT_SRC_KHR for subsequent ones.
	VkImageMemoryBarrier swapchainImgToPresentBarrier{};
	swapchainImgToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapchainImgToPresentBarrier.srcAccessMask = 0;
	swapchainImgToPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	swapchainImgToPresentBarrier.oldLayout = m_windowCtx->imageLayouts[renderBuf.imageIndex];
	swapchainImgToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	swapchainImgToPresentBarrier.image = m_windowCtx->images[renderBuf.imageIndex];
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
		renderBuf.buffer,
		srcStageMask,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &swapchainImgToPresentBarrier
	);


	VkRenderPassBeginInfo presentRenderPassBeginInfo{};
	presentRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	presentRenderPassBeginInfo.renderPass = m_windowCtx->presentRenderPass;
	presentRenderPassBeginInfo.framebuffer = m_windowCtx->framebuffers[renderBuf.imageIndex];
	presentRenderPassBeginInfo.renderArea.offset = { 0, 0 };
	presentRenderPassBeginInfo.renderArea.extent = m_windowCtx->extent;
	presentRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(renderBuf.clearValues.size());
	presentRenderPassBeginInfo.pClearValues = renderBuf.clearValues.data();


	vkCmdBeginRenderPass(renderBuf.buffer, &presentRenderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	{
		// Process GUI rendering
		//m_eventDispatcher->dispatch(UpdateEvent::Renderables{
		//	.renderableType = UpdateEvent::Renderables::Type::GUI,
		//	.commandBuffer = renderBuf.buffer,
		//	.currentFrame = renderBuf.currentFrame
		//}, true);

		vkCmdExecuteCommands(
			renderBuf.buffer,
			static_cast<uint32_t>(secondaryBuffers.size()),
			secondaryBuffers.data()
		);


		// Record all present-stage secondary command buffers
		// TODO: To record both inline commands and execute secondary command buffers, we need to begin the render pass with INLINE_AND_SECONDARY_COMMAND_BUFFERS_KHR, which requires the VK_KHR_maintenance7 device extension.
		//if (!m_secondaryCmdBufsStagePRESENT.empty()) {
		//	throw Log::RuntimeException(__FUNCTION__, __LINE__, "Programmer Error: Cannot simultaneously execute inline commands and secondary command buffers in //present render pass!\nDoing so requires beginning the render pass with the VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_KHR bit.");
		//	//vkCmdExecuteCommands(cmdBuffer, static_cast<uint32_t>(m_secondaryCmdBufsStagePRESENT.size()), m_secondaryCmdBufsStagePRESENT.data());
		//	//m_secondaryCmdBufsStagePRESENT.clear();
		//}
	}
	vkCmdEndRenderPass(renderBuf.buffer);
}


void VkCommandManager::endRenderBuffer(RenderBufferData &renderBuf) {
	VkResult endCmdBufferResult = vkEndCommandBuffer(renderBuf.buffer);
	LOG_ASSERT(endCmdBufferResult == VK_SUCCESS, "Failed to record command buffer!");
}
