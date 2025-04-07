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


            // Instance manager
        VkInstanceManager instanceManager(vkContext);
        instanceManager.init();


            // Device manager
        VkDeviceManager deviceManager(vkContext);
        deviceManager.init();


            // Swap-chain manager
        std::shared_ptr<VkSwapchainManager> swapchainManager = std::make_shared<VkSwapchainManager>(vkContext);
        ServiceLocator::registerService(swapchainManager);

        swapchainManager->init();


            // Buffer manager and graphics pipeline
        std::shared_ptr<BufferManager> bufferManager = std::make_shared<BufferManager>(vkContext);
        ServiceLocator::registerService(bufferManager);

        std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::make_shared<GraphicsPipeline>(vkContext);
        ServiceLocator::registerService(graphicsPipeline);

        bufferManager->init();
        graphicsPipeline->init();
        swapchainManager->createFrameBuffers(); // Only possible when it has a valid render pass (which must first be created in the graphics pipeline)


            // Command manager
        std::shared_ptr<VkCommandManager> commandManager = std::make_shared<VkCommandManager>(vkContext);
        ServiceLocator::registerService(commandManager);

        commandManager->init();


            // Synchronization manager
        std::shared_ptr<VkSyncManager> syncManager = std::make_shared<VkSyncManager>(vkContext);
        ServiceLocator::registerService(syncManager);
        
        syncManager->init();


            // Renderer
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