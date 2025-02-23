/* Engine.cpp - Engine implementation.
*/

#include "Engine.hpp"



Engine::Engine(GLFWwindow *w, VkInstance& instance):
    window(w), vulkInst(instance), 
    deviceManager(instance),
    renderer(instance) {

    if (!isPointerValid(window)) {
        throw std::runtime_error("Engine crashed: Invalid window context!");
    }

    deviceManager.createPhysicalDevice();
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
    }
}
