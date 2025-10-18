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
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<UIPanelManager>();

	m_eventDispatcher->subscribe<InitEvent::ImGui>(selfIndex,
		[this](const InitEvent::ImGui& event) {
			this->onImGuiInit();
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SceneLoadProgress>(selfIndex,
		[this](const UpdateEvent::SceneLoadProgress &event) {
			// NOTE: These updates will happen on the worker thread, but ImGui drawing happens on the main thread.
			// Accessing these members directly is fine as they're simple types and updates will be observed eventually.
			// For complex UI states, a concurrent queue or a deferred update might be needed.
			m_showLoadingModal = true;
			m_currentLoadProgress = event.progress;
			m_currentLoadMessage = event.message;
			m_loadErrorOccurred = false;
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SceneLoadComplete>(selfIndex,
		[this](const UpdateEvent::SceneLoadComplete &event) {
			m_currentLoadProgress = 1.0f;
			m_currentLoadMessage = event.finalMessage;
			m_loadErrorOccurred = !event.loadSuccessful;
			m_loadErrorMessage = event.finalMessage;
		}
	);


	m_eventDispatcher->subscribe<ConfigEvent::SimulationFileParsed>(selfIndex,
		[this](const ConfigEvent::SimulationFileParsed &event) {
			m_fileConfig = event.fileConfig;
			m_simulationConfig = event.simulationConfig;
		}
	);
}


void UIPanelManager::initCommonPanels() {
	m_panelPreferences =	GUI::RegisterPanel("Preferences");
	m_panelAbout =			GUI::RegisterPanel("App Info & Attribution");
	m_panelWelcome =		GUI::RegisterPanel("Welcome to Astrocelerate!", true);

	m_commonPanelCallbacks[m_panelPreferences] =	[this]() { this->renderPreferencesPanel(); };
	m_commonPanelCallbacks[m_panelAbout] =			[this]() { this->renderAboutPanel(); };

	GUI::TogglePanel(m_commonPanelMask, m_panelWelcome, GUI::TOGGLE_ON);
}


void UIPanelManager::initStaticTextures() {
	std::shared_ptr<TextureManager> textureManager = ServiceLocator::GetService<TextureManager>(__FUNCTION__);

	// App logo
	std::string logoPath = FilePathUtils::JoinPaths(ROOT_DIR, "assets/App", "AstrocelerateLogo.png");
	Geometry::Texture texture = textureManager->createIndependentTexture(logoPath, VK_FORMAT_R8G8B8A8_SRGB);
	
	m_appLogoTexProps.size = ImVec2(texture.size.x, texture.size.y);
	m_appLogoTexProps.textureID = TextureUtils::GenerateImGuiTextureID(texture.imageLayout, texture.imageView, texture.sampler);

	// Oriviet Aerospace logo
	//logoPath = FilePathUtils::JoinPaths(ROOT_DIR, "assets/App", "OrivietAerospaceLogo-Mono.png");
	//texture = textureManager->createIndependentTexture(logoPath, VK_FORMAT_R8G8B8A8_SRGB);
	//
	//m_companyLogoTexProps.size = ImVec2(texture.size.x, texture.size.y);
	//m_companyLogoTexProps.textureID = TextureUtils::GenerateImGuiTextureID(texture.imageLayout, texture.imageView, texture.sampler);
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

		// Instanced panels
	if (GUI::IsPanelOpen(m_commonPanelMask, m_panelWelcome))
		renderWelcomePanel();


	m_currentWorkspace->update(currentFrame);
}


void UIPanelManager::renderMenuBar() {
	static std::string selectedFile{};

	if (m_showLoadingModal) {
		ImGui::OpenPopup(m_sceneLoadModelName);
	}
	renderSceneLoadModal(selectedFile);


	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			// Open
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				// Handle Open using tinyfiledialogs
				const char *filterPatterns[] = { "*.yaml" };
				const char *selectedFilePath = tinyfd_openFileDialog(
					"Open Simulation File",
					FilePathUtils::JoinPaths(ROOT_DIR, "samples/").c_str(),          // Default path/filename
					IM_ARRAYSIZE(filterPatterns),
					filterPatterns,
					NULL,        // Optional: filter description (e.g., "Text files")
					0            // Allow multiple selections (0 for single, 1 for multiple)
				);

				if (selectedFilePath) {
					// Reset UI state before starting a new load
					m_showLoadingModal = true;
					m_currentLoadProgress = 0.0f;
					m_currentLoadMessage = "Starting scene load...";
					m_loadErrorOccurred = false;
					m_loadErrorMessage = "";

					m_currentWorkspace->loadSimulationConfig(selectedFilePath);
					selectedFile = FilePathUtils::GetFileName(selectedFilePath);
				}
			}
			ImGuiUtils::CursorOnHover();


			// Save
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				// TODO: Handle Save
			}
			ImGuiUtils::CursorOnHover();


			ImGui::Separator();


			// Preferences
			{
				using namespace GUI;
				bool isOpen = IsPanelOpen(m_commonPanelMask, m_panelPreferences);

				//ImGui::MenuItem(GetPanelName(m_panelPreferences), "Ctrl+Shift+P", &isOpen);
				ImGui::MenuItem(GetPanelName(m_panelPreferences), "", &isOpen);
				ImGuiUtils::CursorOnHover();
				TogglePanel(m_commonPanelMask, m_panelPreferences, isOpen ? TOGGLE_ON : TOGGLE_OFF);
			}

			ImGui::Separator();


			// Exit
			if (ImGui::MenuItem("Exit")) {
				throw Log::EngineExitException();
			}
			ImGuiUtils::CursorOnHover();

			ImGui::EndMenu();
		}
		ImGuiUtils::CursorOnHover();



		if (ImGui::BeginMenu("View")) {
			using namespace GUI;

			// ImGui demo window
			if (IN_DEBUG_MODE) {
				static bool isDemoWindowOpen = false;
				ImGui::MenuItem("ImGui Demo Window (Debug Mode)", "", &isDemoWindowOpen);
				ImGuiUtils::CursorOnHover();
				if (isDemoWindowOpen)
					ImGui::ShowDemoWindow();

				ImGui::Separator();
			}


			// All other panels
			for (auto& [panelID, _] : m_workspacePanelCallbacks) {
				// Only render workspace-specific, persistent panels
				if (m_commonPanels.count(panelID) == 0 && !IsPanelInstanced(panelID)) {
					bool isOpen = IsPanelOpen(m_workspacePanelMask, panelID);
					ImGui::MenuItem(GetPanelName(panelID), "", &isOpen);
					ImGuiUtils::CursorOnHover();
					TogglePanel(m_workspacePanelMask, panelID, isOpen ? TOGGLE_ON : TOGGLE_OFF);
				}
			}


			ImGui::EndMenu();
		}
		ImGuiUtils::CursorOnHover();



		if (ImGui::BeginMenu("Help")) {
			using namespace GUI;
			// About panel
			{
				bool isAboutPanelOpen = IsPanelOpen(m_commonPanelMask, m_panelAbout);

				ImGui::MenuItem(GetPanelName(m_panelAbout), "", &isAboutPanelOpen);
				ImGuiUtils::CursorOnHover();
				TogglePanel(m_commonPanelMask, m_panelAbout, isAboutPanelOpen ? TOGGLE_ON : TOGGLE_OFF);
			}


			ImGui::EndMenu();
		}
		ImGuiUtils::CursorOnHover();



		// Plugins
		if (ImGui::BeginMenu("Plugins")) {
			//ImGui::MenuItem("Weather Prediction");
			//ImGuiUtils::CursorOnHover();
			//ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			//	ImGuiUtils::TextTooltip(ImGuiHoveredFlags_None, "This plugin is not currently supported.");
			//ImGui::PopStyleColor();
			
			ImGui::EndMenu();
		}
		ImGuiUtils::CursorOnHover();


		ImGui::TextLinkOpenURL("Give Feedback", "https://forms.gle/xpaqY4BoVRsGLhbC9/");


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
		OPTION_DEBUGGING_CONSOLE,
		OPTION_DEBUGGING_NEXT_LAUNCH
	};
	static const std::unordered_map<std::variant<SelectedTree, SelectedOption>, const char *> SelectedOptionNames = {
		{TREE_APPEARANCE,		"Appearance"},
			{OPTION_APPEARANCE_COLOR_THEME,		"Color theme"},

		{TREE_DEBUGGING,		"Debugging"},
			{OPTION_DEBUGGING_CONSOLE,			"Console (GUI)"},
			{OPTION_DEBUGGING_NEXT_LAUNCH,		"Next launch"}

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
		if (ImGui::Begin(GUI::GetPanelName(m_panelPreferences), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)) {


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
			ImGuiUtils::CursorOnHover();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(btnWidth, 0.0f))) {
				// TODO: Handle discarding changes
				GUI::TogglePanel(m_commonPanelMask, m_panelPreferences, GUI::TOGGLE_OFF);
			}
			ImGuiUtils::CursorOnHover();

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
			ImGuiUtils::CursorOnHover();

			ImGui::TreePop();
		}
		ImGuiUtils::CursorOnHover();


		// DEBUGGING
		if (ImGui::TreeNodeEx(SelectedOptionNames.at(TREE_DEBUGGING))) {
			if (ImGui::Selectable(SelectedOptionNames.at(OPTION_DEBUGGING_CONSOLE), currentSelection == OPTION_DEBUGGING_CONSOLE)) {
				currentTree = TREE_DEBUGGING;
				currentSelection = OPTION_DEBUGGING_CONSOLE;
			}
			ImGuiUtils::CursorOnHover();

			ImGui::TreePop();
		}
		ImGuiUtils::CursorOnHover();

		ImGui::End();
	}



	// ----- RIGHT PANE (OPTION DETAILS PANE) -----
	if (ImGui::Begin(RIGHT_PANE_ID, nullptr, ImGuiWindowFlags_NoDecoration)) {
		ImGui::AlignTextToFramePadding();

		// Header
		ImGuiUtils::BoldText(SelectedOptionNames.at(currentTree));


		// Content/Details
		switch (currentTree) {
		case TREE_APPEARANCE:
			ImGui::SeparatorText(SelectedOptionNames.at(OPTION_APPEARANCE_COLOR_THEME));
			ImGui::Indent();
			{
				if (currentSelection == OPTION_APPEARANCE_COLOR_THEME)
					ImGui::ScrollToItem();


				using namespace ImGuiTheme;
				ImGui::Text("Theme:");

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150.0f);
				if (ImGui::BeginCombo("##ColorTheme", AppearanceNames.at(g_appContext.GUI.currentAppearance).c_str(), ImGuiComboFlags_NoArrowButton)) {
					for (size_t i = 0; i < SIZE_OF(AppearancesArray); i++) {
						bool isSelected = (g_appContext.GUI.currentAppearance == AppearancesArray[i]);
						if (ImGui::Selectable(AppearanceNames.at(AppearancesArray[i]).c_str(), isSelected)) {
							ApplyTheme(AppearancesArray[i]);
							g_appContext.GUI.currentAppearance = AppearancesArray[i];
						}
						ImGuiUtils::CursorOnHover();

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
			}
			ImGui::Unindent();

			break;
			


		case TREE_DEBUGGING:
			ImGui::SeparatorText(SelectedOptionNames.at(OPTION_DEBUGGING_CONSOLE));
			ImGui::Indent();
			{
				if (currentSelection == OPTION_DEBUGGING_CONSOLE)
					ImGui::ScrollToItem();


				ImGui::Text("Maximum log buffer size:");
				ImGui::SameLine();

				ImGui::SetNextItemWidth(150.0f);
				ImGui::InputInt("##LogBufferSize", &Log::MaxLogLines, 0, 0);
				ImGuiUtils::CursorOnHover(ImGuiMouseCursor_TextInput);
			}
			ImGui::Unindent();


			ImGui::SeparatorText(SelectedOptionNames.at(OPTION_DEBUGGING_NEXT_LAUNCH));
			ImGui::Indent();
			{
				if (currentSelection == OPTION_DEBUGGING_NEXT_LAUNCH)
					ImGui::ScrollToItem();


				// Debug mode
				static bool enableDebMode = false;
				if (ImGui::Checkbox("Enable debug mode", &enableDebMode)) {
					if (enableDebMode) {}
					else {}
				}

				// Show default console
				static bool showDefaultConsole = false;
				if (ImGui::Checkbox("Show default console", &showDefaultConsole)) {
					if (showDefaultConsole) {}
					else {}
				}
			}
			ImGui::Unindent();


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
				ImVec2 viewportSize = { ImGuiUtils::GetAvailableWidth() / 1.5f, availableScrollHeight };

				// Calculate the horizontal offset needed to center the text
				float offsetX = (availableRegion.x - viewportSize.x) * 0.5f;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

				ImGui::Image(m_appLogoTexProps.textureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_appLogoTexProps.size, viewportSize));
				//ImGui::SameLine();
				//ImGui::Image(m_companyLogoTexProps.textureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_companyLogoTexProps.size, viewportSize));
			}

			ImGui::PushFont(g_fontContext.NotoSans.bold);
			{
				ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, "%s (version %s)", APP_NAME, APP_VERSION);
				ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, "Copyright © 2024-2025 %s, D.B.A. Oriviet Aerospace.", AUTHOR_DIACRITIC);
			}
			ImGui::PopFont();


			ImGuiUtils::Padding();


			ImGui::TextWrapped(STD_STR("Astrocelerate was released under Apache License, Version 2.0 (the " + enquote("License") + "). You may obtain a copy of the License").c_str());
			ImGui::SameLine();
			ImGui::TextLinkOpenURL("here.", "http://www.apache.org/licenses/LICENSE-2.0");


			ImGuiUtils::Padding();


			ImGui::TextWrapped("Astrocelerate is Vietnam's first high-performance orbital mechanics and spaceflight simulation engine, designed from the ground up to serve as a sovereign alternative to foreign aerospace software.");


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


				ImGui::TextWrapped("Script parser:");
				ImGui::SameLine();
				ImGui::TextLinkOpenURL("YAML-CPP", "https://github.com/jbeder/yaml-cpp");


				ImGui::TextWrapped("Base Code Editor Implementation:");
				ImGui::SameLine();
				ImGui::TextLinkOpenURL("ImGuiColorTextEdit", "https://github.com/BalazsJako/ImGuiColorTextEdit/tree/master");
				ImGui::SameLine();
				ImGui::TextWrapped("by BalazsJako");


				ImGui::TextWrapped("Simulation Assets:");
				ImGui::Indent();
				{
					ImGui::TextWrapped("Planet textures (Earth, Moon, etc.) by NASA Visualization Technology Applications and Development (VTAD) and");
					ImGui::SameLine();
					ImGui::TextLinkOpenURL("Solar System Scope", "https://www.solarsystemscope.com/textures/");

					ImGui::TextLinkOpenURL("Chandra X-Ray Observatory Model", "https://nasa3d.arc.nasa.gov/detail/jpl-chandra");
					ImGui::SameLine();
					ImGui::TextWrapped("by Brian Kumanchik, NASA/JPL-Caltech");
				}
				ImGui::Unindent();
			}


			ImGui::EndChild();
		}


		static const float btnWidth = 70.0f;
		ImGuiUtils::BottomButtonPadding(btnWidth, 1);

		if (ImGui::Button("OK", ImVec2(btnWidth, 0.0f))) {
			GUI::TogglePanel(m_commonPanelMask, panelID, GUI::TOGGLE_OFF);
		}
		ImGuiUtils::CursorOnHover();

		ImGui::End();
	}
}


