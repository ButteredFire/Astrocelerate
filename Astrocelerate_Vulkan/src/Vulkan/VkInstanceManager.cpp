/* VkInstanceManager.cpp - Vulkan instance management implementation.
*/


#include "VkInstanceManager.hpp"

VkInstanceManager::VkInstanceManager(VulkanContext &context):
    vulkInst(nullptr), vkContext(context) {}

VkInstanceManager::~VkInstanceManager() {
    vkDestroyInstance(vulkInst, nullptr);
}



void VkInstanceManager::init() {
    vkContext.vulkanInstance = initVulkan();
}


VkInstance VkInstanceManager::initVulkan() {
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

    return vulkInst;
}


VkResult VkInstanceManager::createVulkanInstance() {
    // Specifies an application configuration for the driver
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


bool VkInstanceManager::verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t& arraySize) {
    bool allOK = true;
    for (uint32_t i = 0; i < arraySize; i++) {
        if (supportedExtensionNames.count(arrayOfExtensions[i]) == 0) {
            allOK = false;
            std::cerr << "Vulkan extension " << enquoteCOUT(arrayOfExtensions[i]) << " is either invalid or unsupported!\n";
        }

    }

    return allOK;
}


bool VkInstanceManager::verifyVulkanValidationLayers(std::vector<const char*>& layers) {
    bool allOK = true;
    for (const auto& layer : layers) {
        if (supportedLayerNames.count(layer) == 0) {
            allOK = false;
            std::cerr << "Vulkan validation layer " << enquoteCOUT(layer) << " is either invalid or unsupported!\n";
        }
    }

    return allOK;
}


std::vector<VkExtensionProperties> VkInstanceManager::getSupportedVulkanExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // First get the no. of supported extensions

    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()); // Then call the function again to fill the vector
    // Fun fact: vector.data() == &vector[0] (Hint: 3rd parameter, vkEnumerateInstanceExtensionProperties function)
    return supportedExtensions;
}


std::vector<VkLayerProperties> VkInstanceManager::getSupportedVulkanValidationLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());

    return supportedLayers;
}


void VkInstanceManager::setVulkanValidationLayers(std::vector<const char*> layers) {
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
