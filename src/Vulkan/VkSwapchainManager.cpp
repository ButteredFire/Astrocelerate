/* VkSwapchainManager.cpp - Vulkan swap-chain management implementation.
*/

#include "VkSwapchainManager.hpp"

VkSwapchainManager::VkSwapchainManager(GLFWwindow *window, VkCoreResourcesManager *coreResources) :
    m_window(window),
    m_surface(coreResources->getSurface()),
    m_physicalDevice(coreResources->getPhysicalDevice()),
    m_logicalDevice(coreResources->getLogicalDevice()),
    m_queueFamilies(coreResources->getQueueFamilyIndices()) {

    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
    m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

    bindEvents();
    init();

    // Initializes per-frame swapchain image layouts
    m_imageLayouts.assign(m_images.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void VkSwapchainManager::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<VkSwapchainManager>();

    m_eventDispatcher->subscribe<InitEvent::PresentPipeline>(selfIndex,
        [this](const InitEvent::PresentPipeline& event) {
            m_presentPipelineRenderPass = event.renderPass;

            this->createFrameBuffers();

            // At this point, all resources in the swapchain are ready.
            m_eventDispatcher->dispatch(InitEvent::SwapchainManager{});
        }
    );
}


void VkSwapchainManager::init() {
    // Initializes swap-chain
    createSwapChain();

    // Parses data for each image
    createImageViews();
}


void VkSwapchainManager::recreateSwapchain(uint32_t imageIndex, std::vector<VkFence> &inFlightFences) {
    m_eventDispatcher->dispatch(UpdateEvent::ApplicationStatus{
        .appState = Application::State::RECREATING_SWAPCHAIN    
    });

    // Wait for the host to be idle, and all frames to be processed
    vkWaitForFences(m_logicalDevice, static_cast<uint32_t>(inFlightFences.size()), inFlightFences.data(), VK_TRUE, UINT64_MAX);

    {
        // If the window is minimized (i.e., (width, height) = (0, 0), pause the window until it is in the foreground again
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        // Recreate swap-chain
            // We cannot clean up outdated swap-chain objects immediately, as we cannot know when the presentation engine has finished with the current swap-chain resources, including the semaphores it's waiting on.
            // To solve this, we can defer destruction until we are absolutely sure the resources are no longer in use.
        
        //for (const auto &taskID : m_cleanupTaskIDs) {
        //    m_garbageCollector->executeCleanupTask(taskID);
        //}
        std::vector<CleanupID> deferredDestructionList(m_cleanupTaskIDs.begin(), m_cleanupTaskIDs.end());
        m_cleanupTaskIDs.clear();

        // Explicitly remove the old swapchain cleanup ID from the destruction list in `recreateSwapchain`.
        // This is necessary because, by setting VkSwapchainCreateInfoKHR::oldSwapchain to the old swapchain handle, it is implicitly destroyed by being "consumed" by the new swapchain, and thus any vkDestroySwapchain call on the old swapchain would mean destroying the new one instead.
        auto newEnd = std::remove(deferredDestructionList.begin(), deferredDestructionList.end(), m_swapchainCleanupID);
        deferredDestructionList.erase(newEnd, deferredDestructionList.end());


            // Re-initialization
        VkSwapchainKHR oldSwapchain = m_swapChain;
        createSwapChain(oldSwapchain);
        createImageViews();
        createFrameBuffers();

        m_imageLayouts[imageIndex] = VK_IMAGE_LAYOUT_UNDEFINED;

        m_eventDispatcher->dispatch(RecreationEvent::Swapchain{
            .imageIndex = imageIndex,
            .imageLayouts = m_imageLayouts,
            .deferredDestructionList = deferredDestructionList
        });
    }

    m_eventDispatcher->dispatch(UpdateEvent::ApplicationStatus{
        .appState = Application::State::IDLE
    });
}


void VkSwapchainManager::createSwapChain(VkSwapchainKHR oldSwapchain) {
    SwapChainProperties swapChainProperties = GetSwapChainProperties(m_physicalDevice, m_surface);

    m_swapChainExtent = getBestSwapExtent(swapChainProperties.surfaceCapabilities);
    m_surfaceFormat = getBestSurfaceFormat(swapChainProperties.surfaceFormats);
    m_presentMode = getBestPresentMode(swapChainProperties.presentModes);

    // Specifies the number of m_images to be had in the swap-chain
    // It is recommended to request at least 1 more image than the minimum
    m_minImageCount = swapChainProperties.surfaceCapabilities.minImageCount + 1;

    // If the swap chain's image count has a maximum value (0 is a special value that means there is no maximum)
    // and if the desired image count exceeds that maximum,
    // default the image count to the maximum value.
    if (swapChainProperties.surfaceCapabilities.maxImageCount > 0 && m_minImageCount > swapChainProperties.surfaceCapabilities.maxImageCount) {
        m_minImageCount = swapChainProperties.surfaceCapabilities.maxImageCount;
    }

    // Creates the swap-chain structure
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_surface;

    swapChainCreateInfo.imageExtent = m_swapChainExtent;
    swapChainCreateInfo.imageFormat = m_surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = m_presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.minImageCount = m_minImageCount;

    // imageArrayLayers specifies the number of layers each image consists of. Its value is almost always 1,
    // unless you're developing a stereoscopic 3D application.
    swapChainCreateInfo.imageArrayLayers = 1;

    // imageUsage is a bitfield that specifies the type of operations the swap-chain m_images are used for.
    // NOTE:
    // - VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT means that the swap-chain m_images are used as color attachment. In other words, we will render directly to them.
    // If you want to render to a separate image first (for post-processing, etc.) before passing it to the swap-chain image via memory operations,
    // use other bits like VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::vector<uint32_t> familyIndices = m_queueFamilies.getAvailableIndices();

    // If the graphics family supports presentation (i.e., the presentation family is not separate),
    // set the image sharing mode to exclusive mode. MODE_EXCLUSIVE means that m_images are owned
    // by only 1 queue family at a time, and using them from another family requires ownership transference.
    if (m_queueFamilies.graphicsFamily.supportsPresentation) {
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

    // References the old swap-chain
    swapChainCreateInfo.oldSwapchain = oldSwapchain;

    // Creates a VkSwapchainKHR object
    VkResult result = vkCreateSwapchainKHR(m_logicalDevice, &swapChainCreateInfo, nullptr, &m_swapChain);
    
    LOG_ASSERT(result == VK_SUCCESS, "Failed to create swap-chain!");
    
    CleanupTask task;
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_swapChain) };
    task.vkHandles = { m_swapChain };
    task.cleanupFunc = [=]() { vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr); };

    m_swapchainCleanupID = m_garbageCollector->createCleanupTask(task);
    m_cleanupTaskIDs.push_back(m_swapchainCleanupID);


    // Fills swapChainImages with a set of VkImage objects provided after swap-chain creation
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &m_minImageCount, nullptr);
        m_images.resize(m_minImageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &m_minImageCount, m_images.data());
}


