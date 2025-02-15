/* Engine.h: Stores definitions for the Engine class.
*/

#include "Engine.hpp"

Engine::Engine(GLFWwindow* w): window(w) {}

Engine::~Engine() {}

void Engine::run() {
    if (!windowIsValid()) {
        std::cerr << "Unable to run engine: Invalid window context!\n";
        return;
    }
    initVulkan();
    update();
    cleanup();
}

void Engine::initVulkan() {
    // Vulkan initialization logic
}

void Engine::update() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Engine::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
