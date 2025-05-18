/* InputManager.hpp - Handles user input.
*/

#pragma once

#include <set>

#include <Core/ServiceLocator.hpp>
#include <Core/LoggingManager.hpp>

#include <CoreStructs/Input.hpp>

#include <Scene/Camera.hpp>


class EventDispatcher;


class InputManager {
public:
	InputManager();
	~InputManager() = default;


	/* GLFW keyboard input: Defer input processing to update loop.
		GLFW only invokes the key callback function when a key input event happens, not per-frame.
		This means that directly manipulating the application in the callback function (i.e., updating movement in simulation space) is very jittery. Even if delta-time is used, such manipulation cannot synchronize with the update loop.

		The solution is to keep track which keys are pressed, and only process those key presses in the update loop. In other words, the solution is to defer key input processing until the update loop is run.
	*/
	void glfwDeferKeyInput(int action, int key);


	/* Processes keyboard inputs. */
	void processKeyboardInput(double dt);


	/* Processes mouse input. */
	void processMouseInput(double posX, double posY);


	/* Processes mouse scroll. */
	void processMouseScroll(double deltaX, double deltaY);

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	std::shared_ptr<Camera> m_camera;
	std::set<int> m_pressedKeys;


	void bindEvents();
};