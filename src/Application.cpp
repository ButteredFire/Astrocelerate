/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <Core/AppWindow.hpp>
#include <Engine/Engine.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/Constants.h>
#include <Core/GarbageCollector.hpp>
#include <Core/ApplicationContext.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <boxer/boxer.h>

const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;
const std::string WIN_NAME = APP::APP_NAME;

int main() {
    std::cout << "Project " << WIN_NAME << ", version " << APP_VERSION << ". Copyright 2025 Duong Duy Nhat Minh.\n";

    // Binds members in the VulkanInstanceContext struct to their corresponding active Vulkan objects
    VulkanContext vkContext{};

    // Creates a memory manager
    std::shared_ptr<GarbageCollector> garbageCollector = std::make_shared<GarbageCollector>(vkContext);
    ServiceLocator::registerService(garbageCollector);

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


            // Buffer manager
        std::shared_ptr<BufferManager> bufferManager = std::make_shared<BufferManager>(vkContext);
        ServiceLocator::registerService(bufferManager);


            // Command manager
        std::shared_ptr<VkCommandManager> commandManager = std::make_shared<VkCommandManager>(vkContext);
        ServiceLocator::registerService(commandManager);

        commandManager->init();
        bufferManager->init();


            // Graphics pipeline
        std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::make_shared<GraphicsPipeline>(vkContext);
        ServiceLocator::registerService(graphicsPipeline);
        graphicsPipeline->init();

        swapchainManager->createFrameBuffers(); // Only possible when it has a valid render pass (which must first be created in the graphics pipeline)


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

        if (vkContext.logicalDevice != VK_NULL_HANDLE)
            vkDeviceWaitIdle(vkContext.logicalDevice);

        garbageCollector->processCleanupStack();

        boxer::show(e.what(), ("Exception raised from " + std::string(e.origin())).c_str(), boxer::Style::Error, boxer::Buttons::Quit);
        return EXIT_FAILURE;
    }

    vkDeviceWaitIdle(vkContext.logicalDevice);
    garbageCollector->processCleanupStack();
    return EXIT_SUCCESS;
}