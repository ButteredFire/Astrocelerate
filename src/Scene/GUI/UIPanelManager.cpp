#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager() {

	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	m_windowFlags = ImGuiWindowFlags_NoCollapse;
	m_popupWindowFlags = ImGuiWindowFlags_NoDocking;

	bindPanelFlags();
	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void UIPanelManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::GUIContextIsValid>(
		[this](const Event::GUIContextIsValid& event) {
			this->onImGuiInit();
		}
	);

	m_eventDispatcher->subscribe<Event::InputIsValid>(
		[this](const Event::InputIsValid& event) {
			m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);
		}
	);

	m_eventDispatcher->subscribe<Event::OffscreenResourcesAreRecreated>(
		[this](const Event::OffscreenResourcesAreRecreated& event) {
			for (size_t i = 0; i < m_viewportRenderTextureIDs.size(); i++) {
				ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet) m_viewportRenderTextureIDs[i]);
			}

			initDynamicTextures();
		}
	);
}


void UIPanelManager::onImGuiInit() {
	initStaticTextures();
	initDynamicTextures();


	// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_VIEWPORT, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_TELEMETRY, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_ENTITY_INSPECTOR, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_SIMULATION_CONTROL, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_RENDER_SETTINGS, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_ORBITAL_PLANNER, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_CONSOLE, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_APP, GUI::TOGGLE_ON);

	initPanelsFromMask(m_panelMask);


	m_windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
}


void UIPanelManager::initPanelsFromMask(GUI::PanelMask& mask) {
	using namespace GUI;

	for (size_t i = 0; i < FLAG_COUNT; i++) {
		PanelFlag flag = PanelFlagsArray[i];

		if (IsPanelOpen(mask, flag)) {
			auto it = m_panelCallbacks.find(flag);

			if (it == m_panelCallbacks.end() || 
				(it != m_panelCallbacks.end() && it->second == nullptr)
				) {

				Log::Print(Log::T_ERROR, __FUNCTION__, "Cannot open panel: Initialization callback for panel flag #" + std::to_string(i) + " is not available!");

				continue;
			}
		}
	}
}


void UIPanelManager::updatePanels(uint32_t currentFrame) {
	m_currentFrame = currentFrame;
	//renderPanelsMenu();
	
	for (auto&& [flag, callback] : m_panelCallbacks) {
		if (GUI::IsPanelOpen(m_panelMask, flag)) {
			ImGui::SetNextWindowClass(&m_windowClass);
			(this->*callback)();
		}
	}

	// This window will capture all input and prevent interaction with other widgets
	if (m_inputManager->isViewportInputAllowed()) {
		m_inputBlockerIsOn = true;
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		if (ImGui::Begin("##InputBlocker", nullptr,
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground)) {

			ImGui::End();
		}
	}
	else {
		m_inputBlockerIsOn = false;
	}
}


void UIPanelManager::bindPanelFlags() {
	using namespace GUI;

	m_panelCallbacks[PanelFlag::PANEL_VIEWPORT]				= &UIPanelManager::renderViewportPanel;

	m_panelCallbacks[PanelFlag::PANEL_MENU_PREFERENCES]		= &UIPanelManager::renderPreferencesPanel;
	m_panelCallbacks[PanelFlag::PANEL_MENU_ABOUT]			= &UIPanelManager::renderAboutPanel;

	m_panelCallbacks[PanelFlag::PANEL_TELEMETRY]			= &UIPanelManager::renderTelemetryPanel;
	m_panelCallbacks[PanelFlag::PANEL_ENTITY_INSPECTOR]		= &UIPanelManager::renderEntityInspectorPanel;
	m_panelCallbacks[PanelFlag::PANEL_SIMULATION_CONTROL]	= &UIPanelManager::renderSimulationControlPanel;
	m_panelCallbacks[PanelFlag::PANEL_RENDER_SETTINGS]		= &UIPanelManager::renderRenderSettingsPanel;
	m_panelCallbacks[PanelFlag::PANEL_ORBITAL_PLANNER]		= &UIPanelManager::renderOrbitalPlannerPanel;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_CONSOLE]		= &UIPanelManager::renderDebugConsole;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_APP]			= &UIPanelManager::renderDebugApplication;
}


