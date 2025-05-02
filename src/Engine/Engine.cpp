/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"



Engine::Engine(GLFWwindow *w, VulkanContext& context):
    window(w),
    m_vkContext(context) {

    m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);

    if (!isPointerValid(window)) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Engine crashed: Invalid window context!");
    }

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Engine::~Engine() {
    // Frees up all associated memory.
    glfwDestroyWindow(window);
    glfwTerminate();
}


void Engine::initComponents() {
    /* ModelComponents.hpp */
    m_registry->initComponentArray<Component::Mesh>();
    m_registry->initComponentArray<Component::Material>();

    /* RenderComponents.hpp */
    m_registry->initComponentArray<Component::Renderable>();

    /* PhysicsComponents.hpp */
    m_registry->initComponentArray<Component::RigidBody>();
}


/* Starts the engine. */ 
void Engine::run() {
    m_renderer = ServiceLocator::getService<Renderer>(__FUNCTION__);
    update();
}


/* [MEMBER] Updates and processes all events */
void Engine::update() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        Time::UpdateDeltaTime();
        
        m_renderer->update();
    }

    // All of the operations in Renderer::drawFrame are asynchronous. That means that when we exit the loop in mainLoop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
    // To fix that problem, we should wait for the logical device to finish operations before exiting mainLoop and destroying the window :
    vkDeviceWaitIdle(m_vkContext.Device.logicalDevice);
}