void UIPanelManager::renderSceneLoadModal(const std::string &fileName) {
	ImGui::SetNextWindowSize(ImVec2(500.0f, 0.0f));
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ColorUtils::sRGBToLinear(0.0f, 0.0f, 0.0f, 0.80f));
	{
		if (ImGui::BeginPopupModal(m_sceneLoadModelName, &m_showLoadingModal, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration)) {

			ImVec2 availableRegion = ImGui::GetContentRegionAvail();
			float availableScrollHeight = availableRegion.y - ImGuiUtils::GetBottomButtonAreaHeight();

			if (!m_loadErrorOccurred) {
				ImGui::PushFont(g_fontContext.NotoSans.bold);
				{
					ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, "Processing %s", fileName.c_str());
					ImGuiUtils::Padding();
					ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, m_currentLoadMessage.c_str());
				}
				ImGui::PopFont();


				ImGuiUtils::Padding();


				char overlay[32];
				sprintf(overlay, "%.1f%%", m_currentLoadProgress * 100);
				ImGui::ProgressBar(m_currentLoadProgress, ImVec2(-1.0f, 0.0f), overlay);  // -1 for full width, 0 for default height


				ImGuiUtils::Padding();


				ImGui::PushFont(g_fontContext.NotoSans.italic);
					ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, enquote("%s").c_str(), m_fileConfig.description.c_str());
				ImGui::PopFont();


				if (m_currentLoadProgress >= 1.0f) {
					ImGui::CloseCurrentPopup();
					m_showLoadingModal = false;
				}
			}

			else {
				// Display error message and an "OK" button to close
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::PushFont(g_fontContext.NotoSans.bold);
				{
					ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, "Failed to load %s", fileName.c_str());
					ImGuiUtils::Padding();
					ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, m_currentLoadMessage.c_str());
				}
				ImGui::PopStyleColor();
				ImGui::PopFont();


				ImGuiUtils::Padding();


				static const float btnWidth = 70.0f;
				ImGuiUtils::BottomButtonPadding(btnWidth, 1);
				
				if (ImGui::Button("OK", ImVec2(btnWidth, 0.0f))) {
					ImGui::CloseCurrentPopup();
					m_showLoadingModal = false;
				}
				ImGuiUtils::CursorOnHover();
			}


			ImGui::EndPopup();
		}
	}
	ImGui::PopStyleColor();
}


