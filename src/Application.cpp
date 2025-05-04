/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <Core/AppWindow.hpp>
#include <Engine/Engine.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/Constants.h>
#include <Core/ECS.hpp>
#include <Core/GarbageCollector.hpp>
#include <Core/EventDispatcher.hpp>

#include <CoreStructs/ApplicationContext.hpp>

#include <Engine/Components/PhysicsComponents.hpp>

#include <Utils/FilePathUtils.hpp>

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

    // Event dispatcher
    std::shared_ptr<EventDispatcher> eventDispatcher = std::make_shared<EventDispatcher>();
    ServiceLocator::registerService(eventDispatcher);

    // Garbage collector
    std::shared_ptr<GarbageCollector> garbageCollector = std::make_shared<GarbageCollector>(vkContext);
    ServiceLocator::registerService(garbageCollector);

    // ECS Registry
    std::shared_ptr<Registry> globalRegistry = std::make_shared<Registry>();
    ServiceLocator::registerService(globalRegistry);



    //    // ECS architecture test
    //Entity satellite = globalRegistry->createEntity();
    //Entity starship = globalRegistry->createEntity();
    //Entity earth = globalRegistry->createEntity();


    //globalRegistry->initComponentArray<Component::RigidBody>();

    //Component::RigidBody satelliteRB = {
    //    glm::vec3(1.0f),
    //    glm::vec3(1.0f),
    //    glm::vec3(1.0f),
    //    10.0f
    //};

    //Component::RigidBody starshipRB = {
    //    glm::vec3(1.0f) * 2.0f,
    //    glm::vec3(1.0f) * 2.0f,
    //    glm::vec3(1.0f) * 2.0f,
    //    5e6
    //};

    //Component::RigidBody earthRB = {
    //    glm::vec3(1.0f) * 3.0f,
    //    glm::vec3(1.0f) * 3.0f,
    //    glm::vec3(1.0f) * 3.0f,
    //    (5.972 * 10e24)
    //};

    //globalRegistry->addComponent(satellite.id, satelliteRB);
    //globalRegistry->addComponent(starship.id, starshipRB);
    //globalRegistry->addComponent(earth.id, earthRB);

    //auto view = globalRegistry->getView<Component::RigidBody>();

    //for (auto [entity, rb] : view) {
    //    Log::print(Log::T_WARNING, __FUNCTION__, "Entity #" + std::to_string(entity) + ": Mass = " + std::to_string(rb.mass));
    //}


    // Texture manager
    std::shared_ptr<TextureManager> textureManager = std::make_shared<TextureManager>(vkContext);
    ServiceLocator::registerService(textureManager);


    try {
        // Pipeline initialization
            // Creates a window
        Window window(WIN_WIDTH, WIN_HEIGHT, WIN_NAME);
        GLFWwindow *windowPtr = window.getGLFWwindowPtr();
        vkContext.window = windowPtr;

        
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
        ServiceLocator::registerService(swapchainManager);
        swapchainManager->init();


            // Command manager
        std::shared_ptr<VkCommandManager> commandManager = std::make_shared<VkCommandManager>(vkContext);
        ServiceLocator::registerService(commandManager);
        commandManager->init();


        //textureManager->createTexture((std::string(APP_SOURCE_DIR) + std::string("/assets/app/ExperimentalAppLogo.png")).c_str());
        //textureManager->createTexture((std::string(APP_SOURCE_DIR) + std::string("/assets/Models/Spacecraft/SpaceX_Starship/textures/")).c_str());
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/app/ExperimentalAppLogo.png");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "Spacecraft/SpaceX_Starship/textures");
        std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/Cube/Textures/BaseColor.png");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/Plane/Textures/BaseColor.png");
        //std::string path = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/VikingRoom/viking_room.png");
        textureManager->createTexture(path.c_str());


            // Buffer manager
        std::shared_ptr<BufferManager> m_bufferManager = std::make_shared<BufferManager>(vkContext);
        ServiceLocator::registerService(m_bufferManager);
        m_bufferManager->init();


            // Graphics pipeline
        std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::make_shared<GraphicsPipeline>(vkContext);
        ServiceLocator::registerService(graphicsPipeline);
        graphicsPipeline->init();


            // Synchronization manager
        std::shared_ptr<VkSyncManager> syncManager = std::make_shared<VkSyncManager>(vkContext);
        ServiceLocator::registerService(syncManager);
        syncManager->init();


            // Renderers
        std::shared_ptr<UIRenderer> uiRenderer = std::make_shared<UIRenderer>(vkContext);
        ServiceLocator::registerService(uiRenderer);

        std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(vkContext);
        ServiceLocator::registerService(renderer);
        renderer->init();

        
            // Systems
        std::shared_ptr<RenderSystem> renderSystem = std::make_shared<RenderSystem>(vkContext);
        ServiceLocator::registerService(renderSystem);

        std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();
        ServiceLocator::registerService(physicsSystem);

        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::print(e.severity(), e.origin(), e.what());

        if (vkContext.Device.logicalDevice != VK_NULL_HANDLE)
            vkDeviceWaitIdle(vkContext.Device.logicalDevice);

        garbageCollector->processCleanupStack();
        Log::print(Log::T_ERROR, WIN_NAME.c_str(), "Program exited with errors.");

        std::string errOrigin = "Origin: " + std::string(e.origin()) + "\n";
        std::string errLine = "Line: " + std::to_string(e.errorLine()) + "\n";

        std::string msgType;
        Log::logColor(e.severity(), msgType, false);
        std::string severity = "Exception type: " + msgType + "\n";

        std::string title = "Exception raised from " + std::string(e.origin());

        boxer::show((errOrigin + errLine + severity + "\n" + std::string(e.what())).c_str(), title.c_str(), boxer::Style::Error, boxer::Buttons::Quit);
        return EXIT_FAILURE;
    }


    vkDeviceWaitIdle(vkContext.Device.logicalDevice);
    garbageCollector->processCleanupStack();
    Log::print(Log::T_SUCCESS, WIN_NAME.c_str(), "Program exited successfully.");
    return EXIT_SUCCESS;
}