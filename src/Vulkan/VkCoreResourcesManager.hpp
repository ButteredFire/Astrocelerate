/* VkCoreResourcesManager - Manages essential persistent Vulkan resources.
*/

#pragma once

#include <External/GLFWVulkan.hpp>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Application/ResourceManager.hpp>
#include <Core/Data/Device.hpp>
#include <Core/Data/Application.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Vulkan/VkDeviceManager.hpp>
class VkDeviceManager;
#include <Vulkan/VkInstanceManager.hpp>
class VkInstanceManager;


class VkCoreResourcesManager {
public:
    VkCoreResourcesManager(GLFWwindow *window, VkInstanceManager *instanceManager, VkDeviceManager *deviceManager, ResourceManager *gc);
    ~VkCoreResourcesManager() = default;


    /* GETTERS */
    inline Application::State getAppState() const { return m_currentAppState; }

    inline VkInstance getInstance() const { return m_instance; }
	inline VkDebugUtilsMessengerEXT getDebugMessenger() const { return m_dbgMessenger; }
	inline VkSurfaceKHR getSurface() const { return m_surface; }

	inline VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
	inline PhysicalDeviceProperties getChosenDevice() const { return m_chosenDevice; }
	inline std::vector<PhysicalDeviceProperties> getAvailableDevices() const { return m_availableDevices; }

	inline VkDevice getLogicalDevice() const { return m_logicalDevice; }
	inline QueueFamilyIndices getQueueFamilyIndices() const { return m_familyIndices; }
	inline VmaAllocator getVmaAllocator() const { return m_vmaAllocator; }

    
    /* Helpers */
    inline std::string getDeviceName() const { return m_chosenDevice.deviceName; }
    inline VkPhysicalDeviceProperties getDeviceProperties() const { return m_chosenDevice.deviceProperties; }


    /* Other */
    inline VkSurfaceKHR recreateSurface(GLFWwindow *w) {
        CleanupTask task = m_instanceManager->createSurface(m_surface, m_instance, w);
        m_resourceManager->createCleanupTask(task);

        m_eventDispatcher->dispatch(UpdateEvent::CoreResources{
            .surface = m_surface
        });

        return m_surface;
    }

private:
    std::shared_ptr<EventDispatcher> m_eventDispatcher;

    ResourceManager *m_resourceManager;
    VkInstanceManager *m_instanceManager;
    VkDeviceManager *m_deviceManager;

    Application::State m_currentAppState{};

    // Core resources
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_dbgMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    PhysicalDeviceProperties m_chosenDevice{};
    std::vector<PhysicalDeviceProperties> m_availableDevices;

    VkDevice m_logicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices m_familyIndices{};

    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;


    void bindEvents();
};