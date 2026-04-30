#include "RenderSystem.hpp"
#include <cstddef> // For offsetof


RenderSystem::RenderSystem(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, std::shared_ptr<PhysicsRenderBridge> physRendBridge, std::shared_ptr<VkBufferManager> bufferMgr, std::shared_ptr<UIRenderer> uiRenderer, std::shared_ptr<Camera> camera) :
	m_renderDeviceCtx(renderDeviceCtx),
	m_windowCtx(windowCtx),

	m_physRendBridge(physRendBridge),
	m_bufferManager(bufferMgr),
	m_uiRenderer(uiRenderer),
	m_camera(camera) {

	m_ecsRegistry = ServiceLocator::GetService<ECSRegistry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);

	m_logicalDevice = m_renderDeviceCtx->logicalDevice;
	m_queueFamilies = m_renderDeviceCtx->queueFamilies;

	m_sceneReady.store(false);
	m_hasNewData.store(false);

	bindEvents();

	initGUICmdBufSets();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


RenderSystem::~RenderSystem() {}


void RenderSystem::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<RenderSystem>();


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex, 
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_INIT:
				m_sceneReady.store(false);

				break;

			case POST_INITIALIZATION:
				m_sceneReady.store(true);
				break;
			}
		}
	);
}


void RenderSystem::init(const Geometry::GeometryData *geomData, const Ctx::OffscreenPipeline *offscreenData) {
	// Destroy previous per-session resources if RenderSystem is being re-initialized
	if (m_geomData != nullptr)
		m_cleanupManager->executeCleanupTask(m_sessionResourceID);

	m_geomData = geomData;
	m_offscreenData = offscreenData;

	m_sessionResourceID = m_cleanupManager->createCleanupGroup();

	initOrbitVertexArray();
	initGlobalBuffers();
	createVisualizers();
	initVisualizerCmdBufSets();
	initVisualizers();
}


void RenderSystem::initOrbitVertexArray() {
	Buffer::PhysRendFramePacket frame{};
	m_physRendBridge->consume(frame);

	m_orbitWorldVertices.clear();
	m_orbitVertexOffsets.clear();

	for (const auto &entity : frame.entities) {
		if (entity.orbitVertices.has_value()) {
			uint32_t baseVertex = static_cast<uint32_t>(m_orbitWorldVertices.size());
			uint32_t pointCount = static_cast<uint32_t>(entity.orbitVertices.value().size());

			for (const auto &vertex : entity.orbitVertices.value()) {
				m_orbitWorldVertices.emplace_back(
					SpaceUtils::ToRenderSpace_Position(vertex)
				);
			}

			m_orbitVertexOffsets[entity.entityID] = { baseVertex, pointCount };
		}
	}
}