void UIPanelManager::initStaticTextures() {
	std::shared_ptr<TextureManager> textureManager = ServiceLocator::GetService<TextureManager>(__FUNCTION__);

	// Startup Logo
	{
		std::string logoPath = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/App");
		if (g_appContext.GUI.currentAppearance == ImGuiTheme::Appearance::IMGUI_APPEARANCE_DARK_MODE)
			logoPath = FilePathUtils::JoinPaths(logoPath, "StartupLogoTransparentWhite.png");
		else
			logoPath = FilePathUtils::JoinPaths(logoPath, "StartupLogoTransparentBlack.png");

		Geometry::Texture texture = textureManager->createIndependentTexture(logoPath, VK_FORMAT_R8G8B8A8_SRGB);

		m_startupLogoTextureID = (ImTextureID)ImGui_ImplVulkan_AddTexture(texture.sampler, texture.imageView, texture.imageLayout);
		m_startupLogoSize = { texture.size.x, texture.size.y };
	}

	// Application logo
	{
		std::string logoPath = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/App", "ExperimentalAppLogoV1Transparent.png");

		Geometry::Texture texture = textureManager->createIndependentTexture(logoPath, VK_FORMAT_R8G8B8A8_SRGB);

		m_appLogoTextureID = (ImTextureID)ImGui_ImplVulkan_AddTexture(texture.sampler, texture.imageView, texture.imageLayout);
		m_appLogoSize = { texture.size.x, texture.size.y };
	}
}


void UIPanelManager::initDynamicTextures() {
	// Offscreen resources (for the viewport)
	const size_t OFFSCREEN_RESOURCE_COUNT = g_vkContext.OffscreenResources.images.size();
	m_viewportRenderTextureIDs.resize(OFFSCREEN_RESOURCE_COUNT);

	for (size_t i = 0; i < OFFSCREEN_RESOURCE_COUNT; i++) {
		m_viewportRenderTextureIDs[i] = (ImTextureID)ImGui_ImplVulkan_AddTexture(
			g_vkContext.OffscreenResources.samplers[i],
			g_vkContext.OffscreenResources.imageViews[i],
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}
}


void UIPanelManager::updateViewportTexture(uint32_t currentFrame) {
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = g_vkContext.OffscreenResources.imageViews[currentFrame];
	imageInfo.sampler = g_vkContext.OffscreenResources.samplers[currentFrame];
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet imageDescSetWrite{};
	imageDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	imageDescSetWrite.dstBinding = 0;
	imageDescSetWrite.descriptorCount = 1;
	imageDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageDescSetWrite.dstSet = (VkDescriptorSet)m_viewportRenderTextureIDs[currentFrame];
	imageDescSetWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(g_vkContext.Device.logicalDevice, 1, &imageDescSetWrite, 0, nullptr);
}


void UIPanelManager::performBackgroundChecks(GUI::PanelFlag flag) {
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		m_currentFocusedPanel = flag;
}


void UIPanelManager::renderMenuBar() {
	if (ImGui::BeginMenuBar()) {

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
				PanelFlag panelFlag = PanelFlag::PANEL_MENU_PREFERENCES;
				bool isOpen = IsPanelOpen(m_panelMask, panelFlag);

				ImGui::MenuItem(GetPanelName(panelFlag), "Ctrl+Shift+P", &isOpen);
				TogglePanel(m_panelMask, panelFlag, isOpen ? TOGGLE_ON : TOGGLE_OFF);
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
			for (size_t i = 0; i < FLAG_COUNT; i++) {
				PanelFlag flag = PanelFlagsArray[i];

				// Only render normal panels
				if (PanelMenuFlags.find(flag) == PanelMenuFlags.end()) {
					bool isOpen = IsPanelOpen(m_panelMask, flag);
					ImGui::MenuItem(GetPanelName(flag), "", &isOpen);
					TogglePanel(m_panelMask, flag, isOpen ? TOGGLE_ON : TOGGLE_OFF);
				}
			}


			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			using namespace GUI;
			// About panel
			{
				PanelFlag aboutPanelFlag = PanelFlag::PANEL_MENU_ABOUT;
				bool isAboutPanelOpen = IsPanelOpen(m_panelMask, aboutPanelFlag);

				ImGui::MenuItem(GetPanelName(aboutPanelFlag), "", &isAboutPanelOpen);
				TogglePanel(m_panelMask, aboutPanelFlag, isAboutPanelOpen ? TOGGLE_ON : TOGGLE_OFF);
			}


			ImGui::EndMenu();
		}


		ImGui::EndMenuBar();
	}
}


