#include "Renderer.hpp"


Renderer::Renderer(): vulkInst(nullptr) {
    initVulkan();
    createPhysicalDevice();
    createLogicalDevice();
}


Renderer::~Renderer() {
    vkDestroyInstance(vulkInst, nullptr);
};


std::vector<VkExtensionProperties> Renderer::getSupportedVulkanExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // First get the no. of supported extensions

    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()); // Then call the function again to fill the vector
    // Fun fact: vector.data() == &vector[0] (Hint: 3rd parameter, vkEnumerateInstanceExtensionProperties function)
    return supportedExtensions;
}


std::vector<VkLayerProperties> Renderer::getSupportedVulkanValidationLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());

    return supportedLayers;
}


void Renderer::setVulkanValidationLayers(std::vector<const char*> layers) {
    if (enableValidationLayers && verifyVulkanValidationLayers(layers) == false) {
        throw std::runtime_error("Cannot set Vulkan validation layers: Provided layers are either invalid or unsupported!");
    }

    for (const auto& layer : layers) {
        if (UTIL_enabledValidationLayerSet.count(layer) == 0) {
            enabledValidationLayers.push_back(layer);
            UTIL_enabledValidationLayerSet.insert(layer);
        }
    }
}




/* [MEMBER] Initializes Vulkan. */
void Renderer::initVulkan() {
    // Sets up Vulkan extensions and validation layers
        // Caches supported extensions and layers
    supportedExtensions = getSupportedVulkanExtensions();
    supportedLayers = getSupportedVulkanValidationLayers();
    std::cout << "Supported extensions: " << supportedExtensions.size() << '\n';
    std::cout << "Supported layers: " << supportedLayers.size() << '\n';

    // Caches supported extension and validation layers for O(1) verification of extensions/layers to be added later
    for (const auto& extension : supportedExtensions)
        supportedExtensionNames.insert(extension.extensionName);

    for (const auto& layer : supportedLayers)
        supportedLayerNames.insert(layer.layerName);

    /* Rationale behind reserving:
    * The number of supported layers is constant. Therefore, we can reserve a fixed (maximum) block of memory for `enabledValidationLayers`
    * with the size being the number of supported layers to prevent O(n) vector reallocations
    */
    enabledValidationLayers.reserve(supportedLayers.size());

    // Sets validation layers to be bound to a Vulkan instance
    setVulkanValidationLayers({
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_crash_diagnostic",
        "VK_LAYER_LUNARG_screenshot"
        });


    // Creates Vulkan instance
    VkResult initResult = createVulkanInstance();
    if (initResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}


/*[MEMBER] Creates a Vulkan instance.
* @return a VkResult value indicating the instance creation status.
*/
VkResult Renderer::createVulkanInstance() {
    // Creates an application configuration for the driver
    VkApplicationInfo appInfo{}; // INSIGHT: Using braced initialization (alt.: memset) zero-initializes all fields
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Specifies structure type

    appInfo.pApplicationName = APP::APP_NAME;
    appInfo.pEngineName = APP::ENGINE_NAME;

    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Specifies which global extensions and validation layers are to be used
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    instanceInfo.pApplicationInfo = &appInfo;

    // Configures global extensions 
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (verifyVulkanExtensionValidity(glfwExtensions, glfwExtensionCount) == false) {
        throw std::runtime_error("GLFW Instance Extensions contain invalid or unsupported extensions!");
    }
    instanceInfo.enabledExtensionCount = glfwExtensionCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

    // Configures global validation layers
    if (enableValidationLayers) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(enabledValidationLayers.size());
        instanceInfo.ppEnabledLayerNames = enabledValidationLayers.data();
    }
    else {
        instanceInfo.enabledLayerCount = 0;
    }

    // Creates a Vulkan instance from the instance information configured above
    // and initializes the member VkInstance variable
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vulkInst);
    return result;
}


/* [MEMBER] Configures a GPU Physical Device by binding it to an appropriate GPU that supports needed features.
*/
void Renderer::createPhysicalDevice() {
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
}


/* [MEMBER] Creates a GPU Logical Device to interface with the Physical Device.
*/
void Renderer::createLogicalDevice() {
    
}


/* [MEMBER] Verifies whether a given array of Vulkan extensions is available or supported.
* @param arrayOfExtensions: An array containing the names of Vulkan extensions to be evaluated for validity.
* @param arraySize: The size of the array.
*
* @return True if all specified Vulkan extensions are supported, otherwise False.
*/
bool Renderer::verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t& arraySize) {
    bool allOK = true;
    for (uint32_t i = 0; i < arraySize; i++) {
        if (supportedExtensionNames.count(arrayOfExtensions[i]) == 0) {
            allOK = false;
            std::cerr << "Vulkan extension " << enquoteCOUT(arrayOfExtensions[i]) << " is either invalid or unsupported!\n";
        }

    }

    return allOK;
}


/* [MEMBER] Verifies whether a given vector of Vulkan validation layers is available or supported.
* @param layers: A vector containing the names of Vulkan validation layers to be evaluated for validity.
*
* @return True if all specified Vulkan validation layers are supported, otherwise False.
*/
bool Renderer::verifyVulkanValidationLayers(std::vector<const char*>& layers) {
    bool allOK = true;
    for (const auto& layer : layers) {
        if (supportedLayerNames.count(layer) == 0) {
            allOK = false;
            std::cerr << "Vulkan validation layer " << enquoteCOUT(layer) << " is either invalid or unsupported!\n";
        }
    }

    return allOK;
}


/* [MEMBER]: Grades a list of GPUs according to their suitability for Astrocelerate's features.
* @param physicalDevices: A vector of GPUs to be evaluated for suitability.
* 
* @return A vector containing the final scores of all GPUs in the list.
*/
std::vector<PhysicalDeviceScoreProperties> Renderer::rateGPUSuitability(std::vector<VkPhysicalDevice>& physicalDevices) {
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
            (queueFamilyIndices.graphicsFamily.index.has_value())
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


/* [MEMBER] Queries all GPU-supported queue families. 
* @param device: The GPU from which to query queue families.
* 
* @return A QueueFamilyIndices struct, with each family assigned to their corresponding index.
*/
QueueFamilyIndices Renderer::getQueueFamilies(VkPhysicalDevice& device) {
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
    * queueFlags (example)  =  01100100 00001010 1000110 11001001
    * VK_QUEUE_GRAPHICS_BIT =  00000000 00000000 0000000 00000001
    *                          ----------------------------------
    * CONDITION RESULTS     =  00000000 00000000 0000000 00000001_2 = 1_10 (1 => True)
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
    for (const auto& family : queueFamilies) {
        // If graphics queue family supports graphics operations
        if (family.queueFlags & familyIndices.graphicsFamily.FLAG) {
            familyIndices.graphicsFamily.index = index;
        }
        index++;
    }

    return familyIndices;
}