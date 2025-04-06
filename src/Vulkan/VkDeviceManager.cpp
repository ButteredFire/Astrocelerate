/* VkDeviceManager.cpp - Vulkan device management implementation.
*/


#include "VkDeviceManager.hpp"


VkDeviceManager::VkDeviceManager(VulkanContext &context, bool autoCleanup):
    vulkInst(context.vulkanInstance), vkContext(context), cleanOnDestruction(autoCleanup) {

    memoryManager = ServiceLocator::getService<MemoryManager>();

    if (vulkInst == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, "Cannot initialize device manager: Invalid Vulkan instance!");
    }

    if (vkContext.vkSurface == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, "Cannot initialize device manager: Invalid Vulkan window surface!");
    }

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


VkDeviceManager::~VkDeviceManager() {
    if (cleanOnDestruction)
        cleanup();
}



void VkDeviceManager::init() {
    // Initializes required GPU extensions
    requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };


    // Creates a GPU device
    createPhysicalDevice();
    createLogicalDevice();

    // Creates a VMA
    vmaAllocator = memoryManager->createVMAllocator(vkContext.vulkanInstance, GPUPhysicalDevice, GPULogicalDevice);
}


void VkDeviceManager::cleanup() {
    Log::print(Log::T_INFO, __FUNCTION__, "Cleaning up...");

    if (vkIsValid(GPULogicalDevice))
        vkDestroyDevice(GPULogicalDevice, nullptr);
}


void VkDeviceManager::createPhysicalDevice() {
    // Queries available Vulkan-supported GPUs
    uint32_t physDeviceCount = 0;
    vkEnumeratePhysicalDevices(vulkInst, &physDeviceCount, nullptr);

    if (physDeviceCount == 0) {
        cleanup();
        throw Log::RuntimeException(__FUNCTION__, "This machine does not have Vulkan-supported GPUs!");
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

    //std::cout << "\nFinal GPU evaluation:\n";
    //for (auto& score : GPUScores)
    //    std::cout << "\t(GPU: " << enquoteCOUT(score.deviceName) << "; Compatible: " << std::boolalpha << score.isCompatible << "; Optional Score: " << score.optionalScore << ")\n";

    Log::print(Log::T_INFO, __FUNCTION__, ("Selected GPU " + enquote(bestDevice.deviceName)));
    //std::cout << "Most suitable GPU: (GPU: " << enquoteCOUT(bestDevice.deviceName) << "; Compatible: " << std::boolalpha << isDeviceCompatible << "; Optional Score: " << physicalDeviceScore << ")\n\n";

    if (physicalDevice == nullptr || !isDeviceCompatible) {
        cleanup();
        throw Log::RuntimeException(__FUNCTION__, "Failed to find a GPU that supports required features!");
    }

    vkContext.physicalDevice = GPUPhysicalDevice = physicalDevice;
}


void VkDeviceManager::createLogicalDevice() {
    QueueFamilyIndices queueFamilies = getQueueFamilies(GPUPhysicalDevice, vkContext.vkSurface);

    // Verifies that all queue families exist before proceeding with device creation
    std::vector<QueueFamilyIndices::QueueFamily*> allFamilies = queueFamilies.getAllQueueFamilies();
    for (const auto &family : allFamilies)
        if (!queueFamilies.familyExists(*family)) {
            cleanup();
            throw Log::RuntimeException(__FUNCTION__, "Unable to create logical device: " + (family->deviceName) + " is non-existent!");
        }

    // Queues must have a priority in [0.0; 1.0], which influences the scheduling of command buffer execution.
    float queuePriority = 1.0f; 

    // Creates a set of all unique queue families that are necessary for the required queues
    std::vector<VkDeviceQueueCreateInfo> queues;
    std::set<uint32_t> uniqueQueueFamilies;
    for (const auto& family : allFamilies) {
        uniqueQueueFamilies.insert(family->index.value());
    }


    // Creates a device queue for each queue family
    for (auto& family : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueInfo{};

        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1; // Creates only 1 queue per queue family
        queueInfo.pQueuePriorities = &queuePriority;
        queues.push_back(queueInfo);
    }


    // Specifies the features of the device to be used
    VkPhysicalDeviceFeatures deviceFeatures{};  // Vulkan 1.0 features (it's unused, but must still be defined anyway because it's used in `VkDeviceCreateInfo`)

    VkPhysicalDeviceVulkan12Features deviceVk12Features{}; // Vulkan 1.2 features
    deviceVk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceVk12Features.bufferDeviceAddress = VK_TRUE;

    // Creates the logical device
    VkDeviceCreateInfo deviceInfo{};

    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = queues.data();
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.pNext = &deviceVk12Features;

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
        throw Log::RuntimeException(__FUNCTION__, "Unable to create GPU logical device!");
    }

    // Populates each (available) family's device queue
    std::vector<QueueFamilyIndices::QueueFamily*> availableFamilies = queueFamilies.getAvailableQueueFamilies(allFamilies);
    for (auto& family : availableFamilies)
        vkGetDeviceQueue(GPULogicalDevice, family->index.value(), 0, &family->deviceQueue);

        // If graphics queue family supports presentation operations (i.e., the presentation queue is not separate),
        // then set the presentation family's index and VkQueue to be the same as the graphics family's.
    if (queueFamilies.graphicsFamily.supportsPresentation) {
        queueFamilies.presentationFamily.index = queueFamilies.graphicsFamily.index;
        queueFamilies.presentationFamily.deviceQueue = queueFamilies.graphicsFamily.deviceQueue;
    }


    vkContext.logicalDevice = GPULogicalDevice;
    vkContext.queueFamilies = queueFamilies;


    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.mainObjectName = VARIABLE_NAME(GPULogicalDevice);
    task.vkObjects = { GPULogicalDevice };
    task.cleanupFunc = [this]() { vkDestroyDevice(GPULogicalDevice, nullptr); };

    memoryManager->createCleanupTask(task);
}


