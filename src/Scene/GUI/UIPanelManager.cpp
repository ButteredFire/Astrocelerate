#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager(IWorkspace *workspace):
	m_currentWorkspace(workspace) {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void UIPanelManager::preRenderUpdate(uint32_t currentFrame) {
	m_currentWorkspace->preRenderUpdate(currentFrame);
}


void UIPanelManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::GUIContextIsValid>(
		[this](const Event::GUIContextIsValid& event) {
			this->onImGuiInit();
		}
	);
}


void UIPanelManager::initCommonPanels() {
	m_panelPreferences =	GUI::RegisterPanel("Preferences");
	m_panelAbout =			GUI::RegisterPanel("About Astrocelerate");

	m_commonPanelCallbacks[m_panelPreferences] =	[this]() { this->renderPreferencesPanel(); };
	m_commonPanelCallbacks[m_panelAbout] =			[this]() { this->renderAboutPanel(); };
}


void UIPanelManager::initStaticTextures() {
	std::shared_ptr<TextureManager> textureManager = ServiceLocator::GetService<TextureManager>(__FUNCTION__);

	// App logo
	std::string logoPath = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/App", "AstrocelerateLogo-Branded.png");
	Geometry::Texture texture = textureManager->createIndependentTexture(logoPath, VK_FORMAT_R8G8B8A8_SRGB);
	
	m_appLogoTexProps.size = ImVec2(texture.size.x, texture.size.y);
	m_appLogoTexProps.textureID = TextureUtils::GenerateImGuiTextureID(texture.imageLayout, texture.imageView, texture.sampler);

	// Oriviet Aerospace logo
	logoPath = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/App", "OrivietAerospaceLogo.png");
	texture = textureManager->createIndependentTexture(logoPath, VK_FORMAT_R8G8B8A8_SRGB);

	m_companyLogoTexProps.size = ImVec2(texture.size.x, texture.size.y);
	m_companyLogoTexProps.textureID = TextureUtils::GenerateImGuiTextureID(texture.imageLayout, texture.imageView, texture.sampler);
}


void UIPanelManager::onImGuiInit() {
	m_windowFlags = ImGuiWindowFlags_NoCollapse;
	m_windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;

	initStaticTextures();
	initCommonPanels();

	m_currentWorkspace->init();

	setWorkspacePanelMask(m_currentWorkspace->getPanelMask());
	setWorkspacePanelCallbacks(m_currentWorkspace->getPanelCallbacks());
}


void UIPanelManager::renderWorkspace(uint32_t currentFrame) {
	// Common panel callbacks
	for (auto &&[flag, callback] : m_commonPanelCallbacks) {
		if (GUI::IsPanelOpen(m_commonPanelMask, flag)) {
			ImGui::SetNextWindowClass(&m_windowClass);
			callback();
		}
	}


	// Workspace panel callbacks
	for (auto&& [flag, callback] : m_workspacePanelCallbacks) {
		if (GUI::IsPanelOpen(m_workspacePanelMask, flag)) {
			ImGui::SetNextWindowClass(&m_windowClass);
			callback(m_currentWorkspace);
		}
	}

	m_currentWorkspace->update(currentFrame);
}


