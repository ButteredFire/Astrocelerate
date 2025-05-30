/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <Core/AppWindow.hpp>
#include <Engine/Engine.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/Constants.h>

#include <CoreStructs/Contexts.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <boxer/boxer.h>

const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;

int main() {
    Log::PrintAppInfo();

    // Binds members in the VulkanInstanceContext struct to their corresponding active Vulkan objects
    VulkanContext vkContext{};

    // GLFW callback context
    CallbackContext* callbackContext = new CallbackContext{};


    // Creates a window
    Window window(WIN_WIDTH, WIN_HEIGHT, APP_NAME);
    GLFWwindow *windowPtr = window.getGLFWwindowPtr();
    vkContext.window = windowPtr;

    window.initGLFWBindings(callbackContext);

    // Event dispatcher
    std::shared_ptr<EventDispatcher> eventDispatcher = std::make_shared<EventDispatcher>();
    ServiceLocator::RegisterService(eventDispatcher);


    // Garbage collector
    std::shared_ptr<GarbageCollector> garbageCollector = std::make_shared<GarbageCollector>(vkContext);
    ServiceLocator::RegisterService(garbageCollector);


    // ECS Registry
    std::shared_ptr<Registry> globalRegistry = std::make_shared<Registry>();
    ServiceLocator::RegisterService(globalRegistry);


    // Texture manager
    std::shared_ptr<TextureManager> textureManager = std::make_shared<TextureManager>(vkContext);
    ServiceLocator::RegisterService(textureManager);


    // GUI panel manager
    std::shared_ptr<UIPanelManager> uiPanelManager = std::make_shared<UIPanelManager>();
    ServiceLocator::RegisterService(uiPanelManager);

    
    // Subpass binder
    std::shared_ptr<SubpassBinder> subpassBinder = std::make_shared<SubpassBinder>();
    ServiceLocator::RegisterService(subpassBinder);


    // Camera
    glm::dvec3 cameraPosition = glm::vec3(20e6f, 1.5005e+11f, 0.0f);
    std::shared_ptr<Camera> camera = std::make_shared<Camera>(
        windowPtr,
        SpaceUtils::ToRenderSpace(cameraPosition),
        glm::quat()
    );
    ServiceLocator::RegisterService(camera);


    try {
        // Pipeline initialization

        Engine engine(windowPtr, vkContext);
        engine.initComponents();


            // Instance manager
        VkInstanceManager instanceManager(vkContext);
        instanceManager.init();


            // Device manager
        VkDeviceManager deviceManager(vkContext);
        deviceManager.init();


            // Swap-chain manager
        std::shared_ptr<VkSwapchainManager> swapchainManager = std::make_shared<VkSwapchainManager>(vkContext);
        ServiceLocator::RegisterService(swapchainManager);
        swapchainManager->init();


            // Command manager
        std::shared_ptr<VkCommandManager> commandManager = std::make_shared<VkCommandManager>(vkContext);
        ServiceLocator::RegisterService(commandManager);
        commandManager->init();


        //textureManager->createTexture((std::string(APP_SOURCE_DIR) + std::string("/assets/app/ExperimentalAppLogo.png")).c_str());
        //textureManager->createTexture((std::string(APP_SOURCE_DIR) + std::string("/assets/Models/Spacecraft/SpaceX_Starship/textures/")).c_str());
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/app/ExperimentalAppLogo.png");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "Spacecraft/SpaceX_Starship/textures");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models/TestModels", "Cube/Textures/BaseColor.png");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/SolarSailSpaceship/Textures/13892_diffuse.jpg");
        std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Textures", "CelestialBodies", "Earth/EarthMap.jpg");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/Plane/Textures/BaseColor.png");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/VikingRoom/viking_room.png");
        textureManager->createTexture(path.c_str());


            // Buffer manager
        std::shared_ptr<VkBufferManager> m_bufferManager = std::make_shared<VkBufferManager>(vkContext);
        ServiceLocator::RegisterService(m_bufferManager);
        m_bufferManager->init();


        // TODO: Load geometry here!
        

            // Pipelines
        std::shared_ptr<OffscreenPipeline> offscreenPipeline = std::make_shared<OffscreenPipeline>(vkContext);
        ServiceLocator::RegisterService(offscreenPipeline);
        offscreenPipeline->init();

        std::shared_ptr<PresentPipeline> presentPipeline = std::make_shared<PresentPipeline>(vkContext);
        ServiceLocator::RegisterService(presentPipeline);
        presentPipeline->init();


            // Synchronization manager
        std::shared_ptr<VkSyncManager> syncManager = std::make_shared<VkSyncManager>(vkContext);
        ServiceLocator::RegisterService(syncManager);
        syncManager->init();


            // Renderers
        std::shared_ptr<UIRenderer> uiRenderer = std::make_shared<UIRenderer>(vkContext);
        ServiceLocator::RegisterService(uiRenderer);

                // Input (only usable after ImGui initialization)
        std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>();
        ServiceLocator::RegisterService(inputManager);
        callbackContext->inputManager = inputManager.get();

        std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(vkContext);
        ServiceLocator::RegisterService(renderer);
        renderer->init();

        
            // Systems
        std::shared_ptr<RenderSystem> renderSystem = std::make_shared<RenderSystem>(vkContext);
        ServiceLocator::RegisterService(renderSystem);

        std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();
        ServiceLocator::RegisterService(physicsSystem);

        std::shared_ptr<ReferenceFrameSystem> refFrameSystem = std::make_shared<ReferenceFrameSystem>();
        ServiceLocator::RegisterService(refFrameSystem);

        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::Print(e.severity(), e.origin(), e.what());

        if (vkContext.Device.logicalDevice != VK_NULL_HANDLE)
            vkDeviceWaitIdle(vkContext.Device.logicalDevice);

        garbageCollector->processCleanupStack();
        Log::Print(Log::T_ERROR, APP_NAME, "Program exited with errors.");

        std::string errOrigin = "Origin: " + std::string(e.origin()) + "\n";
        std::string errLine = "Line: " + std::to_string(e.errorLine()) + "\n";

        std::string msgType;
        Log::LogColor(e.severity(), msgType, false);
        std::string severity = "Exception type: " + msgType + "\n";

        std::string title = "Exception raised from " + std::string(e.origin());

        boxer::show((errOrigin + errLine + severity + "\n" + std::string(e.what())).c_str(), title.c_str(), boxer::Style::Error, boxer::Buttons::Quit);
        return EXIT_FAILURE;
    }


    vkDeviceWaitIdle(vkContext.Device.logicalDevice);
    garbageCollector->processCleanupStack();
    Log::Print(Log::T_SUCCESS, APP_NAME, "Program exited successfully.");
    return EXIT_SUCCESS;
}