void RenderSystem::initGlobalBuffers() {
	// Global vertex & index buffers
	{
		VkDeviceSize vertBufSize			= SystemUtils::ByteSize(m_geomData->meshVertices);
		VkDeviceSize idxBufSize				= SystemUtils::ByteSize(m_geomData->meshVertexIndices);
		VkBufferUsageFlags commonBufUsage	= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		m_globalVertBufAlloc	= m_bufferManager->allocate(vertBufSize, commonBufUsage | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Buffer::MemIntent::VRAM);
		m_globalIdxBufAlloc		= m_bufferManager->allocate(idxBufSize, commonBufUsage | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Buffer::MemIntent::VRAM);

		VkBufferUtils::WriteToGPUBuffer(m_renderDeviceCtx, m_geomData->meshVertices.data(), m_globalVertBufAlloc.buffer, vertBufSize);
		VkBufferUtils::WriteToGPUBuffer(m_renderDeviceCtx, m_geomData->meshVertexIndices.data(), m_globalIdxBufAlloc.buffer, idxBufSize);
	}



	// Orbit trajectory vertex buffer
	if (!m_orbitWorldVertices.empty()) {
		VkDeviceSize bufSize = SystemUtils::ByteSize(m_orbitWorldVertices);

		m_orbitVertBufAlloc = m_bufferManager->allocate(
			bufSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			Buffer::MemIntent::VRAM
		);

		VkBufferUtils::WriteToGPUBuffer(m_renderDeviceCtx, m_orbitWorldVertices.data(), m_orbitVertBufAlloc.buffer, bufSize);
	}



	// Global UBOs (1 per frame for MAX_FRAMES_IN_FLIGHT frames total)
	{
		VkDeviceSize alignment = m_renderDeviceCtx->chosenDevice.properties.limits.minUniformBufferOffsetAlignment;
		VkDeviceSize alignedSize = SystemUtils::Align(sizeof(Buffer::GlobalUBO), alignment);
		for (int i = 0; i < SimulationConst::MAX_FRAMES_IN_FLIGHT; i++) {
			// Create UBO
			m_globalUBOs[i].bufAlloc		= m_bufferManager->allocate(alignedSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Buffer::MemIntent::RAM_SEQ_ACCESS);
			m_globalUBOs[i].descriptorSet	= m_offscreenData->perFrameDescriptorSets[i];

			m_cleanupManager->addTaskDependency(m_globalUBOs[i].bufAlloc.resourceID, m_sessionResourceID);

			// Update UBO descriptor set
			VkDescriptorBufferInfo uboInfo{};
			uboInfo.buffer = m_globalUBOs[i].bufAlloc.buffer;
			uboInfo.offset = 0;
			uboInfo.range = alignedSize;

			VkWriteDescriptorSet uboDescWrite{};
			uboDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			uboDescWrite.dstSet = m_globalUBOs[i].descriptorSet;
			uboDescWrite.dstBinding = ShaderConst::VERT_BIND_GLOBAL_UBO;
			uboDescWrite.dstArrayElement = 0;
			uboDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboDescWrite.descriptorCount = 1;
			uboDescWrite.pBufferInfo = &uboInfo;

			vkUpdateDescriptorSets(m_renderDeviceCtx->logicalDevice, 1, &uboDescWrite, 0, nullptr);
		}
	}
}


void RenderSystem::createVisualizers() {
	m_visualizers.clear();

	// Geometry Visualizer
	m_visualizers.push_back(
		std::make_unique<GeometryVisualizer>(
			m_renderDeviceCtx, m_windowCtx,
			m_offscreenData, m_geomData,
			&m_globalVertBufAlloc, &m_globalIdxBufAlloc,
			m_bufferManager
		)
	);

	// Orbit Visualizer (conditional)
	if (!m_orbitWorldVertices.empty()) {
		m_visualizers.push_back(
			std::make_unique<OrbitVisualizer>(
				m_renderDeviceCtx, m_windowCtx,
				m_offscreenData,
				&m_orbitVertBufAlloc,
				m_orbitVertexOffsets
			)
		);
	}


	m_visualizerCount = m_visualizers.size();
}


void RenderSystem::initVisualizerCmdBufSets() {
	m_visualizerCmdBufs.resize(m_visualizerCount);
	m_visualizerCommandPools.resize(m_visualizerCount);

	for (int i = 0; i < m_visualizerCount; i++) {
		// NOTE: For the Visualizers to record to secondary command buffers in parallel, their set of buffers MUST be allocated from their own command pool.
		m_visualizerCommandPools[i] = VkCommandUtils::CreateCommandPool(m_logicalDevice, m_queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_sessionResourceID);

		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdBufAllocInfo.commandPool = m_visualizerCommandPools[i];
        // Allocate all frame command buffers at once into the array storage
		cmdBufAllocInfo.commandBufferCount = SimulationConst::MAX_FRAMES_IN_FLIGHT;

		VkResult bufAllocResult = vkAllocateCommandBuffers(m_logicalDevice, &cmdBufAllocInfo, m_visualizerCmdBufs[i].data());
		LOG_ASSERT(bufAllocResult == VK_SUCCESS, "Failed to allocate secondary command buffers for Visualizers!");

		// Create individual cleanup tasks for each allocated command buffer
		for (int j = 0; j < cmdBufAllocInfo.commandBufferCount; j++) {
			CleanupTask task{};
			task.caller = __FUNCTION__;
			task.objectNames = { VARIABLE_NAME(m_visualizerCmdBufs[i][j]) };
			task.cleanupFunc = [this, cmdBuf = m_visualizerCmdBufs[i][j], commandPool = m_visualizerCommandPools[i]]() {
				vkFreeCommandBuffers(m_logicalDevice, commandPool, 1, &cmdBuf);
			};
			ResourceID taskID = m_cleanupManager->createCleanupTask(task);
			m_cleanupManager->addTaskDependency(taskID, m_sessionResourceID);
		}
	}
}


