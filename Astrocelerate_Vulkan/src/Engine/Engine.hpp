/* Engine.hpp - Core engine logic.
*
* Defines the Engine class, responsible for managing the simulation loop, updating
* game state, and coordinating subsystems such as rendering and device management.
*/

#pragma once

#include <vector>
#include <unordered_set>

#include "Renderer.hpp"
#include "../Vulkan/VkInstanceManager.hpp"
#include "../Vulkan/VkDeviceManager.hpp"
#include "../AppWindow.hpp"
#include "../Constants.h"
#include "../LoggingManager.hpp"



class Engine {
public:
	Engine(GLFWwindow *w, VkInstance &instance);
	~Engine();

	// Engine
	void run();

private:
	// Global
	GLFWwindow *window;
	VkInstance &vulkInst;

	Renderer renderer;
	VkDeviceManager deviceManager;


template<typename T>
	static inline bool isPointerValid(T *ptr) { return ptr != nullptr; };
	
	// Engine
	void update();
};
