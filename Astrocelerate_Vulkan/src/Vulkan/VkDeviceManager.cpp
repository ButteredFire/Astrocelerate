/* VkDeviceManager.cpp - Vulkan device management implementation.
*/


#include "VkDeviceManager.hpp"


VkDeviceManager::VkDeviceManager(VulkanContext &context):
    GPUPhysicalDevice(VK_NULL_HANDLE), GPULogicalDevice(VK_NULL_HANDLE), swapChain(VK_NULL_HANDLE),
    vulkInst(context.vulkanInstance), vkContext(context) {

    if (vulkInst == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot initialize device manager: Invalid Vulkan instance!");
    }

    if (vkContext.vkSurface == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot initialize device manager: Invalid Vulkan window surface!");
    }
}

VkDeviceManager::~VkDeviceManager() {
    vkDestroySwapchainKHR(GPULogicalDevice, swapChain, nullptr);
    vkDestroyDevice(GPULogicalDevice, nullptr);
}



void VkDeviceManager::init() {
    // Initializes required GPU extensions
    requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    // Creates a GPU device
    createPhysicalDevice();
    createLogicalDevice();

    // Creates swap-chain
    createSwapChain();
}


void VkDeviceManager::createPhysicalDevice() {
    // Queries available Vulkan-supported GPUs
    uint32_t physDeviceCount = 0;
    vkEnumeratePhysicalDevices(vulkInst, &physDeviceCount, nullptr);

    if (physDeviceCount == 0) {
        throw std::runtime_error("This machine does not have Vulkan-supported GPUs!");
    }

    VkPhysicalDevice physicalDevice = nullptr;
    std::vector<VkPhysicalDevice> physicalDevices(physDeviceCount);
    vkEnumeratePhysicalDevices(vulkInst, &physDeviceCount, physicalDevices.data());

    // Finds the most suitable GPU that supports Astrocelerate's features through GPU scoring
    GPUScores = rateGPUSuitability(physicalDevices);
    PhysicalDeviceScoreProperties bestDevice = *std::max_element(GPUScores.begin(), GPUScores.end(), ScoreComparator);

    physicalDevice = bestDevice.device;
    bool isDeviceCompatible = bestDevice.isCompatible;
    uint32_t physicalDeviceScore = bestDevice.optionalScore;

    std::cout << "\nList of GPUs and their scores:\n";
    for (auto& score : GPUScores)
        std::cout << "\t(GPU: " << enquoteCOUT(score.deviceName) << "; Compatible: " << std::boolalpha << score.isCompatible << "; Optional Score: " << score.optionalScore << ")\n";


    std::cout << "\nMost suitable GPU: (GPU: " << enquoteCOUT(bestDevice.deviceName) << "; Compatible: " << std::boolalpha << isDeviceCompatible << "; Optional Score: " << physicalDeviceScore << ")\n";
    if (inDebugMode) {
        std::cout << "NOTE: Should GPU selection be incorrect, please edit the source code to override the chosen GPU.\n";
        std::cout << "NOTE: Specifically, set `physicalDevice` in `VkDeviceManager::createPhysicalDevice` to a GPU in the vector `GPUScores`.\n";
    }

    if (physicalDevice == nullptr || !isDeviceCompatible) {
        throw std::runtime_error("Failed to find a GPU that supports Astrocelerate's features!");
    }

    vkContext.physicalDevice = GPUPhysicalDevice = physicalDevice;
}


