#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <unordered_set>

#include "../Constants.h"
#include "../LoggingManager.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	// Vulkan Setters & Setters
	

	/* Gets a vector of the renderer's currently enabled Vulkan validation layers.
	* @returns A vector of type `const char*` that contains the names of currently enabled validation layers.
	*/
	inline std::vector<const char*> getEnabledVulkanValidationLayers() const { return enabledValidationLayers; };

	/* Gets a vector of supported Vulkan extensions (may differ from machine to machine).
	* @return A vector of type `VkExtensionProperties` that contains supported Vulkan extensions.
	*/
	std::vector<VkExtensionProperties> getSupportedVulkanExtensions();

	/* Gets a vector of supported Vulkan validation layers.
	* @return A vector of type `VkLayerProperties` that contains supported Vulkan validation layers.
	*/
	std::vector<VkLayerProperties> getSupportedVulkanValidationLayers();

	/* Sets Vulkan validation layers.
	* @param `layers` A vector containing Vulkan validation layer names to be bound to the current list of enabled validation layers.
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
	std::vector<const char*> enabledValidationLayers;
	std::unordered_set<const char*> UTIL_enabledValidationLayerSet; // Purpose: Prevents copying duplicate layers into `enabledValidationLayers`
	std::vector<VkLayerProperties> supportedLayers;
	std::vector<VkExtensionProperties> supportedExtensions;
	std::unordered_set<std::string> supportedLayerNames;
	std::unordered_set<std::string> supportedExtensionNames;

	void initVulkan();
	VkResult createVulkanInstance();
	bool verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t arraySize);
	bool verifyVulkanValidationLayers(std::vector<const char*> layers);
};
