/* Engine.h: Stores definitions for the Engine class.
*/

#include "Engine.hpp"
#include "../Constants.h"

Engine::Engine(GLFWwindow *w): window(w) {}

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

/* [MEMBER] Initializes Vulkan. */
void Engine::initVulkan() {
    createVulkanInstance();
}

/* [MEMBER] Creates a Vulkan instance. */
void Engine::createVulkanInstance() {
    // Creates an application configuration for the driver
    VkApplicationInfo appInfo{}; // NOTE: Using braced initialization (alt.: memset) zero-initializes all fields
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
    instanceInfo.enabledExtensionCount = glfwExtensionCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

        // Configures global validation layers
    instanceInfo.enabledLayerCount = 0; // Temporary

    // Creates a Vulkan instance from the instance information configured above
    // and initializes the member VKInstance variable
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vkInstance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

/* [MEMBER] Updates and processes all events */
void Engine::update() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

/* [MEMBER] On engine termination: Frees up all associated memory. */
void Engine::cleanup() {
    vkDestroyInstance(vkInstance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
