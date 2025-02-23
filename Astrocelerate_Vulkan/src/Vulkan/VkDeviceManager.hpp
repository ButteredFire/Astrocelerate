/* VkDeviceManager.hpp - Manages Vulkan physical and logical devices.
*
* Selects the best GPU available, creates a Vulkan logical device, and
* manages device queues for rendering operations.
*/


#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <iostream>
#include <string>
#include <vector>
#include <optional>

// Local
#include "../LoggingManager.hpp"
#include "../Constants.h"
#include "../VulkanContexts.hpp"



typedef struct PhysicalDeviceScoreProperties {
	VkPhysicalDevice device;
	std::string deviceName;
	bool isCompatible = true;
	uint32_t optionalScore = 0;
} PhysicalDeviceScoreProperties;

inline bool ScoreComparator(const PhysicalDeviceScoreProperties& s1, const PhysicalDeviceScoreProperties& s2) {
	// If (s1 is incompatible && s2 is compatible) OR (s2 is compatible && s1 score <= s2 score)
	return (!s1.isCompatible && s2.isCompatible) || (s2.isCompatible && (s1.optionalScore <= s2.optionalScore));
}


/* Rationale behind using std::optional<uint32_t> instead of uint32_t:
* The index of any given queue family is arbitrary, and thus could theoretically be any uint32_t integer.
* Therefore, it is impossible to determine whether a queue family exists only using some magic number like 0, NULL, or UINT32_MAX.
*
* The solution is to use std::optional<T>. It is a wrapper that contains no value until you assign something to it.
* It works because, if a queue family does not exist, their index will actually be non-existent.
* You can determine if it has a value with std::optional<T>.has_value().
*
* NOTE: Making indices uninitialized variables also doesn't work, because then they will still contain garbage values that
* could theoretically be valid queue family indices.
*/
typedef struct QueueFamilyIndices {
	// Structure for each family
	typedef struct QueueFamily {
		std::optional <uint32_t> index;
		uint32_t FLAG;
		VkQueue deviceQueue;
	} QueueFamily;

	// Family declarations
	QueueFamily graphicsFamily;

	// Binds each family's flag to their corresponding Vulkan flag
	bool initialized = false;
	void init() {
		graphicsFamily.FLAG = VK_QUEUE_GRAPHICS_BIT;
	}
} QueueFamilyIndices;


class VkDeviceManager {
public:
	VkDeviceManager(VulkanContext &context);
	~VkDeviceManager();

	/* Initializes the device creation process. */
	void init();

private:
	VkInstance &vulkInst;
	VulkanContext &vkContext;

	VkPhysicalDevice GPUPhysicalDevice;
	VkDevice GPULogicalDevice;

	std::vector<PhysicalDeviceScoreProperties> GPUScores;

	/* Configures a GPU Physical Device by binding it to an appropriate GPU that supports needed features.
	*/
	VkPhysicalDevice createPhysicalDevice();

	/* Creates a GPU Logical Device to interface with the Physical Device.
	*/
	VkDevice createLogicalDevice();


	/* Grades a list of GPUs according to their suitability for Astrocelerate's features.
	* @param physicalDevices: A vector of GPUs to be evaluated for suitability.
	* @return A vector containing the final scores of all GPUs in the list.
	*/
	std::vector<PhysicalDeviceScoreProperties> rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices);

	/* Queries all GPU-supported queue families.
	* @param device: The GPU from which to query queue families.
	* @return A QueueFamilyIndices struct, with each family assigned to their corresponding index.
	*/
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice& device);
};