void UIPanelManager::renderPanelsMenu() {
	using namespace GUI;

	if (ImGui::Begin("Panels Menu", nullptr, m_windowFlags)) {
		if (IN_DEBUG_MODE) {
			static bool isDemoWindowOpen = false;
			ImGui::Checkbox("ImGui Demo Window", &isDemoWindowOpen);
			if (isDemoWindowOpen) {
				ImGui::ShowDemoWindow();
			}
		}

		for (size_t i = 0; i < FLAG_COUNT; i++) {
			PanelFlag flag = PanelFlagsArray[i];

			bool isOpen = IsPanelOpen(m_panelMask, flag);
			ImGui::Checkbox(GetPanelName(flag), &isOpen);

			TogglePanel(m_panelMask, flag, isOpen ? TOGGLE_ON : TOGGLE_OFF);
		}

		ImGui::End();
	}
}


void UIPanelManager::renderViewportPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_VIEWPORT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) {
		Log::Print(Log::T_ERROR, __FUNCTION__, "The viewport must not be closed!");
		GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_ON);
	}


	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
		performBackgroundChecks(flag);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		// Pause/Play button
		static bool initialLoad = true;
		static float lastTimeScale = 1.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.4f));
		{
			if (m_simulationIsPaused) {
				if (initialLoad) {
					Time::SetTimeScale(0.0f);
					initialLoad = false;
				}

				if (ImGui::Button(ICON_FA_PLAY "  Play")) {
					Time::SetTimeScale(lastTimeScale);
					m_simulationIsPaused = false;
				}
			}
			else {
				if (ImGui::Button(ICON_FA_PAUSE "  Pause")) {
					lastTimeScale = Time::GetTimeScale();
					Time::SetTimeScale(0.0f);
					m_simulationIsPaused = true;
				}
			}
		}
		ImGui::PopStyleColor(2);


		g_appContext.Input.isViewportHoveredOver = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) || m_inputBlockerIsOn;
		g_appContext.Input.isViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || m_inputBlockerIsOn;

		// Resizes the texture to its original aspect ratio before rendering
		ImVec2 originalRenderSize = {
			static_cast<float>(g_vkContext.SwapChain.extent.width),
			static_cast<float>(g_vkContext.SwapChain.extent.height)
		};
		ImVec2 textureSize = ImGuiUtils::ResizeImagePreserveAspectRatio(originalRenderSize, viewportPanelSize);
		
			// Padding to center the texture
		ImVec2 offset{};
		offset.x = (viewportPanelSize.x - textureSize.x) * 0.5f;
		offset.y = (viewportPanelSize.y - textureSize.y) * 0.5f;

		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos({ cursorPos.x + offset.x, cursorPos.y + offset.y });

		ImGui::Image(m_viewportRenderTextureIDs[m_currentFrame], textureSize);
	
		ImGui::End();
	}
}