void RenderSystem::initGUICmdBufSets() {
	m_guiCommandPool = VkCommandUtils::CreateCommandPool(m_logicalDevice, m_queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkCommandBufferAllocateInfo cmdBufAllocInfo{};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	cmdBufAllocInfo.commandPool = m_guiCommandPool;
    // Allocate all GUI secondary command buffers at once
	cmdBufAllocInfo.commandBufferCount = SimulationConst::MAX_FRAMES_IN_FLIGHT;
	VkResult bufAllocResult = vkAllocateCommandBuffers(m_logicalDevice, &cmdBufAllocInfo, m_guiCmdBufs.data());
	LOG_ASSERT(bufAllocResult == VK_SUCCESS, "Failed to allocate secondary command buffers for GUI!");

	for (size_t i = 0; i < m_guiCmdBufs.size(); i++) {
		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(m_guiCmdBufs[i]) };
		task.cleanupFunc = [this, cmdBuf = m_guiCmdBufs[i]]() {
			vkFreeCommandBuffers(m_logicalDevice, m_guiCommandPool, 1, &cmdBuf);
		};
		ResourceID taskID = m_cleanupManager->createCleanupTask(task);
		(void)taskID;
	}
}


void RenderSystem::initVisualizers() {
	for (int i = 0; i < m_visualizerCount; i++)
		m_visualizers[i]->init(m_visualizerCmdBufs[i]);
}


void RenderSystem::tick(std::stop_token stopToken) {
	std::unique_lock<std::mutex> lock(m_tickMutex);
	m_tickCondVar.wait(lock, stopToken, [this, stopToken]() { return m_hasNewData.load(); });

	m_hasNewData.store(false);

	if (stopToken.stop_requested())
		return;

	// Scene
	if (m_sceneReady.load()) {
		Buffer::PhysRendFramePacket frame{};
		m_physRendBridge->consume(frame);

		Buffer::FramePacket packet{};
		packet.frameIndex = m_currentFrame.load();
		packet.physRendFrame = &frame;

		buildFramePacket(&packet);
		renderScene(&packet);
	}


	// Sync with main thread
	auto barrier = m_renderThreadBarrier.lock();
	if (barrier && !stopToken.stop_requested())
		barrier->arrive_and_wait();
}


void RenderSystem::beginWork(uint32_t currentFrame, std::weak_ptr<std::barrier<>> barrier) {
	m_renderThreadBarrier = barrier;
	m_currentFrame.store(currentFrame);
	m_hasNewData.store(true);

	m_tickCondVar.notify_one();
}


std::vector<VkCommandBuffer> RenderSystem::getSceneCommandBuffers(uint32_t currentFrame) {
	std::unique_lock<std::mutex> lock(m_cmdBufProcessMutex);
	m_cmdBufProcessCV.wait(lock, [this, currentFrame]() { return m_finishedRecording[currentFrame]; });

	std::vector<VkCommandBuffer> commandBufs;
	commandBufs.reserve(m_visualizerCount);
	for (int i = 0; i < m_visualizerCount; i++)
		commandBufs.push_back(m_visualizerCmdBufs[i][currentFrame]);

	return commandBufs;
}


VkCommandBuffer RenderSystem::getGUICommandBuffer(uint32_t currentFrame) {
	return m_guiCmdBufs[currentFrame];
}


