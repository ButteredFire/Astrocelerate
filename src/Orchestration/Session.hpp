/* Session.hpp - Implementation for user sessions.
*/

#pragma once

#include <chrono>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Engine/PhysicsSystem.hpp>
#include <Engine/Threading/WorkerThread.hpp>

#include <Scene/SceneManager.hpp>



class Session {
public:
	Session(VkCoreResourcesManager *coreResources, SceneManager *sceneMgr, PhysicsSystem *physicsSystem);
	~Session() = default;

	/* Creates a new session. */
	void init();

	/* A session frame update. */
	void update();

	/* Loads a scene from a simulation file. */
	void loadSceneFromFile(const std::string &filePath);

	/* Cleans up and shuts down THIS session. */
	void end();

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<Registry> m_registry;

	VkCoreResourcesManager *m_coreResources;
	SceneManager *m_sceneManager;
	PhysicsSystem *m_physicsSystem;

	// Physics threading
	std::shared_ptr<WorkerThread> m_physicsWorker;
	std::shared_ptr<WorkerThread> m_inputWorker;
	std::atomic<bool> m_inputThreadIsRunning{ false };
	std::atomic<bool> m_sessionIsValid{ false };
	std::atomic<double> m_accumulator{ 0.0 };


	void bindEvents();

	void reset();
};