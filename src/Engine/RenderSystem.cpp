/* RenderSystem.cpp - Render system implementation.
*/

#include "RenderSystem.hpp"
#include <cstddef> // For offsetof


RenderSystem::RenderSystem(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr, UIRenderer *uiRenderer) :
	m_coreResources(coreResources),
	m_swapchainManager(swapchainMgr),
	m_uiRenderer(uiRenderer) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_resourceManager = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);

	m_logicalDevice = m_coreResources->getLogicalDevice();
	m_queueFamilies = m_coreResources->getQueueFamilyIndices();

	m_sceneReady.store(false);
	m_hasNewData.store(false);

	bindEvents();
	init();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


RenderSystem::~RenderSystem() {}


void RenderSystem::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<RenderSystem>();

	m_eventDispatcher->subscribe<InitEvent::BufferManager>(selfIndex,
		[this](const InitEvent::BufferManager &event) {
			m_globalVertexBuffer = event.globalVertexBuffer;
			m_globalIndexBuffer = event.globalIndexBuffer;
			m_perFrameDescriptorSets = event.perFrameDescriptorSets;
		}
	);


	m_eventDispatcher->subscribe<InitEvent::OffscreenPipeline>(selfIndex,
		[this](const InitEvent::OffscreenPipeline &event) {
			m_texArrayDescriptorSet = event.texArrayDescriptorSet;
			m_pbrDescriptorSet = event.pbrDescriptorSet;

			m_offscreenPipeline = event.pipeline;
			m_offscreenPipelineLayout = event.pipelineLayout;
			m_offscreenRenderPass = event.renderPass;
			m_offscreenFrameBuffers = event.offscreenFrameBuffers;
		}
	);


	m_eventDispatcher->subscribe<RecreationEvent::Swapchain>(selfIndex,
		[this](const RecreationEvent::Swapchain &event) {
			m_swapchainExtent = m_swapchainManager->getSwapChainExtent();
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::Renderables>(selfIndex,
		[this](const UpdateEvent::Renderables& event) {
			using enum UpdateEvent::Renderables::Type;

			switch (event.renderableType) {
			case GUI:
				// The GUI rendering runs on the main thread
				renderGUI(event.commandBuffer, event.currentFrame);
				break;

			default:
				// All other renderable types are assumed to run on a worker RENDERER thread
				m_renderThreadBarrier = event.barrier;
				m_currentFrame.store(event.currentFrame);
				m_hasNewData.store(true);

				m_tickCondVar.notify_one();

				break;
			}
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex, 
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_INIT:
				m_sceneReady.store(false);
				waitForResources(selfIndex);

				break;
			}
		}
	);
}


void RenderSystem::init() {
	// Pre-allocate secondary command buffers for scene rendering
	m_sceneSecondaryCmdBufs.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdBufAllocInfo.commandPool = VkCommandManager::CreateCommandPool(m_logicalDevice, m_queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		cmdBufAllocInfo.commandBufferCount = 1;

		VkResult bufAllocResult = vkAllocateCommandBuffers(m_logicalDevice, &cmdBufAllocInfo, &m_sceneSecondaryCmdBufs[i]);

		LOG_ASSERT(bufAllocResult == VK_SUCCESS, "Failed to allocate single-use command buffer!");

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(m_sceneSecondaryCmdBufs) };
		task.vkHandles = { m_sceneSecondaryCmdBufs[i] };
		task.cleanupFunc = [this, cmdBuf = m_sceneSecondaryCmdBufs[i], commandPool = cmdBufAllocInfo.commandPool]() {
			vkFreeCommandBuffers(m_logicalDevice, commandPool, 1, &cmdBuf);
		};
		m_resourceManager->createCleanupTask(task);
	}
}


void RenderSystem::tick(std::stop_token stopToken) {
	std::unique_lock<std::mutex> lock(m_tickMutex);
	m_tickCondVar.wait(lock, stopToken, [this, stopToken]() { return m_hasNewData.load(); });
	m_hasNewData.store(false);

	if (stopToken.stop_requested())
		return;

	// Scene
	if (m_sceneReady.load()) {
		uint32_t currentFrame = m_currentFrame.load();
		renderScene(currentFrame);

		m_eventDispatcher->dispatch(RequestEvent::ProcessSecondaryCommandBuffers{
			.buffers = { m_sceneSecondaryCmdBufs[currentFrame] },
			.targetStage = RequestEvent::ProcessSecondaryCommandBuffers::Stage::OFFSCREEN
		}, true, true);
	}


	// Sync with main thread
	auto barrier = m_renderThreadBarrier.lock();
	if (barrier && !stopToken.stop_requested())
		barrier->arrive_and_wait();
}


