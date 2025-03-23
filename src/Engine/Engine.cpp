/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"



Engine::Engine(GLFWwindow *w, VulkanContext& context, Renderer& rendererInstance):
    window(w),
    vkContext(context),
    renderer(rendererInstance) {

    Log::print(Log::INFO, __FUNCTION__, "Initializing...");

    if (!isPointerValid(window)) {
        throw std::runtime_error("Engine crashed: Invalid window context!");
    }
}


Engine::~Engine() {
    // Frees up all associated memory.
    glfwDestroyWindow(window);
    glfwTerminate();
}


/* Starts the engine. */ 
void Engine::run() {
    update();
}


/* [MEMBER] Updates and processes all events */
void Engine::update() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.update();
    }

    // All of the operations in Renderer::drawFrame are asynchronous. That means that when we exit the loop in mainLoop, drawing and presentation operations may still be going on. Cleaning up resources while that is happening is a bad idea.
    // To fix that problem, we should wait for the logical device to finish operations before exiting mainLoop and destroying the window :
    vkDeviceWaitIdle(vkContext.logicalDevice);
}
