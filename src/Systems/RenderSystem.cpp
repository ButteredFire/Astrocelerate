/* RenderSystem.cpp - Render system implementation.
*/

#include "RenderSystem.hpp"


RenderSystem::RenderSystem(VulkanContext& context):
	m_vkContext(context) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	m_bufferManager = ServiceLocator::GetService<VkBufferManager>(__FUNCTION__);

	m_subpassBinder = ServiceLocator::GetService<SubpassBinder>(__FUNCTION__);

	m_eventDispatcher->subscribe<Event::UpdateRenderables>(
		[this](const Event::UpdateRenderables& event) {
			// Compute dynamic alignment
			m_dynamicAlignment = SystemUtils::align(sizeof(Buffer::ObjectUBO), m_vkContext.Device.deviceProperties.limits.minUniformBufferOffsetAlignment);


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
			auto meshView = m_registry->getView<Component::MeshRenderable>();
			
			for (const auto& [entity, meshRenderable] : meshView) {
				this->processMeshRenderable(event.commandBuffer, meshRenderable, event.descriptorSet);
			}

			vkCmdNextSubpass(event.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);


			// GUI Rendering
			auto guiView = m_registry->getView<Component::GUIRenderable>();
			for (const auto& [entity, guiRenderable] : guiView) {
				this->processGUIRenderable(event.commandBuffer, guiRenderable);
			}
		}
	);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void RenderSystem::processMeshRenderable(const VkCommandBuffer& cmdBuffer, const Component::MeshRenderable& renderable, const VkDescriptorSet& descriptorSet) {
	uint32_t dynamicOffset = static_cast<uint32_t>(renderable.uboIndex * m_dynamicAlignment);

	// Draw call
		// Binds descriptor sets
			// Descriptor set 0
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkContext.GraphicsPipeline.layout, 0, 1, &descriptorSet, 1, &dynamicOffset);

	// Draws vertices based on the index buffer
	//vkCmdDraw(cmdBuffer, renderable.vertexData.size(), 1, 0, 0);
	vkCmdDrawIndexed(cmdBuffer, renderable.meshOffset.indexCount, 1, renderable.meshOffset.indexOffset, renderable.meshOffset.vertexOffset, 0); // Use vkCmdDrawIndexed instead of vkCmdDraw to draw with the index buffer
}


void RenderSystem::processGUIRenderable(const VkCommandBuffer& cmdBuffer, const Component::GUIRenderable& renderable) {
	try {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	}
	catch (const std::out_of_range& e) {
		// This usually means there is no draw data to render (only happens at end of program).
		return;
	}
}


/*
template<typename Renderable>
void RenderSystem::processRenderable(const VkCommandBuffer& cmdBuffer, Renderable& renderable, SubpassConsts::Type subpassType) {
	switch (subpassType) {
	case SubpassConsts::T_MAIN:
		// Bind vertex and index buffers, then draw
		// vkCmdDraw parameters (Vulkan specs):
		// 	- commandBuffer: The command buffer to record the draw commands into
		// 	- vertexCount: The number of vertices to draw
		// 	- instanceCount: The number of instances to draw (useful for instanced rendering)
		// 	- firstVertex: The index of the first vertex to draw. It is used as an offset into the vertex buffer, and defines the lowest value of gl_VertexIndex.
		// 	- firstInstance: The instance ID of the first instance to draw. It is used as an offset for instanced rendering, and defines the lowest value of gl_InstanceIndex.
		
		// Vertex buffers
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, renderable.vertexBuffers.data(), renderable.vertexBufferOffsets.data());


		// Index buffer (note: you can only have 1 index buffer)
		vkCmdBindIndexBuffer(cmdBuffer, renderable.indexBuffer, 0, VK_INDEX_TYPE_UINT32);


		// Draw call
			// Binds descriptor sets
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkContext.GraphicsPipeline.layout, 0, 1, &renderable.descriptorSet, 0, nullptr);


		// Draws vertices based on the index buffer
		//vkCmdDraw(cmdBuffer, renderable.vertexData.size(), 1, 0, 0);
		vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(renderable.vertexIndexData.size()), 1, 0, 0, 0); // Use vkCmdDrawIndexed instead of vkCmdDraw to draw with the index buffer

		break;

	case SubpassConsts::T_IMGUI:
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

		break;
	
	default:
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Unable to process unknown renderable!");
		break;
	}
}
*/
