/* OrbitalWorkspace.hpp - The workspace UI for orbital mechanics.
*/

#pragma once

#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_vulkan.h>

#include <iconfontcppheaders/IconsFontAwesome6.h>

#include <Core/Data/GUI.hpp>
#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Engine/InputManager.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/TelemetryComponents.hpp>

#include <Rendering/Textures/TextureManager.hpp>

#include <Utils/ColorUtils.hpp>
#include <Utils/SpaceUtils.hpp>
#include <Utils/SystemUtils.hpp>
#include <Utils/ImGuiUtils.hpp>
#include <Utils/TextureUtils.hpp>
#include <Utils/Vulkan/VkDescriptorUtils.hpp>

#include <Scene/GUI/Workspaces/IWorkspace.hpp>



class OrbitalWorkspace : public IWorkspace {
public:
	OrbitalWorkspace();
	~OrbitalWorkspace() override = default;

	void init() override;

	void update(uint32_t currentFrame) override;
	void preRenderUpdate(uint32_t currentFrame) override;

	inline GUI::PanelMask& getPanelMask() override { return m_panelMask; };
	inline std::unordered_map<GUI::PanelID, GUI::PanelCallback>& getPanelCallbacks() override { return m_panelCallbacks; };

	void loadSimulationConfig(const std::string &configPath) override;
	void loadWorkspaceConfig(const std::string &configPath) override;
	void saveSimulationConfig(const std::string &configPath) override;
	void saveWorkspaceConfig(const std::string &configPath) override;

	/* Is the viewport panel focused? (Used for input management) */
	inline bool isViewportFocused() {
		return g_appContext.Input.isViewportFocused;
	}

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<InputManager> m_inputManager;
	std::shared_ptr<Registry> m_registry;

	// Panel IDs & masks
	GUI::PanelMask m_panelMask;

	GUI::PanelID m_panelViewport;
	GUI::PanelID m_panelTelemetry;
	GUI::PanelID m_panelEntityInspector;
	GUI::PanelID m_panelSimulationControl;
	GUI::PanelID m_panelRenderSettings;
	GUI::PanelID m_panelOrbitalPlanner;
	GUI::PanelID m_panelDebugConsole;
	GUI::PanelID m_panelDebugApp;

	std::unordered_map<GUI::PanelID, GUI::PanelCallback> m_panelCallbacks;

	// ImGui window flags
	ImGuiWindowFlags m_windowFlags;
	ImGuiWindowFlags m_popupWindowFlags;

	// Textures
		// Viewport/Offscreen resources
	std::vector<ImTextureID> m_viewportRenderTextureIDs;
	ImVec2 m_lastViewportPanelSize = { 0.0f, 0.0f };

	// Other
	uint32_t m_currentFrame = 0;
	bool m_inputBlockerIsOn = false;  // Viewport input blocker blocker (prevents interactions with other GUI elements if the viewport is focused)
	bool m_simulationIsPaused = true;


	void bindEvents();

	void initPanels();

	void initStaticTextures();
	void initPerFrameTextures();
	void updatePerFrameTextures(uint32_t currentFrame);


	/* Render functions */
		// Viewport
	void renderViewportPanel();

		// Normal panels
	void renderTelemetryPanel();
	void renderEntityInspectorPanel();
	void renderSimulationControlPanel();
	void renderRenderSettingsPanel();
	void renderOrbitalPlannerPanel();
	void renderDebugConsole();
	void renderDebugApplication();
};