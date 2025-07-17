/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"


Engine::Engine(GLFWwindow *w):
    window(w) {

    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
    m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

    LOG_ASSERT(window != nullptr, "Engine crashed: Invalid window context!");

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Engine::~Engine() {
    // Frees up all associated memory.
    glfwDestroyWindow(window);
    glfwTerminate();
}


void Engine::bindEvents() {
    m_eventDispatcher->subscribe<Event::UpdateAppStage>(
        [this](const Event::UpdateAppStage &event) {
            setApplicationStage(event.newStage);
        }
    );
}


void Engine::initComponents() {
    /* Common */
    m_registry->initComponentArray<CommonComponent::Transform>();

    /* Meshes & Models */
    m_registry->initComponentArray<ModelComponent::Mesh>();
    m_registry->initComponentArray<ModelComponent::Material>();

    /* Rendering */
    m_registry->initComponentArray<RenderComponent::SceneData>();
    m_registry->initComponentArray<RenderComponent::MeshRenderable>();
    m_registry->initComponentArray<RenderComponent::GUIRenderable>();

    /* Physics */
    m_registry->initComponentArray<PhysicsComponent::RigidBody>();
    m_registry->initComponentArray<PhysicsComponent::OrbitingBody>();
    m_registry->initComponentArray<PhysicsComponent::ReferenceFrame>();
    m_registry->initComponentArray<PhysicsComponent::ShapeParameters>();

    /* Telemetry */
    m_registry->initComponentArray<TelemetryComponent::RenderTransform>();
}


void Engine::setApplicationStage(Application::Stage newAppStage) {
    Log::Print(Log::T_INFO, __FUNCTION__, "Transitioning application stage from Stage " + TO_STR((int)m_currentAppStage) + " to Stage " + TO_STR((int)newAppStage) + ".");

    m_currentAppStage = newAppStage;
}


void Engine::run() {
    m_physicsSystem = ServiceLocator::GetService<PhysicsSystem>(__FUNCTION__);
    m_refFrameSystem = ServiceLocator::GetService<ReferenceFrameSystem>(__FUNCTION__);

    m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);

    m_renderer = ServiceLocator::GetService<Renderer>(__FUNCTION__);

    update();
}


void Engine::update() {
    using namespace Application;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();


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
    static double accumulator = 0.0;
    static float timeScale = 0;

    static bool initialUpdate = false;
    if (!initialUpdate) {
        initialUpdate = true;
        m_refFrameSystem->updateAllFrames();
    }


    timeScale = Time::GetTimeScale();

    Time::UpdateDeltaTime();
    double deltaTime = Time::GetDeltaTime();
    accumulator += deltaTime * timeScale;

    // Process key input events
    m_eventDispatcher->publish(Event::UpdateInput{
        .deltaTime = deltaTime
        }, true);


    // Update physics
        // TODO: Implement adaptive timestepping instead of a constant TIME_STEP
    while (accumulator >= SimulationConsts::TIME_STEP) {
        const double scaledDeltaTime = SimulationConsts::TIME_STEP * timeScale;

        m_physicsSystem->update(scaledDeltaTime);
        m_refFrameSystem->updateAllFrames();

        accumulator -= scaledDeltaTime;
    }


    // Update rendering
    glm::dvec3 floatingOrigin = m_inputManager->getCamera()->getGlobalTransform().position;
    m_renderer->update(floatingOrigin);
}
