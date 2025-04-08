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

private:
	VulkanContext& vkContext;

	std::shared_ptr<GarbageCollector> garbageCollector;

	std::vector<VkSemaphore> imageReadySemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	/* Creates synchronization objects. */
	void createSyncObjects();
};