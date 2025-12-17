/* UIPanelManager.hpp - Manages the UI panels.
*/

#pragma once

#include <variant>
#include <unordered_map>
#include <unordered_set>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_vulkan.h>

#include <tinyfiledialogs/tinyfiledialogs.h>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Data/GUI.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Scene/GUI/Appearance.hpp>
#include <Scene/GUI/Workspaces/IWorkspace.hpp>
#include <Scene/GUI/Workspaces/SplashScreen.hpp>
#include <Scene/GUI/Workspaces/OrbitalWorkspace.hpp>

#include <Utils/ImGuiUtils.hpp>
#include <Utils/TextureUtils.hpp>


class UIPanelManager {
public:
	UIPanelManager(IWorkspace *workspace);
	~UIPanelManager() = default;

	inline void switchWorkspace(IWorkspace *newWorkspace) {
		m_currentWorkspace = newWorkspace;
		m_eventDispatcher->dispatch(RequestEvent::ReInitImGui{});
	}

	void preRenderUpdate(uint32_t currentFrame);

	/* Renders a workspace. */
	void renderWorkspace(uint32_t currentFrame);

	/* Renders the menu bar. */
	void renderMenuBar();

	inline void setWorkspacePanelMask(GUI::PanelMask &panelMask) { m_workspacePanelMask = panelMask; };
	inline void setWorkspacePanelCallbacks(std::unordered_map<GUI::PanelID, GUI::PanelCallback> &panelCallbacks) { m_workspacePanelCallbacks = panelCallbacks; };

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	TextureUtils::TextureProps m_appLogoTexProps{};
	TextureUtils::TextureProps m_companyLogoTexProps{};

	// Common panel properties
	typedef std::function<void(void)> CommonPanelCallback;

	GUI::PanelMask m_commonPanelMask{};
	std::unordered_map<GUI::PanelID, CommonPanelCallback> m_commonPanelCallbacks;
	std::unordered_set<GUI::PanelID> m_commonPanels;

	GUI::PanelID m_panelPreferences;
	GUI::PanelID m_panelAbout;
	GUI::PanelID m_panelWelcome;

	// Workspace panel properties
	IWorkspace *m_currentWorkspace;
	GUI::PanelMask m_workspacePanelMask{};
	std::unordered_map<GUI::PanelID, GUI::PanelCallback> m_workspacePanelCallbacks;

	// Window flags
	ImGuiWindowFlags m_windowFlags;
	ImGuiWindowClass m_windowClass;


	// Session & Scene data/status
	bool m_showLoadingModal = false;
	float m_currentLoadProgress = 0.0f;
	std::string m_currentLoadMessage = "";
	bool m_loadErrorOccurred = false;
	std::string m_loadErrorMessage = "";

	Application::YAMLFileConfig m_fileConfig{};
	Application::SimulationConfig m_simulationConfig{};


	void bindEvents();

	void initCommonPanels();
	void initStaticTextures();

	/* Initialization procedures that require the ImGui context to be initialized must be done here. */
	void onImGuiInit();

	/* Common render functions */
		// Menu-bar panels
	void renderPreferencesPanel();
	void renderAboutPanel();
	void renderWelcomePanel();

		// Modals
	const char *m_sceneLoadModelName = "Processing Scene";
	void renderSceneLoadModal(const std::string &fileName);
};
