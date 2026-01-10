/* UIRenderer.hpp - Manages ImGui rendering.
*/

#pragma once

#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <iconfontcppheaders/IconsFontAwesome6.h>


#include <Core/Data/Constants.h>
#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/Vulkan/Utils/VkDescriptorUtils.hpp>

#include <Engine/GUI/UIPanelManager.hpp>
#include <Engine/GUI/Data/Appearance.hpp>
#include <Engine/Utils/ColorUtils.hpp>
#include <Engine/Contexts/GUIContext.hpp>
#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Rendering/Pipelines/OffscreenPipeline.hpp>


class UIPanelManager;


class UIRenderer {
public:
	UIRenderer(GLFWwindow *window, VkRenderPass presentPipelineRenderPass, VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr);
	~UIRenderer() = default;

	/* Initializes ImGui. */
	void initImGui();

	void initFonts();

	void updateDockspace();


	/* Re-initializes ImGui. Call this when changes have occurred to core resources (e.g., GLFW window pointer). */
	void reInitImGui(GLFWwindow *window = nullptr);

	/* Refreshes ImGui. Call this when, for instance, the swap-chain is recreated. */
	void refreshImGui();


	/* Renders ImGui windows. */
	void renderFrames(uint32_t currentFrame);


	/* Any updates that cannot happen while the command buffer is currently being processed (e.g., updating descriptor sets) must happen here. */
	void preRenderUpdate(uint32_t currentFrame);

private:
	std::shared_ptr<ECSRegistry> m_ecsRegistry;
	std::shared_ptr<CleanupManager> m_cleanupManager;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	std::shared_ptr<UIPanelManager> m_uiPanelManager;

	CleanupID m_imguiCleanupID{};

	GLFWwindow *m_window;
	VkRenderPass m_presentPipelineRenderPass;
	VkInstance m_instance;
	QueueFamilyIndices m_queueFamilies;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	uint32_t m_minImageCount;


	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	bool m_showDemoWindow = true;


	void bindEvents();
};