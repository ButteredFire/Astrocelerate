/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"


Engine::Engine(GLFWwindow *w):
    m_window(w) {

    initCoreServices();

    ThreadManager::SetMainThreadID(std::this_thread::get_id());

    bindEvents();

    LOG_ASSERT(m_window != nullptr, "Engine crashed: Invalid window context!");

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Engine::~Engine() {
    // Frees up all associated memory.
    glfwDestroyWindow(m_window);
    glfwTerminate();
}


void Engine::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<Engine>();

    m_eventDispatcher->subscribe<UpdateEvent::RegistryReset>(selfIndex,
        [this](const UpdateEvent::RegistryReset &event) {
            this->initComponents();
        }
    );


    m_eventDispatcher->subscribe<UpdateEvent::ApplicationStatus>(selfIndex,
        [this](const UpdateEvent::ApplicationStatus &event) {
            if (event.appStage != Application::Stage::NULL_STAGE)
                setApplicationStage(event.appStage);
        }
    );
}


void Engine::init() {
    initComponents();
    initCoreManagers();
    initEngine();
}


void Engine::initCoreServices() {
    // Event dispatcher
    m_eventDispatcher = std::make_shared<EventDispatcher>();
    ServiceLocator::RegisterService(m_eventDispatcher);


    // Garbage collector
    m_garbageCollector = std::make_shared<GarbageCollector>();
    ServiceLocator::RegisterService(m_garbageCollector);


    // ECS Registry
    m_registry = std::make_shared<Registry>();
    ServiceLocator::RegisterService(m_registry);
}


void Engine::initCoreManagers() {
    // Vulkan resources manager (to hold persistent Vulkan handles)
    m_instanceManager = std::make_shared<VkInstanceManager>();
    m_deviceManager = std::make_shared<VkDeviceManager>();

    m_coreResourcesManager = std::make_shared<VkCoreResourcesManager>(m_window, m_instanceManager.get(), m_deviceManager.get(), m_garbageCollector.get());
    ServiceLocator::RegisterService(m_coreResourcesManager);

    m_vmaAllocator = m_coreResourcesManager->getVmaAllocator();
    m_surface = m_coreResourcesManager->getSurface();

    m_physicalDevice = m_coreResourcesManager->getPhysicalDevice();
    m_deviceProperties = m_coreResourcesManager->getDeviceProperties();

    m_logicalDevice = m_coreResourcesManager->getLogicalDevice();
    m_queueFamilies = m_coreResourcesManager->getQueueFamilyIndices();


    // Swap-chain manager
    m_swapchainManager = std::make_shared<VkSwapchainManager>(m_window, m_coreResourcesManager.get());
    ServiceLocator::RegisterService(m_swapchainManager);


    // Texture manager
    m_textureManager = std::make_shared<TextureManager>(m_coreResourcesManager.get());
    ServiceLocator::RegisterService(m_textureManager);


    // Scene manager
    m_sceneManager = std::make_shared<SceneManager>();
    ServiceLocator::RegisterService(m_sceneManager);
    m_sceneManager->init();


    // GUI management
        // Workspace
        // TODO: Create plugin manager and workspace systems to allow for dynamic workspace discovery and swapping at runtime
    m_currentWorkspace = std::make_unique<OrbitalWorkspace>();    // The default orbital workspace will be used for now

    // Manager
    m_uiPanelManager = std::make_shared<UIPanelManager>(m_currentWorkspace.get());
    ServiceLocator::RegisterService(m_uiPanelManager);


    // Camera
    //glm::dvec3 cameraPosition = glm::vec3(8e8, (PhysicsConsts::AU + 10e8), 1e7);
    m_globalRegistry = ServiceLocator::GetService<Registry>(__FUNCTION__);
    Entity cameraEntity = m_globalRegistry->createEntity("Camera");
    glm::dvec3 cameraPosition = glm::vec3(0.0, 1.3e8, 0.0);

    m_camera = std::make_shared<Camera>(
        cameraEntity,
        m_window,
        cameraPosition,
        glm::quat(1, 0.0, 0.0, 0.0)
    );
    ServiceLocator::RegisterService(m_camera);


    // Input manager
    m_inputManager = std::make_shared<InputManager>();
    ServiceLocator::RegisterService(m_inputManager);
    m_inputManager->init();

    g_callbackContext.inputManager = m_inputManager.get();
}