void VkDeviceManager::createLogicalDevice() {
    QueueFamilyIndices queueFamilies = getQueueFamilies(GPUPhysicalDevice);

    // Verifies that all queue families exist before proceeding with device creation
    std::vector<QueueFamilyIndices::QueueFamily*> allFamilies = queueFamilies.getAllQueueFamilies();
    for (const auto &family : allFamilies)
        if (!queueFamilies.familyExists(*family))
            throw std::runtime_error("Unable to create logical device: Queue family is non-existent!");

    // Queues must have a priority in [0.0; 1.0], which influences the scheduling of command buffer execution.
    float queuePriority = 1.0f; 

    // Creates a set of all unique queue families that are necessary for the required queues
    std::vector<VkDeviceQueueCreateInfo> queues;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilies.graphicsFamily.index.value(),
        queueFamilies.presentationFamily.index.value()
    };

    for (auto& family : uniqueQueueFamilies) {
        // Specifies the queues to be created
        VkDeviceQueueCreateInfo queueInfo{};

        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        queues.push_back(queueInfo);
    }
    // Specifies the features of the device to be used
    VkPhysicalDeviceFeatures deviceFeatures{}; // We don't need anything special, so we'll just init all to VK_FALSE for now.

    // Creates the logical device
    VkDeviceCreateInfo deviceInfo{};

    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = queues.data();
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    
    deviceInfo.pEnabledFeatures = &deviceFeatures;

    /* A note about extensions and validation layers:
    * Extensions and validation layers can be classified into:
    * - Vulkan Instance extensions and layers
    * - Extensions and layers for specific Vulkan objects
    * 
    * In this case, when setting extensions and layers for deviceInfo, we're actually setting
    * device-specific extensions and layers (e.g., VK_KHR_swapchain).
    * 
    * Previous implementations of Vulkan made a distinction between instance and device specific validation layers, 
    * but this is no longer the case.
    * 
    * In the case of deviceInfo, it means that the enabledLayerCount and ppEnabledLayerNames fields are ignored by up-to-date implementations. 
    * However, it is still a good idea to set them anyway to be compatible with older implementations.
    */

    // Sets device-specific extensions
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    // Sets device-specific validation layers
    if (inDebugMode) {
        deviceInfo.enabledLayerCount = static_cast<uint32_t> (vkContext.enabledValidationLayers.size());
        deviceInfo.ppEnabledLayerNames = vkContext.enabledValidationLayers.data();
    }
    else {
        deviceInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(GPUPhysicalDevice, &deviceInfo, nullptr, &GPULogicalDevice);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Unable to create GPU logical device!");
    }

    // Populates each (available) family's device queue
    std::vector<QueueFamilyIndices::QueueFamily*> availableFamilies = queueFamilies.getAvailableQueueFamilies(allFamilies);
    for (uint32_t queueIndex = 0; queueIndex < deviceInfo.queueCreateInfoCount; queueIndex++) {
        // Remember to use `auto&` and not `auto` to prevent modifying a copy instead of the original
        auto& family = *availableFamilies[queueIndex];
        vkGetDeviceQueue(GPULogicalDevice, family.index.value(), queueIndex, &family.deviceQueue);
    }

        // If graphics queue family supports presentation operations (i.e., the presentation queue is not separate),
        // then set the presentation family's index and VkQueue to be the same as the graphics family's.
    if (queueFamilies.graphicsFamily.supportsPresentation) {
        queueFamilies.presentationFamily.index = queueFamilies.graphicsFamily.index;
        queueFamilies.presentationFamily.deviceQueue = queueFamilies.graphicsFamily.deviceQueue;
    }

    vkContext.logicalDevice = GPULogicalDevice;
}


void VkDeviceManager::createSwapChain() {
    SwapChainProperties swapChainProperties = getSwapChainProperties(GPUPhysicalDevice);

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

    QueueFamilyIndices families = getQueueFamilies(GPUPhysicalDevice);
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
    VkResult result = vkCreateSwapchainKHR(GPULogicalDevice, &swapChainCreateInfo, nullptr, &swapChain);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap-chain!");
    }
    
    vkContext.swapChain = swapChain;
}



