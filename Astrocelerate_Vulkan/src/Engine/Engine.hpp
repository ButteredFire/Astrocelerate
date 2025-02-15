/* Engine.h: Stores declarations for the Engine class and engine-related definitions
*/

#pragma once

#include "../AppWindow.hpp"

class Engine {
public:
	Engine(GLFWwindow* w);
	~Engine();

	void run();

private:
	GLFWwindow* window;

	inline bool windowIsValid() const { return window != nullptr; };
	void initVulkan();
	void update();
	void cleanup();
};
