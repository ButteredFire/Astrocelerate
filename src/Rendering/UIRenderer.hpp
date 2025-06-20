/* UIRenderer.hpp - Manages ImGui rendering.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// C++ STLs
#include <iostream>

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <iconfontcppheaders/IconsFontAwesome6.h>

// Local
#include <Core/Engine/ECS.hpp>
#include <Core/Data/Constants.h>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/GarbageCollector.hpp>

#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Data/Contexts/AppContext.hpp>

#include <Scene/GUI/UIPanelManager.hpp>

#include <Rendering/Pipelines/OffscreenPipeline.hpp>

#include <Utils/ColorUtils.hpp>
#include <Utils/Vulkan/VkDescriptorUtils.hpp>


class UIRenderer {
public:
	enum Appearance {
		IMGUI_APPEARANCE_DARK_MODE,
		IMGUI_APPEARANCE_LIGHT_MODE
	};

	UIRenderer();
	~UIRenderer();

	/* Initializes ImGui.
		@param appearance (Default: IMGUI_APPEARANCE_DARK_MODE): The default appearance.
	*/
	void initImGui(UIRenderer::Appearance appearance = Appearance::IMGUI_APPEARANCE_DARK_MODE);

	void initFonts();

	void initDockspace();


	/* Refreshes ImGui. Call this when, for instance, the swap-chain is recreated. */
	void refreshImGui();


	/* Renders ImGui windows. */
	void renderFrames(uint32_t currentFrame);


	/* Updates ImGui textures (i.e., descriptor sets). */
	void updateTextures(uint32_t currentFrame);


	/* Updates the GUI appearance.
		@param appearance (Default is set by UIRenderer::initializeImGui): The appearance to update the GUI to.
	*/
	void updateAppearance(UIRenderer::Appearance appearance);

private:
	UIRenderer::Appearance m_currentAppearance = Appearance::IMGUI_APPEARANCE_DARK_MODE;

	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	std::shared_ptr<UIPanelManager> m_uiPanelManager;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	bool m_showDemoWindow = true;
};