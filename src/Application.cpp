/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <Core/AppWindow.hpp>
#include <Engine/Engine.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/Constants.h>
#include <Core/ECSCore.hpp>
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
    VulkanContext m_vkContext{};

    // Creates a memory manager
    std::shared_ptr<GarbageCollector> garbageCollector = std::make_shared<GarbageCollector>(m_vkContext);
    ServiceLocator::registerService(garbageCollector);

    // Entity manager
    std::shared_ptr<EntityManager> globalEntityManager = std::make_shared<EntityManager>();
    ServiceLocator::registerService(globalEntityManager);

    try {
            // Creates a window
        Window window(WIN_WIDTH, WIN_HEIGHT, WIN_NAME);
        GLFWwindow *windowPtr = window.getGLFWwindowPtr();
        m_vkContext.window = windowPtr;


            // Instance manager
        VkInstanceManager instanceManager(m_vkContext);
        instanceManager.init();


            // Device manager
        VkDeviceManager deviceManager(m_vkContext);
        deviceManager.init();


            // Swap-chain manager
        std::shared_ptr<VkSwapchainManager> swapchainManager = std::make_shared<VkSwapchainManager>(m_vkContext);
        ServiceLocator::registerService(swapchainManager);
        swapchainManager->init();


            // Command manager
        std::shared_ptr<VkCommandManager> commandManager = std::make_shared<VkCommandManager>(m_vkContext);
        ServiceLocator::registerService(commandManager);
        commandManager->init();


            // Texture manager
        std::shared_ptr<TextureManager> textureManager = std::make_shared<TextureManager>(m_vkContext);
        ServiceLocator::registerService(textureManager);
        textureManager->createTexture("../../../assets/app/ExperimentalAppLogo.png");


            // Buffer manager
        std::shared_ptr<BufferManager> m_bufferManager = std::make_shared<BufferManager>(m_vkContext);
        ServiceLocator::registerService(m_bufferManager);
        m_bufferManager->init();


            // Graphics pipeline
        std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::make_shared<GraphicsPipeline>(m_vkContext);
        ServiceLocator::registerService(graphicsPipeline);
        graphicsPipeline->init();

        swapchainManager->createFrameBuffers(); // Only possible when it has a valid render pass (which must first be created in the graphics pipeline)


            // Synchronization manager
        std::shared_ptr<VkSyncManager> syncManager = std::make_shared<VkSyncManager>(m_vkContext);
        ServiceLocator::registerService(syncManager);
        syncManager->init();


            // Renderers
        std::shared_ptr<UIRenderer> uiRenderer = std::make_shared<UIRenderer>(m_vkContext);
        ServiceLocator::registerService(uiRenderer);

        Renderer renderer(m_vkContext);
        renderer.init();


        Engine engine(windowPtr, m_vkContext, renderer);
        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::print(e.severity(), e.origin(), e.what());

        if (m_vkContext.Device.logicalDevice != VK_NULL_HANDLE)
            vkDeviceWaitIdle(m_vkContext.Device.logicalDevice);

        garbageCollector->processCleanupStack();
        Log::print(Log::T_ERROR, WIN_NAME.c_str(), "Program exited with errors.");

        boxer::show(e.what(), ("Exception raised from " + std::string(e.origin())).c_str(), boxer::Style::Error, boxer::Buttons::Quit);
        return EXIT_FAILURE;
    }


    vkDeviceWaitIdle(m_vkContext.Device.logicalDevice);
    garbageCollector->processCleanupStack();
    Log::print(Log::T_SUCCESS, WIN_NAME.c_str(), "Program exited successfully.");
    return EXIT_SUCCESS;
}