void Engine::initEngine() {
    // Command manager
    m_commandManager = std::make_shared<VkCommandManager>(m_coreResourcesManager.get(), m_swapchainManager.get());
    ServiceLocator::RegisterService(m_commandManager);



    // Buffer manager
    m_bufferManager = std::make_shared<VkBufferManager>(m_coreResourcesManager.get(), m_swapchainManager.get());
    ServiceLocator::RegisterService(m_bufferManager);



    // Pipelines
    m_offscreenPipeline = std::make_shared<OffscreenPipeline>(m_coreResourcesManager.get(), m_swapchainManager.get());
    ServiceLocator::RegisterService(m_offscreenPipeline);


    m_presentPipeline = std::make_shared<PresentPipeline>(m_coreResourcesManager.get(), m_swapchainManager.get());
    ServiceLocator::RegisterService(m_presentPipeline);



    // Synchronization manager
    m_syncManager = std::make_shared<VkSyncManager>(m_coreResourcesManager.get(), m_swapchainManager.get());
    ServiceLocator::RegisterService(m_syncManager);



    // Renderers
    m_uiRenderer = std::make_shared<UIRenderer>(m_window, m_presentPipeline->getRenderPass(), m_coreResourcesManager.get(), m_swapchainManager.get());
    ServiceLocator::RegisterService(m_uiRenderer);


    m_renderer = std::make_shared<Renderer>(m_coreResourcesManager.get(), m_swapchainManager.get(), m_commandManager.get(), m_syncManager.get(), m_uiRenderer.get());
    ServiceLocator::RegisterService(m_renderer);



    // Systems
    m_renderSystem = std::make_shared<RenderSystem>(m_coreResourcesManager.get(), m_uiRenderer.get());
    ServiceLocator::RegisterService(m_renderSystem);

    m_physicsSystem = std::make_shared<PhysicsSystem>();
    ServiceLocator::RegisterService(m_physicsSystem);

    m_refFrameSystem = std::make_shared<ReferenceFrameSystem>();
    ServiceLocator::RegisterService(m_refFrameSystem);


    
    // Create new session
    m_currentSession = std::make_shared<Session>(m_coreResourcesManager.get(), m_sceneManager.get(), m_physicsSystem.get(), m_refFrameSystem.get());
    ServiceLocator::RegisterService(m_currentSession);


    m_eventDispatcher->dispatch(UpdateEvent::AppIsStable{});
}


void Engine::initComponents() {
    /* Core */
    m_registry->initComponentArray<CoreComponent::Transform>();

    /* Meshes & Models */
    m_registry->initComponentArray<ModelComponent::Mesh>();
    m_registry->initComponentArray<ModelComponent::Material>();

    /* Rendering */
    m_registry->initComponentArray<RenderComponent::SceneData>();
    m_registry->initComponentArray<RenderComponent::MeshRenderable>();

    /* Physics */
    m_registry->initComponentArray<PhysicsComponent::RigidBody>();
    m_registry->initComponentArray<PhysicsComponent::OrbitingBody>();
    m_registry->initComponentArray<PhysicsComponent::ReferenceFrame>();
    m_registry->initComponentArray<PhysicsComponent::ShapeParameters>();

    /* Spacecraft */
    m_registry->initComponentArray<SpacecraftComponent::Spacecraft>();
    m_registry->initComponentArray<SpacecraftComponent::Thruster>();

    /* Telemetry */
    m_registry->initComponentArray<TelemetryComponent::RenderTransform>();
}


void Engine::setApplicationStage(Application::Stage newAppStage) {
    Log::Print(Log::T_INFO, __FUNCTION__, "Transitioning application stage from Stage " + TO_STR((int)m_currentAppStage) + " to Stage " + TO_STR((int)newAppStage) + ".");

    m_currentAppStage = newAppStage;
}


void Engine::run() {
    m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);
    m_currentSession = ServiceLocator::GetService<Session>(__FUNCTION__);
    m_renderer = ServiceLocator::GetService<Renderer>(__FUNCTION__);

    update();
}


void Engine::update() {
    using namespace Application;

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        m_eventDispatcher->processQueuedEvents();

        switch (m_currentAppStage) {
        case Stage::START_SCREEN:
            updateStartScreen();
            break;

        case Stage::LOADING_SCREEN:
            updateLoadingScreen();
            break;


        case Stage::SETUP_ORBITAL:
            updateOrbitalSetup();
            break;


        case Stage::WORKSPACE_ORBITAL:
            updateOrbitalWorkspace();
            break;
        }
    }


    // All of the operations in Renderer::drawFrame are asynchronous. That means that when we exit the main loop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
    // To fix that problem, we should wait for the logical device to finish operations before exiting mainLoop and destroying the window:
    vkDeviceWaitIdle(m_logicalDevice);
}


void Engine::updateStartScreen() {

}

void Engine::updateLoadingScreen() {

}

void Engine::updateOrbitalSetup() {

}


void Engine::updateOrbitalWorkspace() {
    // Process key input events
    m_eventDispatcher->dispatch(UpdateEvent::Input{
        .deltaTime = Time::GetDeltaTime()
    }, true);
    
    m_currentSession->update();

    glm::dvec3 floatingOrigin = m_inputManager->getCamera()->getGlobalTransform().position;
    m_renderer->update(floatingOrigin);
}
