/* VkWindowManager: Manages Vulkan resources pertaining to the application window (e.g., swapchain, surface, extent).
*/

#pragma once

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/VkSwapchainManager.hpp>
#include <Platform/Vulkan/VkCoreResourcesManager.hpp>
#include <Platform/Windowing/AppWindow.hpp>

#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Engine/Registry/Event/EventDispatcher.hpp>


class VkWindowManager {
public:
	VkWindowManager(GLFWwindow *window, VkCoreResourcesManager *coreResourcesMgr);
	~VkWindowManager() = default;
	

	/* Recreates the swapchain.
		@note If the swapchain is to be recreated in the same window, set both the old and new window arguments to null pointers.

		@param oldWindow: The pointer to the old GLFW window.
		@param newWindow: The pointer to the new GLFW window.

		@return The updated VkWindow context.
	*/
	const Ctx::VkWindow *recreateSwapchain(GLFWwindow *oldWindow, GLFWwindow *newWindow);


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