std::vector<PhysicalDeviceScoreProperties> VkDeviceManager::rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices) {
    std::vector<PhysicalDeviceScoreProperties> GPUScores;
    // Grades each device
    for (VkPhysicalDevice& device : physicalDevices) {
        // Queries basic device properties and optional features (e.g., 64-bit floats for accurate physics computations)
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;


        VkPhysicalDeviceVulkan12Features deviceVk12Features{};
        deviceVk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &deviceVk12Features;
        
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2); // Vulkan 1.2

        // Creates a device rating profile
        PhysicalDeviceScoreProperties deviceRating;
        deviceRating.device = device;
        deviceRating.deviceName = deviceProperties.deviceName;

        // Creates a list of indices of device-supported queue families for later checking
        QueueFamilyIndices queueFamilyIndices = getQueueFamilies(device, vkContext.vkSurface);

        // Creates the GPU's swap-chain properties
        SwapChainProperties swapChain = VkSwapchainManager::getSwapChainProperties(device, vkContext.vkSurface);
        
        // A "list" of minimum requirements; Variable will collapse to "true" if all are satisfied
        bool meetsMinimumRequirements = (
            // If the GPU has an API version >= the instance Vulkan version
            (deviceProperties.apiVersion >= VULKAN_VERSION) &&

            // If the GPU supports geometry shaders
            (deviceFeatures.geometryShader) &&

            // If the GPU supports the buffer device address feature
            (deviceVk12Features.bufferDeviceAddress) &&

            // If the GPU supports required device extensions
            (checkDeviceExtensionSupport(device, requiredDeviceExtensions)) &&

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


QueueFamilyIndices VkDeviceManager::getQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supportsPresentations);
        
        // If current family supports graphics operations
        bool supportsGraphics = ((family.queueFlags & familyIndices.graphicsFamily.FLAG) != 0);

        // If current family is a transfer queue family
        // NOTE: A graphics queue family also implicitly suppports transfer operations, but we are looking for a dedicated transfer queue family.
        bool isTransferFamily = (((family.queueFlags & familyIndices.transferFamily.FLAG) != 0) && (!supportsGraphics));

        if (supportsGraphics) {
            familyIndices.graphicsFamily.index = index;

            // If graphics queue family also supports presentation
            if (supportsPresentations) {
                familyIndices.graphicsFamily.supportsPresentation = true;
                familyIndices.presentationFamily.index = index;
            }
        }

        if (isTransferFamily) {
            familyIndices.transferFamily.index = index;
        }

        // If there exists a separate presentation family (that is separate from the graphics family)
        if (!familyIndices.graphicsFamily.supportsPresentation && supportsPresentations) {
            familyIndices.presentationFamily.index = index;
        }


        index++;
    }

    return familyIndices;
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
            Log::print(Log::T_ERROR, __FUNCTION__, "Device extension " + enquote(ext) + " is not supported!");
        }
    }

    return allOK;
}
