/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"


Engine::Engine() {
    ThreadManager::SetMainThreadID(std::this_thread::get_id());
    initCoreServices();
    bindEvents();

    m_glfwWindow = std::make_unique<Window>(AppConst::DEFAULT_WINDOW_WIDTH, AppConst::DEFAULT_WINDOW_HEIGHT, APP_NAME);
    m_currentWindow = m_glfwWindow->initSplashScreen();

    init();
    std::this_thread::sleep_for(std::chrono::seconds(3)); // Sleep for enough time for the user to see the splash screen

    m_oldWindow = m_currentWindow;
    m_currentWindow = m_glfwWindow->initPrimaryScreen(&g_glfwCallbackCtx);
    setMainWindow(m_oldWindow, m_currentWindow);

    m_eventDispatcher->dispatch(UpdateEvent::AppIsStable{});
    m_eventDispatcher->dispatch(UpdateEvent::ApplicationStatus{
        .appState = Application::State::IDLE
    });

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Engine::~Engine() {}


void Engine::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<Engine>();

    m_eventDispatcher->subscribe<UpdateEvent::RegistryReset>(selfIndex,
        [this](const UpdateEvent::RegistryReset &event) {
            this->initComponents();
        }
    );
}


void Engine::init() {
    initComponents();
    initRenderResources();
    initCoreManagers();
    initEngine();

    prerun();

    // Switch workspace from splash screen to actual GUI
    m_uiPanelManager->switchWorkspace(m_orbitalWorkspace.get());
}


void Engine::setMainWindow(GLFWwindow *oldWindow, GLFWwindow *currentWindow) {
    m_windowCtx = m_windowManager->recreateSwapchain(oldWindow, currentWindow);

    m_eventDispatcher->dispatch(RequestEvent::ReInitImGui{
        .newWindowPtr = m_currentWindow
    });
}


void Engine::initCoreServices() {
    // Event dispatcher
    m_eventDispatcher = std::make_shared<EventDispatcher>();
    ServiceLocator::RegisterService(m_eventDispatcher);


    // Garbage collector
    m_cleanupManager = std::make_shared<CleanupManager>();
    ServiceLocator::RegisterService(m_cleanupManager);


    // ECS Registry
    m_ecsRegistry = std::make_shared<ECSRegistry>();
    ServiceLocator::RegisterService(m_ecsRegistry);
}


void Engine::initRenderResources() {
    m_instanceManager = std::make_shared<VkInstanceManager>();
    m_deviceManager = std::make_shared<VkDeviceManager>();

    // TODO: Make VkCoreResourcesManager own VkInstanceManager and VkDeviceManager instead of the Engine
    m_coreResourcesManager = std::make_shared<VkCoreResourcesManager>(m_currentWindow, m_instanceManager.get(), m_deviceManager.get(), m_cleanupManager.get());
    m_windowManager = std::make_shared<VkWindowManager>(m_currentWindow, m_coreResourcesManager.get());


    m_renderDeviceCtx = m_coreResourcesManager->getCoreContext();
    m_windowCtx = m_windowManager->getContext();
}


void Engine::initCoreManagers() {
    // Texture manager
    m_textureManager = std::make_shared<TextureManager>(m_renderDeviceCtx);
    ServiceLocator::RegisterService(m_textureManager);


    // Scene manager
    m_sceneLoader = std::make_shared<SceneLoader>();
    ServiceLocator::RegisterService(m_sceneLoader);
    m_sceneLoader->init();


    // GUI management
        // Workspaces
        // TODO: Create plugin manager and workspace systems to allow for dynamic workspace discovery and swapping at runtime
    m_splashScreen = std::make_unique<SplashScreen>();

        // Manager
    m_uiPanelManager = std::make_shared<UIPanelManager>(m_splashScreen.get());


    // Camera
    glm::dvec3 cameraPosition = glm::vec3(0.0, 1.3e8, 0.0);
    m_camera = std::make_shared<Camera>(cameraPosition, glm::quat());
    ServiceLocator::RegisterService(m_camera);


    // Input manager
    m_inputManager = std::make_shared<InputManager>();
    ServiceLocator::RegisterService(m_inputManager);
    m_inputManager->init();

    g_glfwCallbackCtx.inputManager = m_inputManager.get();
}


void Engine::initEngine() {
    // Pipelines
    m_offscreenPipeline = std::make_shared<OffscreenPipeline>(m_renderDeviceCtx, m_windowCtx);
    m_presentPipeline = std::make_shared<PresentPipeline>(m_renderDeviceCtx, m_windowCtx);

    m_offscreenData = m_offscreenPipeline->init();

    m_textureManager->setTextureArray(m_offscreenData->texArrayDescriptorSet, m_offscreenData->renderPass);


    // Managers
    m_commandManager = std::make_shared<VkCommandManager>(m_renderDeviceCtx, m_windowCtx, m_offscreenData);
    m_bufferManager = std::make_shared<VkBufferManager>(m_renderDeviceCtx);
    m_syncManager = std::make_shared<VkSyncManager>(m_renderDeviceCtx, m_windowCtx);


    // Workspaces
    m_orbitalWorkspace = std::make_unique<OrbitalWorkspace>(m_renderDeviceCtx, m_offscreenData, m_inputManager);


    // Renderers
    m_uiRenderer = std::make_shared<UIRenderer>(m_currentWindow, m_renderDeviceCtx, m_windowCtx, m_uiPanelManager);
    m_renderSystem = std::make_shared<RenderSystem>(m_renderDeviceCtx, m_windowCtx, m_bufferManager, m_uiRenderer, m_camera);
    m_renderer = std::make_shared<Renderer>(
        m_renderDeviceCtx, m_windowCtx,
        m_windowManager,
        m_commandManager,
        m_syncManager,
        m_uiRenderer,
        m_renderSystem
    );


    m_physicsSystem = std::make_shared<PhysicsSystem>();

    
    // Create new session
    m_currentSession = std::make_shared<Session>(
        m_renderDeviceCtx, m_windowCtx, m_offscreenData,
        m_inputManager,
        m_sceneLoader,
        m_physicsSystem, m_renderSystem
    );
}


