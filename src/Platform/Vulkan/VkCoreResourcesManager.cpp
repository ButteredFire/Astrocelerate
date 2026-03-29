#include "VkCoreResourcesManager.hpp"




VkCoreResourcesManager::VkCoreResourcesManager(GLFWwindow *window, VkInstanceManager *instanceManager, VkDeviceManager *deviceManager, CleanupManager *gc) :
    m_instanceManager(instanceManager),
    m_deviceManager(deviceManager),
    m_cleanupManager(gc) {
    
    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

    // Create persistent Vulkan resources
    CleanupTask task;
    m_renderDeviceCtx = std::make_unique<Ctx::VkRenderDevice>();

    task = instanceManager->createVulkanInstance(m_renderDeviceCtx->instance);
    gc->createCleanupTask(task);

    if (IN_DEBUG_MODE || g_appCtx.Config.debugging_VkValidationLayers) {
        task = instanceManager->createDebugMessenger(m_renderDeviceCtx->dbgMessenger, m_renderDeviceCtx->instance);
        gc->createCleanupTask(task);
    }

    task = instanceManager->createSurface(m_surface, m_renderDeviceCtx->instance, window);
    m_surfaceID = gc->createCleanupTask(task);

    deviceManager->createPhysicalDevice(m_renderDeviceCtx->physicalDevice, m_renderDeviceCtx->chosenDevice, m_renderDeviceCtx->availableDevices, m_renderDeviceCtx->instance, m_surface);
    task = deviceManager->createLogicalDevice(m_renderDeviceCtx->logicalDevice, m_renderDeviceCtx->queueFamilies, m_renderDeviceCtx->physicalDevice, m_surface);
    gc->createRootCleanupTask(task); // Create a root cleanup task for the logical device because it is the latest essential Vulkan resource

    // Henceforth, everything will implicitly have VkDevice as a parent resource

    m_renderDeviceCtx->vmaAllocator = gc->createVMAllocator(m_renderDeviceCtx->instance, m_renderDeviceCtx->physicalDevice, m_renderDeviceCtx->logicalDevice);


    bindEvents();

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void VkCoreResourcesManager::bindEvents() {
    static const EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<VkCoreResourcesManager>();

    m_eventDispatcher->subscribe<UpdateEvent::ApplicationStatus>(selfIndex, 
        [this](const UpdateEvent::ApplicationStatus &event) {
            if (event.appState != Application::State::NULL_STATE)
                m_currentAppState = event.appState;
        }
    );
}
