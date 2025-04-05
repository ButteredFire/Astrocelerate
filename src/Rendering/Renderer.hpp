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
#include <ServiceLocator.hpp>

// Font
#include <../assets/DefaultFont.hpp>


/* Converts a color channel value in sRGB space to an equivalent value in linear space.
* @param color: The sRGB color value.
* @return An equivalent value in linear color space.
*/
static inline float sRGBToLinear(float color) {
	return (color <= Gamma::THRESHOLD) ? (color / Gamma::DIVISOR) : powf((color + Gamma::OFFSET) / Gamma::SCALE, Gamma::EXPONENT);
}


/* Converts a set of sRGB values to an equivalent set of linear color space values. 
* @param r: The Red channel.
* @param g: The Green channel.
* @param b: The Blue channel.
* @param a (Default: 1.0): The Alpha channel.
* @return A 4-component ImGui Vector containing the (r, g, b, a) set in linear color space.
*/
static inline ImVec4 linearRGBA(float r, float g, float b, float a = 1.0f) {
	return ImVec4(sRGBToLinear(r), sRGBToLinear(g), sRGBToLinear(b), a);
}


class Renderer {
public:
	Renderer(VulkanContext &context);
	~Renderer();

	void init();

	/* Updates the rendering. */
	void update();

private:
	VkInstance &vulkInst;
	VulkanContext &vkContext;

	std::shared_ptr<VkSwapchainManager> swapchainManager;
	std::shared_ptr<BufferManager> bufferManager;

	std::shared_ptr<GraphicsPipeline> graphicsPipeline;
	std::shared_ptr<RenderPipeline> renderPipeline;

	uint32_t currentFrame = 0;

	ImFont* font = nullptr;
	VkDescriptorPool imgui_DescriptorPool = VK_NULL_HANDLE;
	bool showDemoWindow = true;


	/* Configures Dear Imgui */
	void configureDearImGui();


	/* Changes the Imgui appearance to have a custom dark theme. */
	void imgui_SetDarkTheme();


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