void VkSwapchainManager::createImageViews() {
	if (m_images.empty()) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot create image views: Swap-chain contains no m_images to process!");
	}

    m_imageViews.clear();
    m_imageViews.resize(m_images.size());

    for (size_t i = 0; i < m_images.size(); i++) {
        uint32_t viewCleanupID = VkImageManager::CreateImageView(m_imageViews[i], m_images[i], m_surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1);

        m_cleanupTaskIDs.push_back(viewCleanupID);
    }
}


void VkSwapchainManager::createFrameBuffers() {
    m_imageFrameBuffers.resize(m_imageViews.size());

    for (size_t i = 0; i < m_imageFrameBuffers.size(); i++) {
        LOG_ASSERT((m_imageViews[i] != VK_NULL_HANDLE), "Cannot read null image view!");

        std::vector<VkImageView> attachments = {
            m_imageViews[i]
        };
        uint32_t framebufferCleanupID = VkImageManager::CreateFramebuffer(m_imageFrameBuffers[i], m_presentPipelineRenderPass, attachments, m_swapChainExtent.width, m_swapChainExtent.height);

        m_cleanupTaskIDs.push_back(framebufferCleanupID);
    }
}


SwapChainProperties VkSwapchainManager::GetSwapChainProperties(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
        Log::Print(Log::T_WARNING, __FUNCTION__, "GPU does not support any surface formats for the given window surface!");
    }
    if (swapChain.presentModes.empty()) {
        Log::Print(Log::T_WARNING, __FUNCTION__, "GPU does not support any presentation modes for the given window surface!");
    }

    return swapChain;
}


VkSurfaceFormatKHR VkSwapchainManager::getBestSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats) {
    LOG_ASSERT(!formats.empty(), "Unable to get surface formats from an empty vector!");

    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return formats[0];
}


VkPresentModeKHR VkSwapchainManager::getBestPresentMode(std::vector<VkPresentModeKHR>& modes) {
    // FIFO_KHR (fallback): V-Sync => No screen tearing and predictable frame pacing, but introduces input lag
        // There is also FIFO_RELAXED_KHR.
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;


    // Iterate through supported modes to find the best fit
    for (const auto &availablePresentMode : modes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break; // Mailbox is generally preferred for performance
        }
        // If Mailbox isn't available, check for FIFO_RELAXED
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            chosenPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
    }


    return chosenPresentMode;
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
    glfwGetFramebufferSize(m_window, &width, &height); // Gets the window resolution in pixels

    // Creates the best swap extent and populate it with the current window resolution
    VkExtent2D bestExtent{};
    bestExtent.width = static_cast<uint32_t>(width);
    bestExtent.height = static_cast<uint32_t>(height);

    // Clamps the width and height within the accepted bounds
    bestExtent.width = std::clamp(bestExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    bestExtent.height = std::clamp(bestExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return bestExtent;
}
