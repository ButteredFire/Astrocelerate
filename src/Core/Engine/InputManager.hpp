/* InputManager.hpp - Handles user input.
*/

#pragma once

#include <mutex>
#include <unordered_set>

#include <imgui/imgui.h>

#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/LoggingManager.hpp>


#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Data/Input.hpp>

#include <Scene/Camera.hpp>


class EventDispatcher;


class InputManager {
public:
	InputManager();
	~InputManager() = default;


	/* Gets a pointer to the Camera instance. */
	inline Camera* getCamera() {
		return m_camera.get();
	}

	void init();

	/* Updates the input manager.
		@param deltaTime: Delta-time.
		@param deltaUpdate: The time difference between now and the last physics update. This allows for linear movement interpolation.
	*/
	void tick(double deltaTime, double deltaUpdate);


	/* Run this function when the window is not in focus. */
	void processInBackground();


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
	bool isViewportHoveredOver();

	bool isCameraOrbiting();

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<Camera> m_camera;

	GLFWwindow *m_window;

	std::unordered_set<int> m_pressedKeys;

	std::mutex m_pressedKeysMutex;
	std::recursive_mutex m_appContextMutex;

	std::atomic<double> m_deltaUpdate = 0.0;  // Time between now and the last physics update
	std::atomic<bool> m_cursorLocked = false;

	void bindEvents();

	void unfocusViewport();
};