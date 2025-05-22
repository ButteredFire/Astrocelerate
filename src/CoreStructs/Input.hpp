/* Input.hpp - Common data pertaining to user input.
*/

#pragma once


namespace Input {
	enum class CameraMovement {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN
	};


	struct Binding {
		std::string keyName;			// The name of the input.
		std::string actionName;			// The name of the action the input is bound to.
		int keyCode;					// The GLFW key code of the input.
		int action;						// The action of the input (GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT).
		int mods;						// The modifiers of the input (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, etc.).
		bool onlyOnCursorLock;			// Is the input valid only when the cursor is locked?
	};
}