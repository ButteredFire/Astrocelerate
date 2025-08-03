/* RenderSystem.cpp - Render system implementation.
*/

#include "RenderSystem.hpp"
#include <cstddef> // For offsetof


RenderSystem::RenderSystem(VkCoreResourcesManager *coreResources, UIRenderer *uiRenderer) :
	m_coreResources(coreResources),
	m_uiRenderer(uiRenderer) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void RenderSystem::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<RenderSystem>();

	m_eventDispatcher->subscribe<InitEvent::BufferManager>(selfIndex,
		[this](const InitEvent::BufferManager &event) {
			m_globalVertexBuffer = event.globalVertexBuffer;
			m_globalIndexBuffer = event.globalIndexBuffer;
			m_perFrameDescriptorSets = event.perFrameDescriptorSets;
			std::cout << "marked one whatdahelll\n";
		}
	);


	m_eventDispatcher->subscribe<InitEvent::OffscreenPipeline>(selfIndex,
		[this](const InitEvent::OffscreenPipeline &event) {
			m_offscreenPipelineLayout = event.pipelineLayout;

			m_texArrayDescriptorSet = event.texArrayDescriptorSet;
			m_pbrDescriptorSet = event.pbrDescriptorSet;
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::Renderables>(selfIndex,
		[this](const UpdateEvent::Renderables& event) {
			using enum UpdateEvent::Renderables::Type;

			switch (event.renderableType) {
			case MESHES:
				this->renderScene(event.commandBuffer, event.currentFrame);
				break;

			case GUI:
				this->renderGUI(event.commandBuffer, event.currentFrame);
				break;
			}
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex, 
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_INIT:
				m_sceneReady = false;
				waitForResources(selfIndex);

				break;
			}
		}
	);
}


void RenderSystem::waitForResources(const EventDispatcher::SubscriberIndex &selfIndex) {
	std::thread([this, selfIndex]() {
		EventFlags eventFlags = EVENT_FLAG_INIT_BUFFER_MANAGER_BIT;

		m_eventDispatcher->waitForEventCallbacks(selfIndex, eventFlags);

		std::cout << "scene is ready\n";
		m_sceneReady = true;
	}).detach();
}


void RenderSystem::renderScene(VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
	if (!m_sceneReady)
		return;

	const VkDescriptorSet &currentDescriptorSet = m_perFrameDescriptorSets[currentFrame];

	// Compute dynamic alignments
	const size_t minUBOAlignment = static_cast<size_t>(m_coreResources->getDeviceProperties().limits.minUniformBufferOffsetAlignment);
	const size_t objectUBOAlignment = SystemUtils::Align(sizeof(Buffer::ObjectUBO), minUBOAlignment);
	const size_t pbrMaterialAlignment = SystemUtils::Align(sizeof(Geometry::Material), minUBOAlignment);


	// Bind vertex and index buffers
		// Vertex buffers
	VkBuffer vertexBuffers[] = {
		m_globalVertexBuffer
	};
	VkDeviceSize vertexBufferOffsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, vertexBufferOffsets);

		// Index buffer (note: you can only have 1 index buffer)
	VkBuffer indexBuffer = m_globalIndexBuffer;
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);


	// Updates the PBR material parameters uniform buffer and draws the meshes
	Geometry::GeometryData *geomData = nullptr;

	auto sceneDataView = m_registry->getView<RenderComponent::SceneData>(); // Every scene has exactly 1 SceneData
	for (const auto &[entity, data] : sceneDataView)
		geomData = data.pGeomData;

	LOG_ASSERT(geomData, "Cannot process mesh renderable: Scene geometry data is invalid!");

	if (IN_DEBUG_MODE) {
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
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 2, 1, &m_texArrayDescriptorSet, 0, nullptr);


	// Update each mesh's UBOs
	auto uboView = m_registry->getView<RenderComponent::MeshRenderable>();
	uint32_t vertexOffset = 0;

	for (const auto &[entity, meshRenderable] : uboView) {
		vertexOffset = geomData->meshOffsets[meshRenderable.meshRange.left].vertexOffset;

		for (uint32_t meshIndex : meshRenderable.meshRange()) {
			// Object UBO
			uint32_t objectUBOOffset = static_cast<uint32_t>(meshIndex * objectUBOAlignment);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 0, 1, &currentDescriptorSet, 1, &objectUBOOffset);


			// Material parameters UBO
			Geometry::MeshOffset &meshOffset = geomData->meshOffsets[meshIndex];
			uint32_t meshMaterialOffset = static_cast<uint32_t>(meshOffset.materialIndex * pbrMaterialAlignment);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 1, 1, &m_pbrDescriptorSet, 1, &meshMaterialOffset);
			

			// Draw call
			vkCmdDrawIndexed(cmdBuffer, meshOffset.indexCount, 1, meshOffset.indexOffset, vertexOffset, 0);
		}
	}
}


void RenderSystem::renderGUI(VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
	m_uiRenderer->renderFrames(currentFrame);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}
