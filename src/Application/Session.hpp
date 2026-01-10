/* Session.hpp - Implementation for user sessions. */

#pragma once

#include <chrono>


#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Threading/WorkerThread.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Engine/Scene/SceneLoader.hpp>
#include <Engine/Systems/PhysicsSystem.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>


class Session {
public:
	Session(VkCoreResourcesManager *coreResources, SceneLoader *sceneMgr, PhysicsSystem *physicsSystem, RenderSystem *renderSystem);
	~Session() = default;

	/* Creates a new session. */
	void init();

	/* A session frame update. */
	void update();

	/* Loads a scene from a simulation file. */
	void loadSceneFromFile(const std::string &filePath);

	/* Cleans up and shuts down THIS session. */
	void endSession();

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ECSRegistry> m_ecsRegistry;
	std::shared_ptr<InputManager> m_inputManager;

	VkCoreResourcesManager *m_coreResources;
	SceneLoader *m_sceneManager;
	PhysicsSystem *m_physicsSystem;
	RenderSystem *m_renderSystem;

	// Physics threading
	std::shared_ptr<WorkerThread> m_physicsWorker;
	std::shared_ptr<WorkerThread> m_renderWorker;
	std::shared_ptr<WorkerThread> m_inputWorker;
	std::atomic<bool> m_inputThreadIsRunning{ false };
	std::atomic<bool> m_sessionIsValid{ false };
	std::atomic<double> m_accumulator{ 0.0 };


	void bindEvents();

	void reset();
};