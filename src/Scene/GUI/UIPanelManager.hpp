/* UIPanelManager.hpp - Manages the UI panels.
*/

#pragma once

#include <unordered_map>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_vulkan.h>

#include <iconfontcppheaders/IconsFontAwesome6.h>

#include <Core/Engine/ECS.hpp>
#include <Core/Data/Constants.h>
#include <Core/Engine/InputManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Application/GarbageCollector.hpp>

#include <Core/Data/GUI.hpp>
#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Data/Contexts/AppContext.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/TelemetryComponents.hpp>

#include <Utils/ColorUtils.hpp>
#include <Utils/SpaceUtils.hpp>
#include <Utils/SystemUtils.hpp>
#include <Utils/ImGuiUtils.hpp>
#include <Utils/Vulkan/VkDescriptorUtils.hpp>


class UIPanelManager {
public:
	UIPanelManager();
	~UIPanelManager() = default;

	/* Initializes panels from a panel mask bit-field. */
	void initPanelsFromMask(GUI::PanelMask& mask);

	/* Updates panels. */
	void updatePanels(uint32_t currentFrame);

	/* Updates the viewport texture descriptor set. */
	void updateViewportTexture(uint32_t currentFrame);

	/* Renders the menu bar. */
	void renderMenuBar();

	/* Is the viewport panel focused? (Used for input management) */
	inline bool isViewportFocused() {
		return g_appContext.Input.isViewportFocused;
	}

	/* Sets the currently focused panel.
		@param panel: The panel to be focused on.
	*/
	inline void setFocusedPanel(GUI::PanelFlag panel) { m_currentFocusedPanel = panel; }


	const size_t FLAG_COUNT = sizeof(GUI::PanelFlagsArray) / sizeof(GUI::PanelFlagsArray[0]);

private:
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<InputManager> m_inputManager;
	std::shared_ptr<Registry> m_registry;

	GUI::PanelMask m_panelMask;

	typedef void(UIPanelManager::*PanelCallback)();  // void(void) function pointer
	std::unordered_map<GUI::PanelFlag, PanelCallback> m_panelCallbacks;

	// Common window flags
	ImGuiWindowFlags m_windowFlags;
	ImGuiWindowFlags m_popupWindowFlags;
	ImGuiWindowClass m_windowClass;

	// Viewport
		// Sampling as a texture
	std::vector<ImTextureID> m_viewportRenderTextureIDs;
	ImVec2 m_lastViewportPanelSize = { 0.0f, 0.0f };
	uint32_t m_currentFrame = 0;

		// Input blocker to prevent interactions with other GUI elements if the viewport is focused
	bool m_inputBlockerIsOn = false;

		// Other
	bool m_simulationIsPaused = true;


	GUI::PanelFlag m_currentFocusedPanel = GUI::PanelFlag::NULL_PANEL;


	void bindEvents();

	/* Initialization procedures that require the ImGui context to be initialized must be done here. */
	void onImGuiInit();


	/* ImGui calls that must happen between ImGui::Begin() and ImGui::End() will be invoked here.
		@important All render*() functions must call this function right after ImGui::Begin()!
	*/
	void performBackgroundChecks(GUI::PanelFlag flag);


	/* Binds the panel flags to their respective initalization callback functions. */
	void bindPanelFlags();


	/* Initializes descriptor sets for the viewport texture. */
	void initViewportTextures();

	void renderPanelsMenu();

	void renderViewportPanel();

	void renderTelemetryPanel();
	void renderEntityInspectorPanel();
	void renderSimulationControlPanel();
	void renderRenderSettingsPanel();
	void renderPreferencesPanel();
	void renderOrbitalPlannerPanel();

	void renderDebugConsole();
	void renderDebugApplication();
};
