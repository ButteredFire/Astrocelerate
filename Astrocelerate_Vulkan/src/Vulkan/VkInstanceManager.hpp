/* VkInstanceManager.hpp - Manages Vulkan instance creation.
*
* Encapsulates the creation and cleanup of the Vulkan instance, including
* validation layers, extensions, and debug messaging.
*/


#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

// Local
#include "VkDeviceManager.hpp"
#include "../LoggingManager.hpp"
#include "../Constants.h"



class VkInstanceManager {
public:
	VkInstanceManager();
	~VkInstanceManager();

	/* Initializes Vulkan.
	* @return A pointer to a Vulkan instance.
	*/
	VkInstance initVulkan();

	/* Creates a Vulkan instance.
	* @return a VkResult value indicating the instance creation status.
	*/
	VkResult createVulkanInstance();

	/* Verifies whether a given array of Vulkan extensions is available or supported.
	* @param arrayOfExtensions: An array containing the names of Vulkan extensions to be evaluated for validity.
	* @param arraySize: The size of the array.
	*
	* @return True if all specified Vulkan extensions are supported, otherwise False.
	*/
	bool verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t& arraySize);


	/* Verifies whether a given vector of Vulkan validation layers is available or supported.
	* @param layers: A vector containing the names of Vulkan validation layers to be evaluated for validity.
	*
	* @return True if all specified Vulkan validation layers are supported, otherwise False.
	*/
	bool verifyVulkanValidationLayers(std::vector<const char*>& layers);


	/* Gets the renderer's currently enabled Vulkan validation layers.
	* @return A vector of type `const char*` that contains the names of currently enabled validation layers.
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


	inline VkInstance getInstance() { 
		if (vulkInst == nullptr) {
			throw std::runtime_error("Cannot get Vulkan instance: Vulkan has not been initialized!");
		}
		return vulkInst;
	};


	/* Sets Vulkan validation layers.
	* @param layers: A vector containing Vulkan validation layer names to be bound to the current list of enabled validation layers.
	*/
	void setVulkanValidationLayers(std::vector<const char*> layers);

private:
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
};