void UIPanelManager::renderPreferencesPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_MENU_PREFERENCES;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	// Options
	static const enum SelectedTree {
		TREE_APPEARANCE,
		TREE_DEBUGGING
	};
	static const enum SelectedOption {
		OPTION_APPEARANCE_COLOR_THEME,
		OPTION_DEBUGGING_CONSOLE
	};
	static const std::unordered_map<std::variant<SelectedTree, SelectedOption>, const char*> SelectedOptionNames = {
		{TREE_APPEARANCE,		"Appearance"},
			{OPTION_APPEARANCE_COLOR_THEME,		"Color theme"},

		{TREE_DEBUGGING,		"Debugging"},
			{OPTION_DEBUGGING_CONSOLE,			"Console"}

	};
	static SelectedTree currentTree = TREE_APPEARANCE;
	static SelectedOption currentSelection = OPTION_APPEARANCE_COLOR_THEME;

	// Child panes
	static const char* LEFT_PANE_ID = "##LeftPane";
	static const char* RIGHT_PANE_ID = "##RightPane";


	// Sets fixed size and initial position for the parent window
	ImGui::SetNextWindowSize(ImVec2(800.0f, 500.0f));
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_None, ImVec2(0.5f, 0.5f));

	// ----- PARENT WINDOW (Dockspace) -----
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	{
		if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)) {
			performBackgroundChecks(flag);

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
				ImGuiDockNode* leftNode = ImGui::DockBuilderGetNode(leftDockID);
				ImGuiDockNode* rightNode = ImGui::DockBuilderGetNode(rightDockID);

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
				GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_OFF);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(btnWidth, 0.0f))) {
				// TODO: Handle discarding changes
				GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_OFF);
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
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_MENU_ABOUT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;


	ImGui::SetNextWindowSize(ImVec2(1000.0f, 500.0f));
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
		performBackgroundChecks(flag);


		// The About section
		ImVec2 availableRegion = ImGui::GetContentRegionAvail();
		float availableScrollHeight = ImGui::GetContentRegionAvail().y - ImGuiUtils::GetBottomButtonAreaHeight();

		if (ImGui::BeginChild("AboutScrollRegion", ImVec2(0.0f, availableScrollHeight))) {
			// Startup and Logo images
			{
				static const float middleGap = 10.0f;
				ImVec2 splitPanelSize = { (ImGuiUtils::GetAvailableWidth() - middleGap) / 2.0f, availableScrollHeight };

				ImGui::Image(m_startupLogoTextureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_startupLogoSize, splitPanelSize));

				ImGui::SameLine(0.0f, middleGap);

				ImGui::Image(m_appLogoTextureID, ImGuiUtils::ResizeImagePreserveAspectRatio(m_appLogoSize, splitPanelSize));
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
			GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_OFF);
		}

		ImGui::End();
	}
}


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	//if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	static const ImVec2 separatorPadding(10.0f, 10.0f);
	
	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		auto view = m_registry->getView<PhysicsComponent::RigidBody, PhysicsComponent::ReferenceFrame, TelemetryComponent::RenderTransform>();
		size_t entityCount = 0;

		for (const auto& [entity, rigidBody, refFrame, renderT] : view) {
			// As the content is dynamically generated, we need each iteration to have its ImGui ID to prevent conflicts.
			// Since entity IDs are always unique, we can use them as ImGui IDs.
			ImGui::PushID(static_cast<int>(entity));


			ImGuiUtils::BoldText("%s (ID: %d)", m_registry->getEntity(entity).name.c_str(), entity);


			// --- Rigid-body Debug Info ---
			if (ImGui::CollapsingHeader("Rigid-body Data")) {
				float velocityAbs = glm::length(rigidBody.velocity);
				ImGuiUtils::BoldText("Velocity");

				ImGuiUtils::ComponentField(
					{
						{"X", rigidBody.velocity.x},
						{"Y", rigidBody.velocity.y},
						{"Z", rigidBody.velocity.z}
					},
					"%.2f", "\tVector"
				);
				ImGui::Text("\tAbsolute: |v| ~= %.4f m/s", velocityAbs);

				ImGui::Dummy(ImVec2(0.5f, 0.5f));

				float accelerationAbs = glm::length(rigidBody.acceleration);
				ImGuiUtils::BoldText("Acceleration");

				ImGuiUtils::ComponentField(
					{
						{"X", rigidBody.acceleration.x},
						{"Y", rigidBody.acceleration.y},
						{"Z", rigidBody.acceleration.z}
					},
					"%.2f", "\tVector"
				);
				ImGui::Text("\tAbsolute: |a| ~= %.4f m/s^2", accelerationAbs);


				// Display mass: scientific notation for large values, fixed for small
				if (std::abs(rigidBody.mass) >= 1e6f) {
					ImGuiUtils::BoldText("Mass: %.2e kg", rigidBody.mass);
				}
				else {
					ImGuiUtils::BoldText("Mass: %.2f kg", rigidBody.mass);
				}

			}


			// --- Reference Frame Debug Info ---
			if (ImGui::CollapsingHeader("Reference Frame Data")) {
				// Parent ID
				if (refFrame.parentID.has_value()) {
					ImGuiUtils::BoldText("Parent: %s (ID: %d)", m_registry->getEntity(refFrame.parentID.value()).name.c_str(), refFrame.parentID.value());
				}
				else {
					ImGuiUtils::BoldText("Parent: None");
				}

				ImGuiUtils::BoldText("\tScaling (simulation)");
				ImGui::Text("\t\tPhysical radius: %.10f m", refFrame.scale);
				ImGui::Text("\t\tIntended ratio to parent: %.10f (relative scale)", refFrame.relativeScale);

				ImGuiUtils::BoldText("\tScaling (render)");
				ImGui::Text("\t\tVisual scale: %.10f units", renderT.visualScale);
				
				if (m_registry->hasComponent<TelemetryComponent::RenderTransform>(refFrame.parentID.value())) {
					const TelemetryComponent::RenderTransform& parentRenderT = m_registry->getComponent<TelemetryComponent::RenderTransform>(refFrame.parentID.value());
					ImGui::Text("\t\tActual ratio to parent: %.10f units", (renderT.visualScale / parentRenderT.visualScale));
				}

				// Local Transform
				ImGuiUtils::BoldText("Local Transform");

				ImGuiUtils::ComponentField(
					{
						{"X", refFrame.localTransform.position.x},
						{"Y", refFrame.localTransform.position.y},
						{"Z", refFrame.localTransform.position.z}
					},
					"%.2f", "\tPosition"
				);
				ImGui::Text("\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.localTransform.position));


				ImGuiUtils::ComponentField(
					{
						{"W", refFrame.localTransform.rotation.w},
						{"X", refFrame.localTransform.rotation.x},
						{"Y", refFrame.localTransform.rotation.y},
						{"Z", refFrame.localTransform.rotation.z}
					},
					"%.2f", "\tRotation"
				);

				// Global Transform
				ImGuiUtils::BoldText("Global Transform");


				ImGuiUtils::ComponentField(
					{
						{"X", refFrame.globalTransform.position.x},
						{"Y", refFrame.globalTransform.position.y},
						{"Z", refFrame.globalTransform.position.z}
					},
					"%.2f", "\tPosition (simulation)"
				);
				ImGui::Text("\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.globalTransform.position));


				ImGuiUtils::ComponentField(
					{
						{"X", renderT.position.x},
						{"Y", renderT.position.y},
						{"Z", renderT.position.z}
					},
					"%.2f", "\tPosition (render)"
				);
				ImGui::Text("\tMagnitude: ||vec|| ~= %.2f units", glm::length(renderT.position));


				ImGuiUtils::ComponentField(
					{
						{"W", refFrame.globalTransform.rotation.w},
						{"X", refFrame.globalTransform.rotation.x},
						{"Y", refFrame.globalTransform.rotation.y},
						{"Z", refFrame.globalTransform.rotation.z}
					},
					"%.2f", "\tRotation"
				);
			}


			if (entityCount < view.size() - 1) {
				ImGui::Dummy(separatorPadding);
			}
			entityCount++;

			ImGui::PopID();
		}

		ImGui::Dummy(separatorPadding);


		static Camera* camera = m_inputManager->getCamera();
		CommonComponent::Transform cameraTransform = camera->getGlobalTransform();
		glm::vec3 scaledCameraPosition = SpaceUtils::ToRenderSpace_Position(cameraTransform.position);

		ImGuiUtils::BoldText("Camera");

		ImGuiUtils::BoldText("Global transform");

		ImGuiUtils::ComponentField(
			{
				{"X", cameraTransform.position.x},
				{"Y", cameraTransform.position.y},
				{"Z", cameraTransform.position.z}
			},
			"%.1e", "\tPosition (simulation)"
		);

		ImGuiUtils::ComponentField(
			{
				{"X", scaledCameraPosition.x},
				{"Y", scaledCameraPosition.y},
				{"Z", scaledCameraPosition.z}
			},
			"%.2f", "\tPosition (render)"
		);

		ImGuiUtils::ComponentField(
			{
				{"W", cameraTransform.rotation.w},
				{"X", cameraTransform.rotation.x},
				{"Y", cameraTransform.rotation.y},
				{"Z", cameraTransform.rotation.z}
			},
			"%.2f", "\tRotation"
		);


		ImGui::End();
	}
}


