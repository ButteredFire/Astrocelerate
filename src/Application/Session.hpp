/* Session.hpp - Implementation for user sessions. */

#pragma once

#include <chrono>
#include <memory>


#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Threading/WorkerThread.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Engine/Input/InputManager.hpp>
#include <Engine/Scene/Parsing/SceneLoader.hpp>
#include <Engine/Systems/RenderSystem.hpp>
#include <Engine/Systems/PhysicsSystem.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>


class Session {
public:
	Session(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData, std::shared_ptr<InputManager> inputMgr, std::shared_ptr<SceneLoader> sceneLoader, std::shared_ptr<PhysicsSystem> physicsSystem, std::shared_ptr<RenderSystem> renderSystem);
	~Session() = default;

	void init();

	void tick();

	/* Loads a scene from a simulation file. */
	void loadSceneFromFile(const std::string &filePath);

	/* Cleans up and shuts down THIS session. */
	void endSession();

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ECSRegistry> m_ecsRegistry;

	std::shared_ptr<InputManager> m_inputManager;
	std::shared_ptr<SceneLoader> m_sceneLoader;
	std::shared_ptr<PhysicsSystem> m_physicsSystem;
	std::shared_ptr<RenderSystem> m_renderSystem;

	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	const Ctx::VkWindow *m_windowCtx;
	const Ctx::OffscreenPipeline *m_offscreenData;

	// Threading
	std::shared_ptr<WorkerThread> m_physicsWorker;
	std::shared_ptr<WorkerThread> m_renderWorker;
	std::atomic<bool> m_inputThreadIsRunning{ false };
	std::atomic<bool> m_sessionIsValid{ false };
	std::atomic<double> m_accumulator{ 0.0 };


	void bindEvents();

	void reset();

	void initScene(const std::string &filePath);
};