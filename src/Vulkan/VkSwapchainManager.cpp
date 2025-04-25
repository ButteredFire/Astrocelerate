/* VkSwapchainManager.cpp - Vulkan swap-chain management implementation.
*/

#include "VkSwapchainManager.hpp"

VkSwapchainManager::VkSwapchainManager(VulkanContext& context):
    m_vkContext(context) {

    m_garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

    if (m_vkContext.Device.physicalDevice == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, "Cannot initialize swap-chain manager: The GPU's physical device handle is null!");
    }

    if (m_vkContext.Device.logicalDevice == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, "Cannot initialize swap-chain manager: The GPU's logical device handle is null!");
    }

    m_swapChain = m_vkContext.SwapChain.swapChain;

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkSwapchainManager::~VkSwapchainManager() {}


void VkSwapchainManager::init() {
    /* WARNING: The init() method is also called by recreateSwapchain */
    // Initializes swap-chain
    createSwapChain();

    // Parses data for each image
    createImageViews();
    m_vkContext.SwapChain.imageViews = m_imageViews;
}


void VkSwapchainManager::recreateSwapchain() {
    Log::print(Log::T_INFO, __FUNCTION__, "Recreating swap-chain...");

    // If the window is minimized (i.e., (width, height) = (0, 0), pause the window until it is in the foreground again
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_vkContext.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_vkContext.window, &width, &height);
        glfwWaitEvents();
    }

    // Recreates swap-chain
        // Waits for the host to be idle
    vkDeviceWaitIdle(m_vkContext.Device.logicalDevice);

        // Cleans up outdated swap-chain objects
    for (const auto& taskID : m_cleanupTaskIDs) {
        m_garbageCollector->executeCleanupTask(taskID);
    }
    m_cleanupTaskIDs.clear();


    init();
    createFrameBuffers();
}


std::pair<VkImageView, uint32_t> VkSwapchainManager::createImageView(VulkanContext& m_vkContext, VkImage& image, VkFormat imgFormat) {
    std::shared_ptr<GarbageCollector> m_garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;

    // Specifies how the data is interpreted
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treats m_images as 2D textures
    viewCreateInfo.format = imgFormat; // Specifies image format

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

    VkImageView imageView;

    VkResult result = vkCreateImageView(m_vkContext.Device.logicalDevice, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to create image view!");
    }


    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_imageView) };
    task.vkObjects = { m_vkContext.Device.logicalDevice, imageView };
    task.cleanupFunc = [m_vkContext, imageView]() { vkDestroyImageView(m_vkContext.Device.logicalDevice, imageView, nullptr); };

    uint32_t imageViewTaskID = m_garbageCollector->createCleanupTask(task);


    return { imageView, imageViewTaskID };
}


void VkSwapchainManager::createSwapChain() {
    SwapChainProperties swapChainProperties = getSwapChainProperties(m_vkContext.Device.physicalDevice, m_vkContext.vkSurface);

    VkExtent2D extent = getBestSwapExtent(swapChainProperties.surfaceCapabilities);
    VkSurfaceFormatKHR surfaceFormat = getBestSurfaceFormat(swapChainProperties.surfaceFormats);
    VkPresentModeKHR presentMode = getBestPresentMode(swapChainProperties.presentModes);

    // Specifies the number of m_images to be had in the swap-chain
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
    swapChainCreateInfo.surface = m_vkContext.vkSurface;

    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.minImageCount = imageCount;

    // imageArrayLayers specifies the number of layers each image consists of. Its value is almost always 1,
    // unless you're developing a stereoscopic 3D application.
    swapChainCreateInfo.imageArrayLayers = 1;

    // imageUsage is a bitfield that specifies the type of operations the swap-chain m_images are used for.
    // NOTE:
    // - VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT means that the swap-chain m_images are used as color attachment. In other words, we will render directly to them.
    // If you want to render to a separate image first (for post-processing, etc.) before passing it to the swap-chain image via memory operations,
    // use other bits like VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices families = VkDeviceManager::getQueueFamilies(m_vkContext.Device.physicalDevice, m_vkContext.vkSurface);
    std::vector<uint32_t> familyIndices = families.getAvailableIndices();

    // If the graphics family supports presentation (i.e., the presentation family is not separate),
    // set the image sharing mode to exclusive mode. MODE_EXCLUSIVE means that m_images are owned
    // by only 1 queue family at a time, and using them from another family requires ownership transference.
    if (families.graphicsFamily.supportsPresentation) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // Else (i.e., the graphics family does not support presentation / the graphics and presentation families are separate),
    // set the image sharing mode to concurrent mode. MODE_CONCURRENT means that m_images can be used across different families.
    else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = familyIndices.data();
    }

    // Specifies a transform applied to swap-chain m_images (e.g., rotation) (in this case, none, i.e., the current transform)
    swapChainCreateInfo.preTransform = swapChainProperties.surfaceCapabilities.currentTransform;

    // Specifies if the alpha channel should be used for blending with other windows in the window system.
    // In this case, we will ignore the alpha channel.
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // References the old swap-chain (which is null for now)
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Creates a VkSwapchainKHR object
    VkResult result = vkCreateSwapchainKHR(m_vkContext.Device.logicalDevice, &swapChainCreateInfo, nullptr, &m_swapChain);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to create swap-chain!");
    }

    // Saves swap-chain properties
    m_vkContext.SwapChain.swapChain = m_swapChain;
    // Saves swap-chain properties
    m_vkContext.SwapChain.surfaceFormat = surfaceFormat;
    m_vkContext.SwapChain.extent = extent;

    // Fills swapChainImages with a set of VkImage objects provided after swap-chain creation
    vkGetSwapchainImagesKHR(m_vkContext.Device.logicalDevice, m_vkContext.SwapChain.swapChain, &imageCount, nullptr);
    m_images.resize(imageCount);
    m_vkContext.SwapChain.minImageCount = imageCount;
    vkGetSwapchainImagesKHR(m_vkContext.Device.logicalDevice, m_vkContext.SwapChain.swapChain, &imageCount, m_images.data());


    CleanupTask task;
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_swapChain) };
    task.vkObjects = { m_swapChain };
    task.cleanupFunc = [&]() { vkDestroySwapchainKHR(m_vkContext.Device.logicalDevice, m_swapChain, nullptr); };

    uint32_t swapChainTaskID = m_garbageCollector->createCleanupTask(task);
    m_cleanupTaskIDs.push_back(swapChainTaskID);
}