std::vector<PhysicalDeviceScoreProperties> VkDeviceManager::rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices) {
    std::vector<PhysicalDeviceScoreProperties> GPUScores;
    // Grades each device
    for (VkPhysicalDevice& device : physicalDevices) {
        // Queries basic device properties and optional features (e.g., 64-bit floats for accurate physics computations)
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // Creates a device rating profile
        PhysicalDeviceScoreProperties deviceRating;
        deviceRating.device = device;
        deviceRating.deviceName = deviceProperties.deviceName;

        // Creates a list of indices of device-supported queue families for later checking
        QueueFamilyIndices queueFamilyIndices = getQueueFamilies(device);

        // Creates the GPU's swap-chain properties
        SwapChainProperties swapChain = getSwapChainProperties(device);
        
        // A "list" of minimum requirements; Variable will collapse to "true" if all are satisfied
        bool meetsMinimumRequirements = (
            // If the GPU supports geometry shaders
            (deviceFeatures.geometryShader) &&

            // If the GPU supports required device extensions
            (checkDeviceExtensionSupport(device, requiredDeviceExtensions)) &&

            // If the GPU has an API version above 1.0
            (deviceProperties.apiVersion > VK_API_VERSION_1_0) &&

            // If the GPU has a graphics queue family
            (queueFamilyIndices.graphicsFamily.index.has_value()) &&

            // If the GPU has either:
            // 1. A dedicated presentation queue family, OR
            // 2. A graphics queue family that also supports presentation
            (queueFamilyIndices.presentationFamily.index.has_value() ||
            queueFamilyIndices.graphicsFamily.supportsPresentation) &&

            // If the GPU's swap-chain:
            // 1. is compatible with the window surface, AND
            // 2. supports presentation modes
            // NOTE: This check must be put before the device extension check
            // (to ensure that the swap-chain actually exists before checking)
            (!swapChain.surfaceFormats.empty() && !swapChain.presentModes.empty())
        );

        if (!meetsMinimumRequirements) {
            deviceRating.isCompatible = false;
            GPUScores.push_back(deviceRating);
            continue;
        }

        std::vector<std::pair<bool, uint32_t>> optionalFeatures = {
            // Discrete GPUs have a significant performance advantage
            {deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 3},

            // Vulkan 1.2 unifies many extensions and improves stability
            {deviceProperties.apiVersion >= VK_API_VERSION_1_2, 1},

            // Vulkan 1.3 adds dynamic rendering, reducing the need for render passes
            {deviceProperties.apiVersion >= VK_API_VERSION_1_3, 1},

            // 64-bit floats enable accurate physics computations 
            {deviceFeatures.shaderFloat64, 2},

            // Maximum possible size of textures affects graphics quality
            {true, deviceProperties.limits.maxImageDimension2D}
        };

        for (const auto& [hasFeature, weight] : optionalFeatures) {
            if (hasFeature)
                deviceRating.optionalScore += weight;
        }

        // Adds device rating to the list
        GPUScores.push_back(deviceRating);
    }

    return GPUScores;
}


