/* Session.hpp - Implementation for user sessions.
*/

#pragma once

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Engine/PhysicsSystem.hpp>

#include <Simulation/Systems/ReferenceFrameSystem.hpp>

#include <Scene/SceneManager.hpp>



class Session {
public:
	Session(VkCoreResourcesManager *coreResources, SceneManager *sceneMgr, PhysicsSystem *physicsSystem, ReferenceFrameSystem *refFrameSystem);
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
	ReferenceFrameSystem *m_refFrameSystem;


	bool m_sessionInitialized = false;
	bool m_initialRefFrameUpdate = false;


	void bindEvents();

	void reset();
};