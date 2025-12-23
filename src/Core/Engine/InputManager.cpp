#include "InputManager.hpp"
#include <Core/Application/EventDispatcher.hpp>


InputManager::InputManager() {
	using namespace Input;

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_camera = ServiceLocator::GetService<Camera>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void InputManager::init() {
	m_eventDispatcher->dispatch(InitEvent::InputManager{});
}


void InputManager::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<InputManager>();

	//m_eventDispatcher->subscribe<UpdateEvent::Input>(selfIndex,
	//	[this](const UpdateEvent::Input& event) {
	//		this->processKeyboardInput(event.deltaTime);
	//
	//		this->m_camera->update(event.timeSinceLastPhysicsUpdate);
	//	}
	//);


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_INIT:
				this->m_camera->reset();
			}
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::CoreResources>(selfIndex,
		[this](const UpdateEvent::CoreResources &event) {
			if (event.window != nullptr)
				m_window = event.window;
		}
	);
}


void InputManager::tick() {
	processKeyboardInput(Time::GetDeltaTime());

	// If camera is in Orbital mode and the mouse button is down (i.e., m_cursorLocked is True), change the cursor icon
	if (isCameraOrbiting() && m_cursorLocked.load())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	// By default, ImGui automatically sets the cursor to Arrow so we don't need to have an `else` here
}


void InputManager::processInBackground() {
	unfocusViewport();
}


bool InputManager::isViewportInputAllowed() {
	return isViewportFocused() && m_cursorLocked.load();
}

bool InputManager::isViewportFocused() {
	std::lock_guard<std::recursive_mutex> lock(m_appContextMutex);
	return g_appContext.Input.isViewportFocused && g_appContext.Input.isViewportHoveredOver;
}

bool InputManager::isViewportUnfocused() {
	return !isViewportFocused() && m_cursorLocked.load();
}


bool InputManager::isViewportHoveredOver() {
	std::lock_guard<std::recursive_mutex> lock(m_appContextMutex);
	return g_appContext.Input.isViewportHoveredOver;
}


bool InputManager::isCameraOrbiting() {
	return !m_camera->inFreeFlyMode();
}


void InputManager::glfwDeferKeyInput(int key, int scancode, int action, int mods) {
	std::lock_guard lock(m_pressedKeysMutex);

	if (action == GLFW_PRESS)			m_pressedKeys.insert(key);
	else if (action == GLFW_RELEASE)	m_pressedKeys.erase(key);
}


void InputManager::processKeyboardInput(double dt) {
	using namespace Input;

	//std::lock_guard lock(m_pressedKeysMutex);

	for (const int key : m_pressedKeys) {
		if (isViewportInputAllowed())
			m_camera->processKeyboardInput(key, dt);

		if (key == GLFW_KEY_ESCAPE)
			unfocusViewport();
	}
}

void InputManager::processMouseClicks(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && isViewportHoveredOver()) {
		if (isCameraOrbiting()) {
			// If camera is orbiting something, only temporarily lock the cursor and change its icon (see InputManager::tick)
			if (action == GLFW_PRESS) {
				m_cursorLocked.store(true);
			}
			else if (action == GLFW_RELEASE) {
				unfocusViewport();
			}
		}

		else {
			// If camera is not orbiting (i.e., in free-fly mode), lock and hide the cursor until the Escape key is pressed
			m_cursorLocked.store(true);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
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


void InputManager::unfocusViewport() {
	m_cursorLocked.store(false);
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
