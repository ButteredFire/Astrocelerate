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

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imconfig.h>
#include <imgui/imgui_internal.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_truetype.h>
#include <imgui/imstb_textedit.h>

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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
