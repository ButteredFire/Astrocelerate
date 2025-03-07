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
#include <Vulkan/VkDeviceManager.hpp>
#include <LoggingManager.hpp>
#include <VulkanContexts.hpp>
#include <Constants.h>



class VkInstanceManager {
public:
	VkInstanceManager(VulkanContext &context);
	~VkInstanceManager();

	/* Initializes the Vulkan instance setup process. */
	void init();

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
		if (vulkInst == VK_NULL_HANDLE) {
			throw std::runtime_error("Cannot get Vulkan instance: Vulkan has not been initialized!");
		}
		return vulkInst;
	};


	/* Adds Vulkan extensions to the current list of enabled extensions.
	* @param extensions: A vector containing Vulkan extension names to be bound to the current list of enabled extensions.
	*/
	void addVulkanExtensions(std::vector<const char*> extensions);


	/* Adds Vulkan validation layers to the current list of enabled validation layers.
	* @param layers: A vector containing Vulkan validation layer names to be bound to the current list of enabled validation layers.
	*/
	void addVulkanValidationLayers(std::vector<const char*> layers);

private:
	VkInstance vulkInst;
	VulkanContext& vkContext;
	VkSurfaceKHR windowSurface;

	std::vector<const char*> enabledExtensions;
	std::vector<const char*> enabledValidationLayers;
	std::unordered_set<const char*> UTIL_enabledExtensionSet; // Purpose: Prevents copying extensions
	std::unordered_set<const char*> UTIL_enabledValidationLayerSet; // Purpose: Prevents copying duplicate layers
	std::vector<VkLayerProperties> supportedLayers;
	std::vector<VkExtensionProperties> supportedExtensions;
	std::unordered_set<std::string> supportedLayerNames;
	std::unordered_set<std::string> supportedExtensionNames;


	/* Initializes Vulkan.
	*/
	void initVulkan();

	/* Creates a Vulkan instance.
	* @return a VkResult value indicating the instance creation status.
	*/
	VkResult createVulkanInstance();

	/* Creates a Vulkan surface on which to display rendered images.
	*/
	VkResult createSurface();

	/* Verifies whether a given array of Vulkan extensions is available or supported.
	* @param extensions: A vector containing the names of Vulkan extensions to be evaluated for validity.
	*
	* @return True if all specified Vulkan extensions are supported, otherwise False.
	*/
	bool verifyVulkanExtensions(std::vector<const char*> extensions);


	/* Verifies whether a given vector of Vulkan validation layers is available or supported.
	* @param layers: A vector containing the names of Vulkan validation layers to be evaluated for validity.
	*
	* @return True if all specified Vulkan validation layers are supported, otherwise False.
	*/
	bool verifyVulkanValidationLayers(std::vector<const char*>& layers);
};