#pragma once

#ifndef KEYBOARD_UTILS_H
#define KEYBOARD_UTILS_H

#include <GLFW/glfw3.h>

class Keyboard {
private:
	GLFWwindow* m_Window;

public:
	Keyboard();
	~Keyboard();

	bool isPressed(int glfwKeyPress) const;
	bool isReleased(int glfwKeyPress) const;

	void setTargetWindow(GLFWwindow* window);
};

#endif