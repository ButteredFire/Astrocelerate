/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include "AppWindow.hpp"
#include "Engine/Engine.hpp"
#include "Constants.h"
#include "VulkanContexts.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>


const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;
const std::string WIN_NAME = APP::APP_NAME;

int main() {
    std::cout << "Project " << APP::APP_NAME << ", version " << APP_VERSION << '\n';

    // Binds members in the VulkanInstanceContext struct to their corresponding active Vulkan objects
    VulkanContext vkContext{};
    
        // Creates a window
    Window window(WIN_WIDTH, WIN_HEIGHT, WIN_NAME);
    GLFWwindow *windowPtr = window.getGLFWwindowPtr();
    vkContext.window = windowPtr;

        // Creates an instance manager and initializes instance-related objects
    VkInstanceManager instanceManager(vkContext);
    instanceManager.init();

        // Creates a device manager and initializes GPU-related objects
    VkDeviceManager deviceManager(vkContext);
    deviceManager.init();

    if (vkContext.physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "WARNING: Physical device not initialized!\n";
    }

    Engine engine(windowPtr, vkContext);

    try {
        engine.run();
    }
    catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}