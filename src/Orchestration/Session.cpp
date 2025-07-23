#include "Session.hpp"


Session::Session() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_sceneManager = ServiceLocator::GetService<SceneManager>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "New session initialized.");
}


void Session::bindEvents() {
	m_eventDispatcher->subscribe<Event::AppIsStable>(
		[this](const Event::AppIsStable &event) {
			m_bufferManager = ServiceLocator::GetService<VkBufferManager>(__FUNCTION__);
			m_offscreenPipeline = ServiceLocator::GetService<OffscreenPipeline>(__FUNCTION__);

			m_refFrameSystem = ServiceLocator::GetService<ReferenceFrameSystem>(__FUNCTION__);
			m_physicsSystem = ServiceLocator::GetService<PhysicsSystem>(__FUNCTION__);
		}
	);


	m_eventDispatcher->subscribe<Event::RequireInitSession>(
		[this](const Event::RequireInitSession &event) {
			this->loadSceneFromFile(event.simulationFilePath);
		}
	);


	m_eventDispatcher->subscribe<Event::UpdateSessionStatus>(
		[this](const Event::UpdateSessionStatus &event) {
			using namespace Event;

			switch (event.sessionStatus) {
			case UpdateSessionStatus::Status::NOT_READY:
				m_sessionInitialized = false;
				break;

			case UpdateSessionStatus::Status::PREPARE_FOR_INIT:
				m_sessionInitialized = false;
				break;

			case UpdateSessionStatus::Status::INITIALIZED:
				m_sessionInitialized = true;
				break;
			}
		}
	);
}


void Session::init() {
	m_eventDispatcher->publish(Event::UpdateSessionStatus{
		.sessionStatus = Event::UpdateSessionStatus::Status::NOT_READY
	});
}


void Session::update() {
	if (!m_sessionInitialized)
		return;

	static double accumulator = 0.0;
	static float timeScale = 0;

	static bool initialUpdate = false;
	if (!initialUpdate) {
		initialUpdate = true;
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
	vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);

	// Signal per-session managers to prepare the necessary resources for new session initialization
	m_eventDispatcher->publish(Event::UpdateSessionStatus{
		.sessionStatus = Event::UpdateSessionStatus::Status::PREPARE_FOR_INIT
	});
	

	// Clear the registry and recreate its base resources
	m_registry->clear();
	m_eventDispatcher->publish(Event::RegistryReset{});


	// Load scene from file
	m_sceneManager->loadSceneFromFile(filePath);
	m_offscreenPipeline->init();


	// Signal all managers that the newly initialized resources are safe to use
	m_eventDispatcher->publish(Event::UpdateSessionStatus{
		.sessionStatus = Event::UpdateSessionStatus::Status::INITIALIZED
	});
}


void Session::end() {

}