void RenderSystem::buildFramePacket(Buffer::FramePacket *packet) {
	if (m_camera->inFreeFlyMode())  packet->camFloatingOrigin = m_camera->getAbsoluteTransform().position;
	else                            packet->camFloatingOrigin = m_camera->getOrbitedEntityPosition();

	Camera::Configuration camConfig = m_camera->getConfig();


	/* ----- GLOBAL UBO ----- */
	{
		auto pointLights = m_ecsRegistry->getView<RenderComponent::PointLight>();

		packet->globalUBO.viewMatrix = m_camera->getRenderSpaceViewMatrix();
		packet->globalUBO.cameraPos = SpaceUtils::ToRenderSpace_Position(
			m_camera->getRelativeTransform().position
		);
		packet->globalUBO.floatingOrigin = SpaceUtils::ToRenderSpace_Position(packet->camFloatingOrigin);

		if (pointLights.size() > 0) {
			const auto &[entityWithPointLight, pointLight] = pointLights[0];

			packet->globalUBO.lightPos = glm::dvec3(pointLight.position) - SpaceUtils::ToRenderSpace_Position(packet->camFloatingOrigin);
			packet->globalUBO.lightColor = pointLight.color;
			packet->globalUBO.lightRadiantFlux = pointLight.radiantFlux;
		}

		packet->globalUBO.ambientStrength = 0.0;

		packet->globalUBO.projMatrix = glm::perspectiveRH_ZO(glm::radians(camConfig.zoom),
															 camConfig.aspectRatio,
															 camConfig.farClipPlane,
															 camConfig.nearClipPlane);
		/*
			GLM was originally designed for OpenGL, and because of that, the Y-coordinate of the clip coordinates is flipped.
			If this behavior is left as is, then images will be flipped upside down.
			One way to change this behavior is to flip the sign on the Y-axis scaling factor in the projection matrix.
		*/
		packet->globalUBO.projMatrix[1][1] *= -1; // Flip y-axis

		memcpy(m_globalUBOs[packet->frameIndex].bufAlloc.mappedData, &packet->globalUBO, sizeof(packet->globalUBO));
	}


	/* VISUALIZER WORK */
	const Buffer::FramePacket &framePacket = *packet;

	for (int i = 0; i < m_visualizerCount; i++)
		m_visualizers[i]->prepareFrame(packet->frameIndex, framePacket);
}


void RenderSystem::renderScene(Buffer::FramePacket *packet) {
	// Reset all visualizer command buffers for this frame
	for (int i = 0; i < m_visualizerCount; i++)
		vkResetCommandBuffer(m_visualizerCmdBufs[i][packet->frameIndex], 0);

	// Record & let conditional variable evaluate predicate
	m_finishedRecording[packet->frameIndex] = false;
	{
		for (int i = 0; i < m_visualizerCount; i++)
			m_visualizers[i]->render(packet->frameIndex);
	}
	m_finishedRecording[packet->frameIndex] = true;

	m_cmdBufProcessCV.notify_one();
}


void RenderSystem::renderGUI(uint32_t currentFrame, uint32_t imageIndex) {
	vkResetCommandBuffer(m_guiCmdBufs[currentFrame], 0);

	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = m_windowCtx->presentRenderPass;
	inheritanceInfo.framebuffer = m_windowCtx->framebuffers[imageIndex];
	inheritanceInfo.subpass = 0;

	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // RENDER_PASS_CONTINUE_BIT signals that the entire (secondary) command buffer will be executed within a single render pass instance
	cmdBufBeginInfo.pInheritanceInfo = &inheritanceInfo;


	vkBeginCommandBuffer(m_guiCmdBufs[currentFrame], &cmdBufBeginInfo);
	{
		m_uiRenderer->renderFrames(currentFrame);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_guiCmdBufs[currentFrame]);
	}
	vkEndCommandBuffer(m_guiCmdBufs[currentFrame]);
}
