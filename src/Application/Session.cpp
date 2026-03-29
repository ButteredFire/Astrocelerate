#include "Session.hpp"


Session::Session(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData, std::shared_ptr<InputManager> inputMgr, std::shared_ptr<SceneLoader> sceneLoader, std::shared_ptr<PhysicsSystem> physicsSystem, std::shared_ptr<RenderSystem> renderSystem) :
	m_renderDeviceCtx(renderDeviceCtx),
	m_windowCtx(windowCtx),
	m_offscreenData(offscreenData),
	m_inputManager(inputMgr),
	m_sceneLoader(sceneLoader),
	m_physicsSystem(physicsSystem),
	m_renderSystem(renderSystem) {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_ecsRegistry = ServiceLocator::GetService<ECSRegistry>(__FUNCTION__);

	bindEvents();

	init();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "New session initialized.");
}


void Session::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<Session>();

	m_eventDispatcher->subscribe<RequestEvent::InitSession>(selfIndex,
		[this](const RequestEvent::InitSession &event) {
			this->loadSceneFromFile(event.simulationFilePath);
		}
	);


	m_eventDispatcher->subscribe<InitEvent::BufferManager>(selfIndex, 
		[this](const InitEvent::BufferManager &event) {
			// Signal all managers that the newly initialized resources, PLUS any dynamic resources created during the INITIALIZED stage, are safe to use
			m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
				.sessionStatus = UpdateEvent::SessionStatus::Status::POST_INITIALIZATION
			});
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex, 
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case RESET:
			case PREPARE_FOR_INIT:
				m_sessionIsValid = false;
				m_accumulator = 0.0;
				break;

			case INITIALIZED:
				m_sessionIsValid = true;
				break;
			}
		}
	);
}


void Session::init() {
	m_physicsWorker = ThreadManager::CreateThread("PHYSICS");
	m_renderWorker	= ThreadManager::CreateThread("RENDERER");

	reset();
}


void Session::reset() {
	vkDeviceWaitIdle(m_renderDeviceCtx->logicalDevice);

	m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
		.sessionStatus = UpdateEvent::SessionStatus::Status::PREPARE_FOR_RESET
	});

	vkDeviceWaitIdle(m_renderDeviceCtx->logicalDevice);

	m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
		.sessionStatus = UpdateEvent::SessionStatus::Status::RESET
	});
}


void Session::tick() {
	// Update physics
	if (m_sessionIsValid && !m_physicsWorker->isRunning()) {
		m_physicsWorker->set([this](std::stop_token stopToken) {
			while (!m_physicsWorker->stopRequested()) {
				ThreadManager::SleepIfMainThreadHalted(m_physicsWorker.get());

				m_physicsSystem->tick(m_physicsWorker.get());
			}
		});

		m_physicsWorker->start();
	}


	// Update rendering
	if (m_sessionIsValid && !m_renderWorker->isRunning()) {
		m_renderWorker->set([this](std::stop_token stopToken) {
			while (!m_renderWorker->stopRequested()) {
				ThreadManager::SleepIfMainThreadHalted(m_renderWorker.get());

				m_renderSystem->tick(stopToken);
			}
		});
	
		m_renderWorker->start();
	}


	// Update other session-specific data
	
		// Camera linear interpolation
	static Camera *camera = m_inputManager->getCamera();
	camera->tick(m_physicsSystem->getDeltaTick());
}


void Session::loadSceneFromFile(const std::string &filePath) {
	endSession();


	// Signal per-session managers to prepare the necessary resources for new session initialization.
	m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
		.sessionStatus = UpdateEvent::SessionStatus::Status::PREPARE_FOR_INIT
	});


	// Load scene from file
		// Detach scene loading from the main thread
	auto sceneLoadThread = ThreadManager::CreateThread("SCENE_INIT");
	sceneLoadThread->set([this, filePath](std::stop_token stopToken) {
		try {
			auto fileData = m_sceneLoader->loadSceneFromFile(filePath);		// NOTE: GeometryLoader owns Geometry::GeometryData

			m_physicsSystem->init(fileData.fileConfig, fileData.simulationConfig);
			m_renderSystem->init(fileData.geometryData, m_offscreenData);


			// Propagate scene initialization status to GUI
			m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
				.progress = 1.0f,
				.message = "Scene initialization complete."
			});
			m_eventDispatcher->dispatch(UpdateEvent::SceneLoadComplete{
				.loadSuccessful = true,
				.finalMessage = "Scene initialization complete."
			});

			// Signal all managers that the newly initialized resources are safe to use
			m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
				.sessionStatus = UpdateEvent::SessionStatus::Status::INITIALIZED
			});
			m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
				.sessionStatus = UpdateEvent::SessionStatus::Status::POST_INITIALIZATION
			});
		}
		catch (const std::exception &e) {
			Log::Print(Log::T_ERROR, __FUNCTION__, std::string(e.what()));
			m_eventDispatcher->dispatch(UpdateEvent::SceneLoadComplete{
				.loadSuccessful = false,
				.finalMessage = STD_STR(e.what())
			});

			reset();
		}
	});

	sceneLoadThread->start(true);
}


void Session::endSession() {
	// Signal all listening managers to stop accessing per-session resources, and per-session managers to destroy old resources
	reset();


	// Wait for worker threads to finish
	m_physicsWorker->requestStop();
	m_renderWorker->requestStop();

	m_physicsWorker->waitForStop();
	m_renderWorker->waitForStop();


	// Clear the registry and recreate its base resources
	m_ecsRegistry->clear();
	m_eventDispatcher->dispatch(UpdateEvent::RegistryReset{});
	m_eventDispatcher->resetEventCallbackRegistry();
}