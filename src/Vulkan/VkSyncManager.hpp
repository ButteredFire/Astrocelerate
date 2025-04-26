/* VkSyncManager.hpp - Manages device and host synchronization.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <iostream>

// Local
#include <Core/ApplicationContext.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/GarbageCollector.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/Constants.h>


class VkSyncManager {
public:
	VkSyncManager(VulkanContext& context);
	~VkSyncManager();

	void init();

	/* Creates a single-use fence.
		@param vkContext: The application context.
		@param signaled (Default: False): A boolean specifying whether the fence's initial state should be signaled (True), or not (False).

		@return The fence in question.
	*/
	static VkFence createSingleUseFence(VulkanContext& vkContext, bool signaled = false);


	/* Waits for a single-use fence to be signaled. After waiting, the fence will be destroyed.
		@param vkContext: The application context.
		@param fence: The fence in question.
		@param timeout: The fence wait time.
	*/
	static void waitForSingleUseFence(VulkanContext& vkContext, VkFence& fence, uint64_t timeout = UINT64_MAX);

private:
	VulkanContext& m_vkContext;

	std::shared_ptr<GarbageCollector> m_garbageCollector;

	std::vector<VkSemaphore> m_imageReadySemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	/* Creates synchronization objects. */
	void createSyncObjects();
};