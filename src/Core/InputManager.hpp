/* InputManager.hpp - Handles user input.
*/

#pragma once

#include <set>

#include <imgui/imgui.h>

#include <Core/ServiceLocator.hpp>
#include <Core/LoggingManager.hpp>

#include <CoreStructs/Contexts.hpp>
#include <CoreStructs/Input.hpp>

#include <Scene/Camera.hpp>


class EventDispatcher;


class InputManager {
public:
	InputManager(AppContext& appContext);
	~InputManager() = default;


	/* Gets a pointer to the Camera instance. */
	inline Camera* getCamera() {
		return m_camera.get();
	}

	void init();


	/* GLFW keyboard input: Defer input processing to update loop.
		GLFW only invokes the key callback function when a key input event happens, not per-frame.
		This means that directly manipulating the application in the callback function (i.e., updating movement in simulation space) is very jittery. Even if delta-time is used, such manipulation cannot synchronize with the update loop.

		The solution is to keep track which keys are pressed, and only process those key presses in the update loop. In other words, the solution is to defer key input processing until the update loop is run.
	*/
	void glfwDeferKeyInput(int key, int scancode, int action, int mods);


	/* Processes keyboard input. */
	void processKeyboardInput(double dt);


	/* Processes mouse clicks. */
	void processMouseClicks(GLFWwindow* window, int button, int action, int mods);


	/* Processes mouse movement. */
	void processMouseMovement(double posX, double posY);


	/* Processes mouse scroll. */
	void processMouseScroll(double deltaX, double deltaY);


	bool isViewportInputAllowed();
	bool isViewportFocused();
	bool isViewportUnfocused();

private:
	AppContext& m_appContext;

	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<Camera> m_camera;

	std::set<int> m_pressedKeys;

	std::unordered_map<int, Input::CameraMovement> m_keyToCamMovementBindings;

	ImGuiIO& m_guiIO;

	bool m_cursorLocked = false;

	void bindEvents();
};