/* OrbitalWorkspace.hpp - The workspace UI for orbital mechanics.
*/

#pragma once

#include <any>
#include <sstream>
#include <iostream>
#include <unordered_set>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_vulkan.h>

#include <iconfontcppheaders/IconsFontAwesome6.h>


#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Utils/SpaceUtils.hpp>
#include <Core/Utils/SystemUtils.hpp>

#include <Platform/Vulkan/Utils/VkDescriptorUtils.hpp>

#include <Engine/GUI/Data/GUI.hpp>
#include <Engine/GUI/CodeEditor.hpp>
#include <Engine/GUI/Workspaces/IWorkspace.hpp>
#include <Engine/Input/InputManager.hpp>
#include <Engine/Utils/ColorUtils.hpp>
#include <Engine/Utils/ImGuiUtils.hpp>
#include <Engine/Utils/TextureUtils.hpp>
#include <Engine/Contexts/GUIContext.hpp>
#include <Engine/Registry/ECS/Components/PhysicsComponents.hpp>
#include <Engine/Registry/ECS/Components/TelemetryComponents.hpp>
#include <Engine/Registry/ECS/Components/SpacecraftComponents.hpp>
#include <Engine/Rendering/Textures/TextureManager.hpp>

#include <Simulation/Data/CoordSys.hpp>


// Custom hash function for pairs
struct PairHash {
	template <typename T1, typename T2>
	std::size_t operator()(const std::pair<T1, T2> &p) const {
		// A common way to combine hashes, though more sophisticated methods exist
		return std::hash<T1>()(p.first) ^ (std::hash<T2>()(p.second) << 1);
	}
};


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
		return g_guiCtx.Input.isViewportFocused;
	}

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ECSRegistry> m_ecsRegistry;

	std::shared_ptr<VkCoreResourcesManager> m_coreResources;
	std::shared_ptr<VkSwapchainManager> m_swapchainManager;
	std::shared_ptr<InputManager> m_inputManager;


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
	GUI::PanelID m_panelSceneResourceTree;
	GUI::PanelID m_panelSceneResourceDetails;
	GUI::PanelID m_panelCodeEditor;

	std::unordered_map<GUI::PanelID, GUI::PanelCallback> m_panelCallbacks;

	// ImGui window flags
	ImGuiWindowFlags m_windowFlags;
	ImGuiWindowFlags m_popupWindowFlags;

	// Textures
		// Viewport/Offscreen resources
	std::vector<VkImageView> m_offscreenImageViews;
	std::vector<VkSampler> m_offscreenSamplers;
	std::vector<ImTextureID> m_viewportRenderTextureIDs;
	ImVec2 m_lastViewportPanelSize = { 0.0f, 0.0f };
	float m_lastTimeScale = 0.0f;
	bool m_sceneSampleInitialized = false;		// Mirrors SessionStatus::Status::INITIALIZED
	bool m_sceneSampleReady = false;			// Mirrors SessionStatus::Status::POST_INITIALIZATION

	// Other
		// General
	uint32_t m_currentFrame = 0;
	bool m_inputBlockerIsOn = false;  // Viewport input blocker blocker (prevents interactions with other GUI elements if the viewport is focused)
	bool m_simulationIsPaused = true;

		// Scene resources
	enum class m_ResourceType {
		SPACECRAFT,
		CELESTIAL_BODIES,
		PROPAGATORS,
		SOLVERS,
		SCRIPTS,
		COORDINATE_SYSTEMS
	};
	std::unordered_set<std::pair<EntityID, m_ResourceType>, PairHash> m_sceneResourceEntityData{};
	m_ResourceType m_currentSceneResourceType;
	EntityID m_currentSceneResourceEntityID;

		// Code editor
	CodeEditor m_codeEditor;
	std::string m_simulationConfigPath = "";
	std::vector<char> m_simulationScriptData{};
	bool m_simulationConfigChanged = false;


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
	void renderSceneResourceTree();
	void renderSceneResourceDetails();
	void renderCodeEditor();
};