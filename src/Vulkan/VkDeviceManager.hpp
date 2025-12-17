/* VkDeviceManager.hpp - Manages Vulkan physical and logical devices.
*
* Selects the best GPU available, creates a Vulkan logical device, and
* manages device queues for rendering operations.
*/


#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <set>

#include <vk_mem_alloc.h>
#include <External/GLFWVulkan.hpp>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/ResourceManager.hpp>
#include <Core/Data/Constants.h>
#include <Core/Engine/ServiceLocator.hpp>

#include <Vulkan/VkSwapchainManager.hpp>



class VkDeviceManager {
public:
	VkDeviceManager();
	~VkDeviceManager() = default;


	/* Initializes the device creation process. */
	void init();


	/* Configures a GPU Physical Device by binding it to an appropriate GPU that supports needed features. */
	void createPhysicalDevice(VkPhysicalDevice &physDevice, PhysicalDeviceProperties &chosenDevice, std::vector<PhysicalDeviceProperties> &availableDevices, VkInstance instance, VkSurfaceKHR surface);


	/* Creates a GPU Logical Device to interface with the Physical Device. */
	CleanupTask createLogicalDevice(VkDevice &logicalDevice, QueueFamilyIndices &queueFamilies, VkPhysicalDevice physDevice, VkSurfaceKHR surface);


	/* Queries all GPU-supported queue families.
		@param device: The GPU from which to query queue families.
		@return A QueueFamilyIndices struct, with each family assigned to their corresponding index.
	*/
	static QueueFamilyIndices getQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);

private:
	std::vector<const char*> m_requiredDeviceExtensions;
	QueueFamilyIndices m_queueFamilyIndices{};


	/* Grades a list of GPUs according to their suitability for Astrocelerate's features.
		@param physicalDevices: A vector of GPUs to be evaluated for suitability.
		@param surface: The window surface.

		@return A vector containing the final scores of all GPUs in the list.
	*/
	std::vector<PhysicalDeviceProperties> rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices, VkSurfaceKHR surface);
	
	
	/* Checks whether a GPU supports a list of extensions.
		@param device: The GPU to be evaluated for extension support.
		@param extensions: A vector containing extensions to be checked.
		
		@return True if the GPU supports all extensions in the list of extensions, False otherwise.
	*/
	bool checkDeviceExtensionSupport(VkPhysicalDevice& device, std::vector<const char*>& extensions);
};