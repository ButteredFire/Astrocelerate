/* Engine.hpp - Core engine logic.
*
* Defines the Engine class, responsible for managing the simulation loop, updating
* game state, and coordinating subsystems such as rendering.
*/

#pragma once

#include <vector>
#include <unordered_set>

#include <Engine/Renderer.hpp>
#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkPipelineManager.hpp>
#include <AppWindow.hpp>
#include <Constants.h>
#include <LoggingManager.hpp>
#include <VulkanContexts.hpp>



class Engine {
public:
	Engine(GLFWwindow *w, VulkanContext &context);
	~Engine();

	// Engine
	void run();

private:
	// Global
	GLFWwindow *window;
	VulkanContext &vkContext;

	Renderer renderer;

template<typename T>
	static inline bool isPointerValid(T *ptr) { return ptr != nullptr; };
	
	// Engine
	void update();
};
