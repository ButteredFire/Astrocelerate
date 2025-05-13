#pragma once  

// VMA  
#include <vk_mem_alloc.h>  

// C++ STLs  
#include <variant>  
#include <vector>  
#include <map>  

// Local  
#include <Core/LoggingManager.hpp>  
#include <CoreStructs/Device.hpp>  

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

       std::vector<VkDescriptorSet> descriptorSets;
   } GraphicsPipeline;
};

/* Checks whether a Vulkan object is valid/non-null. */  
template<typename T> bool vkIsValid(T vulkanObject) { return (vulkanObject != T()); }