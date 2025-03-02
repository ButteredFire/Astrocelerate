/* Renderer.hpp - Handles Vulkan-based rendering.
*
* Defines the Renderer class, which manages Vulkan rendering logic.
*/


#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// C++ STLs
#include <iostream>
#include <optional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Dear ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

// Local
#include "../Vulkan/VkInstanceManager.hpp"
#include "../Vulkan/VkDeviceManager.hpp"
#include "../Constants.h"
#include "../LoggingManager.hpp"



class Renderer {
public:
	Renderer(VulkanContext &context);
	~Renderer();

	/* Updates the rendering. */
	void update();

private:
	VkInstance &vulkInst;
	VulkanContext &vkContext;
};
