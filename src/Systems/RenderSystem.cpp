/* RenderSystem.cpp - Render system implementation.
*/

#include "RenderSystem.hpp"


RenderSystem::RenderSystem() {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_bufferManager = ServiceLocator::GetService<VkBufferManager>(__FUNCTION__);
	m_imguiRenderer = ServiceLocator::GetService<UIRenderer>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void RenderSystem::bindEvents() {
	m_eventDispatcher->subscribe<Event::UpdateRenderables>(
		[this](const Event::UpdateRenderables& event) {
			// Compute dynamic alignment
			m_dynamicAlignment = SystemUtils::Align(sizeof(Buffer::ObjectUBO), g_vkContext.Device.deviceProperties.limits.minUniformBufferOffsetAlignment);


			// Bind vertex and index buffers
				// Vertex buffers
			VkBuffer vertexBuffers[] = {
				m_bufferManager->getVertexBuffer()
			};
			VkDeviceSize vertexBufferOffsets[] = { 0 };

			vkCmdBindVertexBuffers(event.commandBuffer, 0, 1, vertexBuffers, vertexBufferOffsets);

			// Index buffer (note: you can only have 1 index buffer)
			VkBuffer indexBuffer = m_bufferManager->getIndexBuffer();
			vkCmdBindIndexBuffer(event.commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);


			// Mesh rendering
			auto view = m_registry->getView<Component::MeshRenderable>();

			for (const auto& [entity, meshRenderable] : view) {
				this->processMeshRenderable(event.commandBuffer, meshRenderable, event.descriptorSet);
			}
		}
	);


	m_eventDispatcher->subscribe<Event::UpdateGUI>(
		[this](const Event::UpdateGUI& event) {
			auto view = m_registry->getView<Component::GUIRenderable>();
			for (const auto& [entity, guiRenderable] : view) {
				this->processGUIRenderable(event.commandBuffer, guiRenderable, event.currentFrame);
			}
		}
	);
}


void RenderSystem::processMeshRenderable(const VkCommandBuffer& cmdBuffer, const Component::MeshRenderable& renderable, const VkDescriptorSet& descriptorSet) {
	uint32_t dynamicOffset = static_cast<uint32_t>(renderable.uboIndex * m_dynamicAlignment);

	// Draw call
		// Binds descriptor sets
			// Descriptor set 0
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.layout, 0, 1, &descriptorSet, 1, &dynamicOffset);

	// Draws vertices based on the index buffer
	vkCmdDrawIndexed(cmdBuffer, renderable.meshOffset.indexCount, 1, renderable.meshOffset.indexOffset, renderable.meshOffset.vertexOffset, 0); // Use vkCmdDrawIndexed instead of vkCmdDraw to draw with the index buffer
}


void RenderSystem::processGUIRenderable(const VkCommandBuffer& cmdBuffer, const Component::GUIRenderable& renderable, uint32_t currentFrame) {
	m_imguiRenderer->renderFrames(currentFrame);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}
