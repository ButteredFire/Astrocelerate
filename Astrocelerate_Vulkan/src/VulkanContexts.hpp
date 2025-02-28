#pragma once

// A structure that manages commonly accessed or global Vulkan objects.
typedef struct VulkanContext {
    GLFWwindow *window;

    // Instance creation
    VkInstance vulkanInstance;
    VkSurfaceKHR vkSurface;
    std::vector<const char*> enabledValidationLayers;
    
    // Devices
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
} VulkanContext;