void UIPanelManager::renderEntityInspectorPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ENTITY_INSPECTOR;
	//if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");
	
		ImGui::End();
	}
}


void UIPanelManager::renderSimulationControlPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_SIMULATION_CONTROL;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags))	{
		performBackgroundChecks(flag);

		static float padding = 0.85f;

		// Numerical integrator selector
		// TODO: Implement integrator switching
		{
			static std::string currentIntegrator = "Fourth Order Runge-Kutta";
			ImGui::Text("Numerical Integrator:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGuiUtils::GetAvailableWidth());

			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			{
				if (ImGui::BeginCombo("##NumericalIntegratorCombo", currentIntegrator.c_str())) {
					// TODO
					ImGui::EndCombo();
				}
			}
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				ImGui::BeginTooltip();
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::TextUnformatted("Numerical integrator switching is not currently supported.");
				ImGui::PopStyleColor();
				ImGui::EndTooltip();
			}
		}


		// Slider to change time scale
		{
			static const char* sliderLabel = "Time Scale:";
			static const char* sliderID = "##TimeScaleSliderFloat";
			static float timeScale = (Time::GetTimeScale() <= 0.0f) ? 1.0f : Time::GetTimeScale();
			static const float MIN_VAL = 1.0f, MAX_VAL = 1000.0f;
			static const float RECOMMENDED_SCALE_VAL_THRESHOLD = 100.0f;
			if (m_simulationIsPaused) {
				// Disable time-scale changing and grey out elements if the simulation is paused
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			ImGui::Text(sliderLabel);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGuiUtils::GetAvailableWidth());
			ImGui::SliderFloat(sliderID, &timeScale, MIN_VAL, MAX_VAL, "%.1fx", ImGuiSliderFlags_AlwaysClamp);
			if (!m_simulationIsPaused) {
				// Edge case: Prevents modifying the time scale when the simulation control panel is open while the simulation is still running
				Time::SetTimeScale(timeScale);
			}

			if (m_simulationIsPaused) {
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			if (timeScale > RECOMMENDED_SCALE_VAL_THRESHOLD) {
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
				ImGui::TextWrapped(ICON_FA_TRIANGLE_EXCLAMATION " Warning: Higher time scales may cause inaccuracies in the simulation.");
				ImGui::PopStyleColor(); // Don't forget to pop the style color!
			}
		}



		// Camera settings
		{
			static Camera* camera = m_inputManager->getCamera();
			static float speedMagnitude = 8.0f;

			static bool initialCameraLoad = true;
			if (initialCameraLoad) {
				camera->movementSpeed = std::powf(10.0f, speedMagnitude);
				initialCameraLoad = false;
			}

			ImGui::Text("Camera Speed Magnitude:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGuiUtils::GetAvailableWidth());
			if (ImGui::DragFloat("##CameraSpeedDragFloat", &speedMagnitude, 1.0f, 1.0f, 12.0f, "1e+%.0f", ImGuiSliderFlags_AlwaysClamp)) {
				camera->movementSpeed = std::powf(10.0f, speedMagnitude);
			}
		}


		ImGui::End();
	}
}


void UIPanelManager::renderRenderSettingsPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_RENDER_SETTINGS;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");

		ImGui::End();
	}
}


