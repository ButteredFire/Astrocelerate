#include "InputManager.hpp"
#include <Core/EventDispatcher.hpp>


InputManager::InputManager() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_camera = ServiceLocator::GetService<Camera>(__FUNCTION__);

	bindEvents();

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void InputManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::UpdateInput>(
		[this](const Event::UpdateInput& event) {
			this->processKeyboardInput(event.deltaTime);
		}
	);
}


void InputManager::glfwDeferKeyInput(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		m_pressedKeys.insert(key);
	}
	else if (action == GLFW_RELEASE) {
		m_pressedKeys.erase(key);
	}
}


void InputManager::processKeyboardInput(double dt) {
	using namespace Input;

	for (const int key : m_pressedKeys) {
		// This only happens when the cursor is locked.
		// When the cursor is not locked, such keys can be bound to other actions.
		if (m_cursorLocked) {
			if (key == GLFW_KEY_W) {
				m_camera->processKeyboardInput(CameraMovement::FORWARD, dt);
			}

			if (key == GLFW_KEY_S) {
				m_camera->processKeyboardInput(CameraMovement::BACKWARD, dt);
			}

			if (key == GLFW_KEY_A) {
				m_camera->processKeyboardInput(CameraMovement::LEFT, dt);
			}

			if (key == GLFW_KEY_D) {
				m_camera->processKeyboardInput(CameraMovement::RIGHT, dt);
			}

			if (key == GLFW_KEY_SPACE) {
				m_camera->processKeyboardInput(CameraMovement::UP, dt);
			}

			if (key == GLFW_KEY_LEFT_SHIFT) {
				m_camera->processKeyboardInput(CameraMovement::DOWN, dt);
			}
		}
	}
}

void InputManager::processMouseClicks(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		m_cursorLocked = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		m_cursorLocked = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}


void InputManager::processMouseMovement(double dposX, double dposY) {
	static float lastX = 0.0f, lastY = 0.0f;
	static bool initialInput = true;

	float posX = static_cast<float>(dposX);
	float posY = static_cast<float>(dposY);

	if (initialInput) {
		lastX = posX;
		lastY = posY;

		initialInput = false;
	}

	/* NOTE:
		For reversed input, for instance, reversed Y, set deltaY to (lastY - posY) instead of (posY - lastY).
	*/
	float deltaX = posX - lastX;
	float deltaY = posY - lastY;

	lastX = posX;
	lastY = posY;

	if (m_cursorLocked)
		m_camera->processMouseInput(deltaX, deltaY);
}


void InputManager::processMouseScroll(double deltaX, double deltaY) {
	if (m_cursorLocked)
		m_camera->processMouseScroll(static_cast<float>(deltaY));
}
