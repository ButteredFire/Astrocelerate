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


#include <Core/Data/Contexts/AppContext.hpp>

#include <Scene/GUI/UIPanelManager.hpp>
#include <Scene/GUI/Appearance.hpp>

#include <Rendering/Pipelines/OffscreenPipeline.hpp>

#include <Utils/ColorUtils.hpp>
#include <Utils/Vulkan/VkDescriptorUtils.hpp>

class UIPanelManager;


class UIRenderer {
public:
	UIRenderer(GLFWwindow *window, VkRenderPass presentPipelineRenderPass, VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr);
	~UIRenderer();

	/* Initializes ImGui. */
	void initImGui();

	void initFonts();

	void updateDockspace();


	/* Refreshes ImGui. Call this when, for instance, the swap-chain is recreated. */
	void refreshImGui();


	/* Renders ImGui windows. */
	void renderFrames(uint32_t currentFrame);


	/* Any updates that cannot happen while the command buffer is currently being processed (e.g., updating descriptor sets) must happen here. */
	void preRenderUpdate(uint32_t currentFrame);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	std::shared_ptr<UIPanelManager> m_uiPanelManager;

	GLFWwindow *m_window;
	VkRenderPass m_presentPipelineRenderPass;
	VkInstance m_instance;
	QueueFamilyIndices m_queueFamilies;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	uint32_t m_minImageCount;


	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	bool m_showDemoWindow = true;
};