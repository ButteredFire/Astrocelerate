#pragma once

// A struct that holds commonly accessed or global Vulkan objects.
typedef struct VulkanContext {
    VkInstance vulkanInstance;
    std::vector<const char*> enabledValidationLayers;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
} VulkanContext;