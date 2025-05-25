/* VkDeviceManager.cpp - Vulkan device management implementation.
*/


#include "VkDeviceManager.hpp"


VkDeviceManager::VkDeviceManager(VulkanContext &context):
    m_vulkInst(context.vulkanInstance), m_vkContext(context) {

    m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

    if (m_vulkInst == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot initialize device manager: Invalid Vulkan instance!");
    }

    if (m_vkContext.vkSurface == VK_NULL_HANDLE) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot initialize device manager: Invalid Vulkan window surface!");
    }

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


VkDeviceManager::~VkDeviceManager() {}



void VkDeviceManager::init() {
    // Initializes required GPU extensions
    m_requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };


    // Creates a GPU device
    createPhysicalDevice();
    createLogicalDevice();

    // Creates a VMA
    m_vmaAllocator = m_garbageCollector->createVMAllocator(m_vkContext.vulkanInstance, m_GPUPhysicalDevice, m_GPULogicalDevice);
}


void VkDeviceManager::createPhysicalDevice() {
    // Queries available Vulkan-supported GPUs
    uint32_t physDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_vulkInst, &physDeviceCount, nullptr);

    if (physDeviceCount == 0) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "This machine does not have Vulkan-supported GPUs!");
    }

    VkPhysicalDevice physicalDevice = nullptr;
    std::vector<VkPhysicalDevice> physicalDevices(physDeviceCount);
    vkEnumeratePhysicalDevices(m_vulkInst, &physDeviceCount, physicalDevices.data());

    // Finds the most suitable GPU that supports Astrocelerate's features through GPU scoring
    m_GPUScores = rateGPUSuitability(physicalDevices);
    PhysicalDeviceScoreProperties bestDevice = *std::max_element(m_GPUScores.begin(), m_GPUScores.end(), ScoreComparator);

    physicalDevice = bestDevice.device;
    bool isDeviceCompatible = bestDevice.isCompatible;
    uint32_t physicalDeviceScore = bestDevice.optionalScore;

    //std::cout << "\nFinal GPU evaluation:\n";
    //for (auto& score : m_GPUScores)
    //    std::cout << "\t(GPU: " << enquoteCOUT(score.deviceName) << "; Compatible: " << std::boolalpha << score.isCompatible << "; Optional Score: " << score.optionalScore << ")\n";

    Log::print(Log::T_INFO, __FUNCTION__, ("Out of " + std::to_string(physDeviceCount) + " GPU(s), GPU " + enquote(bestDevice.deviceName) + " was selected with the highest grading score of " + std::to_string(physicalDeviceScore) + "."));
    //std::cout << "Most suitable GPU: (GPU: " << enquoteCOUT(bestDevice.deviceName) << "; Compatible: " << std::boolalpha << isDeviceCompatible << "; Optional Score: " << physicalDeviceScore << ")\n\n";

    if (physicalDevice == nullptr || !isDeviceCompatible) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to find a GPU that supports required features!");
    }

    m_vkContext.Device.physicalDevice = m_GPUPhysicalDevice = physicalDevice;
}


void VkDeviceManager::createLogicalDevice() {
    QueueFamilyIndices queueFamilies = getQueueFamilies(m_GPUPhysicalDevice, m_vkContext.vkSurface);

    // Verifies that all queue families exist before proceeding with device creation
    std::vector<QueueFamilyIndices::QueueFamily*> allFamilies = queueFamilies.getAllQueueFamilies();
    for (const auto &family : allFamilies)
        if (!queueFamilies.familyExists(*family)) {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Unable to create logical device: " + (family->deviceName) + " is non-existent!");
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
        // Base features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

        // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features deviceVk12Features{};
    deviceVk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceVk12Features.bufferDeviceAddress = VK_TRUE;
    deviceVk12Features.descriptorIndexing = VK_TRUE;


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
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_requiredDeviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = m_requiredDeviceExtensions.data();

    // Sets device-specific validation layers
    if (inDebugMode) {
        deviceInfo.enabledLayerCount = static_cast<uint32_t> (m_vkContext.enabledValidationLayers.size());
        deviceInfo.ppEnabledLayerNames = m_vkContext.enabledValidationLayers.data();
    }
    else {
        deviceInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(m_GPUPhysicalDevice, &deviceInfo, nullptr, &m_GPULogicalDevice);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Unable to create GPU logical device!");
    }

    // Populates each (available) family's device queue
    std::vector<QueueFamilyIndices::QueueFamily*> availableFamilies = queueFamilies.getAvailableQueueFamilies(allFamilies);
    for (auto& family : availableFamilies)
        vkGetDeviceQueue(m_GPULogicalDevice, family->index.value(), 0, &family->deviceQueue);

        // If graphics queue family supports presentation operations (i.e., the presentation queue is not separate),
        // then set the presentation family's index and VkQueue to be the same as the graphics family's.
    if (queueFamilies.graphicsFamily.supportsPresentation) {
        queueFamilies.presentationFamily.index = queueFamilies.graphicsFamily.index;
        queueFamilies.presentationFamily.deviceQueue = queueFamilies.graphicsFamily.deviceQueue;
    }


    m_vkContext.Device.logicalDevice = m_GPULogicalDevice;
    m_vkContext.Device.queueFamilies = queueFamilies;


    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_GPULogicalDevice) };
    task.vkObjects = { m_GPULogicalDevice };
    task.cleanupFunc = [this]() { vkDestroyDevice(m_GPULogicalDevice, nullptr); };

    m_garbageCollector->createCleanupTask(task);
}


std::vector<PhysicalDeviceScoreProperties> VkDeviceManager::rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices) {
    std::vector<PhysicalDeviceScoreProperties> m_GPUScores;
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

        m_vkContext.Device.deviceProperties = deviceProperties;

        // Creates a device rating profile
        PhysicalDeviceScoreProperties deviceRating;
        deviceRating.device = device;
        deviceRating.deviceName = deviceProperties.deviceName;

        // Creates a list of indices of device-supported queue families for later checking
        QueueFamilyIndices queueFamilyIndices = getQueueFamilies(device, m_vkContext.vkSurface);

        // Creates the GPU's swap-chain properties
        SwapChainProperties swapChain = VkSwapchainManager::getSwapChainProperties(device, m_vkContext.vkSurface);
        
        // A "list" of minimum requirements; Variable will collapse to "true" if all are satisfied
        const bool meetsMinimumRequirements = (
            // If the GPU has an API version >= the instance Vulkan version
            (deviceProperties.apiVersion >= VULKAN_VERSION) &&

            // If the GPU supports geometry shaders
            (deviceFeatures.geometryShader) &&

            // If the GPU supports anisotropy filtering
            (deviceFeatures.samplerAnisotropy) &&

            // If the GPU supports required device extensions
            (checkDeviceExtensionSupport(device, m_requiredDeviceExtensions)) &&

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
            m_GPUScores.push_back(deviceRating);
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
        m_GPUScores.push_back(deviceRating);
    }

    return m_GPUScores;
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
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Device extension " + enquote(ext) + " is not supported!");
        }
    }

    return allOK;
}
