/* Renderer.hpp - Handles Vulkan-based rendering.
*
* Defines the Renderer class, which manages Vulkan rendering logic.
*/


#pragma once

#include <External/GLFWVulkan.hpp>
#include <External/GLM.hpp>

#include <imgui/imgui.h>


#include <iostream>
#include <optional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>


#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/Constants.h>
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Rendering/UIRenderer.hpp>

#include <Vulkan/VkSyncManager.hpp>
#include <Vulkan/VkCommandManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkCoreResourcesManager.hpp>



class Renderer {
public:
	Renderer(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr, VkCommandManager *commandMgr, VkSyncManager *syncMgr, UIRenderer *uiRenderer);
	~Renderer();

	/* Updates the rendering. */
	void update(glm::dvec3& renderOrigin);

	void preRenderUpdate(uint32_t currentFrame, glm::dvec3 &renderOrigin);

private:
	std::shared_ptr<Registry> m_globalRegistry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	VkCoreResourcesManager *m_coreResources;
	VkSwapchainManager *m_swapchainManager;
	VkCommandManager *m_commandManager;
	VkSyncManager *m_syncManager;
	UIRenderer *m_uiRenderer;


	std::vector<VkSemaphore> m_imageReadySemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

	uint32_t m_currentFrame = 0;

	// Session data
	bool m_sessionReady = false;

	void bindEvents();

	void init();

	/* Renders a frame. 
		At a high level, rendering a frame in Vulkan consists of a common set of steps:
			- Wait for the previous frame to finish
			- Acquire an image from the swap chain
			- Record a command buffer which draws the scene onto that image
			- Submit the recorded command buffer
			- Present the swap chain image
	*/
	void drawFrame(glm::dvec3& renderOrigin);
};
