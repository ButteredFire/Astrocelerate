/* VkInstanceManager.cpp - Vulkan instance management implementation.
*/


#include "VkInstanceManager.hpp"

VkInstanceManager::VkInstanceManager(VulkanContext &context):
    vkContext(context) {}

VkInstanceManager::~VkInstanceManager() {
    cleanup();
}


void VkInstanceManager::init() {
    initVulkan();
}


void VkInstanceManager::cleanup() {
    vkDestroySurfaceKHR(vulkInst, windowSurface, nullptr);

    // Destroy the instance last (because the destruction of other Vulkan objects depend on it)
    vkDestroyInstance(vulkInst, nullptr);
}


void VkInstanceManager::initVulkan() {
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
    * The number of supported extensions/layers is constant. Therefore, we can reserve a fixed (maximum) block of memory
    * for enabled extensions/layers vectors to prevent O(n) vector reallocations.
    */
    enabledExtensions.reserve(supportedExtensions.size());
    enabledValidationLayers.reserve(supportedLayers.size());

    // Sets validation layers to be bound to a Vulkan instance
    addVulkanValidationLayers({
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_crash_diagnostic",
        "VK_LAYER_LUNARG_screenshot"
    });


    // Creates Vulkan instance
    VkResult initResult = createVulkanInstance();
    if (initResult != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
    vkContext.vulkanInstance = vulkInst;

    // Creates Vulkan surface
    VkResult surfaceInitResult = createSurface();
    if (surfaceInitResult != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("Failed to create Vulkan window surface!");
    }
    vkContext.vkSurface = windowSurface;
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

        // Copies GLFW extensions into enabledExtensions
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        enabledExtensions.push_back(glfwExtensions[i]);

    if (verifyVulkanExtensions(enabledExtensions) == false) {
        enabledExtensions.clear();
        cleanup();
        throw std::runtime_error("GLFW Instance Extensions contain invalid or unsupported extensions!");
    }

    instanceInfo.enabledExtensionCount = glfwExtensionCount;
    instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

    // Configures global validation layers
    if (inDebugMode) {
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


VkResult VkInstanceManager::createSurface() {
    /* Using glfwCreateWindowSurface to create a window surface is platform-agnostic.
    * On the other hand, while the VkSurfaceKHR object is itself platform agnostic, its creation isn't
    * because it depends on window system details (meaning that the creation structs vary across platforms,
    * e.g., VkWin32SurfaceCreateInfoKHR for Windows).
    */
    VkResult result = glfwCreateWindowSurface(vulkInst, vkContext.window, nullptr, &windowSurface);
    return result;
}


bool VkInstanceManager::verifyVulkanExtensions(std::vector<const char*> extensions) {
    bool allOK = true;
    for (const auto& ext : extensions) {
        if (supportedExtensionNames.count(ext) == 0) {
            allOK = false;
            std::cerr << "Vulkan extension " << enquoteCOUT(ext) << " is either invalid or unsupported!\n";
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


void VkInstanceManager::addVulkanExtensions(std::vector<const char*> extensions) {
    if (verifyVulkanExtensions(extensions) == false) {
        cleanup();
        throw std::runtime_error("Cannot set Vulkan extensions: Provided extensions are either invalid or unsupported!");
    }

    for (const auto& ext : extensions) {
        if (UTIL_enabledExtensionSet.count(ext) == 0) {
            enabledExtensions.push_back(ext);
            UTIL_enabledExtensionSet.insert(ext);
        }
    }
}


void VkInstanceManager::addVulkanValidationLayers(std::vector<const char*> layers) {
    if (inDebugMode && verifyVulkanValidationLayers(layers) == false) {
        cleanup();
        throw std::runtime_error("Cannot set Vulkan validation layers: Provided layers are either invalid or unsupported!");
    }

    for (const auto& layer : layers) {
        if (UTIL_enabledValidationLayerSet.count(layer) == 0) {
            enabledValidationLayers.push_back(layer);
            UTIL_enabledValidationLayerSet.insert(layer);
        }
    }

    vkContext.enabledValidationLayers = enabledValidationLayers;
}
