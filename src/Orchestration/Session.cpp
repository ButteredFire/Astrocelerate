#include "Session.hpp"


Session::Session(VkCoreResourcesManager *coreResources, SceneManager *sceneMgr, PhysicsSystem *physicsSystem, ReferenceFrameSystem *refFrameSystem) :
	m_coreResources(coreResources),
	m_sceneManager(sceneMgr),
	m_physicsSystem(physicsSystem),
	m_refFrameSystem(refFrameSystem) {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

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


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case RESET:
				m_sessionInitialized = false;
				break;

			case PREPARE_FOR_INIT:
				m_sessionInitialized = false;
				break;

			case INITIALIZED:
				m_sessionInitialized = true;
				break;
			}
		}
	);
}


void Session::init() {
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
	if (!m_sessionInitialized)
		return;

	static double accumulator = 0.0;
	static float timeScale = 0;

	if (!m_initialRefFrameUpdate) {
		m_initialRefFrameUpdate = true;
		m_refFrameSystem->updateAllFrames();
	}


	timeScale = Time::GetTimeScale();

	Time::UpdateDeltaTime();
	double deltaTime = Time::GetDeltaTime();
	accumulator += deltaTime * timeScale;

	// Update physics
		// TODO: Implement adaptive timestepping instead of a constant TIME_STEP
	while (accumulator >= SimulationConsts::TIME_STEP) {
		const double scaledDeltaTime = SimulationConsts::TIME_STEP * timeScale;

		m_physicsSystem->update(scaledDeltaTime);
		m_refFrameSystem->updateAllFrames();

		accumulator -= scaledDeltaTime;
	}
}


void Session::loadSceneFromFile(const std::string &filePath) {
	// Signal all listening managers to stop accessing per-session resources, and per-session managers to destroy old resources
	reset();


	// Clear the registry and recreate its base resources
	m_registry->clear();
	m_eventDispatcher->dispatch(UpdateEvent::RegistryReset{});
	m_eventDispatcher->resetEventCallbackRegistry();


	// Signal per-session managers to prepare the necessary resources for new session initialization.
	m_eventDispatcher->dispatch(UpdateEvent::SessionStatus{
		.sessionStatus = UpdateEvent::SessionStatus::Status::PREPARE_FOR_INIT
	});


	// Load scene from file
		// Detach scene loading from the main thread
	std::thread([this, filePath]() {
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

			// Update reference frame system once in update loop
			m_initialRefFrameUpdate = false;
		}
		catch (const std::exception &e) {
			// Catch any exceptions during the worker thread's CPU-bound execution.
			Log::Print(Log::T_ERROR, __FUNCTION__, std::string(e.what()));
			m_eventDispatcher->dispatch(UpdateEvent::SceneLoadComplete{
				.loadSuccessful = false,
				.finalMessage = STD_STR(e.what())
			});

			reset();
			// Signal an error via the SceneInitializationComplete event
			//m_eventDispatcher.dispatch(Event::SceneInitializationComplete{ false, "Scene loading failed: " + std::string(e.what()) });
		}
	}).detach(); // Detach to run independently
}


void Session::end() {

}