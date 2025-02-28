/* VkDeviceManager.cpp - Vulkan device management implementation.
*/


#include "VkDeviceManager.hpp"


VkDeviceManager::VkDeviceManager(VulkanContext &context):
    GPUPhysicalDevice(VK_NULL_HANDLE), GPULogicalDevice(VK_NULL_HANDLE),
    vulkInst(context.vulkanInstance), vkContext(context) {

    if (vulkInst == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot initialize device manager: Invalid Vulkan instance!");
    }

    if (vkContext.vkSurface == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot initialize device manager: Invalid Vulkan window surface!");
    }
}

VkDeviceManager::~VkDeviceManager() {
    vkDestroyDevice(GPULogicalDevice, nullptr);
}



void VkDeviceManager::init() {
    // Creates a GPU device
    vkContext.physicalDevice = createPhysicalDevice();
    vkContext.logicalDevice = createLogicalDevice();

}


VkPhysicalDevice VkDeviceManager::createPhysicalDevice() {
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

    if (physicalDevice == nullptr || !isDeviceCompatible) {
        throw std::runtime_error("Failed to find a GPU that supports Astrocelerate's features!");
    }

    GPUPhysicalDevice = physicalDevice;

    return GPUPhysicalDevice;
}


VkDevice VkDeviceManager::createLogicalDevice() {
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
    deviceInfo.enabledExtensionCount = 0; // We don't need any for now

    // Sets device-specific validation layers
    if (enableValidationLayers) {
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
        // then set the presentation family's VkQueue to be the same as the graphics family's.
    if (queueFamilies.graphicsFamily.supportsPresentation) {
        queueFamilies.presentationFamily.deviceQueue = queueFamilies.graphicsFamily.deviceQueue;
    }

    return GPULogicalDevice;
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

        // A "list" of minimum requirements; Variable will collapse to "true" if all are satisfied
        bool meetsMinimumRequirements = (
            // If the GPU supports geometry shaders
            (deviceFeatures.geometryShader) &&

            // If the GPU has an API version above 1.0
            (deviceProperties.apiVersion > VK_API_VERSION_1_0) &&

            // If the GPU has a graphics queue family
            (queueFamilyIndices.graphicsFamily.index.has_value()) &&

            // If the GPU has either:
            // 1. A dedicated presentation queue family
            // 2. A graphics queue family that also supports presentation
            (queueFamilyIndices.presentationFamily.index.has_value() ||
            queueFamilyIndices.graphicsFamily.supportsPresentation)
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
