#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Local
#include <Vulkan/VkDeviceManager.hpp>

// A structure that manages commonly accessed or global Vulkan objects.
typedef struct VulkanContext {
    GLFWwindow *window;

    // Instance creation
    VkInstance vulkanInstance = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    std::vector<const char*> enabledValidationLayers;
    
    // Devices
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;

    // Swap-chain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkExtent2D swapChainExtent = VkExtent2D();
    VkSurfaceFormatKHR surfaceFormat = VkSurfaceFormatKHR();

    // Pipeline
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
} VulkanContext;