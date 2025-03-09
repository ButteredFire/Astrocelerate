/* VkSwapchainManager.cpp - Vulkan swap-chain management implementation.
*/

#include "VkSwapchainManager.hpp"

VkSwapchainManager::VkSwapchainManager(VulkanContext& context):
    swapChain(VK_NULL_HANDLE),
    vkContext(context) {

    if (vkContext.physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot initialize swap-chain manager: The GPU's physical device handle is null!");
    }

    if (vkContext.logicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot initialize swap-chain manager: The GPU's logical device handle is null!");
    }

    swapChain = vkContext.swapChain;
}

VkSwapchainManager::~VkSwapchainManager() {
    vkDestroySwapchainKHR(vkContext.logicalDevice, swapChain, nullptr);
}


void VkSwapchainManager::init() {
    createSwapChain();
}


void VkSwapchainManager::createSwapChain() {
    SwapChainProperties swapChainProperties = getSwapChainProperties(vkContext.physicalDevice, vkContext.vkSurface);

    VkExtent2D extent = getBestSwapExtent(swapChainProperties.surfaceCapabilities);
    VkSurfaceFormatKHR surfaceFormat = getBestSurfaceFormat(swapChainProperties.surfaceFormats);
    VkPresentModeKHR presentMode = getBestPresentMode(swapChainProperties.presentModes);

    // Specifies the number of images to be had in the swap-chain
    // It is recommended to request at least 1 more image than the minimum
    uint32_t imageCount = swapChainProperties.surfaceCapabilities.minImageCount + 1;

    // If the swap chain's image count has a maximum value (0 is a special value that means there is no maximum)
    // and if the desired image count exceeds that maximum,
    // default the image count to the maximum value.
    if (swapChainProperties.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainProperties.surfaceCapabilities.maxImageCount) {
        imageCount = swapChainProperties.surfaceCapabilities.maxImageCount;
    }

    // Creates the swap-chain structure
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = vkContext.vkSurface;

    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.minImageCount = imageCount;

    // imageArrayLayers specifies the number of layers each image consists of. Its value is almost always 1,
    // unless you're developing a stereoscopic 3D application.
    swapChainCreateInfo.imageArrayLayers = 1;

    // imageUsage is a bitfield that specifies the type of operations the swap-chain images are used for.
    // NOTE:
    // - VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT means that the swap-chain images are used as color attachment. In other words, we will render directly to them.
    // If you want to render to a separate image first (for post-processing, etc.) before passing it to the swap-chain image via memory operations,
    // use other bits like VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices families = VkDeviceManager::getQueueFamilies(vkContext.physicalDevice, vkContext.vkSurface);
    std::vector<uint32_t> familyIndices = families.getAvailableIndices();

    // If the graphics family supports presentation (i.e., the presentation family is not separate),
    // set the image sharing mode to exclusive mode. MODE_EXCLUSIVE means that images are owned
    // by only 1 queue family at a time, and using them from another family requires ownership transference.
    if (families.graphicsFamily.supportsPresentation) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // Else (i.e., the graphics family does not support presentation / the graphics and presentation families are separate),
    // set the image sharing mode to concurrent mode. MODE_CONCURRENT means that images can be used across different families.
    else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = familyIndices.data();
    }

    // Specifies a transform applied to swap-chain images (e.g., rotation) (in this case, none, i.e., the current transform)
    swapChainCreateInfo.preTransform = swapChainProperties.surfaceCapabilities.currentTransform;

    // Specifies if the alpha channel should be used for blending with other windows in the window system.
    // In this case, we will ignore the alpha channel.
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // References the old swap-chain (which is null for now)
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Creates a VkSwapchainKHR object
    VkResult result = vkCreateSwapchainKHR(vkContext.logicalDevice, &swapChainCreateInfo, nullptr, &swapChain);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap-chain!");
    }

    // Saves swap-chain properties
    vkContext.swapChain = swapChain;
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    
    // Fills swapChainImages with a set of VkImage objects provided after swap-chain creation
    vkGetSwapchainImagesKHR(vkContext.logicalDevice, vkContext.swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vkContext.logicalDevice, vkContext.swapChain, &imageCount, swapChainImages.data());
}


std::vector<VkImageView> VkSwapchainManager::createImageViews(std::vector<VkImage>& images) {
    std::vector<VkImageView> imageViews(images.size());
    for (const auto& image : images) {

    }

    return imageViews;
}



SwapChainProperties VkSwapchainManager::getSwapChainProperties(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
    SwapChainProperties swapChain;

    // Queries swap-chain properties
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChain.surfaceCapabilities);

    // Queries surface formats
    uint32_t numOfSurfaceFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numOfSurfaceFormats, nullptr);

    swapChain.surfaceFormats.resize(numOfSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numOfSurfaceFormats, swapChain.surfaceFormats.data());

    // Queries surface present modes
    uint32_t numOfPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numOfPresentModes, nullptr);

    swapChain.presentModes.resize(numOfPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numOfPresentModes, swapChain.presentModes.data());


    if (swapChain.surfaceFormats.empty()) {
        std::cerr << "Warning: GPU does not support any surface formats for the given window surface!" << '\n';
    }
    if (swapChain.presentModes.empty()) {
        std::cerr << "Warning: GPU does not support any presentation modes for the given window surface!" << '\n';
    }

    return swapChain;
}


VkSurfaceFormatKHR VkSwapchainManager::getBestSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.empty()) {
        throw std::runtime_error("Unable to get surface formats from an empty vector!");
    }

    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return formats[0];
}


VkPresentModeKHR VkSwapchainManager::getBestPresentMode(std::vector<VkPresentModeKHR>& modes) {
    for (const auto& mode : modes) {
        // MAILBOX_KHR: Triple buffering => Best for performance and smoothness, but requires more GPU memory
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    // FIFO_KHR (fallback): V-Sync => No screen tearing and predictable frame pacing, but introduces input lag
    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VkSwapchainManager::getBestSwapExtent(VkSurfaceCapabilitiesKHR& capabilities) {
    // If the current extent is not equal to UINT32_MAX (a special value), Vulkan is forcing a specific resolution.
    // In other words, the resolution of the swap-chain images is equal to the resolution of the window that we're drawing to (in pixels).
    // Therefore, we must use the current extent as it is.
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    // Else, it means we can create possible resolutions within the [minImageExtent, maxImageExtent] range.
    // In this case, the best swap extent is the one whose resolution best matches the window.
    int width, height;
    glfwGetFramebufferSize(vkContext.window, &width, &height); // Gets the window resolution in pixels

    // Creates the best swap extent and populate it with the current window resolution
    VkExtent2D bestExtent{};
    bestExtent.width = static_cast<uint32_t>(width);
    bestExtent.height = static_cast<uint32_t>(height);

    // Clamps the width and height within the accepted bounds
    bestExtent.width = std::clamp(bestExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    bestExtent.height = std::clamp(bestExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return bestExtent;
}
