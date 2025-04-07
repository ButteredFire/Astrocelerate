/* VkInstanceManager.cpp - Vulkan instance management implementation.
*/


#include "VkInstanceManager.hpp"

VkInstanceManager::VkInstanceManager(VulkanContext& context):
    vkContext(context) {
    
    memoryManager = ServiceLocator::getService<MemoryManager>();

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkInstanceManager::~VkInstanceManager() {}


void VkInstanceManager::init() {
    initVulkan();
    createVulkanInstance();
    createDebugMessenger();
    createSurface();
}


void VkInstanceManager::cleanup() {
    Log::print(Log::T_INFO, __FUNCTION__, "Cleaning up...");

    if (inDebugMode && vkIsValid(debugMessenger))
        destroyDebugUtilsMessengerEXT(vulkInst, debugMessenger, nullptr);

    if (vkIsValid(windowSurface))
        vkDestroySurfaceKHR(vulkInst, windowSurface, nullptr);

    // Destroy the instance last (because the destruction of other Vulkan objects depend on it)
    if (vkIsValid(vulkInst))
        vkDestroyInstance(vulkInst, nullptr);
}


void VkInstanceManager::initVulkan() {
    // Sets up Vulkan extensions and validation layers
        // Caches supported extensions and layers
    supportedExtensions = getSupportedVulkanExtensions();
    supportedLayers = getSupportedVulkanValidationLayers();
    Log::print(Log::T_INFO, __FUNCTION__, ("Supported extensions: " + std::to_string(supportedExtensions.size())));
    Log::print(Log::T_INFO, __FUNCTION__, ("Supported layers: " + std::to_string(supportedLayers.size())));

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
        "VK_LAYER_LUNARG_screenshot",
        //"VK_LAYER_LUNARG_api_dump"
    });
}

void VkInstanceManager::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}


void VkInstanceManager::createDebugMessenger() {
    if (!inDebugMode) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VkResult result = createDebugUtilsMessengerEXT(vulkInst, &createInfo, nullptr, &debugMessenger);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to create debug messenger!");
    }

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.mainObjectName = VARIABLE_NAME(debugMessenger);
    task.vkObjects = { vulkInst, debugMessenger };
    task.cleanupFunc = [&]() { destroyDebugUtilsMessengerEXT(vulkInst, debugMessenger, nullptr); };
    task.cleanupConditions = { inDebugMode };

    memoryManager->createCleanupTask(task);
}


void VkInstanceManager::createVulkanInstance() {
    // Specifies an application configuration for the driver
    VkApplicationInfo appInfo{}; // INSIGHT: Using braced initialization (alt.: memset) zero-initializes all fields
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Specifies structure type

    appInfo.pApplicationName = APP::APP_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // Major, minor, patch
    
    //appInfo.pEngineName = APP::ENGINE_NAME;
    //appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.apiVersion = VULKAN_VERSION;

    // Specifies which global extensions and validation layers are to be used
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    // Configures global extensions 
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // Copies GLFW extensions into enabledExtensions
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        addVulkanExtensions({ glfwExtensions[i] });

        // Sets up additional extensions
    if (inDebugMode)
        addVulkanExtensions({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME });

    if (verifyVulkanExtensions(enabledExtensions) == false) {
        enabledExtensions.clear();
        cleanup();
        throw Log::RuntimeException(__FUNCTION__, "GLFW Instance Extensions contain invalid or unsupported extensions!");
    }

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

    // Configures global validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (inDebugMode) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(enabledValidationLayers.size());
        instanceInfo.ppEnabledLayerNames = enabledValidationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
    else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = nullptr;
    }

    // Creates a Vulkan instance from the instance information configured above
    // and initializes the member VkInstance variable
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vulkInst);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to create Vulkan instance!");
    }

    vkContext.vulkanInstance = vulkInst;

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.mainObjectName = VARIABLE_NAME(vulkInst);
    task.vkObjects = { vulkInst };
    task.cleanupFunc = [&]() { vkDestroyInstance(vulkInst, nullptr); };

    memoryManager->createCleanupTask(task);
}


void VkInstanceManager::createSurface() {
    /* Using glfwCreateWindowSurface to create a window surface is platform-agnostic.
    * On the other hand, while the VkSurfaceKHR object is itself platform agnostic, its creation isn't
    * because it depends on window system details (meaning that the creation structs vary across platforms,
    * e.g., VkWin32SurfaceCreateInfoKHR for Windows).
    */
    VkResult result = glfwCreateWindowSurface(vulkInst, vkContext.window, nullptr, &windowSurface);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to create Vulkan window surface!");
    }

    vkContext.vkSurface = windowSurface;

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.mainObjectName = VARIABLE_NAME(windowSurface);
    task.vkObjects = { vulkInst, windowSurface };
    task.cleanupFunc = [this]() { vkDestroySurfaceKHR(vulkInst, windowSurface, nullptr); };

    memoryManager->createCleanupTask(task);
}


bool VkInstanceManager::verifyVulkanExtensions(std::vector<const char*> extensions) {
    bool allOK = true;
    for (const auto& ext : extensions) {
        if (supportedExtensionNames.count(ext) == 0) {
            allOK = false;
            Log::print(Log::T_ERROR, __FUNCTION__, ("Vulkan extension " + enquote(ext) + " is either invalid or unsupported!"));
        }

    }

    return allOK;
}


bool VkInstanceManager::verifyVulkanValidationLayers(std::vector<const char*>& layers) {
    bool allOK = true;
    for (const auto& layer : layers) {
        if (supportedLayerNames.count(layer) == 0) {
            allOK = false;
            Log::print(Log::T_ERROR, __FUNCTION__, ("Vulkan validation layer " + enquote(layer) + " is either invalid or unsupported!"));
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
        throw Log::RuntimeException(__FUNCTION__, "Cannot set Vulkan extensions: Provided extensions are either invalid or unsupported!");
    }

    for (const auto& ext : extensions) {
        if (UTIL_enabledExtensionSet.count(ext) == 0) {
            Log::print(Log::T_DEBUG, __FUNCTION__, ("Extension " + enquote(ext) + " verified. Enabling..."));
            enabledExtensions.push_back(ext);
            UTIL_enabledExtensionSet.insert(ext);
        }
    }
}


void VkInstanceManager::addVulkanValidationLayers(std::vector<const char*> layers) {
    if (inDebugMode && verifyVulkanValidationLayers(layers) == false) {
        cleanup();
        throw Log::RuntimeException(__FUNCTION__, "Cannot set Vulkan validation layers: Provided layers are either invalid or unsupported!");
    }

    for (const auto& layer : layers) {
        if (UTIL_enabledValidationLayerSet.count(layer) == 0) {
            Log::print(Log::T_DEBUG, __FUNCTION__, ("Validation layer " + enquote(layer) + " verified. Enabling..."));
            enabledValidationLayers.push_back(layer);
            UTIL_enabledValidationLayerSet.insert(layer);
        }
    }

    vkContext.enabledValidationLayers = enabledValidationLayers;
}
