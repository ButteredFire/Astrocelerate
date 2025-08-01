/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"


Engine::Engine(GLFWwindow *w):
    window(w) {

    ThreadManager::SetMainThreadID(std::this_thread::get_id());

    initPersistentServices();
    initComponents();

    bindEvents();

    LOG_ASSERT(window != nullptr, "Engine crashed: Invalid window context!");

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Engine::~Engine() {
    // Frees up all associated memory.
    glfwDestroyWindow(window);
    glfwTerminate();
}


void Engine::initPersistentServices() {
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


void Engine::bindEvents() {
    m_eventDispatcher->subscribe<Event::RegistryReset>(
        [this](const Event::RegistryReset &event) {
            this->initComponents();
        }
    );


    m_eventDispatcher->subscribe<Event::UpdateAppStage>(
        [this](const Event::UpdateAppStage &event) {
            setApplicationStage(event.newStage);
        }
    );
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

    while (!glfwWindowShouldClose(window)) {
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
    vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);
}


void Engine::updateStartScreen() {

}

void Engine::updateLoadingScreen() {

}

void Engine::updateOrbitalSetup() {

}


void Engine::updateOrbitalWorkspace() {
    // Process key input events
    m_eventDispatcher->dispatch(Event::UpdateInput{
        .deltaTime = Time::GetDeltaTime()
        }, true);
    
    // Update rendering
    m_currentSession->update();

    glm::dvec3 floatingOrigin = m_inputManager->getCamera()->getGlobalTransform().position;
    m_renderer->update(floatingOrigin);
}
