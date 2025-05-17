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


void Window::initGLFWBindings(CallbackContext* context) {
    glfwSetWindowUserPointer(m_window, static_cast<void*>(context));

    glfwSetKeyCallback(m_window, KeyCallback);

    glfwSetCursorPosCallback(m_window, MouseCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetScrollCallback(m_window, ScrollCallback);
}


void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->glfwDeferKeyInput(action, key);
}


void Window::MouseCallback(GLFWwindow* window, double posX, double posY) {
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseInput(posX, posY);
}


void Window::ScrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseScroll(deltaX, deltaY);
}
