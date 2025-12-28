#include "Session.hpp"


Session::Session(VkCoreResourcesManager *coreResources, SceneManager *sceneMgr, PhysicsSystem *physicsSystem, RenderSystem *renderSystem) :
	m_coreResources(coreResources),
	m_sceneManager(sceneMgr),
	m_physicsSystem(physicsSystem),
	m_renderSystem(renderSystem) {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);

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


	m_eventDispatcher->subscribe<InitEvent::OffscreenPipeline>(selfIndex,
		[this](const InitEvent::OffscreenPipeline &event) {
			// This event is just an alias for InitEvent::OffscreenPipeline for the sake of readability.
			// Services that don't have a direct tie to the offscreen pipeline should listen to this event.
			m_eventDispatcher->dispatch(InitEvent::Scene{});

			// Signal all managers that the newly initialized resources are safe to use
			m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
				.sessionStatus = UpdateEvent::SessionStatus::Status::INITIALIZED
			});
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
	//m_inputWorker = ThreadManager::CreateThread("USER_INPUT");

	reset();
}


void Session::reset() {
	vkDeviceWaitIdle(m_coreResources->getLogicalDevice());

	m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
		.sessionStatus = UpdateEvent::SessionStatus::Status::PREPARE_FOR_RESET
	});

	vkDeviceWaitIdle(m_coreResources->getLogicalDevice());

	m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
		.sessionStatus = UpdateEvent::SessionStatus::Status::RESET
	});
}


void Session::update() {
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


	// Process key input events
	//if (!m_inputWorker->isRunning()) {
	//	m_inputWorker->set([this](std::stop_token stopToken) {
	//		while (!m_inputWorker->stopRequested()) {
	//			std::unique_lock<std::mutex> lock(g_appContext.MainThread.haltMutex);
	//			g_appContext.MainThread.haltCV.wait(lock, []() { return !g_appContext.MainThread.isHalted.load(); });
	//
	//			m_inputManager->tick(Time::GetDeltaTime(), m_accumulator);
	//		}
	//	});
	//
	//	m_inputWorker->start();
	//}
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
			m_sceneManager->loadSceneFromFile(filePath);
			m_eventDispatcher->dispatch(RequestEvent::InitSceneResources{});

			// Propagate scene initialization status to GUI
			m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
				.progress = 1.0f,
				.message = "Scene initialization complete."
			});
			m_eventDispatcher->dispatch(UpdateEvent::SceneLoadComplete{
				.loadSuccessful = true,
				.finalMessage = "Scene initialization complete."
			});
		}
		catch (const std::exception &e) {
			// Catch any exceptions during the worker thread's CPU-bound execution.
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
	m_registry->clear();
	m_eventDispatcher->dispatch(UpdateEvent::RegistryReset{});
	m_eventDispatcher->resetEventCallbackRegistry();
}