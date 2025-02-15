/* AppWindow.hpp: Stores declarations for the Window class.
*/

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

class Window {
private:
	const int WIDTH;
	const int HEIGHT;
	std::string windowName;

	GLFWwindow* window;
	
public:
	Window(int width, int height, std::string windowName);
	~Window();

	inline GLFWwindow* getGLFWwindowPtr() { return window; };
};
