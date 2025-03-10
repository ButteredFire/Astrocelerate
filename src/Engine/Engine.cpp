/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"



Engine::Engine(GLFWwindow *w, VulkanContext& context):
    window(w),
    vkContext(context),
    renderer(context) {

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
}
