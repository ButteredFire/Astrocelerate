/* Renderer.hpp - Handles Vulkan-based rendering.
*
* Defines the Renderer class, which manages Vulkan rendering logic.
*/


#pragma once

#include <External/GLFWVulkan.hpp>
#include <External/GLM.hpp>

#include <imgui/imgui.h>


#include <vector>
#include <barrier>
#include <iostream>
#include <optional>
#include <algorithm>
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

	void preRenderUpdate(uint32_t currentFrame, glm::dvec3 &renderOrigin);

	/* Updates the rendering. */
	void update(glm::dvec3& renderOrigin);

	/* Recreates the swap-chain. */
	void recreateSwapchain(GLFWwindow *newWindowPtr = nullptr);
	void recreateSwapchain(GLFWwindow *newWindowPtr, uint32_t imageIndex, std::vector<VkFence> &inFlightFences);

private:
	std::shared_ptr<Registry> m_globalRegistry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ResourceManager> m_resourceManager;

	VkCoreResourcesManager *m_coreResources;
	VkSwapchainManager *m_swapchainManager;
	VkCommandManager *m_commandManager;
	VkSyncManager *m_syncManager;
	UIRenderer *m_uiRenderer;


	std::optional<CleanupID> m_swapchainCleanupID;

	std::vector<VkSemaphore> m_imageReadySemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

	uint32_t m_currentFrame = 0;

	// Session data
	bool m_sessionReady = false;
	bool m_pauseUpdateLoop = false;

	const uint32_t RENDERER_THREAD_COUNT = 1 + 1; // + 1 to count the main thread, which also participates in rendering
	std::shared_ptr<std::barrier<>> m_renderThreadBarrier;


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
