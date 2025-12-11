/* AppWindow.cpp: Stores definitions of the methods in the Window class.
*/

#include "AppWindow.hpp"
#include <Core/Engine/InputManager.hpp>

Window::Window(const uint32_t width, const uint32_t height, const std::string &windowName)
    : m_WIDTH(width), m_HEIGHT(height), m_windowName(windowName) {

    glfwInit();
    m_monitor = glfwGetPrimaryMonitor();

    loadDefaultHints();
}


Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::initSplashScreen() {
    loadDefaultHints();

    // Make window borderless
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

    constexpr uint32_t SPLASH_WIDTH = 700;
    constexpr uint32_t SPLASH_HEIGHT = 400;

    m_window = glfwCreateWindow(SPLASH_WIDTH, SPLASH_HEIGHT, "Astrocelerate | Loading...", nullptr, nullptr);

    const GLFWvidmode *mode = glfwGetVideoMode(m_monitor);
    const uint32_t CURRENT_SCREEN_WIDTH = mode->width;
    const uint32_t CURRENT_SCREEN_HEIGHT = mode->height;

    // Center window
    const uint32_t X = (CURRENT_SCREEN_WIDTH - SPLASH_WIDTH) / 2;
    const uint32_t Y = (CURRENT_SCREEN_HEIGHT - SPLASH_HEIGHT) / 2;
    glfwSetWindowPos(m_window, X, Y);

    //std::cout << std::showbase << std::hex << "Splash window: " << m_window << '\n';
}


void Window::initPrimaryScreen(CallbackContext *context) {
    // Initialize window
    glfwDestroyWindow(m_window);

    loadDefaultHints();

        // Fullscreen:
            // m_window = glfwCreateWindow(width, height, windowName.c_str(), m_monitor, nullptr);
        // Set width and height:
            //m_window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    m_window = glfwCreateWindow(m_WIDTH, m_HEIGHT, m_windowName.c_str(), nullptr, nullptr);
    glfwMaximizeWindow(m_window);
    glfwMakeContextCurrent(m_window);
    glfwFocusWindow(m_window);


    // Initialize GLFW bindings
    glfwSetWindowUserPointer(m_window, static_cast<void *>(context));

    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetCursorPosCallback(m_window, MouseCallback);
    glfwSetMouseButtonCallback(m_window, MouseBtnCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);


    // Dispatch update event
    //std::cout << std::showbase << std::hex << "Main window: " << m_window << '\n';
    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
    m_eventDispatcher->dispatch(UpdateEvent::CoreResources{
        .window = m_window
    });
}


void Window::loadDefaultHints() {
    // Explicitly tell GLFW not to create an OpenGL context (since we're using Vulkan)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);  // GLFW_DECORATED: Should window decorations (e.g., border, minimize/maximize/close widgets) be shown?
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);  // GLFW_RESIZABLE: Should the window be resizable?
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);    // GLFW_VISIBLE: Should the windowed mode window be initially visible?
    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);  // GLFW_FLOATING: Should the window always be on top?
}


void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //std::cout << "key callback\n";
	CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->glfwDeferKeyInput(key, scancode, action, mods);

    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED))
        // If window is not focused, stop processing input when it's in the background
        context->inputManager->processInBackground();
}


void Window::MouseCallback(GLFWwindow* window, double posX, double posY) {
    //std::cout << "mouse callback\n";
    CallbackContext *context = static_cast<CallbackContext *>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseMovement(posX, posY);
}


void Window::MouseBtnCallback(GLFWwindow* window, int button, int action, int mods) {
    //std::cout << "mouse btn callback\n";
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseClicks(window, button, action, mods);
}


void Window::ScrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
    //std::cout << "scroll callback\n";
    CallbackContext* context = static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    context->inputManager->processMouseScroll(deltaX, deltaY);
}
