/* Engine.hpp - Core engine logic.
*
* Defines the Engine class, responsible for managing the simulation loop, updating
* game state, and coordinating subsystems such as rendering.
*/

#pragma once

#include <vector>
#include <unordered_set>

#include <Core/Application/AppWindow.hpp>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Application/GarbageCollector.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Application.hpp>
#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/InputManager.hpp>

#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkCommandManager.hpp>
#include <Vulkan/VkSyncManager.hpp>
#include <Vulkan/VkBufferManager.hpp>

#include <Rendering/Renderer.hpp>
#include <Rendering/Textures/TextureManager.hpp>
#include <Rendering/Pipelines/OffscreenPipeline.hpp>
#include <Rendering/Pipelines/PresentPipeline.hpp>
#include <Rendering/Geometry/GeometryLoader.hpp>

#include <Engine/RenderSystem.hpp>
#include <Engine/PhysicsSystem.hpp>
#include <Engine/Components/ModelComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/CoreComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/TelemetryComponents.hpp>
#include <Engine/Components/SpacecraftComponents.hpp>
#include <Engine/Threading/ThreadManager.hpp>

#include <Orchestration/Session.hpp>

#include <Simulation/Systems/Time.hpp>
#include <Simulation/Systems/ReferenceFrameSystem.hpp>

#include <Scene/SceneManager.hpp>
#include <Scene/GUI/UIPanelManager.hpp>
#include <Scene/GUI/Workspaces/OrbitalWorkspace.hpp>

#include <Utils/SpaceUtils.hpp>


class Engine {
public:
	Engine(GLFWwindow *w);
	~Engine();

	void initPersistentServices();

	void initComponents();

	void setApplicationStage(Application::Stage newAppStage);

	/* Starts the engine. */
	void run();

private:
	GLFWwindow *window;
	Application::Stage m_currentAppStage = Application::Stage::WORKSPACE_ORBITAL;

	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<Renderer> m_renderer;

	std::shared_ptr<InputManager> m_inputManager;
	std::shared_ptr<Session> m_currentSession;

	void bindEvents();

	/* Updates and processes all events */
	void update();


	void updateStartScreen();				// Update method for the start/welcome screen
	void updateLoadingScreen();				// Update method for the loading screen

	void updateOrbitalSetup();				// Update method for the orbital mechanics setup screen
	void updateOrbitalWorkspace();			// Update method for the orbital mechanics workspaces
};
