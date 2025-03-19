/* AppWindow.cpp: Stores definitions of the methods in the Window class.
*/

#include "AppWindow.hpp"

Window::Window(int width, int height, std::string windowName)
    : WIDTH(width), HEIGHT(height), windowName(windowName) {

    glfwInit();
    // Explicitly tell GLFW not to create an OpenGL context (since we're using Vulkan)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Disables window from being resized post-creation (to support custom window resizing logic)
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}




