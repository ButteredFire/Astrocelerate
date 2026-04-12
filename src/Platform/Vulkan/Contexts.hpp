/* Contexts: Contexts for Vulkan handles.
*/
#pragma once

#include <vector>

#include <vk_mem_alloc.h>

#include <Platform/External/GLFWVulkan.hpp>

#include <Platform/Vulkan/Data/Device.hpp>


namespace Ctx {
    struct VkRenderDevice {
        VkInstance instance;
		VkDebugUtilsMessengerEXT dbgMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VmaAllocator vmaAllocator;
        QueueFamilyIndices queueFamilies;
        
		PhysicalDeviceProperties chosenDevice;
		std::vector<PhysicalDeviceProperties> availableDevices;
    };


    struct VkWindow {
        VkSurfaceKHR surface;
        VkSurfaceFormatKHR surfaceFormat;
        VkSwapchainKHR swapchain;
        VkExtent2D extent;
        VkPresentModeKHR presentMode;
        VkRenderPass presentRenderPass;
        uint32_t minImageCount;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkImageLayout> imageLayouts;

        uint32_t swapchainResourceID;
    };


    struct OffscreenPipeline {
        // ----- OPAQUE GEOMETRY PIPELINE -----
        VkRenderPass renderPass;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        std::vector<VkDescriptorSet> perFrameDescriptorSets;
        VkDescriptorSet materialDescriptorSet;
        VkDescriptorSet texArrayDescriptorSet;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkSampler> imageSamplers;
        std::vector<VkFramebuffer> frameBuffers;


        // ----- ORBIT GEOMETRY PIPELINE -----
        VkPipeline orbitPipeline;
    };
}
