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
    m_monitor = glfwGetPrimaryMonitor();

    // Fullscreen:
    // m_window = glfwCreateWindow(width, height, windowName.c_str(), m_monitor, nullptr);

    // Set width and height:
    //m_window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

    // Maximized:
    m_window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwMaximizeWindow(m_window);
}


Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}


void Window::initGLFWBindings(CallbackContext* context) {
    glfwSetWindowUserPointer(m_window, static_cast<void*>(context));

    glfwSetKeyCallback(m_window, KeyCallback);

    glfwSetCursorPosCallback(m_window, MouseCallback);

    glfwSetMouseButtonCallback(m_window, MouseBtnCallback);

    glfwSetScrollCallback(m_window, ScrollCallback);
}


void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->glfwDeferKeyInput(key, scancode, action, mods);
}


void Window::MouseCallback(GLFWwindow* window, double posX, double posY) {
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseMovement(posX, posY);
}


void Window::MouseBtnCallback(GLFWwindow* window, int button, int action, int mods) {
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseClicks(window, button, action, mods);
}


void Window::ScrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseScroll(deltaX, deltaY);
}
