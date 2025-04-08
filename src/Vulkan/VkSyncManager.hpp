/* VkSyncManager.hpp - Manages device and host synchronization.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <iostream>

// Local
#include <ApplicationContext.hpp>
#include <LoggingManager.hpp>
#include <GarbageCollector.hpp>
#include <ServiceLocator.hpp>
#include <Constants.h>


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
	VulkanContext& vkContext;

	std::shared_ptr<GarbageCollector> garbageCollector;

	std::vector<VkSemaphore> imageReadySemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	/* Creates synchronization objects. */
	void createSyncObjects();
};