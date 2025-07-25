/* Session.hpp - Implementation for user sessions.
*/

#pragma once

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Vulkan/VkBufferManager.hpp>

#include <Engine/PhysicsSystem.hpp>

#include <Rendering/Pipelines/OffscreenPipeline.hpp>

#include <Simulation/Systems/ReferenceFrameSystem.hpp>

#include <Scene/SceneManager.hpp>

class Session {
public:
	Session();
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

	std::shared_ptr<SceneManager> m_sceneManager;
	std::shared_ptr<VkBufferManager> m_bufferManager;
	std::shared_ptr<OffscreenPipeline> m_offscreenPipeline;

	std::shared_ptr<ReferenceFrameSystem> m_refFrameSystem;
	std::shared_ptr<PhysicsSystem> m_physicsSystem;

	bool m_sessionInitialized = false;
	bool m_initialRefFrameUpdate = false;


	void bindEvents();

	void reset();
};