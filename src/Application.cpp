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
    VulkanContext vkContext{};

    // Creates a memory manager
    std::shared_ptr<GarbageCollector> garbageCollector = std::make_shared<GarbageCollector>(vkContext);
    ServiceLocator::registerService(garbageCollector);

    // Entity manager
    std::shared_ptr<EntityManager> entityManager = std::make_shared<EntityManager>();
    ServiceLocator::registerService(entityManager);

    // Test components
    struct TransformComponent {
        glm::vec3 position;
    };

    struct PhysicsComponent {
        glm::vec3 velocityVector;
        int acceleration;
    };

    // Test entities
    Entity satellite = entityManager->createEntity();
    Entity launchVehicle = entityManager->createEntity();
    Entity staticCelestialObject = entityManager->createEntity();

    // Test component arrays
    ComponentArray<TransformComponent> transforms;
    ComponentArray<PhysicsComponent> physicsComponents;

    // Entity attachment
    transforms.attach(satellite, TransformComponent{ {0.0f, 0.0f, 1.0f} });
    transforms.attach(launchVehicle, TransformComponent{ {5.0f, 8.0f, 3.5f} });
    transforms.attach(staticCelestialObject, TransformComponent{ {83914.32f, 12514.2134f, 30234.32f} });

    physicsComponents.attach(satellite, PhysicsComponent{ {1.0f, 1.0f, 1.0f}, 15 });
    physicsComponents.attach(launchVehicle, PhysicsComponent{ {0.0f, 0.0f, 0.0f}, 0 });


    // Iterating over entities that have the TransformComponent and PhysicsComponent components
    auto view = View::getView(transforms, physicsComponents);
    view.forEach([](Entity entity, TransformComponent& transform, PhysicsComponent& physicsComponent) {
        Log::print(Log::T_INFO, __FUNCTION__, "Entity #" + std::to_string(entity) + 
            "; Transform data: (" 
                + std::to_string(transform.position.x) + ", " 
                + std::to_string(transform.position.y) + ", " 
                + std::to_string(transform.position.z) + 

            "); Physics data: (Velocity vector: (" 
                + std::to_string(physicsComponent.velocityVector.x) + ", " 
                + std::to_string(physicsComponent.velocityVector.y) + ", " 
                + std::to_string(physicsComponent.velocityVector.z) + 
                "); Acceleration: " + std::to_string(physicsComponent.acceleration) + "m/s^2)");
    });

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


            // Renderers
        std::shared_ptr<UIRenderer> uiRenderer = std::make_shared<UIRenderer>(vkContext);
        ServiceLocator::registerService(uiRenderer);

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