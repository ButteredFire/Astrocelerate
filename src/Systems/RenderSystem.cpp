/* RenderSystem.cpp - Render system implementation.
*/

#include "RenderSystem.hpp"


void RenderSystem::processRenderable(VulkanContext& m_vkContext, VkCommandBuffer& cmdBuffer, RenderableComponent& renderable) {
	switch (renderable.renderableType) {
	case RenderableComponentType::T_RENDERABLE_VERTEX:
		// Bind vertex and index buffers, then draw
		/* vkCmdDraw parameters (Vulkan specs):
			- commandBuffer: The command buffer to record the draw commands into
			- vertexCount: The number of vertices to draw
			- instanceCount: The number of instances to draw (useful for instanced rendering)
			- firstVertex: The index of the first vertex to draw. It is used as an offset into the vertex buffer, and defines the lowest value of gl_VertexIndex.
			- firstInstance: The instance ID of the first instance to draw. It is used as an offset for instanced rendering, and defines the lowest value of gl_InstanceIndex.
		*/
		// Vertex buffers
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, renderable.vertexBuffers.data(), renderable.vertexBufferOffsets.data());


		// Index buffer (note: you can only have 1 index buffer)
		vkCmdBindIndexBuffer(cmdBuffer, renderable.indexBuffer, 0, VK_INDEX_TYPE_UINT32);


		// Draw call
			// Binds descriptor sets
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkContext.GraphicsPipeline.layout, 0, 1, &renderable.descriptorSet, 0, nullptr);


		// Draws vertices based on the index buffer
		vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(renderable.vertexIndexData.size()), 1, 0, 0, 0); // Use vkCmdDrawIndexed instead of vkCmdDraw to draw with the index buffer

		break;

	case RenderableComponentType::T_RENDERABLE_GUI:
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

		break;
	
	default:
		throw Log::RuntimeException(__FUNCTION__, "Unable to process unknown renderable!");
		break;
	}
}
