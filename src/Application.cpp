/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <AppWindow.hpp>
#include <Engine/Engine.hpp>
#include <ServiceLocator.hpp>
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
    //MemoryManager memoryManager(vkContext);
    std::shared_ptr<MemoryManager> memoryManager = std::make_shared<MemoryManager>(vkContext);
    ServiceLocator::registerService(memoryManager);

    try {
            // Creates a window
        Window window(WIN_WIDTH, WIN_HEIGHT, WIN_NAME);
        GLFWwindow *windowPtr = window.getGLFWwindowPtr();
        vkContext.window = windowPtr;

            // Creates an instance manager and initializes instance-related objects
        VkInstanceManager instanceManager(vkContext, false);
        instanceManager.init();

        ServiceLocator::registerService(memoryManager);

            // Creates a device manager and initializes GPU-related objects
        VkDeviceManager deviceManager(vkContext, false);
        deviceManager.init();

            // Creates a swap-chain manager for swap-chain operations
        std::shared_ptr<VkSwapchainManager> swapchainManager = std::make_shared<VkSwapchainManager>(vkContext, false);
        swapchainManager->init();
        ServiceLocator::registerService(swapchainManager);

            // Creates a buffer manager
        std::shared_ptr<BufferManager> bufferManager = std::make_shared<BufferManager>(vkContext, false);
        bufferManager->init();

        ServiceLocator::registerService(bufferManager);

            // Creates a graphics pipeline
        std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::make_shared<GraphicsPipeline>(vkContext, false);
        graphicsPipeline->init();
        ServiceLocator::registerService(graphicsPipeline);

            // Creates a rendering pipeline
        std::shared_ptr<RenderPipeline> renderPipeline = std::make_shared<RenderPipeline>(vkContext, false);
        renderPipeline->init();
        ServiceLocator::registerService(renderPipeline);

            // Creates a renderer
        Renderer renderer(vkContext);
        renderer.init();

        Engine engine(windowPtr, vkContext, renderer);
        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::print(e.severity(), e.origin(), e.what());

        memoryManager->processCleanupStack();

        boxer::show(e.what(), ("Exception raised from " + std::string(e.origin())).c_str(), boxer::Style::Error, boxer::Buttons::Quit);
        return EXIT_FAILURE;
    }

    memoryManager->processCleanupStack();
    return EXIT_SUCCESS;
}