void VkSwapchainManager::createImageViews() {
	if (m_images.empty()) {
		throw Log::RuntimeException(__FUNCTION__, "Cannot create image views: Swap-chain contains no m_images to process!");
	}

    m_imageViews.clear();
    m_imageViews.resize(m_images.size());

    for (size_t i = 0; i < m_images.size(); i++) {
        std::pair<VkImageView, uint32_t> imageViewProperties = createImageView(m_vkContext, m_images[i], m_vkContext.SwapChain.surfaceFormat.format);

        m_imageViews[i] = imageViewProperties.first;
        m_cleanupTaskIDs.push_back(imageViewProperties.second);
    }
}


void VkSwapchainManager::createFrameBuffers() {
    m_imageFrameBuffers.resize(m_imageViews.size());

    for (size_t i = 0; i < m_imageFrameBuffers.size(); i++) {
        if (m_imageViews[i] == VK_NULL_HANDLE) {
            throw Log::RuntimeException(__FUNCTION__, "Cannot read null image view!");
        }

        VkImageView attachments[] = {
            m_imageViews[i]
        };

        VkFramebufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        bufferCreateInfo.renderPass = m_vkContext.GraphicsPipeline.renderPass;
        bufferCreateInfo.attachmentCount = (sizeof(attachments) / sizeof(VkImageView));
        bufferCreateInfo.pAttachments = attachments;
        bufferCreateInfo.width = m_vkContext.SwapChain.extent.width;
        bufferCreateInfo.height = m_vkContext.SwapChain.extent.height;
        bufferCreateInfo.layers = 1; // Refer to swapChainCreateInfo.imageArrayLayers in createSwapChain()


        VkResult result = vkCreateFramebuffer(m_vkContext.Device.logicalDevice, &bufferCreateInfo, nullptr, &m_imageFrameBuffers[i]);
        if (result != VK_SUCCESS) {
            throw Log::RuntimeException(__FUNCTION__, "Failed to create frame buffer for image #" + std::to_string(i) + "!");
        }

        VkFramebuffer framebuffer = m_imageFrameBuffers[i];

        CleanupTask task{};
        task.caller = __FUNCTION__;
        task.objectNames = { VARIABLE_NAME(m_framebuffer) };
        task.vkObjects = { m_vkContext.Device.logicalDevice, framebuffer };
        task.cleanupFunc = [this, framebuffer]() { vkDestroyFramebuffer(m_vkContext.Device.logicalDevice, framebuffer, nullptr); };

        uint32_t framebufferTaskID = m_garbageCollector->createCleanupTask(task);
        m_cleanupTaskIDs.push_back(framebufferTaskID);
    }

    m_vkContext.SwapChain.imageFrameBuffers = m_imageFrameBuffers;
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
    // In other words, the resolution of the swap-chain m_images is equal to the resolution of the window that we're drawing to (in pixels).
    // Therefore, we must use the current extent as it is.
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    // Else, it means we can create possible resolutions within the [minImageExtent, maxImageExtent] range.
    // In this case, the best swap extent is the one whose resolution best matches the window.
    int width, height;
    glfwGetFramebufferSize(m_vkContext.window, &width, &height); // Gets the window resolution in pixels

    // Creates the best swap extent and populate it with the current window resolution
    VkExtent2D bestExtent{};
    bestExtent.width = static_cast<uint32_t>(width);
    bestExtent.height = static_cast<uint32_t>(height);

    // Clamps the width and height within the accepted bounds
    bestExtent.width = std::clamp(bestExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    bestExtent.height = std::clamp(bestExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return bestExtent;
}
