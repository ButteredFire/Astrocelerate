#include "VkCoreResourcesManager.hpp"




VkCoreResourcesManager::VkCoreResourcesManager(GLFWwindow *window, VkInstanceManager *instanceManager, VkDeviceManager *deviceManager, GarbageCollector *gc) {
    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

    // Create persistent Vulkan resources
    CleanupTask task;

    task = instanceManager->createVulkanInstance(m_instance);
    gc->createCleanupTask(task);

    if (IN_DEBUG_MODE) {
        task = instanceManager->createDebugMessenger(m_dbgMessenger, m_instance);
        gc->createCleanupTask(task);
    }

    task = instanceManager->createSurface(m_surface, m_instance, window);
    gc->createCleanupTask(task);

    deviceManager->createPhysicalDevice(m_physicalDevice, m_chosenDevice, m_availableDevices, m_instance, m_surface);

    task = deviceManager->createLogicalDevice(m_logicalDevice, m_familyIndices, m_physicalDevice, m_surface);
    gc->createCleanupTask(task);

    m_vmaAllocator = gc->createVMAllocator(m_instance, m_physicalDevice, m_logicalDevice);


    bindEvents();

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void VkCoreResourcesManager::bindEvents() {
    static const EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<VkCoreResourcesManager>();

    m_eventDispatcher->subscribe<UpdateEvent::ApplicationStatus>(selfIndex, 
        [this](const UpdateEvent::ApplicationStatus &event) {
            if (event.appStage != Application::Stage::NULL_STAGE)
                m_currentAppStage = event.appStage;

            if (event.appState != Application::State::NULL_STATE)
                m_currentAppState = event.appState;
        }
    );
}
