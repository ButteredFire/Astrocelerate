/* Engine.hpp - Core engine logic.
*
* Defines the Engine class, responsible for managing the simulation loop, updating
* game state, and coordinating subsystems such as rendering.
*/

#pragma once

#include <vector>
#include <unordered_set>

#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkCommandManager.hpp>
#include <Vulkan/VkSyncManager.hpp>

#include <Rendering/Renderer.hpp>
#include <Rendering/TextureManager.hpp>
#include <Rendering/GraphicsPipeline.hpp>

#include <Shaders/BufferManager.hpp>

#include <Core/AppWindow.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/ApplicationContext.hpp>



class Engine {
public:
	Engine(GLFWwindow *w, VulkanContext &context, Renderer& rendererInstance);
	~Engine();

	// Engine
	void run();

private:
	// Global
	GLFWwindow *window;
	VulkanContext &vkContext;

	Renderer& renderer;

template<typename T>
	static inline bool isPointerValid(T *ptr) { return ptr != nullptr; };
	
	// Engine
	void update();
};
