/* UIRenderer.hpp - Manages ImGui rendering.
*/

#pragma once

// GLFW & Vulkan
#include <glfw_vulkan.hpp>

// C++ STLs
#include <iostream>

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// Local
#include <Core/ECS.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/GarbageCollector.hpp>

#include <CoreStructs/Contexts/VulkanContext.hpp>
#include <CoreStructs/Contexts/AppContext.hpp>

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

	ImFont* m_pFont = nullptr;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	bool m_showDemoWindow = true;
};