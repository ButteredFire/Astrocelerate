/* Engine.h: Stores declarations for the Engine class and engine-related definitions
*/

#pragma once

#include <vector>
#include <unordered_set>

#include "Renderer.h"
#include "../AppWindow.hpp"
#include "../Constants.h"
#include "../LoggingManager.h"

class Engine {
public:
	Engine(GLFWwindow *w);
	~Engine();

	// Engine
	void run();

private:
	// Global
	GLFWwindow* window;
	Renderer renderer;

	inline bool windowIsValid() const { return window != nullptr; };
	
	// Engine
	void update();
};
