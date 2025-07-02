/* RenderSystem.cpp - Render system implementation.
*/

#include "RenderSystem.hpp"
#include <cstddef> // For offsetof


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
			this->processMeshRenderable(event);
		}
	);


	m_eventDispatcher->subscribe<Event::UpdateGUI>(
		[this](const Event::UpdateGUI& event) {
			this->processGUIRenderable(event);
		}
	);
}


void RenderSystem::processMeshRenderable(const Event::UpdateRenderables &event) {
	// Compute dynamic alignments
	const size_t minUBOAlignment = static_cast<size_t>(g_vkContext.Device.deviceProperties.limits.minUniformBufferOffsetAlignment);
	const size_t objectUBOAlignment = SystemUtils::Align(sizeof(Buffer::ObjectUBO), minUBOAlignment);
	const size_t pbrMaterialAlignment = SystemUtils::Align(sizeof(Geometry::Material), minUBOAlignment);


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


	// Updates the PBR material parameters uniform buffer and draws the meshes
	Geometry::GeometryData *geomData = nullptr;

	auto sceneDataView = m_registry->getView<RenderComponent::SceneData>(); // Every scene has exactly 1 SceneData
	for (const auto &[entity, data] : sceneDataView)
		geomData = data.pGeomData;

	LOG_ASSERT(geomData, "Cannot process mesh renderable: Scene geometry data is invalid!");


	// Update each mesh's UBOs
	auto uboView = m_registry->getView<RenderComponent::MeshRenderable>();
	for (const auto &[entity, meshRenderable] : uboView) {
		for (uint32_t meshIndex = meshRenderable.meshRange.left; meshIndex <= meshRenderable.meshRange.right; meshIndex++) {
			// Object UBO
			uint32_t objectUBOOffset = static_cast<uint32_t>(meshIndex * objectUBOAlignment);
			vkCmdBindDescriptorSets(event.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.layout, 0, 1, &event.descriptorSet, 1, &objectUBOOffset);
			
			
			// Material parameters UBO
			Geometry::MeshOffset &meshOffset = geomData->meshOffsets[meshIndex];
			uint32_t materialUBOOffset = static_cast<uint32_t>(meshOffset.materialIndex * pbrMaterialAlignment);
			vkCmdBindDescriptorSets(event.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.layout, 1, 1, &g_vkContext.OffscreenPipeline.pbrDescriptorSet, 1, &materialUBOOffset);


			vkCmdDrawIndexed(event.commandBuffer, meshOffset.indexCount, 1, meshOffset.indexOffset, meshOffset.vertexOffset, 0);
		}
	}
}


void RenderSystem::processGUIRenderable(const Event::UpdateGUI &event) {
	auto view = m_registry->getView<RenderComponent::GUIRenderable>();
	for (const auto &[entity, guiRenderable] : view) {
		m_imguiRenderer->renderFrames(event.currentFrame);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), event.commandBuffer);
	}
}
