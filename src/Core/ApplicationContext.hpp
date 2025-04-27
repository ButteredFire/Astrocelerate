#pragma once


// VMA
#include <vk_mem_alloc.h>

#include <Core/LoggingManager.hpp>
#include <vector>
#include <optional>
#include <variant>


using VulkanHandles = std::variant<
	VmaAllocator,
	VmaAllocation,
	VkDebugUtilsMessengerEXT,
	VkInstance,
	VkPhysicalDevice,   // Usually not destroyed explicitly, but listed for completeness
	VkDevice,
	VkQueue,            // Generally managed by Vulkan and not destroyed manually
	VkCommandPool,
	VkCommandBuffer,
	VkBuffer,
	VkBufferView,
	VkImage,
	VkImageView,
	VkFramebuffer,
	VkRenderPass,
	VkShaderModule,
	VkPipeline,
	VkPipelineLayout,
	VkDescriptorSetLayout,
	VkDescriptorPool,
	VkDescriptorSet,    // Generally managed by `VkDescriptorPool`
	VkSampler,
	VkFence,
	VkSemaphore,
	VkEvent,
	VkQueryPool,
	VkSwapchainKHR,     // Requires `VK_KHR_swapchain` extension
	VkSurfaceKHR,       // Requires `VK_KHR_surface`
	VkDeviceMemory
>;


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
struct QueueFamilyIndices {
	// Structure for each family
	struct QueueFamily {
		std::optional<uint32_t> index;
		uint32_t FLAG = NULL;
		VkQueue deviceQueue = VK_NULL_HANDLE;
		std::string deviceName = "";
		bool supportsPresentation = false;
	};

	// Family declarations
	QueueFamily graphicsFamily;
	QueueFamily presentationFamily;
	QueueFamily transferFamily;

	/* Binds each family's flag to their corresponding Vulkan flag */
	void init() {
		graphicsFamily.deviceName = "Graphics queue family";
		presentationFamily.deviceName = "Presentation queue family";
		transferFamily.deviceName = "Transfer queue family";

		graphicsFamily.FLAG = VK_QUEUE_GRAPHICS_BIT;
		transferFamily.FLAG = VK_QUEUE_TRANSFER_BIT;
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
			&presentationFamily,
			&transferFamily
		};
	}

	/* Gets the list of available queue families (based on whether each family has a valid index).
	* @param queueFamilies (optional): A vector of queue families to be filtered for available ones.
	* If queueFamilies is not provided, the function will evaluate all queue families in the QueueFamilyIndices struct for availability.
	* @return A vector that contains available queue families.
	*/
	std::vector<QueueFamily*> getAvailableQueueFamilies(std::vector<QueueFamily*> queueFamilies = {}) {
		std::vector<QueueFamily*> available;
		const std::vector<QueueFamily*> allFamilies = ((queueFamilies.size() == 0) ? getAllQueueFamilies() : queueFamilies);

		for (auto& family : allFamilies)
			if (family->index.has_value())
				available.push_back(family);

		return available;
	}

	/* Gets the list of available family indices (based on whether each family has a valid index).
	* @param queueFamilies (optional): A vector of queue families to be filtered for available ones.
	* If queueFamilies is not provided, the function will evaluate all queue families in the QueueFamilyIndices struct for availability.
	* @return A vector that contains the indices of available queue families.
	*/
	std::vector<uint32_t> getAvailableIndices(std::vector<QueueFamily*> queueFamilies = {}) {
		std::vector<uint32_t> available;
		const std::vector<QueueFamily*> allFamilies = ((queueFamilies.size() == 0) ? getAllQueueFamilies() : queueFamilies);

		for (auto& family : allFamilies)
			if (family->index.has_value())
				available.push_back(family->index.value());

		return available;
	}
};


// A structure that stores commonly accessed or global Vulkan objects.
struct VulkanContext {
    GLFWwindow* window;
	VmaAllocator vmaAllocator = VK_NULL_HANDLE;


    // Instance creation
    VkInstance vulkanInstance = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    std::vector<const char*> enabledValidationLayers;


    // Devices
	struct Device {
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		QueueFamilyIndices queueFamilies = QueueFamilyIndices();

		VkPhysicalDeviceProperties deviceProperties;
	} Device;


    // Swap-chain
	struct SwapChain {
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;

		std::vector<VkImageView> imageViews;
		std::vector<VkFramebuffer> imageFrameBuffers;

		VkExtent2D extent = VkExtent2D();
		VkSurfaceFormatKHR surfaceFormat = VkSurfaceFormatKHR();
		uint32_t minImageCount = 0;
	} SwapChain;


	// Textures
	struct Texture {
		VkImageLayout imageLayout;
		VkImageView imageView;
		VkSampler sampler;
	} Texture;


	// Command objects
	struct CommandObjects {
        std::vector<VkCommandBuffer> graphicsCmdBuffers;
		std::vector<VkCommandBuffer> transferCmdBuffers;
	} CommandObjects;


	// Synchronization objects
	struct SyncObjects {
		std::vector<VkSemaphore> imageReadySemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
	} SyncObjects;


    // Graphics pipeline
    struct GraphicsPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
		uint32_t subpassCount = 0;

		VkImageView depthImageView = VK_NULL_HANDLE;

		std::vector<VkDescriptorSet> m_descriptorSets;
    } GraphicsPipeline;
};


/* Checks whether a Vulkan object is valid/non-null. */
template<typename T> bool vkIsValid(T vulkanObject) { return (vulkanObject != T()); }