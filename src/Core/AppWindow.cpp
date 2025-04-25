/* AppWindow.cpp: Stores definitions of the methods in the Window class.
*/

#include "AppWindow.hpp"

Window::Window(int width, int height, std::string windowName)
    : m_WIDTH(width), m_HEIGHT(height), m_windowName(windowName) {

    glfwInit();
    // Explicitly tell GLFW not to create an OpenGL context (since we're using Vulkan)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Disables window from being resized post-creation (to support custom window resizing logic)
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}




