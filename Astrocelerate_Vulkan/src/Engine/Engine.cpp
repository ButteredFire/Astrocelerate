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

/* Sets Vulkan validation layers. */
void Engine::setVulkanValidationLayers(std::vector<const char*> layers) {
    // TODO: Check validity of each layer

    std::copy(layers.begin(), layers.end(), std::back_inserter(validationLayers));
}

/* Verifies whether a given array of Vulkan extensions is included in the list of supported extensions.
* @param `arrayOfExtensions` An array containing the names of Vulkan extensions to be evaluated for validity.
* @param `arraySize` The size of the array.
* 
* @return A boolean value representing whether all provided Vulkan extensions are valid (true), or not (false).
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



/* [MEMBER] Initializes Vulkan. */
void Engine::initVulkan() {
    // Creates Vulkan instance
    VkResult initResult = createVulkanInstance();
    if (initResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    // Sets validation layers
    setVulkanValidationLayers({
        "VK_LAYER_KHRONOS_validation"
    });
}

/* [MEMBER] Creates a Vulkan instance. */
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
    instanceInfo.enabledLayerCount = 0; // Temporary

    // Creates a Vulkan instance from the instance information configured above
    // and initializes the member VkInstance variable
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vulkInst);
    return result;
}

/* [MEMBER] Gets a vector of supported Vulkan extensions (may differ from machine to machine).
* @return A vector that contains supported Vulkan extensions, each of type `VkExtensionProperties`.
*/
std::vector<VkExtensionProperties> Engine::getSupportedVulkanExtensions() {
    uint32_t numOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numOfExtensions, nullptr); // First get the no. of supported extensions

    std::vector<VkExtensionProperties> supportedExtensions(numOfExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numOfExtensions, supportedExtensions.data()); // Then call the function again to fill the vector

    return supportedExtensions;
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
