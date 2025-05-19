/* AppWindow.hpp: Stores declarations for the Window class.
*/

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <CoreStructs/Contexts.hpp>

#include <Scene/Camera.hpp>

class Window {
public:
	Window(int width, int height, std::string windowName);
	~Window();

	inline GLFWwindow* getGLFWwindowPtr() { return m_window; };

	void initGLFWBindings(CallbackContext* context);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void MouseCallback(GLFWwindow* window, double posX, double posY);

	static void MouseBtnCallback(GLFWwindow* window, int button, int action, int mods);

	static void ScrollCallback(GLFWwindow* window, double deltaX, double deltaY);
	
private:
	const int m_WIDTH;
	const int m_HEIGHT;
	std::string m_windowName;

	GLFWmonitor* m_monitor;
	GLFWwindow* m_window;
};
