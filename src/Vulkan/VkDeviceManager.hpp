/* VkDeviceManager.hpp - Manages Vulkan physical and logical devices.
*
* Selects the best GPU available, creates a Vulkan logical device, and
* manages device queues for rendering operations.
*/


#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Constants.h>

	// VMA
#include <vk_mem_alloc.h>


// C++ STLs
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <set>

// Local
#include <Vulkan/VkSwapchainManager.hpp>
#include <LoggingManager.hpp>
#include <MemoryManager.hpp>
#include <ServiceLocator.hpp>
#include <ApplicationContext.hpp>



typedef struct PhysicalDeviceScoreProperties {
	VkPhysicalDevice device = VK_NULL_HANDLE;
	std::string deviceName = "";
	bool isCompatible = true;
	uint32_t optionalScore = 0;
} PhysicalDeviceScoreProperties;

inline bool ScoreComparator(const PhysicalDeviceScoreProperties& s1, const PhysicalDeviceScoreProperties& s2) {
	// If (s1 is incompatible && s2 is compatible) OR (s2 is compatible && s1 score <= s2 score)
	return (!s1.isCompatible && s2.isCompatible) || (s2.isCompatible && (s1.optionalScore <= s2.optionalScore));
}


class VkDeviceManager {
public:
	VkDeviceManager(VulkanContext& context, bool autoCleanup = true);
	~VkDeviceManager();

	/* Initializes the device creation process. */
	void init();
	void cleanup();


	/* Queries all GPU-supported queue families.
		@param device: The GPU from which to query queue families.
		@return A QueueFamilyIndices struct, with each family assigned to their corresponding index.
	*/
	static QueueFamilyIndices getQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);

private:
	bool cleanOnDestruction = true;
	VkInstance vulkInst = VK_NULL_HANDLE;
	VulkanContext& vkContext;
	std::shared_ptr<MemoryManager> memoryManager;

	VmaAllocator vmaAllocator = VK_NULL_HANDLE;

	VkPhysicalDevice GPUPhysicalDevice = VK_NULL_HANDLE;
	VkDevice GPULogicalDevice = VK_NULL_HANDLE;

	QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices();

	std::vector<const char*> requiredDeviceExtensions;
	std::vector<PhysicalDeviceScoreProperties> GPUScores;

	/* Configures a GPU Physical Device by binding it to an appropriate GPU that supports needed features. */
	void createPhysicalDevice();


	/* Creates a GPU Logical Device to interface with the Physical Device. */
	void createLogicalDevice();


	/* Grades a list of GPUs according to their suitability for Astrocelerate's features.
		@param physicalDevices: A vector of GPUs to be evaluated for suitability.
		@return A vector containing the final scores of all GPUs in the list.
	*/
	std::vector<PhysicalDeviceScoreProperties> rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices);
	
	
	/* Checks whether a GPU supports a list of extensions.
		@param device: The GPU to be evaluated for extension support.
		@param extensions: A vector containing extensions to be checked.
		
		@return True if the GPU supports all extensions in the list of extensions, otherwise False.
	*/
	bool checkDeviceExtensionSupport(VkPhysicalDevice& device, std::vector<const char*>& extensions);
};