void UIPanelManager::renderMenuBar() {
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			// Open
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				// TODO: Handle Open
			}

			// Save
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				// TODO: Handle Save
			}

			ImGui::Separator();

			// Preferences
			{
				using namespace GUI;
				bool isOpen = IsPanelOpen(m_commonPanelMask, m_panelPreferences);

				//ImGui::MenuItem(GetPanelName(m_panelPreferences), "Ctrl+Shift+P", &isOpen);
				ImGui::MenuItem(GetPanelName(m_panelPreferences), "", &isOpen);
				TogglePanel(m_commonPanelMask, m_panelPreferences, isOpen ? TOGGLE_ON : TOGGLE_OFF);
			}

			ImGui::Separator();

			// Exit
			if (ImGui::MenuItem("Exit")) {
				// TODO: Handle Exit
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			using namespace GUI;

			// ImGui demo window
			if (IN_DEBUG_MODE) {
				static bool isDemoWindowOpen = false;
				ImGui::MenuItem("ImGui Demo Window (Debug Mode)", "", &isDemoWindowOpen);
				if (isDemoWindowOpen)
					ImGui::ShowDemoWindow();

				ImGui::Separator();
			}

			// All other panels
			for (auto& [panelID, _] : m_workspacePanelCallbacks) {
				// Only render workspace-specific panels
				if (m_commonPanels.find(panelID) == m_commonPanels.end()) {
					bool isOpen = IsPanelOpen(m_workspacePanelMask, panelID);
					ImGui::MenuItem(GetPanelName(panelID), "", &isOpen);
					TogglePanel(m_workspacePanelMask, panelID, isOpen ? TOGGLE_ON : TOGGLE_OFF);
				}
			}


			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			using namespace GUI;
			// About panel
			{
				bool isAboutPanelOpen = IsPanelOpen(m_commonPanelMask, m_panelAbout);

				ImGui::MenuItem(GetPanelName(m_panelAbout), "", &isAboutPanelOpen);
				TogglePanel(m_commonPanelMask, m_panelAbout, isAboutPanelOpen ? TOGGLE_ON : TOGGLE_OFF);
			}


			ImGui::EndMenu();
		}

		// Plugins
		if (ImGui::BeginMenu("Plugins")) {
			if (ImGui::MenuItem("Weather Prediction")) {
				// ...
			}

			ImGui::EndMenu();
		}


		ImGui::EndMainMenuBar();
	}
}


