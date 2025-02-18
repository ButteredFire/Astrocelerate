/* Engine.h: Stores definitions for the Engine class.
*/

#include "Engine.hpp"
#include "../Constants.h"

#define enquoteCOUT(S) '"' << (S) << '"'

Engine::Engine(GLFWwindow *w): window(w), vulkInst(nullptr) {}

Engine::~Engine() {}

/* Starts the engine. */
void Engine::run() {
    if (!windowIsValid()) {
        throw std::runtime_error("Engine crashed: Invalid window context!");
        return;
    }
    initVulkan();
    update();
    cleanup();
}

/* Sets Vulkan validation layers.
* @param `layers` A vector containing Vulkan validation layer names to be bound to the current list of enabled validation layers.
*/
void Engine::setVulkanValidationLayers(std::vector<const char*> layers) {
    if (enableValidationLayers && verifyVulkanValidationLayers(layers) == false) {
        throw std::runtime_error("Cannot set Vulkan validation layers: Provided layers are either invalid or unsupported!");
    }

    std::copy(layers.begin(), layers.end(), std::back_inserter(validationLayers));
}

/* Verifies whether a given array of Vulkan extensions is available or supported.
* @param `arrayOfExtensions` An array containing the names of Vulkan extensions to be evaluated for validity.
* @param `arraySize` The size of the array.
* 
* @return A boolean value indicating whether all provided Vulkan extensions are valid (true), or not (false).
*/
bool Engine::verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t arraySize) {
    std::vector<VkExtensionProperties> supportedExtensions = getSupportedVulkanExtensions();
    
    // Use an unordered set to achieve constant lookup time later
    std::unordered_set<std::string> supportedExtNames;
    for (const auto& ext : supportedExtensions)
        supportedExtNames.insert(ext.extensionName);
    
    bool allOK = true;

    for (uint32_t i = 0; i < arraySize; i++) {
        if (supportedExtNames.count(arrayOfExtensions[i]) == 0) {
            allOK = false;
            std::cerr << "Vulkan extension " << enquoteCOUT(arrayOfExtensions[i]) << " is either invalid or unsupported!\n";
        }

    }

    return allOK;
}

/* Verifies whether a given vector of Vulkan validation layers is available or supported.
* @param `layers` A vector containing the names of Vulkan validation layers to be evaluated for validity.
* 
* @return A boolean value indicating whether all provided Vulkan validation layers are valid (true), or not (false).
*/
bool Engine::verifyVulkanValidationLayers(std::vector<const char*> layers) {
    std::vector<VkLayerProperties> supportedLayers = getSupportedVulkanValidationLayers();

    std::unordered_set<std::string> layerNames;
    for (const auto& layer : supportedLayers)
        layerNames.insert(layer.layerName);

    bool allOK = true;
    for (const auto& layer : layers) {
        if (layerNames.count(layer) == 0) {
            allOK = false;
            std::cerr << "Vulkan validation layer " << enquoteCOUT(layer) << " is either invalid or unsupported!\n";
        }
    }

    return allOK;
}

/* [MEMBER] Initializes Vulkan. */
void Engine::initVulkan() {
    // Sets validation layers
    setVulkanValidationLayers({
        "VK_LAYER_KHRONOS_validation"
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
VkResult Engine::createVulkanInstance() {
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
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (verifyVulkanExtensionValidity(glfwExtensions, glfwExtensionCount) == false) {
        throw std::runtime_error("GLFW Instance Extensions contain invalid or unsupported extensions!");
    }
    instanceInfo.enabledExtensionCount = glfwExtensionCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

        // Configures global validation layers
    if (enableValidationLayers) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        instanceInfo.enabledLayerCount = 0;
    }

    // Creates a Vulkan instance from the instance information configured above
    // and initializes the member VkInstance variable
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vulkInst);
    return result;
}

/* [MEMBER] Gets a vector of supported Vulkan extensions (may differ from machine to machine).
* @return A vector that contains supported Vulkan extensions, each of type `VkExtensionProperties`.
*/
std::vector<VkExtensionProperties> Engine::getSupportedVulkanExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // First get the no. of supported extensions

    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()); // Then call the function again to fill the vector
        // Fun fact: vector.data() == &vector[0] (Hint: 3rd parameter, vkEnumerateInstanceExtensionProperties function)
    return supportedExtensions;
}

/* [MEMBER] Gets a vector of supported Vulkan validation layers.
* @return A vector that contains supported Vulkan validation layers, each of type `VkLayerProperties`.
*/
std::vector<VkLayerProperties> Engine::getSupportedVulkanValidationLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());

    return supportedLayers;
}

/* [MEMBER] Updates and processes all events */
void Engine::update() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

/* [MEMBER] On engine termination: Frees up all associated memory. */
void Engine::cleanup() {
    vkDestroyInstance(vulkInst, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
