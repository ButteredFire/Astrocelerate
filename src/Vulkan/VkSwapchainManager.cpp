/* VkSwapchainManager.cpp - Vulkan swap-chain management implementation.
*/

#include "VkSwapchainManager.hpp"

VkSwapchainManager::VkSwapchainManager(VulkanContext& context, bool autoCleanup) :
    vkContext(context), cleanOnDestruction(autoCleanup) {

    memoryManager = ServiceLocator::getService<MemoryManager>();

    if (vkContext.physicalDevice == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, "Cannot initialize swap-chain manager: The GPU's physical device handle is null!");
    }

    if (vkContext.logicalDevice == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, "Cannot initialize swap-chain manager: The GPU's logical device handle is null!");
    }

    swapChain = vkContext.swapChain;

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkSwapchainManager::~VkSwapchainManager() {
    if (cleanOnDestruction)
        cleanup();
}


void VkSwapchainManager::init() {
    /* WARNING: The init() method is also called by recreateSwapchain */
    // Initializes swap-chain
    createSwapChain();

    // Parses data for each image
    createImageViews();
    vkContext.swapChainImageViews = swapChainImageViews;
}


void VkSwapchainManager::cleanup() {
    for (const auto& taskID : cleanupTaskIDs) {
        memoryManager->executeCleanupTask(taskID);
    }
    cleanupTaskIDs.clear();

    //// Frees image views memory
    //for (const auto& imageView : swapChainImageViews) {
    //    if (vkIsValid(imageView))
    //        vkDestroyImageView(vkContext.logicalDevice, imageView, nullptr);
    //}

    //// Frees swap-chain memory
    //if (vkIsValid(swapChain))
    //    vkDestroySwapchainKHR(vkContext.logicalDevice, swapChain, nullptr);
}


void VkSwapchainManager::recreateSwapchain() {
    Log::print(Log::T_INFO, __FUNCTION__, "Recreating swap-chain...");

    std::shared_ptr<RenderPipeline> renderPipeline = ServiceLocator::getService<RenderPipeline>();

    // If the window is minimized (i.e., (width, height) = (0, 0), pause the window until it is in the foreground again
    int width = 0, height = 0;
    glfwGetFramebufferSize(vkContext.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(vkContext.window, &width, &height);
        glfwWaitEvents();
    }

    // Recreates swap-chain
        // Waits for the host to be idle
    vkDeviceWaitIdle(vkContext.logicalDevice);

        // Cleans up outdated swap-chain objects
    renderPipeline->destroyFrameBuffers(); // Call this before destroying image views as framebuffers depend on them
    cleanup();
    
    init();
    renderPipeline->createFrameBuffers();
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
        throw Log::RuntimeException(__FUNCTION__, "Failed to create swap-chain!");
    }

    // Saves swap-chain properties
    vkContext.swapChain = swapChain;
    // Saves swap-chain properties
    vkContext.surfaceFormat = surfaceFormat;
    vkContext.swapChainExtent = extent;

    // Fills swapChainImages with a set of VkImage objects provided after swap-chain creation
    vkGetSwapchainImagesKHR(vkContext.logicalDevice, vkContext.swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkContext.minImageCount = imageCount;
    vkGetSwapchainImagesKHR(vkContext.logicalDevice, vkContext.swapChain, &imageCount, swapChainImages.data());


    CleanupTask task;
    task.caller = __FUNCTION__;
    task.mainObjectName = VARIABLE_NAME(swapChain);
    task.vkObjects = { swapChain };
    task.cleanupFunc = [&]() { vkDestroySwapchainKHR(vkContext.logicalDevice, swapChain, nullptr); };

    uint32_t swapChainTaskID = memoryManager->createCleanupTask(task);
    cleanupTaskIDs.push_back(swapChainTaskID);
}


void VkSwapchainManager::createImageViews() {
	if (swapChainImages.empty()) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Cannot create image views: Swap-chain contains no images to process!");
	}

    swapChainImageViews.clear();
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = swapChainImages[i];

        // Specifies how the data is interpreted
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treats images as 2D textures
        viewCreateInfo.format = vkContext.surfaceFormat.format; // Specifies image format

        /* Defines how the color channels of the image should be interpreted (swizzling).
        * In essence, swizzling remaps the color channels of the image to how they should be interpreted by the image view.
        * 
        * Since we have already chosen an image format in getBestSurfaceFormat(...) as "VK_FORMAT_R8G8B8A8_SRGB" (which stores data in RGBA order), we leave the swizzle mappings as "identity" (i.e., unchanged). This means: Red maps to Red, Green maps to Green, Blue maps to Blue, and Alpha maps to Alpha.
        * 
        * In cases where the image format differs from what the shaders or rendering pipeline expect, swizzling can be used to remap the channels. 
        * For instance, if we want to change our image format to "B8G8R8A8" (BGRA order instead of RGBA), we must swap the Red and Blue channels:
        * 
        * componentMapping.r = VK_COMPONENT_SWIZZLE_B; // Red -> Blue (R -> B)
        * componentMapping.g = VK_COMPONENT_SWIZZLE_IDENTITY; // Green value is the same (RG -> BG)
        * componentMapping.b = VK_COMPONENT_SWIZZLE_R; // Blue -> Red (RGB -> BGR)
        * componentMapping.a = VK_COMPONENT_SWIZZLE_IDENTITY; // Green value is the same (RGBA -> BGRA)
        */
        VkComponentMapping colorMappingStruct{};
        colorMappingStruct.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorMappingStruct.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorMappingStruct.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorMappingStruct.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewCreateInfo.components = colorMappingStruct;

        // Specifies the image's purpose and which part of it should be accessed
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Images will be used as color targets
        viewCreateInfo.subresourceRange.baseMipLevel = 0; // Images will have no mipmapping levels
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0; // Images will have no multiple layers
        viewCreateInfo.subresourceRange.layerCount = 1;

        
        VkResult result = vkCreateImageView(vkContext.logicalDevice, &viewCreateInfo, nullptr, &swapChainImageViews[i]);
        if (result != VK_SUCCESS) {
            throw Log::RuntimeException(__FUNCTION__, "Failed to read image data!");
        }

        VkImageView imageView = swapChainImageViews[i];

        CleanupTask task{};
        task.caller = __FUNCTION__;
        task.mainObjectName = VARIABLE_NAME(imageView);
        task.vkObjects = { vkContext.logicalDevice, imageView };
        task.cleanupFunc = [this, imageView]() { vkDestroyImageView(vkContext.logicalDevice, imageView, nullptr); };

        uint32_t imageViewTaskID = memoryManager->createCleanupTask(task);
        cleanupTaskIDs.push_back(imageViewTaskID);
    }
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
        Log::print(Log::T_WARNING, __FUNCTION__, "GPU does not support any surface formats for the given window surface!");
    }
    if (swapChain.presentModes.empty()) {
        Log::print(Log::T_WARNING, __FUNCTION__, "GPU does not support any presentation modes for the given window surface!");
    }

    return swapChain;
}


VkSurfaceFormatKHR VkSwapchainManager::getBestSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.empty()) {
        cleanup();
        throw Log::RuntimeException(__FUNCTION__, "Unable to get surface formats from an empty vector!");
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
