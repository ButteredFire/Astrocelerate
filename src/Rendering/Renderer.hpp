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
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// Local
#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Rendering/RenderPipeline.hpp>
#include <Rendering/GraphicsPipeline.hpp>
#include <Shaders/BufferManager.hpp>
#include <Constants.h>
#include <LoggingManager.hpp>

// Font
#include <../assets/DefaultFont.hpp>



class Renderer {
public:
	Renderer(VulkanContext &context, VkSwapchainManager& swapchainMgr, BufferManager& bufMgr, GraphicsPipeline& graphicsPipelineInstance, RenderPipeline& renderPipelineInstance);
	~Renderer();

	void init();

	/* Updates the rendering. */
	void update();

private:
	VkInstance &vulkInst;
	VulkanContext &vkContext;

	VkSwapchainManager& swapchainManager;
	BufferManager& bufferManager;

	GraphicsPipeline& graphicsPipeline;
	RenderPipeline& renderPipeline;

	uint32_t currentFrame = 0;

	ImFont* font = nullptr;
	VkDescriptorPool imgui_DescriptorPool = VK_NULL_HANDLE;
	bool showDemoWindow = true;


	/* Configures Dear Imgui */
	void configureDearImGui();


	/* Reconfigures Dear Imgui on swapchain recreation. */
	void refreshDearImgui();


	/* Renders the Imgui Demo Window */
	void imgui_RenderDemoWindow();


	/* Renders a frame. 
	* At a high level, rendering a frame in Vulkan consists of a common set of steps:
	*	- Wait for the previous frame to finish
	*	- Acquire an image from the swap chain
	*	- Record a command buffer which draws the scene onto that image
	*	- Submit the recorded command buffer
	*	- Present the swap chain image
	*/
	void drawFrame();
};
