/* AppWindow.hpp: Stores declarations for the Window class.
*/

#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <External/GLFWVulkan.hpp>

#include <string>
#include <iostream>

#include <Core/Application/EventDispatcher.hpp>
#include <Core/Data/Contexts/CallbackContext.hpp>


class Window {
public:
	Window(const uint32_t width, const uint32_t height, const std::string &windowName);
	~Window();

	/* Initializes the window for the splash screen. */
	void initSplashScreen();

	/* Initializes the window for the primary/main screen.
		NOTE: It is assumed that the Event Dispatcher service has been initialized.
	*/
	void initPrimaryScreen(CallbackContext *context);

	void loadDefaultHints();

	inline GLFWwindow* getGLFWwindowPtr() { return (m_mainWindow == nullptr) ? m_splashWindow : m_mainWindow; };

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void MouseCallback(GLFWwindow* window, double posX, double posY);

	static void MouseBtnCallback(GLFWwindow* window, int button, int action, int mods);

	static void ScrollCallback(GLFWwindow* window, double deltaX, double deltaY);
	
private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	const uint32_t m_WIDTH;
	const uint32_t m_HEIGHT;
	std::string m_windowName;

	GLFWmonitor *m_monitor;

	GLFWwindow *m_splashWindow;
	GLFWwindow *m_mainWindow;
};
