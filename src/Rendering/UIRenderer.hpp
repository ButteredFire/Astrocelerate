/* UIRenderer.hpp - Manages ImGui rendering.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <iostream>

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// Local
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/ApplicationContext.hpp>

#include <Rendering/GraphicsPipeline.hpp>

#include <Utils/ColorUtils.hpp>

// Font
#include <../assets/DefaultFont.hpp>


class UIRenderer {
public:
	enum Appearance {
		IMGUI_APPEARANCE_DARK_MODE,
		IMGUI_APPEARANCE_LIGHT_MODE
	};

	UIRenderer(VulkanContext& context);
	~UIRenderer();

	/* Initializes ImGui.
		@param appearance (Default: IMGUI_APPEARANCE_DARK_MODE): The default appearance.
	*/
	void initializeImGui(UIRenderer::Appearance appearance = Appearance::IMGUI_APPEARANCE_DARK_MODE);


	/* Refreshes ImGui. Call this when, for instance, the swap-chain is recreated. */
	void refreshImGui();


	/* Renders ImGui windows. */
	void renderFrames();

	/* Updates the GUI appearance.
		@param appearance (Default is set by UIRenderer::initializeImGui): The appearance to update the GUI to.
	*/
	void updateAppearance(UIRenderer::Appearance appearance);

private:
	VulkanContext& vkContext;
	UIRenderer::Appearance currentAppearance = Appearance::IMGUI_APPEARANCE_DARK_MODE;

	std::shared_ptr<GarbageCollector> garbageCollector;
	std::shared_ptr<GraphicsPipeline> graphicsPipeline;

	ImFont* font = nullptr;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	bool showDemoWindow = true;
};