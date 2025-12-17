/* VkSyncManager.hpp - Manages device and host synchronization.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// C++ STLs
#include <iostream>

// Local

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/ResourceManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Data/Constants.h>

#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkCoreResourcesManager.hpp>


class VkSyncManager {
public:
	VkSyncManager(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr);
	~VkSyncManager();


	inline std::vector<VkSemaphore> getImageReadySemaphores() const { return m_imageReadySemaphores; }
	inline std::vector<VkSemaphore> getRenderFinishedSemaphores() const { return m_renderFinishedSemaphores; }
	inline std::vector<VkFence> getInFlightFences() const { return m_inFlightFences; }

	void init();

	/* Creates a single-use fence.
		@param logicalDevice: The current logical device.
		@param signaled (Default: False): A boolean specifying whether the fence's initial state should be signaled (True), or not (False).

		@return The fence in question.
	*/
	static VkFence CreateSingleUseFence(VkDevice logicalDevice, bool signaled = false);


	/* Waits for a single-use fence to be signaled. After waiting, the fence will be destroyed.
		@param logicalDevice: The current logical device.
		@param fence: The fence in question.
		@param timeout: The fence wait time.
	*/
	static void WaitForSingleUseFence(VkDevice logicalDevice, VkFence& fence, uint64_t timeout = UINT64_MAX);


	/* Creates per-swapchain-image semaphores. */
	void createPerImageSemaphores();


	/* Creates per-frame semaphores. */
	void createPerFrameSemaphores();

private:
	std::shared_ptr<ResourceManager> m_resourceManager;

	VkCoreResourcesManager *m_coreResources;
	VkSwapchainManager *m_swapchainManager;

	VkDevice m_logicalDevice;

	std::vector<VkSemaphore> m_imageReadySemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	/* Creates in-flight fences. */
	void createPerFrameFences();
};