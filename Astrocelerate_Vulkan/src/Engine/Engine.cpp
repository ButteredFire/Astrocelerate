/* Engine.cpp: Stores definitions for the Engine class.
*/

#include "Engine.hpp"



Engine::Engine(GLFWwindow *w): window(w) {
    if (!windowIsValid()) {
        throw std::runtime_error("Engine crashed: Invalid window context!");
        return;
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
    }
}
