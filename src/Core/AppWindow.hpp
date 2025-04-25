/* AppWindow.hpp: Stores declarations for the Window class.
*/

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

class Window {
private:
	const int m_WIDTH;
	const int m_HEIGHT;
	std::string m_windowName;

	GLFWwindow *m_window;
	
public:
	Window(int width, int height, std::string windowName);
	~Window();

	inline GLFWwindow* getGLFWwindowPtr() { return m_window; };
};
