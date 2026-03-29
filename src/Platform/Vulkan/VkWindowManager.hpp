/* VkWindowManager: Manages Vulkan resources pertaining to the application window (e.g., swapchain, surface, extent).
*/

#pragma once

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/VkSwapchainManager.hpp>
#include <Platform/Vulkan/VkCoreResourcesManager.hpp>

#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Engine/Registry/Event/EventDispatcher.hpp>


class VkWindowManager {
public:
	VkWindowManager(GLFWwindow *window, VkCoreResourcesManager *coreResourcesMgr);
	~VkWindowManager() = default;
	

	/* Recreates the swapchain.
		@param newWindowPtr (Default: nullptr): The pointer to the new GLFW window. If the swapchain is to be recreated in the same window, leave this as a null pointer.

		@return The updated VkWindow context.
	*/
	const Ctx::VkWindow *recreateSwapchain(GLFWwindow *newWindowPtr = nullptr);


	/* Gets a pointer to the (read-only) window context.  */
	inline const Ctx::VkWindow *getContext() { return m_windowCtx.get(); } // `const T*` is a pointer to const data (hence it is read-only), NOT a const pointer (`T* const`), which only enforces const on what the pointer points to and not the actual data.

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	GLFWwindow *m_window;
	VkCoreResourcesManager *m_coreResourcesMgr;
	VkSwapchainManager m_swapchainMgr;

	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	std::unique_ptr<Ctx::VkWindow> m_windowCtx;

	void updateCtx();

	void bindEvents();
};