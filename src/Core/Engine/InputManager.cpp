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


void InputManager::tick(double deltaTime, double deltaUpdate) {
	processKeyboardInput(deltaTime);
	m_deltaUpdate.store(deltaUpdate);
	//m_camera->update(deltaUpdate);
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

	std::lock_guard lock(m_pressedKeysMutex);

	// Unlocks the cursor when the viewport loses focus (this solves desynchronization between g_appContext.Input.isViewportFocused and m_cursorLocked)
	if (m_pressedKeys.contains(GLFW_KEY_ESCAPE) || isViewportUnfocused())
		unfocusViewport();

	for (const int key : m_pressedKeys) {
		if (isViewportInputAllowed())
			m_camera->processKeyboardInput(key, dt);
	}
}

void InputManager::processMouseClicks(GLFWwindow* window, int button, int action, int mods) {
	// Create a hand cursor and schedule it for destruction
	static std::function<GLFWcursor *()> createHandCursor = [this]() {
		std::shared_ptr<ResourceManager> gc = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);
		GLFWcursor *handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
		
		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(handCursor) };
		task.cleanupFunc = [handCursor]() { glfwDestroyCursor(handCursor); };
		gc->createCleanupTask(task);

		return handCursor;
	};
	static GLFWcursor *handCursor = createHandCursor();
	

	std::cout << std::boolalpha << (button == GLFW_MOUSE_BUTTON_LEFT) << " && " << isViewportFocused() << '\n';
	if (button == GLFW_MOUSE_BUTTON_LEFT && isViewportFocused()) {
		// If camera is orbiting something, only temporarily lock the cursor and (TODO) change its icon
		if (isCameraOrbiting()) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			if (action == GLFW_PRESS) {
				m_cursorLocked.store(true);
				//glfwSetCursor(window, handCursor);
			}
			else if (action == GLFW_RELEASE) {
				m_cursorLocked.store(false);
				//glfwSetCursor(window, NULL);
			}
		}


		// If camera is not orbiting (i.e., in free-fly mode), lock and hide the cursor until the Escape key is pressed
		else {
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