void UIPanelManager::renderPreferencesPanel() {
	// Options
	static const enum SelectedTree {
		TREE_APPEARANCE,
		TREE_DEBUGGING
	};
	static const enum SelectedOption {
		OPTION_APPEARANCE_COLOR_THEME,
		OPTION_DEBUGGING_CONSOLE
	};
	static const std::unordered_map<std::variant<SelectedTree, SelectedOption>, const char *> SelectedOptionNames = {
		{TREE_APPEARANCE,		"Appearance"},
			{OPTION_APPEARANCE_COLOR_THEME,		"Color theme"},

		{TREE_DEBUGGING,		"Debugging"},
			{OPTION_DEBUGGING_CONSOLE,			"Console"}

	};
	static SelectedTree currentTree = TREE_APPEARANCE;
	static SelectedOption currentSelection = OPTION_APPEARANCE_COLOR_THEME;

	// Child panes
	static const char *LEFT_PANE_ID = "##LeftPane";
	static const char *RIGHT_PANE_ID = "##RightPane";


	// Sets fixed size and initial position for the parent window
	ImGui::SetNextWindowSize(ImVec2(800.0f, 500.0f));
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_None, ImVec2(0.5f, 0.5f));

	// ----- PARENT WINDOW (Dockspace) -----
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	{
		if (ImGui::Begin(GUI::GetPanelName(m_panelAbout), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)) {


			ImGuiID parentDockspaceID = ImGui::GetID("MainDialogDockspace");

			// Calculates space for the dockspace such that there is room for buttons at the bottom
			ImVec2 dockspaceSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGuiUtils::GetBottomButtonAreaHeight());

			ImGui::DockSpace(parentDockspaceID, dockspaceSize, ImGuiDockNodeFlags_None);

			static bool initialLayoutSetUp = false;
			if (!initialLayoutSetUp) {
				initialLayoutSetUp = true;

				// NOTE: It is good practice to clear any existing nodes for this dockspace to ensure the desired initial layout is applied.
				ImGui::DockBuilderRemoveNode(parentDockspaceID);
				ImGui::DockBuilderAddNode(parentDockspaceID, ImGuiDockNodeFlags_DockSpace);

				// IMPORTANT: Sets the size of the dockspace node before splitting for accurate percentage splits
				ImGui::DockBuilderSetNodeSize(parentDockspaceID, dockspaceSize);


				// Splits the dockspace (25% for left, remaining for right)
				ImGuiID leftDockID;
				ImGuiID rightDockID;
				ImGui::DockBuilderSplitNode(parentDockspaceID, ImGuiDir_Left, 0.25f, &leftDockID, &rightDockID);

				// Sets flags on split nodes, because setting them individually (at ImGui::Begin) doesn't work for some reason
				ImGuiDockNode *leftNode = ImGui::DockBuilderGetNode(leftDockID);
				ImGuiDockNode *rightNode = ImGui::DockBuilderGetNode(rightDockID);

				if (leftNode) {
					leftNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoCloseButton;
				}
				if (rightNode) {
					rightNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoCloseButton;
					//rightNode->LocalFlags |= ImGuiDockNodeFlags_NoResizeX;
				}

				// Anticipates future windows and docks them by name
				ImGui::DockBuilderDockWindow(LEFT_PANE_ID, leftDockID);
				ImGui::DockBuilderDockWindow(RIGHT_PANE_ID, rightDockID);


				ImGui::DockBuilderFinish(parentDockspaceID);
			}


			// Buttons
			static const float btnWidth = 70.0f;
			static const int btnCount = 2;
			ImGuiUtils::BottomButtonPadding(btnWidth, btnCount);

			if (ImGui::Button("OK", ImVec2(btnWidth, 0.0f))) {
				// TODO: Handle data-saving
				GUI::TogglePanel(m_commonPanelMask, m_panelPreferences, GUI::TOGGLE_OFF);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(btnWidth, 0.0f))) {
				// TODO: Handle discarding changes
				GUI::TogglePanel(m_commonPanelMask, m_panelPreferences, GUI::TOGGLE_OFF);
			}

			ImGui::End();
		}
	}
	ImGui::PopStyleVar(2);



	// ----- LEFT PANE (OPTIONS PANE) -----
	if (ImGui::Begin(LEFT_PANE_ID, nullptr, ImGuiWindowFlags_NoDecoration)) {
		// APPEARANCE
		if (ImGui::TreeNodeEx(SelectedOptionNames.at(TREE_APPEARANCE), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Selectable(SelectedOptionNames.at(OPTION_APPEARANCE_COLOR_THEME), currentSelection == OPTION_APPEARANCE_COLOR_THEME)) {
				currentTree = TREE_APPEARANCE;
				currentSelection = OPTION_APPEARANCE_COLOR_THEME;
			}

			ImGui::TreePop();
		}


		// DEBUGGING
		if (ImGui::TreeNodeEx(SelectedOptionNames.at(TREE_DEBUGGING))) {
			if (ImGui::Selectable(SelectedOptionNames.at(OPTION_DEBUGGING_CONSOLE), currentSelection == OPTION_DEBUGGING_CONSOLE)) {
				currentTree = TREE_DEBUGGING;
				currentSelection = OPTION_DEBUGGING_CONSOLE;
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}



	// ----- RIGHT PANE (OPTION DETAILS PANE) -----
	if (ImGui::Begin(RIGHT_PANE_ID, nullptr, ImGuiWindowFlags_NoDecoration)) {
		ImGui::AlignTextToFramePadding();

		// Header
		ImGui::SeparatorText(SelectedOptionNames.at(currentTree));


		// Content/Details
		switch (currentTree) {
		case TREE_APPEARANCE:
			ImGuiUtils::BoldText(SelectedOptionNames.at(OPTION_APPEARANCE_COLOR_THEME));
			{
				if (currentSelection == OPTION_APPEARANCE_COLOR_THEME)
					ImGui::ScrollToItem();

				using namespace ImGuiTheme;
				ImGui::TextWrapped("\tTheme:");

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150.0f);
				if (ImGui::BeginCombo("##ColorTheme", AppearanceNames.at(g_appContext.GUI.currentAppearance).c_str())) {
					for (size_t i = 0; i < SIZE_OF(AppearancesArray); i++) {
						bool isSelected = (g_appContext.GUI.currentAppearance == AppearancesArray[i]);
						if (ImGui::Selectable(AppearanceNames.at(AppearancesArray[i]).c_str(), isSelected)) {
							ApplyTheme(AppearancesArray[i]);
							g_appContext.GUI.currentAppearance = AppearancesArray[i];
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
			}

			break;

		case TREE_DEBUGGING:
			ImGuiUtils::BoldText(SelectedOptionNames.at(OPTION_DEBUGGING_CONSOLE));
			{
				if (currentSelection == OPTION_DEBUGGING_CONSOLE)
					ImGui::ScrollToItem();

				ImGui::TextWrapped("\tMaximum log buffer size:");
				ImGui::SameLine();

				ImGui::SetNextItemWidth(150.0f);
				ImGui::InputInt("##LogBufferSize", &Log::MaxLogLines, 0, 0);
			}

			break;
		}

		ImGui::End();
	}
}


void UIPanelManager::renderAboutPanel() {
	ImGui::SetNextWindowSize(ImVec2(1000.0f, 500.0f));
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	GUI::PanelID panelID = m_panelAbout;

	if (ImGui::Begin(GUI::GetPanelName(panelID), nullptr, m_windowFlags | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {

		// The About section
		ImVec2 availableRegion = ImGui::GetContentRegionAvail();
		float availableScrollHeight = availableRegion.y - ImGuiUtils::GetBottomButtonAreaHeight();

		if (ImGui::BeginChild("AboutScrollRegion", ImVec2(0.0f, availableScrollHeight))) {
			// Application logo
			{
				//ImGuiUtils::MoveCursorToMiddle(m_appLogoTexProps.size);
				ImVec2 viewportSize = { ImGuiUtils::GetAvailableWidth() / 2, availableScrollHeight };

				ImGui::Image(m_appLogoTexProps.textureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_appLogoTexProps.size, viewportSize));
				ImGui::SameLine();
				ImGui::Image(m_companyLogoTexProps.textureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_companyLogoTexProps.size, viewportSize));
			}

			ImGuiUtils::BoldText("%s (version %s)", APP_NAME, APP_VERSION);
			ImGui::TextWrapped("Copyright © 2024-2025 %s, D.B.A. Oriviet Aerospace. All Rights Reserved.", AUTHOR_DIACRITIC);

			ImGuiUtils::Padding();

			ImGui::SeparatorText("Attribution");
			{
				ImGui::TextWrapped("Graphics API:");
				ImGui::SameLine();
				ImGui::TextLinkOpenURL("Vulkan 1.2", "https://www.vulkan.org/");


				ImGui::TextWrapped("GUI Library:");
				ImGui::SameLine();
				ImGui::TextLinkOpenURL("Dear ImGui (docking branch)", "https://github.com/ocornut/imgui/");
				ImGui::SameLine();
				ImGui::TextWrapped("by Omar Cornut");


				ImGui::TextWrapped("Simulation Assets:");

				ImGui::BulletText("");
				ImGui::SameLine();
				ImGui::TextLinkOpenURL("Earth 3D Model", "https://science.nasa.gov/resource/earth-3d-model/");
				ImGui::SameLine();
				ImGui::TextWrapped("by NASA Visualization Technology Applications and Development (VTAD)");

				ImGui::BulletText("");
				ImGui::SameLine();
				ImGui::TextLinkOpenURL("Chandra X-Ray Observatory Model", "https://nasa3d.arc.nasa.gov/detail/jpl-chandra");
				ImGui::SameLine();
				ImGui::TextWrapped("by Brian Kumanchik, NASA/JPL-Caltech");
			}


			ImGui::EndChild();
		}


		static const float btnWidth = 70.0f;
		ImGuiUtils::BottomButtonPadding(btnWidth, 1);

		if (ImGui::Button("OK", ImVec2(btnWidth, 0.0f))) {
			GUI::TogglePanel(m_commonPanelMask, panelID, GUI::TOGGLE_OFF);
		}

		ImGui::End();
	}
}