/* UIPanelManager.hpp - Manages the UI panels.
*/

#pragma once

#include <unordered_map>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>

#include <Core/ECS.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/EventDispatcher.hpp>

#include <CoreStructs/GUI.hpp>
#include <CoreStructs/Contexts.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/WorldSpaceComponents.hpp>

#include <Utils/SpaceUtils.hpp>
#include <Utils/Vulkan/VkDescriptorUtils.hpp>


class UIPanelManager {
public:
	UIPanelManager(VulkanContext& context);
	~UIPanelManager() = default;

	/* Initializes panels from a panel mask bit-field. */
	void initPanelsFromMask(GUI::PanelMask& mask);


	/* Updates panels. */
	void updatePanels();


	/* Is the viewport window focused? (Used for input management) */
	inline bool isViewportFocused() {
		return m_viewportFocused;
	}


	const size_t FLAG_COUNT = sizeof(GUI::PanelFlagsArray) / sizeof(GUI::PanelFlagsArray[0]);

private:
	VulkanContext& m_vkContext;

	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<Registry> m_registry;

	GUI::PanelMask m_panelMask;

	typedef void(UIPanelManager::*PanelCallback)();  // void(void) function pointer
	std::unordered_map<GUI::PanelFlag, PanelCallback> m_panelCallbacks;

	// Viewport sampling as a texture
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_viewportTextureDescSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_viewportRenderTextureDescSet = VK_NULL_HANDLE;

	// Viewport
	bool m_viewportFocused = false;


	void bindEvents();

	/* Binds the panel flags to their respective initalization callback functions. */
	void bindPanelFlags();

	/* Initializes a descriptor set for the viewport texture. */
	void initViewportTextureDescriptorSet();

	void renderPanelsMenu();

	void renderViewportPanel();

	void renderTelemetryPanel();
	void renderEntityInspectorPanel();
	void renderSimulationControlPanel();
	void renderRenderSettingsPanel();
	void renderOrbitalPlannerPanel();
	void renderDebugConsole();
};
