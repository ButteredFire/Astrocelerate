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
#include <Core/Application/ResourceManager.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Application.hpp>

#include <Core/Engine/ECS.hpp>
#include <Core/Engine/InputManager.hpp>

#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkCoreResourcesManager.hpp>
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

#include <Scene/SceneManager.hpp>
#include <Scene/GUI/UIPanelManager.hpp>
#include <Scene/GUI/Workspaces/SplashScreen.hpp>
#include <Scene/GUI/Workspaces/OrbitalWorkspace.hpp>

#include <Utils/SpaceUtils.hpp>


class Engine {
public:
	Engine(GLFWwindow *w);
	~Engine();

	void init();

	void setWindowPtr(GLFWwindow *w);

	void run();

private:
	GLFWwindow *m_window;

	Application::State m_currentAppState = Application::State::IDLE;

	std::shared_ptr<VkInstanceManager> m_instanceManager;
	std::shared_ptr<VkDeviceManager> m_deviceManager;
	std::shared_ptr<VkCoreResourcesManager> m_coreResourcesManager;


	// Core resources
	VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_deviceProperties{};

	VkDevice m_logicalDevice = VK_NULL_HANDLE;
	QueueFamilyIndices m_queueFamilies{};

	std::shared_ptr<WorkerThread> m_watchdogThread;


	// Core services
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ResourceManager> m_resourceManager;
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<TextureManager> m_textureManager;
	std::shared_ptr<SceneManager> m_sceneManager;
	std::unique_ptr<IWorkspace> m_splashScreen;
	std::unique_ptr<IWorkspace> m_orbitalWorkspace;
	std::shared_ptr<UIPanelManager> m_uiPanelManager;
	std::shared_ptr<Registry> m_globalRegistry;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<InputManager> m_inputManager;


	// Engine resource managers
	std::shared_ptr<VkSwapchainManager> m_swapchainManager;
	std::shared_ptr<VkCommandManager> m_commandManager;
	std::shared_ptr<VkBufferManager> m_bufferManager;
	std::shared_ptr<OffscreenPipeline> m_offscreenPipeline;
	std::shared_ptr<PresentPipeline> m_presentPipeline;
	std::shared_ptr<VkSyncManager> m_syncManager;
	std::shared_ptr<UIRenderer> m_uiRenderer;
	std::shared_ptr<Renderer> m_renderer;
	std::shared_ptr<RenderSystem> m_renderSystem;
	std::shared_ptr<PhysicsSystem> m_physicsSystem;
	std::shared_ptr<Session> m_currentSession;


	std::thread m_sessionThread;


	void bindEvents();

	void initComponents();

	void initCoreServices();

	void initCoreManagers();

	void initEngine();


	void prerun();	// Runs the main loop once to ensure all resources have been initialized
	void tick();
};
