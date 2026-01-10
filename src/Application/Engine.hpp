#pragma once

#include <vector>
#include <unordered_set>

#include <Application/Session.hpp>


#include <Core/Data/Constants.h>
#include <Core/Data/Application.hpp>
#include <Core/Utils/SpaceUtils.hpp>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Threading/ThreadManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>

#include <Platform/Vulkan/VkInstanceManager.hpp>
#include <Platform/Vulkan/VkDeviceManager.hpp>
#include <Platform/Vulkan/VkCoreResourcesManager.hpp>
#include <Platform/Vulkan/VkSwapchainManager.hpp>
#include <Platform/Vulkan/VkCommandManager.hpp>
#include <Platform/Vulkan/VkSyncManager.hpp>
#include <Platform/Vulkan/VkBufferManager.hpp>
#include <Platform/Windowing/AppWindow.hpp>

#include <Engine/GUI/UIPanelManager.hpp>
#include <Engine/GUI/Workspaces/SplashScreen.hpp>
#include <Engine/GUI/Workspaces/OrbitalWorkspace.hpp>
#include <Engine/Scene/SceneLoader.hpp>
#include <Engine/Input/InputManager.hpp>
#include <Engine/Systems/RenderSystem.hpp>
#include <Engine/Systems/PhysicsSystem.hpp>
#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Registry/ECS/Components/ModelComponents.hpp>
#include <Engine/Registry/ECS/Components/RenderComponents.hpp>
#include <Engine/Registry/ECS/Components/CoreComponents.hpp>
#include <Engine/Registry/ECS/Components/PhysicsComponents.hpp>
#include <Engine/Registry/ECS/Components/TelemetryComponents.hpp>
#include <Engine/Registry/ECS/Components/SpacecraftComponents.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>
#include <Engine/Rendering/Renderer.hpp>
#include <Engine/Rendering/Geometry/GeometryLoader.hpp>
#include <Engine/Rendering/Textures/TextureManager.hpp>
#include <Engine/Rendering/Pipelines/OffscreenPipeline.hpp>
#include <Engine/Rendering/Pipelines/PresentPipeline.hpp>

#include <Simulation/Systems/Time.hpp>


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
	std::shared_ptr<CleanupManager> m_cleanupManager;
	std::shared_ptr<ECSRegistry> m_ecsRegistry;
	std::shared_ptr<TextureManager> m_textureManager;
	std::shared_ptr<SceneLoader> m_sceneManager;
	std::unique_ptr<IWorkspace> m_splashScreen;
	std::unique_ptr<IWorkspace> m_orbitalWorkspace;
	std::shared_ptr<UIPanelManager> m_uiPanelManager;
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

	void shutdown();
};
