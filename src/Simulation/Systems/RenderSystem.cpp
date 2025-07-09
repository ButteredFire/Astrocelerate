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

	if (IN_DEBUG_MODE || 1) {
		static bool printedOnce = false;
		if (!printedOnce) {
			printedOnce = true;
			printf("Mesh count: %d\n", (int)geomData->meshCount);
			printf("Mesh offsets:\n");
			for (int i = 0; i < geomData->meshOffsets.size(); i++) {
				Geometry::MeshOffset &offset = geomData->meshOffsets[i];
				printf("\t[%d]:\n", i);
				printf("\t\tIndex count: %d\n", offset.indexCount);
				printf("\t\tIndex offset: %d\n", offset.indexOffset);
				printf("\t\tVertex offset: %d\n", offset.vertexOffset);
				printf("\t\tMaterial index: %d\n", offset.materialIndex);
			}
			printf("\nMesh materials:\n");
			for (int i = 0; i < geomData->meshMaterials.size(); i++) {
				Geometry::Material &mat = geomData->meshMaterials[i];
				printf("\t[%d]\n", i);
				printf("\t\tAlbedo color:\n\t\t\t[0, 1]: (%.3f, %.3f, %.3f)\n\t\t\t[0, 255]: (%.3f, %.3f, %.3f)\n", mat.albedoColor.x, mat.albedoColor.y, mat.albedoColor.z, (mat.albedoColor.x * 255), (mat.albedoColor.y * 255), (mat.albedoColor.z * 255));
				printf("\t\tAlbedo map index: %d\n", mat.albedoMapIndex);
				printf("\t\tAO map index: %d\n", mat.aoMapIndex);
				printf("\t\tEmissive color: (%.3f, %.3f, %.3f)\n", mat.emissiveColor.x, mat.emissiveColor.y, mat.emissiveColor.z);
				printf("\t\tEmissive map index: %d\n", mat.emissiveMapIndex);
				printf("\t\tHeight map index: %d\n", mat.heightMapIndex);
				printf("\t\tMetallic factor: %.3f\n", mat.metallicFactor);
				printf("\t\tNormal map index: %.3f\n", mat.roughnessFactor);
				printf("\t\tMetallic-Roughness map index: %d\n", mat.metallicRoughnessMapIndex);
				printf("\t\tNormal map index: %d\n", mat.normalMapIndex);
				printf("\t\tOpacity: %.3f\n", mat.opacity);

			}
		}
	}


	// Update global data
		// Textures array
	vkCmdBindDescriptorSets(event.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.layout, 2, 1, &g_vkContext.Textures.texArrayDescriptorSet, 0, nullptr);


	// Update each mesh's UBOs
	auto uboView = m_registry->getView<RenderComponent::MeshRenderable>();
	uint32_t vertexOffset = 0;

	for (const auto &[entity, meshRenderable] : uboView) {
		vertexOffset = geomData->meshOffsets[meshRenderable.meshRange.left].vertexOffset;

		for (uint32_t meshIndex : meshRenderable.meshRange()) {
			// Object UBO
			uint32_t objectUBOOffset = static_cast<uint32_t>(meshIndex * objectUBOAlignment);
			vkCmdBindDescriptorSets(event.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.layout, 0, 1, &event.descriptorSet, 1, &objectUBOOffset);


			// Material parameters UBO
			Geometry::MeshOffset &meshOffset = geomData->meshOffsets[meshIndex];
			uint32_t meshMaterialOffset = static_cast<uint32_t>(meshOffset.materialIndex * pbrMaterialAlignment);

			vkCmdBindDescriptorSets(event.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkContext.OffscreenPipeline.layout, 1, 1, &g_vkContext.Textures.pbrDescriptorSet, 1, &meshMaterialOffset);
			

			// Draw call
			vkCmdDrawIndexed(event.commandBuffer, meshOffset.indexCount, 1, meshOffset.indexOffset, vertexOffset, 0);
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
