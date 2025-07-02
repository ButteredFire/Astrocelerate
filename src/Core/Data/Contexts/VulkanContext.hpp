#pragma once

#include <External/GLFWVulkan.hpp>
#include <vk_mem_alloc.h> 

#include <variant>  
#include <vector>  
#include <map>  
#include <array>

#include <Core/Data/Constants.h>
#include <Core/Data/Device.hpp>


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

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkImageLayout> imageLayouts;  // This is for swapchain recreation purposes (i.e., robust PRESENT_SRC_KHR vs UNDEFINED detection)
        std::vector<VkFramebuffer> imageFrameBuffers;

        VkExtent2D extent = VkExtent2D();
        VkSurfaceFormatKHR surfaceFormat = VkSurfaceFormatKHR();
        uint32_t minImageCount = 0;
    } SwapChain;

    // Offscreen rendering resources
    struct OffscreenResources {
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkSampler> samplers;
        std::vector<VkFramebuffer> framebuffers;

        // For outdated images, image views, and framebuffers on viewport resize
        std::vector<uint32_t> pendingCleanupIDs;
        uint32_t resizedFrameIndex;
    } OffscreenResources;

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

    // Presentation pipeline (graphics pipeline)
    struct PresentPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpassCount = 0;
    } PresentPipeline;

    // Offscreen pipeline (graphics pipeline)
    struct OffscreenPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpassCount = 0;

        VkImageView depthImageView = VK_NULL_HANDLE;

        std::vector<VkDescriptorSet> perFrameDescriptorSets;

        VkDescriptorSet pbrDescriptorSet;
    } OffscreenPipeline;
};

extern VulkanContext g_vkContext;


/* Checks whether a Vulkan object is valid/non-null. */
template<typename T> bool vkIsValid(T vulkanObject) { return (vulkanObject != T()); }