QueueFamilyIndices VkDeviceManager::getQueueFamilies(VkPhysicalDevice& device) {
    /* Explanation behind VkQueueFamilyProperties::queueFlags (family.queueFlags below):
    * Vulkan uses bitfields for queue flags (i.e., queueFlags is a bitfield: a set of flags stored in a single integer).
    * Vulkan uses bitfields for queue capabilities because a queue family can support multiple operations simultaneously
    * (e.g., graphics, compute, transfer). Instead of using separate boolean variables, it stores these capabilities in a single integer,
    * where each bit represents a different queue type.
    *
    * Therefore, to check if a queue family supports an operation, you can just use the bitwise AND between queueFlags and the operation's bit.
    * If (queueFlags & VK_QUEUE_[OPERATION]_BIT != 0), meaning that that operation flag/bit is TRUE/On, then the queue family supports it.
    *
    * Example: To check whether a queue family supports graphics operations, perform an AND operation between queueFlags and VK_QUEUE_GRAPHICS_BIT:
    *
    * queueFlags (example)     01100100 00001010 1000110 11001001
    * VK_QUEUE_GRAPHICS_BIT    00000000 00000000 0000000 00000001
    *                        & ----------------------------------
    * CONDITION RESULTS        00000000 00000000 0000000 00000001_2 = 1_10 (1 => True)
    *
    * Of course, the condition result can be any number (not just 0 or 1) because it returns an integer.
    *
    * Essentially, Vulkan allows multiple capabilities to be stored in a single variable while enabling fast bitwise checks.
    * Ngl, this is a pretty fucking clever approach to efficient memory management!
    */
    QueueFamilyIndices familyIndices;
    familyIndices.init();

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, queueFamilies.data());

    uint32_t index = 0;
    for (uint32_t index = 0; index < familyCount; index++) {
        const auto &family = queueFamilies[index];

        // If current family is able to present rendered images to the window surface
        VkBool32 supportsPresentations = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, vkContext.vkSurface, &supportsPresentations);
        
        // If current family supports graphics operations
        bool supportsGraphics = ((family.queueFlags & familyIndices.graphicsFamily.FLAG) != 0);

        if (supportsGraphics) {
            familyIndices.graphicsFamily.index = index;

            // If graphics queue family also supports presentation
            if (supportsPresentations) {
                familyIndices.graphicsFamily.supportsPresentation = true;
                familyIndices.presentationFamily.index = index;
            }
        }

        // If there exists a separate presentation family (that is separate from the graphics family)
        if (!familyIndices.graphicsFamily.supportsPresentation && supportsPresentations) {
            familyIndices.presentationFamily.index = index;
        }


        index++;
    }

    return familyIndices;
}


SwapChainProperties VkDeviceManager::getSwapChainProperties(VkPhysicalDevice& device) {
    SwapChainProperties swapChain;

    // Queries swap-chain properties
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkContext.vkSurface, &swapChain.surfaceCapabilities);

    // Queries surface formats
    uint32_t numOfSurfaceFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkContext.vkSurface, &numOfSurfaceFormats, nullptr);

    swapChain.surfaceFormats.resize(numOfSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkContext.vkSurface, &numOfSurfaceFormats, swapChain.surfaceFormats.data());

    // Queries surface present modes
    uint32_t numOfPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkContext.vkSurface, &numOfPresentModes, nullptr);

    swapChain.presentModes.resize(numOfPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkContext.vkSurface, &numOfPresentModes, swapChain.presentModes.data());


    if (swapChain.surfaceFormats.empty()) {
        std::cerr << "Warning: GPU does not support any surface formats for the given window surface!" << '\n';
    }
    if (swapChain.presentModes.empty()) {
        std::cerr << "Warning: GPU does not support any presentation modes for the given window surface!" << '\n';
    }

    return swapChain;
}


VkSurfaceFormatKHR VkDeviceManager::getBestSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.empty()) {
        throw std::runtime_error("Unable to get surface formats from an empty vector!");
    }
    
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    
    return formats[0];
}


VkPresentModeKHR VkDeviceManager::getBestPresentMode(std::vector<VkPresentModeKHR>& modes) {
    for (const auto& mode : modes) {
        // MAILBOX_KHR: Triple buffering => Best for performance and smoothness, but requires more GPU memory
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    // FIFO_KHR (fallback): V-Sync => No screen tearing and predictable frame pacing, but introduces input lag
    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VkDeviceManager::getBestSwapExtent(VkSurfaceCapabilitiesKHR& capabilities) {
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


bool VkDeviceManager::checkDeviceExtensionSupport(VkPhysicalDevice& device, std::vector<const char*>& extensions) {
    uint32_t numOfExtensions = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &numOfExtensions, nullptr);

    std::vector<VkExtensionProperties> deviceExtensions(numOfExtensions);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &numOfExtensions, deviceExtensions.data());

    // Searches device extensions to check for existence
    bool allOK = true;
    for (const auto& ext : extensions) {
        bool found = false;

        for (const auto& devExt : deviceExtensions)
            if (strcmp(devExt.extensionName, ext) == 0) {
                found = true;
                break;
            }

        if (!found) {
            allOK = false;
            std::cerr << "Device extension " << enquoteCOUT(ext) << " is not supported!\n";
        }
    }

    return allOK;
}
