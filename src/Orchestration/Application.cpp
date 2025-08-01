/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <Core/Application/AppWindow.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Data/Contexts/CallbackContext.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Orchestration/Engine.hpp>
#include <Orchestration/Session.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <boxer/boxer.h>


const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;


void processCleanupStack() {
    if (g_vkContext.Device.logicalDevice != VK_NULL_HANDLE)
        vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);

    std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
    garbageCollector->processCleanupStack();
}


int main() {
    Log::PrintAppInfo();

    // Creates a window
    Window window(WIN_WIDTH, WIN_HEIGHT, APP_NAME);
    GLFWwindow *windowPtr = window.getGLFWwindowPtr();
    g_vkContext.window = windowPtr;

    window.initGLFWBindings(&g_callbackContext);

    try {
        Engine engine(windowPtr);

        // Texture manager
        std::shared_ptr<TextureManager> textureManager = std::make_shared<TextureManager>();
        ServiceLocator::RegisterService(textureManager);


        // Scene manager
        std::shared_ptr<SceneManager> sceneManager = std::make_shared<SceneManager>();
        ServiceLocator::RegisterService(sceneManager);
        sceneManager->init();


        // GUI management
            // Workspace
            // TODO: Create plugin manager and workspace systems to allow for dynamic workspace discovery and swapping at runtime
        std::unique_ptr<IWorkspace> currentWorkspace;
        currentWorkspace = std::make_unique<OrbitalWorkspace>();    // The default orbital workspace will be used for now

            // Manager
        std::shared_ptr<UIPanelManager> uiPanelManager = std::make_shared<UIPanelManager>(currentWorkspace.get());
        ServiceLocator::RegisterService(uiPanelManager);


        // Camera
        //glm::dvec3 cameraPosition = glm::vec3(8e8, (PhysicsConsts::AU + 10e8), 1e7);
        std::shared_ptr<Registry> globalRegistry = ServiceLocator::GetService<Registry>(__FUNCTION__);
        Entity cameraEntity = globalRegistry->createEntity("Camera");
        glm::dvec3 cameraPosition = glm::vec3(0.0, 1.3e8, 0.0);

        std::shared_ptr<Camera> camera = std::make_shared<Camera>(
            cameraEntity,
            windowPtr,
            cameraPosition,
            glm::quat(1, 0.0, 0.0, 0.0)
        );
        ServiceLocator::RegisterService(camera);


        // Input manager
        std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>();
        ServiceLocator::RegisterService(inputManager);
        inputManager->init();
        g_callbackContext.inputManager = inputManager.get();


        // Pipeline initialization
            // Instance manager
        VkInstanceManager instanceManager;
        instanceManager.init();


            // Device manager
        VkDeviceManager deviceManager;
        deviceManager.init();


            // Swap-chain manager
        std::shared_ptr<VkSwapchainManager> swapchainManager = std::make_shared<VkSwapchainManager>();
        ServiceLocator::RegisterService(swapchainManager);
        swapchainManager->init();


            // Command manager
        std::shared_ptr<VkCommandManager> commandManager = std::make_shared<VkCommandManager>();
        ServiceLocator::RegisterService(commandManager);
        commandManager->init();


            // Buffer manager
        std::shared_ptr<VkBufferManager> m_bufferManager = std::make_shared<VkBufferManager>();
        ServiceLocator::RegisterService(m_bufferManager);
        m_bufferManager->init();


            // Pipelines
        std::shared_ptr<OffscreenPipeline> offscreenPipeline = std::make_shared<OffscreenPipeline>();
        ServiceLocator::RegisterService(offscreenPipeline);

        std::shared_ptr<PresentPipeline> presentPipeline = std::make_shared<PresentPipeline>();
        ServiceLocator::RegisterService(presentPipeline);
        presentPipeline->init();


            // Synchronization manager
        std::shared_ptr<VkSyncManager> syncManager = std::make_shared<VkSyncManager>();
        ServiceLocator::RegisterService(syncManager);
        syncManager->init();


            // Renderers
        std::shared_ptr<UIRenderer> uiRenderer = std::make_shared<UIRenderer>();
        ServiceLocator::RegisterService(uiRenderer);


        std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>();
        ServiceLocator::RegisterService(renderer);

        
            // Systems
        std::shared_ptr<RenderSystem> renderSystem = std::make_shared<RenderSystem>();
        ServiceLocator::RegisterService(renderSystem);

        std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();
        ServiceLocator::RegisterService(physicsSystem);

        std::shared_ptr<ReferenceFrameSystem> refFrameSystem = std::make_shared<ReferenceFrameSystem>();
        ServiceLocator::RegisterService(refFrameSystem);


        std::shared_ptr<Session> currentSession = std::make_shared<Session>();
        ServiceLocator::RegisterService(currentSession);
        currentSession->init();


        std::shared_ptr<EventDispatcher> eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
        eventDispatcher->dispatch(Event::AppIsStable{});


        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::Print(e.severity(), e.origin(), e.what());
        
        processCleanupStack();
        Log::Print(Log::T_ERROR, APP_NAME, "Program exited with errors.");

        std::string title = "Exception raised from " + STD_STR(e.origin());

        std::string errOrigin = "Origin: " + STD_STR(e.origin()) + "\n";
        std::string errLine = "Line: " + TO_STR(e.errorLine()) + "\n";

        std::string threadInfo = "Thread: " + STD_STR(e.threadInfo()) + "\n";

        std::string msgType;
        Log::LogColor(e.severity(), msgType, false);
        std::string severity = "Exception type: " + msgType + "\n";

        boxer::show((errOrigin + errLine + threadInfo + severity + "\n" + STD_STR(e.what())).c_str(), title.c_str(), boxer::Style::Error, boxer::Buttons::Quit);

        Log::EndLogging();
        return EXIT_FAILURE;
    }
#ifdef NDEBUG
    catch (...) {
        boxer::show("Caught an unknown exception!\nThis should not happen. Please raise an issue at https://github.com/ButteredFire/Astrocelerate!", "Unknown Exception", boxer::Style::Error, boxer::Buttons::Quit);

        Log::EndLogging();
        return EXIT_FAILURE;
    }
#endif


    vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);
    
    processCleanupStack();

    Log::Print(Log::T_SUCCESS, APP_NAME, "Program exited successfully.");

    Log::EndLogging();
    return EXIT_SUCCESS;
}