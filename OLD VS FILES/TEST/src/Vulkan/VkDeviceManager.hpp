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
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <set>

// Local
#include "../LoggingManager.hpp"
#include "../Constants.h"
#include "../VulkanContexts.hpp"



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

// A structure that manages GPU queue families.
typedef struct QueueFamilyIndices {
	// Structure for each family
	typedef struct QueueFamily {
		std::optional <uint32_t> index;
		uint32_t FLAG = NULL;
		VkQueue deviceQueue = VK_NULL_HANDLE;
		bool supportsPresentation = false;
	} QueueFamily;

	// Family declarations
	QueueFamily graphicsFamily;
	QueueFamily presentationFamily; 

	// Binds each family's flag to their corresponding Vulkan flag
	bool initialized = false;
	void init() {
		graphicsFamily.FLAG = VK_QUEUE_GRAPHICS_BIT;
	}

	/* Checks whether a queue family exists (based on whether it has a valid index).
	* @return True if the queue family exists, otherwise False.
	*/
	bool familyExists(QueueFamily family) {
		return family.index.has_value();
	}

	/* Gets the list of all queue families in the QueueFamilyIndices struct.
	* @return A vector that contains all queue families.
	*/
	std::vector<QueueFamily*> getAllQueueFamilies() {
		return {
			&graphicsFamily,
			&presentationFamily
		};
	}

	/* Gets the list of available queue families (based on whether each family has a valid index).
	* @param queueFamilies (optional): A vector of queue families to be filtered for available ones.
	* If queueFamilies is not provided, the function will evaluate all queue families in the QueueFamilyIndices struct for availability.
	* @return A vector that contains available queue families.
	*/
	std::vector<QueueFamily*> getAvailableQueueFamilies(std::vector<QueueFamily*> queueFamilies = {}) {
		std::vector<QueueFamily*> available;
		const std::vector<QueueFamily*> allFamilies = ( (queueFamilies.size() == 0)? getAllQueueFamilies() : queueFamilies );

		for (auto& family : allFamilies)
			if (family->index.has_value())
				available.push_back(family);

		return available;
	}
} QueueFamilyIndices;


// A structure that manages properties of a swap chain.
typedef struct SwapChainProperties {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
} SwapChainProperties;



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

	std::vector<const char*> requiredDeviceExtensions;
	std::vector<PhysicalDeviceScoreProperties> GPUScores;

	/* Configures a GPU Physical Device by binding it to an appropriate GPU that supports needed features.
	* @return A VkPhysicalDevice object storing the GPU Physical Device.
	*/
	VkPhysicalDevice createPhysicalDevice();


	/* Creates a GPU Logical Device to interface with the Physical Device.
	* @return A VkDevice object storing the GPU Logical Device.
	*/
	VkDevice createLogicalDevice();


	/* Creates a swap-chain. */
	void createSwapChain();



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


	/* Queries the properties of a GPU's swap-chain.
	* @param device: The GPU from which to query swap-chain properties.
	* @return A SwapChainProperties struct containing details of the GPU's swap-chain.
	*/
	SwapChainProperties getSwapChainProperties(VkPhysicalDevice& device);


	/* Gets the surface format that is the most suitable for Astrocelerate.
	* 
	* Requirements for the best surface format:
	* 
	* - Surface format: VK_VK_FORMAT_B8G8R8A8_SRGB (8-bit RGBA)
	* 
	* More specifically, the surface must have RGBA color channels that
	* are stored in 8-bit unsigned integers each, as well as sGRB encoding.
	* 
	* - Color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (sGRB color space)
	* 
	* More specifically, the surface must support sRGB color space.
	* @param formats: A vector containing surface formats of type VkSurfaceFormatKHR to be evaluated.
	* @return The best surface format in the vector, OR the first format in the vector if none satisfies both requirements.
	*/
	VkSurfaceFormatKHR getBestSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats);


	/* Gets the swap-chain presentation mode that is the most suitable for Astrocelerate.
	* 
	* Requirement for the best presentation mode: Must support VK_PRESENT_MODE_FIFO_KHR
	* 
	* MAILBOX_KHR works like FIFO_KHR, but instead of waiting in a queue,
	* new frames replace old frames if they are ready (like triple buffering).
	* This minimizes latency while preventing tearing.
	* 
	* However, MAILBOX_KHR consumes more GPU memory, so FIFO_KHR (V-Sync)
	* is set to be the fallback option, as it is guaranteed to be available on all GPUs.
	* 
	* @param modes: A vector containing presentation modes of type VkPresentModeKHR to be evaluated.
	* @return The best presentation mode in the vector, OR VK_PRESENT_MODE_FIFO_KHR if none satisfies the requirement.
	*/
	VkPresentModeKHR getBestPresentMode(std::vector<VkPresentModeKHR>& modes);


	/* Gets the best swap extent (resolution of images in the swap-chain).
	* @param capabilities: A VkSurfaceCapabilitiesKHR struct from which to get the swap extent.
	* @return The best swap extent within accepted resolution boundaries.
	*/
	VkExtent2D getBestSwapExtent(VkSurfaceCapabilitiesKHR& capabilities);



	/* Checks whether a GPU supports a list of extensions.
	* @param device: The GPU to be evaluated for extension support.
	* @param extensions: A vector containing extensions to be checked.
	* 
	* @return True if the GPU supports all extensions in the list of extensions, otherwise False.
	*/
	bool checkDeviceExtensionSupport(VkPhysicalDevice& device, std::vector<const char*>& extensions);
};