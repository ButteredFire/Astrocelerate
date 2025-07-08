#include "InputManager.hpp"
#include <Core/Application/EventDispatcher.hpp>


InputManager::InputManager():
	m_guiIO(ImGui::GetIO()) {
	using namespace Input;

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_camera = ServiceLocator::GetService<Camera>(__FUNCTION__);

	bindEvents();

	m_keyToCamMovementBindings = {
		{GLFW_KEY_W,					CameraMovement::FORWARD},
		{GLFW_KEY_S,					CameraMovement::BACKWARD},
		{GLFW_KEY_A,					CameraMovement::LEFT},
		{GLFW_KEY_D,					CameraMovement::RIGHT},
		{GLFW_KEY_SPACE,				CameraMovement::UP},
		{GLFW_KEY_LEFT_SHIFT,			CameraMovement::DOWN}
	};

	

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void InputManager::init() {
	m_eventDispatcher->publish(Event::InputIsValid{});
}


void InputManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::UpdateInput>(
		[this](const Event::UpdateInput& event) {
			this->m_camera->update();
			this->processKeyboardInput(event.deltaTime);
		}
	);
}


bool InputManager::isViewportInputAllowed() {
	return isViewportFocused() && m_cursorLocked;
}

bool InputManager::isViewportFocused() {
	return g_appContext.Input.isViewportFocused && g_appContext.Input.isViewportHoveredOver;
}

bool InputManager::isViewportUnfocused() {
	return !isViewportFocused() && m_cursorLocked;
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

	// Unlocks the cursor when the viewport loses focus (this solves desynchronization between g_appContext.Input.isViewportFocused and m_cursorLocked)
	if (m_pressedKeys.contains(GLFW_KEY_ESCAPE) || isViewportUnfocused()) {
		m_cursorLocked = false;
		glfwSetInputMode(m_camera->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	for (const int key : m_pressedKeys) {
		if (isViewportInputAllowed()) {
			if (m_keyToCamMovementBindings.count(key))
				m_camera->processKeyboardInput(m_keyToCamMovementBindings[key], dt);
		}
	}
}

void InputManager::processMouseClicks(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && isViewportFocused()) {
		m_cursorLocked = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

	if (isViewportInputAllowed())
		m_camera->processMouseInput(deltaX, deltaY);
}


void InputManager::processMouseScroll(double deltaX, double deltaY) {
	if (isViewportInputAllowed())
		m_camera->processMouseScroll(static_cast<float>(deltaY));
}
