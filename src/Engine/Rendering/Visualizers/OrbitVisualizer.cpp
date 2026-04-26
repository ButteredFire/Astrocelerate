#include "OrbitVisualizer.hpp"


OrbitVisualizer::OrbitVisualizer(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData, const Buffer::BufferAlloc *orbitVertBuffer, std::unordered_map<uint32_t, std::pair<size_t, uint32_t>> orbitVertexOffsets) :
	m_renderDeviceCtx(renderDeviceCtx),
	m_windowCtx(windowCtx),

	m_offscreenData(offscreenData),

	m_orbitVertBuffer(orbitVertBuffer),
	m_orbitVertexOffsets(orbitVertexOffsets)
{}


void OrbitVisualizer::init(std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> secondCmdBufs) {
	m_secondCmdBufs = std::move(secondCmdBufs);

	VkDeviceSize minUBOAlignment = m_renderDeviceCtx->chosenDevice.properties.limits.minUniformBufferOffsetAlignment;
	m_alignedGlobalUBOSize = SystemUtils::Align(sizeof(Buffer::GlobalUBO), minUBOAlignment);
}


void OrbitVisualizer::prepareFrame(uint32_t frameIdx, const Buffer::FramePacket &framePacket) {}


void OrbitVisualizer::render(uint32_t frameIdx) {
	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = m_offscreenData->renderPass;
	inheritanceInfo.framebuffer = m_offscreenData->frameBuffers[frameIdx];
	inheritanceInfo.subpass = 0;

	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // RENDER_PASS_CONTINUE_BIT signals that the entire (secondary) command buffer will be executed within a single render pass instance
	cmdBufBeginInfo.pInheritanceInfo = &inheritanceInfo;


	vkBeginCommandBuffer(m_secondCmdBufs[frameIdx], &cmdBufBeginInfo);
	{
		vkCmdBindPipeline(m_secondCmdBufs[frameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenData->orbitPipeline);

		// Specify viewport and scissor states (since they're dynamic states)
			// Viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_windowCtx->extent.width);
		viewport.height = static_cast<float>(m_windowCtx->extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_secondCmdBufs[frameIdx], 0, 1, &viewport);

			// Scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_windowCtx->extent;
		vkCmdSetScissor(m_secondCmdBufs[frameIdx], 0, 1, &scissor);


		// Bind orbit vertex buffer
		VkBuffer vertexBufs[] = { m_orbitVertBuffer->buffer };
		VkDeviceSize vertBufOffsets[] = { 0 };
		vkCmdBindVertexBuffers(
			m_secondCmdBufs[frameIdx], 
			0, 1, vertexBufs, vertBufOffsets
		);


		// Bind global UBO
		uint32_t globalUBOOffset = static_cast<uint32_t>(m_alignedGlobalUBOSize);
		vkCmdBindDescriptorSets(
			m_secondCmdBufs[frameIdx],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_offscreenData->pipelineLayout,
			0, 1, &m_offscreenData->perFrameDescriptorSets[frameIdx],
			1, &globalUBOOffset
		);


		// Draw
		for (const auto &[entityID, offsetInfo] : m_orbitVertexOffsets) {
			const auto &[baseVertex, pointCount] = offsetInfo;

			//glm::vec4 color = getOrbitColor(entityID);
			glm::vec4 color(0.0, 1.0, 1.0, 1.0);


			vkCmdPushConstants(
				m_secondCmdBufs[frameIdx],
				m_offscreenData->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0, sizeof(glm::vec4), &color
			);

			vkCmdDraw(
				m_secondCmdBufs[frameIdx],
				pointCount, 1,
				baseVertex, 0
			);
		}

	}
	vkEndCommandBuffer(m_secondCmdBufs[frameIdx]);
}
