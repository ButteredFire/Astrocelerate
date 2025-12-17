/* VkInstanceManager.hpp - Manages Vulkan instance creation.
*
* Encapsulates the creation and cleanup of the Vulkan instance, including
* validation layers, extensions, and debug messaging.
*/


#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// C++ STLs
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

// Local
#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/ResourceManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Core/Data/Constants.h>


inline VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


inline void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}


inline static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	/* Message severity levels:
	* VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
	*
	* VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
	* 
	* VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error, but very likely a bug in your application
	* 
	* VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes
	*/

	Log::MsgType severity;
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		severity = Log::T_VERBOSE;
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		severity = Log::T_INFO;
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		severity = Log::T_WARNING;
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		severity = Log::T_ERROR;
		break;

	default:
		severity = Log::T_INFO;
		break;
	}

	Log::Print(severity, "Validation Layer", std::string(pCallbackData->pMessage));

	return VK_FALSE;
}


class VkInstanceManager {
public:
	VkInstanceManager();
	~VkInstanceManager() = default;


	/* Initializes the Vulkan setup process. */
	void init();


	/* Creates a debug messenger to display validation layer messages.
			@param dbgMessenger: The messenger to be created.
			@param instance: The Vulkan instance.

			@return The debug messenger's cleanup task task.
		*/
	CleanupTask createDebugMessenger(VkDebugUtilsMessengerEXT &dbgMessenger, VkInstance instance);


	/* Creates a Vulkan instance.
		@param instance: The Vulkan instance to be created.

		@return The instance's cleanup task task.
	*/
	CleanupTask createVulkanInstance(VkInstance &instance);


	/* Creates a Vulkan surface on which to display rendered images.
		@param surface: The window surface to be created.
		@param instance: The Vulkan instance.
		@param window: The pointer to the current window.

		@return The surface's cleanup task task.
	*/
	CleanupTask createSurface(VkSurfaceKHR &surface, VkInstance instance, GLFWwindow *window);


	/* Gets the renderer's currently enabled Vulkan validation layers.
		@return A vector of type `const char*` that contains the names of currently enabled validation layers.
	*/
	inline std::vector<const char*> getEnabledVulkanValidationLayers() const { return m_enabledValidationLayers; };


	/* Queries supported Vulkan extensions (may differ from machine to machine).
		@return A vector of type `VkExtensionProperties` that contains supported Vulkan extensions.
	*/
	std::vector<VkExtensionProperties> getSupportedVulkanExtensions();


	/* Queries supported Vulkan validation layers.
		@return A vector of type `VkLayerProperties` that contains supported Vulkan validation layers.
	*/
	std::vector<VkLayerProperties> getSupportedVulkanValidationLayers();


	/* Adds Vulkan extensions to the current list of enabled extensions.
		@param extensions: A vector containing Vulkan extension names to be bound to the current list of enabled extensions.
	*/
	void addVulkanExtensions(std::vector<const char*> extensions);


	/* Adds Vulkan validation layers to the current list of enabled validation layers.
		@param layers: A vector containing Vulkan validation layer names to be bound to the current list of enabled validation layers.
	*/
	void addVulkanValidationLayers(std::vector<const char*> layers);

private:
	std::vector<const char*> m_enabledExtensions;
	std::vector<const char*> m_enabledValidationLayers;
	std::unordered_set<const char*> m_UTIL_enabledExtensionSet; // Purpose: Prevents copying extensions
	std::unordered_set<const char*> m_UTIL_enabledValidationLayerSet; // Purpose: Prevents copying duplicate layers
	std::vector<VkLayerProperties> m_supportedLayers;
	std::vector<VkExtensionProperties> m_supportedExtensions;
	std::unordered_set<std::string> m_supportedLayerNames;
	std::unordered_set<std::string> m_supportedExtensionNames;


	/* Initializes Vulkan. */
	void initVulkan();


	/* Configures the debug messenger. */
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);


	/* Verifies whether a given array of Vulkan extensions is available or supported.
		@param extensions: A vector containing the names of Vulkan extensions to be evaluated for validity.
		
		@return True if all specified Vulkan extensions are supported, False otherwise.
	*/
	bool verifyVulkanExtensions(std::vector<const char*> extensions);


	/* Verifies whether a given vector of Vulkan validation layers is available or supported.
		@param layers: A vector containing the names of Vulkan validation layers to be evaluated for validity.
		
		@return True if all specified Vulkan validation layers are supported, False otherwise.
	*/
	bool verifyVulkanValidationLayers(std::vector<const char*>& layers);
};