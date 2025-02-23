#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <optional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "../Constants.h"
#include "../LoggingManager.h"

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
typedef struct QueueFamilyIndex {
	std::optional <uint32_t> index;
	uint32_t FLAG;
} QueueFamilyIndex;

typedef struct QueueFamilyIndices {
	QueueFamilyIndex graphicsFamily;

	// Binds each family's flag to their corresponding Vulkan flag
	bool initialized = false;
	void init() {
		if (!initialized) {
			initialized = true;
			graphicsFamily.FLAG = VK_QUEUE_GRAPHICS_BIT;
		}
	}
} QueueFamilyIndices;

class Renderer {
public:
	Renderer();
	~Renderer();

	// Vulkan Setters & Setters
	

	/* Gets the renderer's currently enabled Vulkan validation layers.
	* @returns A vector of type `const char*` that contains the names of currently enabled validation layers.
	*/
	inline std::vector<const char*> getEnabledVulkanValidationLayers() const { return enabledValidationLayers; };

	/* Queries supported Vulkan extensions (may differ from machine to machine).
	* @return A vector of type `VkExtensionProperties` that contains supported Vulkan extensions.
	*/
	std::vector<VkExtensionProperties> getSupportedVulkanExtensions();

	/* Queries supported Vulkan validation layers.
	* @return A vector of type `VkLayerProperties` that contains supported Vulkan validation layers.
	*/
	std::vector<VkLayerProperties> getSupportedVulkanValidationLayers();

	/* Sets Vulkan validation layers.
	* @param layers: A vector containing Vulkan validation layer names to be bound to the current list of enabled validation layers.
	*/
	void setVulkanValidationLayers(std::vector<const char*> layers);

private:

	// Vulkan
		// If program is not run in DEBUG mode, disable validation layers. Else, enable them.
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	VkInstance vulkInst;
	VkPhysicalDevice GPUPhysicalDevice;
	VkDevice GPULogicalDevice;
	std::vector<const char*> enabledValidationLayers;
	std::unordered_set<const char*> UTIL_enabledValidationLayerSet; // Purpose: Prevents copying duplicate layers into `enabledValidationLayers`
	std::vector<VkLayerProperties> supportedLayers;
	std::vector<VkExtensionProperties> supportedExtensions;
	std::unordered_set<std::string> supportedLayerNames;
	std::unordered_set<std::string> supportedExtensionNames;

	std::vector<PhysicalDeviceScoreProperties> GPUScores;

	void initVulkan();
	VkResult createVulkanInstance();
	void createPhysicalDevice();
	void createLogicalDevice();

	bool verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t& arraySize);
	bool verifyVulkanValidationLayers(std::vector<const char*>& layers);
	
	std::vector<PhysicalDeviceScoreProperties> rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices);
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice& device);
};
