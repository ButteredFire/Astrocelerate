/* VkCoreResourcesManager - Manages essential persistent Vulkan resources.
*/

#pragma once


#include <Core/Data/Application.hpp>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/VkDeviceManager.hpp>
#include <Platform/Vulkan/VkInstanceManager.hpp>
#include <Platform/Vulkan/Data/Device.hpp>

#include <Engine/Registry/Event/EventDispatcher.hpp>


class VkDeviceManager;
class VkInstanceManager;


class VkCoreResourcesManager {
public:
    VkCoreResourcesManager(GLFWwindow *window, VkInstanceManager *instanceManager, VkDeviceManager *deviceManager, CleanupManager *gc);
    ~VkCoreResourcesManager() = default;


    inline Application::State getAppState() const               { return m_currentAppState; }
    inline const Ctx::VkRenderDevice *getCoreContext() const    { return m_renderDeviceCtx.get(); }
	inline VkSurfaceKHR getSurface() const                      { return m_surface; }


    inline VkSurfaceKHR recreateSurface(GLFWwindow *oldWindow, GLFWwindow *newWindow) {
        m_cleanupManager->executeCleanupTask(m_surfaceID);
        glfwDestroyWindow(oldWindow);

        CleanupTask task = m_instanceManager->createSurface(m_surface, m_renderDeviceCtx->instance, newWindow);
        m_surfaceID = m_cleanupManager->createCleanupTask(task);

        return m_surface;
    }

private:
    std::shared_ptr<EventDispatcher> m_eventDispatcher;

    CleanupManager *m_cleanupManager;
    VkInstanceManager *m_instanceManager;
    VkDeviceManager *m_deviceManager;

    Application::State m_currentAppState{};

    // Core resources
    std::unique_ptr<Ctx::VkRenderDevice> m_renderDeviceCtx;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    ResourceID m_surfaceID;


    void bindEvents();
};