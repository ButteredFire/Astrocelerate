/* Renderer.hpp - Handles Vulkan-based rendering.
*
* Defines the Renderer class, which manages Vulkan rendering logic.
*/


#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <iostream>
#include <optional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Local
#include "../Vulkan/VkInstanceManager.hpp"
#include "../Vulkan/VkDeviceManager.hpp"
#include "../Constants.h"
#include "../LoggingManager.hpp"



class Renderer {
public:
	Renderer(VulkanContext &context);
	~Renderer();

private:
	VkInstance &vulkInst;
	VulkanContext &vkContext;
};
