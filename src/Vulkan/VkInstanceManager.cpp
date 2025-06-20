/* VkInstanceManager.cpp - Vulkan instance management implementation.
*/


#include "VkInstanceManager.hpp"

VkInstanceManager::VkInstanceManager() {
    
    m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkInstanceManager::~VkInstanceManager() {}


void VkInstanceManager::init() {
    initVulkan();
    createVulkanInstance();
    createDebugMessenger();
    createSurface();
}


void VkInstanceManager::initVulkan() {
    // Sets up Vulkan extensions and validation layers
        // Caches supported extensions and layers
    m_supportedExtensions = getSupportedVulkanExtensions();
    m_supportedLayers = getSupportedVulkanValidationLayers();
    Log::Print(Log::T_INFO, __FUNCTION__, ("Supported extensions: " + std::to_string(m_supportedExtensions.size())));
    Log::Print(Log::T_INFO, __FUNCTION__, ("Supported layers: " + std::to_string(m_supportedLayers.size())));

    // Caches supported extension and validation layers for O(1) verification of extensions/layers to be added later
    for (const auto& extension : m_supportedExtensions)
        m_supportedExtensionNames.insert(extension.extensionName);

    for (const auto& layer : m_supportedLayers)
        m_supportedLayerNames.insert(layer.layerName);

    /* Rationale behind reserving:
    * The number of supported extensions/layers is constant. Therefore, we can reserve a fixed (maximum) block of memory
    * for enabled extensions/layers vectors to prevent O(n) vector reallocations.
    */
    m_enabledExtensions.reserve(m_supportedExtensions.size());
    m_enabledValidationLayers.reserve(m_supportedLayers.size());

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
    if (!IN_DEBUG_MODE) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VkResult result = createDebugUtilsMessengerEXT(m_vulkInst, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create debug messenger!");
    }

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_debugMessenger) };
    task.vkObjects = { m_vulkInst, m_debugMessenger };
    task.cleanupFunc = [&]() { destroyDebugUtilsMessengerEXT(m_vulkInst, m_debugMessenger, nullptr); };
    task.cleanupConditions = { IN_DEBUG_MODE };

    m_garbageCollector->createCleanupTask(task);
}


void VkInstanceManager::createVulkanInstance() {
    // Specifies an application configuration for the driver
    VkApplicationInfo appInfo{}; // INSIGHT: Using braced initialization (alt.: memset) zero-initializes all fields
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Specifies structure type

    appInfo.pApplicationName = APP_NAME;
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

        // Copies GLFW extensions into m_enabledExtensions
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        addVulkanExtensions({ glfwExtensions[i] });

        // Sets up additional extensions
    if (IN_DEBUG_MODE)
        addVulkanExtensions({
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        });

    if (verifyVulkanExtensions(m_enabledExtensions) == false) {
        m_enabledExtensions.clear();
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "GLFW Instance Extensions contain invalid or unsupported extensions!");
    }

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
    instanceInfo.ppEnabledExtensionNames = m_enabledExtensions.data();

    // Configures global validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (IN_DEBUG_MODE) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(m_enabledValidationLayers.size());
        instanceInfo.ppEnabledLayerNames = m_enabledValidationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
    else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = nullptr;
    }

    // Creates a Vulkan instance from the instance information configured above
    // and initializes the member VkInstance variable
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &m_vulkInst);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create Vulkan instance!");
    }

    g_vkContext.vulkanInstance = m_vulkInst;

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_vulkInst) };
    task.vkObjects = { m_vulkInst };
    task.cleanupFunc = [&]() { vkDestroyInstance(m_vulkInst, nullptr); };

    m_garbageCollector->createCleanupTask(task);
}


void VkInstanceManager::createSurface() {
    /* Using glfwCreateWindowSurface to create a window surface is platform-agnostic.
    * On the other hand, while the VkSurfaceKHR object is itself platform agnostic, its creation isn't
    * because it depends on window system details (meaning that the creation structs vary across platforms,
    * e.g., VkWin32SurfaceCreateInfoKHR for Windows).
    */
    VkResult result = glfwCreateWindowSurface(m_vulkInst, g_vkContext.window, nullptr, &m_windowSurface);
    if (result != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create Vulkan window surface!");
    }

    g_vkContext.vkSurface = m_windowSurface;

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { VARIABLE_NAME(m_windowSurface) };
    task.vkObjects = { m_vulkInst, m_windowSurface };
    task.cleanupFunc = [this]() { vkDestroySurfaceKHR(m_vulkInst, m_windowSurface, nullptr); };

    m_garbageCollector->createCleanupTask(task);
}


bool VkInstanceManager::verifyVulkanExtensions(std::vector<const char*> extensions) {
    bool allOK = true;
    for (const auto& ext : extensions) {
        if (m_supportedExtensionNames.count(ext) == 0) {
            allOK = false;
            Log::Print(Log::T_ERROR, __FUNCTION__, ("Vulkan extension " + enquote(ext) + " is either invalid or unsupported!"));
        }

    }

    return allOK;
}


bool VkInstanceManager::verifyVulkanValidationLayers(std::vector<const char*>& layers) {
    bool allOK = true;
    for (const auto& layer : layers) {
        if (m_supportedLayerNames.count(layer) == 0) {
            allOK = false;
            Log::Print(Log::T_ERROR, __FUNCTION__, ("Vulkan validation layer " + enquote(layer) + " is either invalid or unsupported!"));
        }
    }

    return allOK;
}


std::vector<VkExtensionProperties> VkInstanceManager::getSupportedVulkanExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // First get the no. of supported extensions

    std::vector<VkExtensionProperties> m_supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_supportedExtensions.data()); // Then call the function again to fill the vector
    // Fun fact: vector.data() == &vector[0] (Hint: 3rd parameter, vkEnumerateInstanceExtensionProperties function)
    return m_supportedExtensions;
}


std::vector<VkLayerProperties> VkInstanceManager::getSupportedVulkanValidationLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> m_supportedLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, m_supportedLayers.data());

    return m_supportedLayers;
}


void VkInstanceManager::addVulkanExtensions(std::vector<const char*> extensions) {
    if (verifyVulkanExtensions(extensions) == false) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot set Vulkan extensions: Provided extensions are either invalid or unsupported!");
    }

    for (const auto& ext : extensions) {
        if (m_UTIL_enabledExtensionSet.count(ext) == 0) {
            Log::Print(Log::T_DEBUG, __FUNCTION__, ("Extension " + enquote(ext) + " verified. Enabling..."));
            m_enabledExtensions.push_back(ext);
            m_UTIL_enabledExtensionSet.insert(ext);
        }
    }
}


void VkInstanceManager::addVulkanValidationLayers(std::vector<const char*> layers) {
    if (IN_DEBUG_MODE && verifyVulkanValidationLayers(layers) == false) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot set Vulkan validation layers: Provided layers are either invalid or unsupported!");
    }

    for (const auto& layer : layers) {
        if (m_UTIL_enabledValidationLayerSet.count(layer) == 0) {
            Log::Print(Log::T_DEBUG, __FUNCTION__, ("Validation layer " + enquote(layer) + " verified. Enabling..."));
            m_enabledValidationLayers.push_back(layer);
            m_UTIL_enabledValidationLayerSet.insert(layer);
        }
    }

    g_vkContext.enabledValidationLayers = m_enabledValidationLayers;
}