void Engine::initComponents() {
    /* Core */
    m_ecsRegistry->initComponentArray<CoreComponent::Transform>();
    m_ecsRegistry->initComponentArray<CoreComponent::Identifiers>();

    /* Meshes & Models */
    m_ecsRegistry->initComponentArray<ModelComponent::Mesh>();
    m_ecsRegistry->initComponentArray<ModelComponent::Material>();

    /* Rendering */
    m_ecsRegistry->initComponentArray<RenderComponent::PointLight>();
    m_ecsRegistry->initComponentArray<RenderComponent::MeshRenderable>();

    /* Physics */
    m_ecsRegistry->initComponentArray<PhysicsComponent::RigidBody>();
    m_ecsRegistry->initComponentArray<PhysicsComponent::Propagator>();
    m_ecsRegistry->initComponentArray<PhysicsComponent::OrbitingBody>();
    m_ecsRegistry->initComponentArray<PhysicsComponent::NutationAngles>();
    m_ecsRegistry->initComponentArray<PhysicsComponent::ShapeParameters>();
    m_ecsRegistry->initComponentArray<PhysicsComponent::CoordinateSystem>();

    /* Spacecraft */
    m_ecsRegistry->initComponentArray<SpacecraftComponent::Thruster>();
    m_ecsRegistry->initComponentArray<SpacecraftComponent::Spacecraft>();

    /* Telemetry */
    m_ecsRegistry->initComponentArray<TelemetryComponent::RenderTransform>();
}


void Engine::run() {
    startThreadMonitor();

    try {
        while (!glfwWindowShouldClose(m_currentWindow))
            tick();
    }
    catch (...) {
        shutdown();
        throw;  // Propagate to higher-level catch (main)
    }

    shutdown();
}


void Engine::startThreadMonitor() {
    m_watchdogThread = ThreadManager::CreateThread("WATCHDOG");

    m_watchdogThread->set([this](std::stop_token stopToken) {
        const uint32_t HALT_ELAPSE_INTERVAL = 10; // seconds
        uint32_t haltElapseCnt = 1;

        while (!m_watchdogThread->stopRequested()) {
            /*
                By default, std::atomic::load uses std::memory_order_seq_cst (sequentially consistent). It's the safest but slowest memory ordering as it enforces a total global ordering operation across all threads.
                Since the watchdog thread only needs to know how much time has roughly elapsed since the last heartbeat, we can use std::memory_order_relaxed. It's the weakest memory ordering that only guarantees atomicity and nothing else, therefore giving us a performance advantage at the expense of no strict synchronization between threads (which is tolerable).
            */
            auto lastHeartbeat = g_appCtx.MainThread.heartbeatTimePoint.load(std::memory_order_relaxed);
            auto now = std::chrono::steady_clock::now();


            if (now - lastHeartbeat >= std::chrono::milliseconds(AppConst::MAX_MAIN_THREAD_TIMEOUT)) {
                // If the time elapsed between now and the last heartbeat exceeds the maximum timeout, signal to all worker threads that the main thread has halted
                if (!g_appCtx.MainThread.isHalted.load()) {
                    m_eventDispatcher->dispatch(UpdateEvent::ApplicationStatus{
                        .appState = Application::State::MAIN_THREAD_HALTING
                    }, false, true);

                    ThreadManager::SignalMainThreadHalt();
                }
                

                if (now - lastHeartbeat >= std::chrono::seconds(haltElapseCnt * HALT_ELAPSE_INTERVAL)) {
                    Log::Print(Log::T_WARNING, __FUNCTION__, "Main thread has been blocked for " + std::to_string(haltElapseCnt * HALT_ELAPSE_INTERVAL) + " seconds.");
                    haltElapseCnt++;
                }
            }
            else {
                // Else (time elapsed < max. timeout threshold), signal that the main thread has resumed
                if (g_appCtx.MainThread.isHalted.load()) {
                    m_eventDispatcher->dispatch(UpdateEvent::ApplicationStatus{
                        .appState = Application::State::IDLE
                    }, false, true);

                    ThreadManager::SignalMainThreadResume();
                    haltElapseCnt = 1;
                }
            }


            // The watchdog thread doesn't need to run constantly; we should have it work on intervals to avoid 100% CPU usage.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    m_watchdogThread->start();
}


void Engine::prerun() {
    for (int i = 0; i < SimulationConst::MAX_FRAMES_IN_FLIGHT; i++)
        tick();

    vkDeviceWaitIdle(m_renderDeviceCtx->logicalDevice);
}


void Engine::tick() {
    // Update main thread heartbeat
    g_appCtx.MainThread.heartbeatTimePoint.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    // Polling
    glfwPollEvents();
    m_eventDispatcher->pollQueuedEvents();

    // Tick
    Buffer::FramePacket framePacket{};

    m_inputManager->tick();

    m_currentSession->tick();

    m_renderer->tick();
}


void Engine::shutdown() {
    vkDeviceWaitIdle(m_renderDeviceCtx->logicalDevice);

    m_eventDispatcher->dispatch(UpdateEvent::ApplicationStatus{
      .appState = Application::State::SHUTDOWN
    });

    m_watchdogThread->requestStop();
    m_watchdogThread->waitForStop();

    m_currentSession->endSession();
}
