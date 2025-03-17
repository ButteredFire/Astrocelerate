#pragma once


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
		bool supportsPresentation = false;
	};

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


// A structure that manages commonly accessed or global Vulkan objects.
struct VulkanContext {
    GLFWwindow* window;

    // Instance creation
    VkInstance vulkanInstance = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    std::vector<const char*> enabledValidationLayers;

    // Devices
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
	QueueFamilyIndices queueFamilies = QueueFamilyIndices();

    // Swap-chain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImageView> swapChainImageViews;
    VkExtent2D swapChainExtent = VkExtent2D();
    VkSurfaceFormatKHR surfaceFormat = VkSurfaceFormatKHR();
    uint32_t minImageCount = 0;

    // Pipelines
    struct GraphicsPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
    } GraphicsPipeline;

    struct RenderPipeline {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        // Synchronization
        VkSemaphore imageReadySemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;
    } RenderPipeline;

};