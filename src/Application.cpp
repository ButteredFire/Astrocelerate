/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <AppWindow.hpp>
#include <Engine/Engine.hpp>
#include <Constants.h>
#include <MemoryManager.hpp>
#include <ApplicationContext.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <boxer/boxer.h>

const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;
const std::string WIN_NAME = APP::APP_NAME;

int main() {
    std::cout << "Project " << APP::APP_NAME << ", version " << APP_VERSION << '\n';

    // Binds members in the VulkanInstanceContext struct to their corresponding active Vulkan objects
    VulkanContext vkContext{};

    // Creates a memory manager
    MemoryManager memoryManager;

    try {
    
            // Creates a window
        Window window(WIN_WIDTH, WIN_HEIGHT, WIN_NAME);
        GLFWwindow *windowPtr = window.getGLFWwindowPtr();
        vkContext.window = windowPtr;

            // Creates an instance manager and initializes instance-related objects
        VkInstanceManager instanceManager(vkContext, memoryManager, false);
        instanceManager.init();

            // Creates a device manager and initializes GPU-related objects
        VkDeviceManager deviceManager(vkContext, memoryManager, false);
        deviceManager.init();

            // Creates a swap-chain manager for swap-chain operations
        VkSwapchainManager swapchainManager(vkContext, memoryManager, false);
        swapchainManager.init();

            // Creates a graphics pipeline
        GraphicsPipeline graphicsPipeline(vkContext, memoryManager, false);
        graphicsPipeline.init();

            // Creates a buffer manager
        BufferManager bufferManager(vkContext, memoryManager, false);
        bufferManager.init();

            // Creates a rendering pipeline
        RenderPipeline renderPipeline(vkContext, memoryManager, bufferManager, false);
        renderPipeline.init();

            // Creates a renderer
        Renderer renderer(vkContext, swapchainManager, renderPipeline);

        Engine engine(windowPtr, vkContext, renderer);
        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::print(Log::T_ERROR, e.origin(), e.what());

        memoryManager.executeStack();

        boxer::show(e.what(), ("Exception raised from " + std::string(e.origin())).c_str(), boxer::Style::Error, boxer::Buttons::Quit);
        return EXIT_FAILURE;
    }

    memoryManager.executeStack();
    return EXIT_SUCCESS;
}