#pragma once

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
};