void UIPanelManager::renderOrbitalPlannerPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ORBITAL_PLANNER;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");

		ImGui::End();
	}
}


void UIPanelManager::renderDebugConsole() {
	static bool scrolledOnWindowFocus = false;
	static bool notAtBottom = false;
	static size_t prevLogBufSize = Log::LogBuffer.size();

	static bool processedLogTypes = false;
	static std::vector<std::string> logTypes;

	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_CONSOLE;
	//if (!GUI::IsPanelOpen(m_panelMask, flag)) return;


	if (!processedLogTypes) {
		for (size_t i = 0; i < SIZE_OF(Log::MsgTypes); i++) {
			std::string msgType;
			Log::LogColor(Log::MsgTypes[i], msgType, false);
			logTypes.push_back(msgType);
		}
		
		processedLogTypes = true;
	}

	LOG_ASSERT(logTypes.size() > 0, "Unable to render debug console: Log types cannot be loaded!");
	static std::string selectedLogType = logTypes[0]; // logTypes[0] = All log types


	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);


		ImGui::Text("Sort by log type:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f); // Sets a fixed width of 150 pixels
		if (ImGui::BeginCombo("##SortByLogTypeCombo", selectedLogType.c_str())) {
			// NOTE: The "##" prefix tells ImGui to use the label as the internal ID for the widget. This means the label will not be displayed.
			for (const auto& logType : logTypes) {
				bool isSelected = (selectedLogType == logType);
				if (ImGui::Selectable(logType.c_str(), isSelected))
					selectedLogType = logType;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}


		// Optional flag: ImGuiWindowFlags_HorizontalScrollbar
		if (ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), ImGuiChildFlags_Borders, m_windowFlags)) {
			notAtBottom = (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 1.0f);

			for (const auto& log : Log::LogBuffer) {
				if (selectedLogType != logTypes[0] && selectedLogType != log.displayType)
					continue;

				ImGui::PushStyleColor(ImGuiCol_Text, ColorUtils::LogMsgTypeToImVec4(log.type));
				ImGui::TextWrapped("[%s] [%s]: %s", log.displayType.c_str(), log.caller.c_str(), log.message.c_str());
				ImGui::PopStyleColor();

				// Auto-scroll to the bottom only if the scroll position is already at the bottom
				if (!notAtBottom)
					ImGui::SetScrollHereY(1.0f);
			}


			// Auto-scroll to bottom once on switching to this panel
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && !scrolledOnWindowFocus) {
				ImGui::SetScrollHereY(1.0f);
				scrolledOnWindowFocus = true;
			}


			prevLogBufSize = Log::LogBuffer.size();

			// Reset the flag if the window is not focused
			if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow))
				scrolledOnWindowFocus = false;


			ImGui::EndChild();
		}

		ImGui::End();
	}
}


void UIPanelManager::renderDebugApplication() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_APP;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	static ImGuiIO& io = ImGui::GetIO();
	
	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		ImGui::Dummy(ImVec2(2.0f, 2.0f));

		ImGui::Text("Input:");
		ImGui::Text("\tViewport:");
		{
			ImGui::BulletText("\tHovered over: %s", BOOLALPHACAP(g_appContext.Input.isViewportHoveredOver));
			ImGui::BulletText("\tFocused: %s", BOOLALPHACAP(g_appContext.Input.isViewportFocused));
			ImGui::BulletText("\tInput blocker on: %s", BOOLALPHACAP(m_inputBlockerIsOn));
		}

		ImGui::Separator();

		ImGui::Text("\tViewport controls (Input manager)");
		{
			ImGui::BulletText("\tInput allowed: %s", BOOLALPHACAP(m_inputManager->isViewportInputAllowed()));
			ImGui::BulletText("\tFocused: %s", BOOLALPHACAP(m_inputManager->isViewportFocused()));
			ImGui::BulletText("\tUnfocused: %s", BOOLALPHACAP(m_inputManager->isViewportUnfocused()));
		}
	
		ImGui::End();
	}
}