void RenderSystem::waitForResources(const EventDispatcher::SubscriberIndex &selfIndex) {
	auto thread = ThreadManager::CreateThread("WAIT_RENDER_RESOURCES");
	thread->set([this, selfIndex](std::stop_token stopToken) {
		EventFlags eventFlags = EVENT_FLAG_INIT_BUFFER_MANAGER_BIT;

		m_eventDispatcher->waitForEventCallbacks(selfIndex, eventFlags);

		m_sceneReady.store(true);
	});
	thread->start();
}


void RenderSystem::renderScene(uint32_t currentFrame) {
	vkResetCommandBuffer(m_sceneSecondaryCmdBufs[currentFrame], 0);

	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = m_offscreenRenderPass;
	inheritanceInfo.framebuffer = m_offscreenFrameBuffers[currentFrame];
	inheritanceInfo.subpass = 0;

	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // RENDER_PASS_CONTINUE_BIT signals that the entire (secondary) command buffer will be executed within a single render pass instance
	cmdBufBeginInfo.pInheritanceInfo = &inheritanceInfo;

	vkBeginCommandBuffer(m_sceneSecondaryCmdBufs[currentFrame], &cmdBufBeginInfo);
	{
		vkCmdBindPipeline(m_sceneSecondaryCmdBufs[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipeline);


		// Specify viewport and scissor states (since they're dynamic states)
			// Viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapchainExtent.width);
		viewport.height = static_cast<float>(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_sceneSecondaryCmdBufs[currentFrame], 0, 1, &viewport);

			// Scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapchainExtent;
		vkCmdSetScissor(m_sceneSecondaryCmdBufs[currentFrame], 0, 1, &scissor);


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
		vkCmdBindVertexBuffers(m_sceneSecondaryCmdBufs[currentFrame], 0, 1, vertexBuffers, vertexBufferOffsets);

			// Index buffer (note: you can only have 1 index buffer)
		VkBuffer indexBuffer = m_globalIndexBuffer;
		vkCmdBindIndexBuffer(m_sceneSecondaryCmdBufs[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);


		// Updates the PBR material parameters uniform buffer and draws the meshes
		Geometry::GeometryData *geomData = nullptr;

		auto sceneDataView = m_registry->getView<RenderComponent::SceneData>(); // Every scene has exactly 1 SceneData
		for (const auto &[entity, data] : sceneDataView)
			geomData = data.pGeomData;

		LOG_ASSERT(geomData, "Cannot process mesh renderable: Scene geometry data is invalid!");

#ifndef NDEBUG
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
#endif


		// Update global data
			// Textures array
		vkCmdBindDescriptorSets(m_sceneSecondaryCmdBufs[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 2, 1, &m_texArrayDescriptorSet, 0, nullptr);


		// Update each mesh's UBOs
		auto uboView = m_registry->getView<RenderComponent::MeshRenderable>();
		uint32_t vertexOffset = 0;

		const VkDescriptorSet &currentDescriptorSet = m_perFrameDescriptorSets[currentFrame];

		for (const auto &[entity, meshRenderable] : uboView) {
			vertexOffset = geomData->meshOffsets[meshRenderable.meshRange.left].vertexOffset;

			for (uint32_t meshIndex : meshRenderable.meshRange) {
				// Object UBO
				uint32_t objectUBOOffset = static_cast<uint32_t>(meshIndex * objectUBOAlignment);
				vkCmdBindDescriptorSets(m_sceneSecondaryCmdBufs[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 0, 1, &currentDescriptorSet, 1, &objectUBOOffset);


				// Material parameters UBO
				Geometry::MeshOffset &meshOffset = geomData->meshOffsets[meshIndex];
				uint32_t meshMaterialOffset = static_cast<uint32_t>(meshOffset.materialIndex * pbrMaterialAlignment);

				vkCmdBindDescriptorSets(m_sceneSecondaryCmdBufs[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 1, 1, &m_pbrDescriptorSet, 1, &meshMaterialOffset);


				// Draw call
				vkCmdDrawIndexed(m_sceneSecondaryCmdBufs[currentFrame], meshOffset.indexCount, 1, meshOffset.indexOffset, vertexOffset, 0);
			}
		}
	}
	vkEndCommandBuffer(m_sceneSecondaryCmdBufs[currentFrame]);
}


void RenderSystem::renderGUI(VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
	m_uiRenderer->renderFrames(currentFrame);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}