void UIPanelManager::renderWelcomePanel() {
	ImGui::SetNextWindowSize(ImVec2(800.0f, 550.0f));
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static const char *panelName = GUI::GetPanelName(m_panelWelcome);
	static bool panelOpen = GUI::IsPanelOpen(m_commonPanelMask, m_panelWelcome);
	if (ImGui::Begin(panelName, &panelOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar)) {

		ImVec2 availableRegion = ImGui::GetContentRegionAvail();
		float availableScrollHeight = availableRegion.y - ImGuiUtils::GetBottomButtonAreaHeight();

		// Render logo
		ImVec2 viewportSize = { ImGuiUtils::GetAvailableWidth() / 1.5f, availableScrollHeight };

			// Calculate the horizontal offset needed to center the text
		float offsetX = (availableRegion.x - viewportSize.x) * 0.5f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

		ImGui::Image(m_appLogoTexProps.textureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_appLogoTexProps.size, viewportSize));


		// Body
		ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, ImGuiUtils::IconString(ICON_FA_SATELLITE, "Welcome to Astrocelerate!").c_str());

		ImGuiUtils::Padding();

		ImGui::Text("To get started, please open a simulation script by going to File > Open.");
		ImGui::Text("A few sample scripts have been provided. Feel free to play around with them!");

		ImGuiUtils::Padding();

		ImGui::TextWrapped("The source code for Astrocelerate is available in");
		ImGui::SameLine();
		ImGui::TextLinkOpenURL("this repository.", "https://github.com/ButteredFire/Astrocelerate/");
		ImGui::TextWrapped("If you have any questions or concerns, you can submit an issue there. Contributions are absolutely welcome!");

		ImGuiUtils::Padding();

		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
		{
			ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, ImGuiUtils::IconString(ICON_FA_TRIANGLE_EXCLAMATION, "WE WANT TO HEAR WHAT YOU HAVE TO SAY!").c_str());
		}
		ImGui::PopStyleColor();

		ImGui::TextWrapped("Astrocelerate is in its early development phase. Your feedback is absolutely instrumental in shaping the future of Astrocelerate as an orbital mechanics simulation engine.");
		ImGui::TextWrapped("We deeply value your feedback. To submit one, please fill in");
		ImGui::SameLine();
		ImGui::TextLinkOpenURL("this form.", "https://forms.gle/xpaqY4BoVRsGLhbC9/");
		ImGui::TextWrapped("Alternatively, you can directly give feedback via");
		ImGui::SameLine();
		ImGui::TextLinkOpenURL("GitHub Discussions.", "https://github.com/ButteredFire/Astrocelerate/discussions/");

		ImGuiUtils::Padding();

		ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, "Thank you, and may the future of spaceflight rise and shine!");

		ImGui::End();
	}
	GUI::TogglePanel(m_commonPanelMask, m_panelWelcome, panelOpen ? GUI::TOGGLE_ON : GUI::TOGGLE_OFF);
}
