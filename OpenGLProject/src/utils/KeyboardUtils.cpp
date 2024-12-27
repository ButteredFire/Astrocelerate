#include "KeyboardUtils.h"

Keyboard::Keyboard() {

}

Keyboard::~Keyboard() {

}

bool Keyboard::isPressed(int glfwKeyPress) const {
	return glfwGetKey(m_Window, glfwKeyPress) == GLFW_PRESS;
}

bool Keyboard::isReleased(int glfwKeyPress) const {
	return glfwGetKey(m_Window, glfwKeyPress) == GLFW_RELEASE;
}

void Keyboard::setTargetWindow(GLFWwindow* window) {
	m_Window = window;
}