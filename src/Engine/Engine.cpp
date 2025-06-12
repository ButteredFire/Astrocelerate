/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"


Engine::Engine(GLFWwindow *w):
    window(w) {

    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
    m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

    if (!isPointerValid(window)) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Engine crashed: Invalid window context!");
    }

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Engine::~Engine() {
    // Frees up all associated memory.
    glfwDestroyWindow(window);
    glfwTerminate();
}


void Engine::initComponents() {
    /* ModelComponents.hpp */
    m_registry->initComponentArray<ModelComponent::Mesh>();
    m_registry->initComponentArray<ModelComponent::Material>();

    /* RenderComponents.hpp */
    m_registry->initComponentArray<RenderComponent::MeshRenderable>();
    m_registry->initComponentArray<RenderComponent::GUIRenderable>();

    /* PhysicsComponents.hpp */
    m_registry->initComponentArray<PhysicsComponent::RigidBody>();
    m_registry->initComponentArray<PhysicsComponent::OrbitingBody>();

    /* WorldSpaceComponents.hpp */
    m_registry->initComponentArray<WorldSpaceComponent::Transform>();
    m_registry->initComponentArray<WorldSpaceComponent::ReferenceFrame>();

    /* TelemetryComponents.hpp */
    m_registry->initComponentArray<TelemetryComponent::RenderTransform>();
}


/* Starts the engine. */ 
void Engine::run() {
    m_physicsSystem = ServiceLocator::GetService<PhysicsSystem>(__FUNCTION__);
    m_refFrameSystem = ServiceLocator::GetService<ReferenceFrameSystem>(__FUNCTION__);

    m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);

    m_renderer = ServiceLocator::GetService<Renderer>(__FUNCTION__);

    update();
}


/* [MEMBER] Updates and processes all events */
void Engine::update() {
    double accumulator = 0.0;
    float timeScale = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glm::dvec3 floatingOrigin = m_inputManager->getCamera()->getGlobalTransform().position;

        timeScale = Time::GetTimeScale();

        Time::UpdateDeltaTime();
        double deltaTime = Time::GetDeltaTime();
        accumulator += deltaTime * timeScale;

        // Update physics
            // TODO: Implement adaptive timestepping instead of a constant TIME_STEP
        while (accumulator >= SimulationConsts::TIME_STEP) {
            const double scaledDeltaTime = SimulationConsts::TIME_STEP * timeScale;

            m_physicsSystem->update(scaledDeltaTime);
            m_refFrameSystem->updateAllFrames(floatingOrigin);

            accumulator -= scaledDeltaTime;
        }

        // Process key input events
        m_eventDispatcher->publish(Event::UpdateInput{
            .deltaTime = deltaTime
        }, true);
        

        // Update rendering
        m_renderer->update(floatingOrigin);
    }

    // All of the operations in Renderer::drawFrame are asynchronous. That means that when we exit the loop in mainLoop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
    // To fix that problem, we should wait for the logical device to finish operations before exiting mainLoop and destroying the window:
    vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);
}
