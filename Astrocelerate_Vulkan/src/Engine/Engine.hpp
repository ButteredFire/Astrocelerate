/* Engine.h: Stores declarations for the Engine class and engine-related definitions
*/

#pragma once

#include <vector>
#include <unordered_set>

#include "Renderer.hpp"
#include "../AppWindow.hpp"
#include "../Constants.h"
#include "../LoggingManager.h"

class Engine {
public:
	Engine(GLFWwindow *w, Renderer *rdr);
	~Engine();

	// Engine
	void run();

private:
	// Global
	GLFWwindow *window;
	Renderer *renderer;

template<typename T>
	inline bool isPointerValid(T *ptr) const { return ptr != nullptr; };
	
	// Engine
	void update();
};
