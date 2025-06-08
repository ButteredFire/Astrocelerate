#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager() {

	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	m_windowFlags = ImGuiWindowFlags_NoCollapse;
	m_popupWindowFlags = ImGuiWindowFlags_NoDocking;

	bindPanelFlags();

	// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_VIEWPORT, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_TELEMETRY, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_ENTITY_INSPECTOR, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_SIMULATION_CONTROL, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_RENDER_SETTINGS, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_ORBITAL_PLANNER, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_CONSOLE, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_INPUT, GUI::TOGGLE_ON);

	initPanelsFromMask(m_panelMask);
	
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
}


void UIPanelManager::onImGuiInit() {
	initViewportTextureDescriptorSet();

	//setFocusedPanel(GUI::PanelFlag::PANEL_VIEWPORT);
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
	renderPanelsMenu();
	
	for (auto&& [flag, callback] : m_panelCallbacks) {
		if (GUI::IsPanelOpen(m_panelMask, flag)) {
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


void UIPanelManager::renderMenuBar() {
	if (ImGui::BeginMenuBar()) {
		ImGui::PushFont(g_fontContext.Roboto.light);
		ImGui::Text("%s (version %s). Copyright (c) 2024-2025 %s.", APP_NAME, APP_VERSION, AUTHOR);
		ImGui::PopFont();

		/*
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				// Handle Open
			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				// Handle Save
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) {
				// Handle Exit
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About")) {
				// Handle About
			}
			ImGui::EndMenu();
		}
		*/

		ImGui::EndMenuBar();
	}
}


void UIPanelManager::performBackgroundChecks(GUI::PanelFlag flag) {
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		m_currentFocusedPanel = flag;
}


void UIPanelManager::bindPanelFlags() {
	using namespace GUI;

	m_panelCallbacks[PanelFlag::PANEL_VIEWPORT]				= &UIPanelManager::renderViewportPanel;
	m_panelCallbacks[PanelFlag::PANEL_TELEMETRY]			= &UIPanelManager::renderTelemetryPanel;
	m_panelCallbacks[PanelFlag::PANEL_ENTITY_INSPECTOR]		= &UIPanelManager::renderEntityInspectorPanel;
	m_panelCallbacks[PanelFlag::PANEL_SIMULATION_CONTROL]	= &UIPanelManager::renderSimulationControlPanel;
	m_panelCallbacks[PanelFlag::PANEL_RENDER_SETTINGS]		= &UIPanelManager::renderRenderSettingsPanel;
	m_panelCallbacks[PanelFlag::PANEL_ORBITAL_PLANNER]		= &UIPanelManager::renderOrbitalPlannerPanel;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_CONSOLE]		= &UIPanelManager::renderDebugConsole;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_INPUT]			= &UIPanelManager::renderDebugInput;
}


void UIPanelManager::initViewportTextureDescriptorSet() {
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


void UIPanelManager::renderPanelsMenu() {
	using namespace GUI;

	if (ImGui::Begin("Panels Menu", nullptr, m_windowFlags)) {
		static bool isDemoWindowOpen = false;
		ImGui::Checkbox("ImGui Demo Window", &isDemoWindowOpen);
		if (isDemoWindowOpen)
			ImGui::ShowDemoWindow();


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

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		// If the viewport panel size has changed, resize the viewport
		//viewportPanelSize.x != m_lastViewportPanelSize.x || viewportPanelSize.y != m_lastViewportPanelSize.y
		if (ImGui::Button("Resize")) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Resize initiated.");
			m_lastViewportPanelSize = viewportPanelSize;

			m_eventDispatcher->publish(Event::ViewportIsResized{
				.currentFrame = m_currentFrame,
				.newWidth = static_cast<uint32_t>(viewportPanelSize.x),
				.newHeight = static_cast<uint32_t>(viewportPanelSize.y)
			});
		}

		ImGui::SameLine();

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

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = g_vkContext.OffscreenResources.imageViews[m_currentFrame];
		imageInfo.sampler = g_vkContext.OffscreenResources.samplers[m_currentFrame];
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet imageDescSetWrite{};
		imageDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageDescSetWrite.dstBinding = 0;
		imageDescSetWrite.descriptorCount = 1;
		imageDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescSetWrite.dstSet = (VkDescriptorSet)m_viewportRenderTextureIDs[m_currentFrame];
		imageDescSetWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(g_vkContext.Device.logicalDevice, 1, &imageDescSetWrite, 0, nullptr);

		ImGui::Image(m_viewportRenderTextureIDs[m_currentFrame], viewportPanelSize);
	
		ImGui::End();
	}
}


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	//if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	
	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		auto view = m_registry->getView<Component::RigidBody, Component::ReferenceFrame>();
		size_t entityCount = 0;

		for (const auto& [entity, rigidBody, refFrame] : view) {
			// As the content is dynamically generated, we need each iteration to have its ImGui ID to prevent conflicts.
			// Since entity IDs are always unique, we can use them as ImGui IDs.
			ImGui::PushID(static_cast<int>(entity));


			ImGui::Text("Entity ID #%d", entity);


			// --- Rigid-body Entity Debug Info ---
			if (ImGui::CollapsingHeader("Rigid-body Entity Debug Info")) {
				float velocityAbs = glm::length(rigidBody.velocity);
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::Text("\tVelocity:");
				ImGui::PopFont();

				ImGui::Text("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.velocity.x, rigidBody.velocity.y, rigidBody.velocity.z);
				ImGui::Text("\t\tAbsolute: |v| ~= %.4f m/s", velocityAbs);

				ImGui::Dummy(ImVec2(0.5f, 0.5f));

				float accelerationAbs = glm::length(rigidBody.acceleration);
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::Text("\tAcceleration:");
				ImGui::PopFont();

				ImGui::Text("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.acceleration.x, rigidBody.acceleration.y, rigidBody.acceleration.z);
				ImGui::Text("\t\tAbsolute: |a| ~= %.4f m/s^2", accelerationAbs);


				// Display mass: scientific notation for large values, fixed for small
				if (std::abs(rigidBody.mass) >= 1e6f) {
					ImGui::Text("\tMass: %.2e kg", rigidBody.mass);
				}
				else {
					ImGui::Text("\tMass: %.2f kg", rigidBody.mass);
				}

			}


			// --- Reference Frame Entity Debug Info ---
			if (ImGui::CollapsingHeader("Reference Frame Entity Debug Info")) {
				// Parent ID
				ImGui::PushFont(g_fontContext.Roboto.bold);
				if (refFrame.parentID.has_value()) {
					ImGui::Text("\tParent Entity ID: %d", refFrame.parentID.value());
				}
				else {
					ImGui::Text("\tParent Entity ID: None");
				}
				ImGui::PopFont();

				ImGui::Text("\t\tScale (simulation): %.10f m (radius)",
					refFrame.scale);
				ImGui::Text("\t\tScale (render): %.10f m (radius)",
					SpaceUtils::GetRenderableScale(SpaceUtils::ToRenderSpace(refFrame.scale)));

				// Local Transform
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::Text("\tLocal Transform:");
				ImGui::PopFont();

				ImGui::Text("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
					refFrame.localTransform.position.x,
					refFrame.localTransform.position.y,
					refFrame.localTransform.position.z);
				ImGui::Text("\t\t\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.localTransform.position));

				ImGui::Text("\t\tRotation: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)",
					refFrame.localTransform.rotation.x,
					refFrame.localTransform.rotation.y,
					refFrame.localTransform.rotation.z,
					refFrame.localTransform.rotation.w);

				// Global Transform
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::Text("\tGlobal Transform (normalized):");
				ImGui::PopFont();

				ImGui::Text("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
					refFrame.globalTransform.position.x,
					refFrame.globalTransform.position.y,
					refFrame.globalTransform.position.z);
				ImGui::Text("\t\t\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.globalTransform.position));

				ImGui::Text("\t\tRotation: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)",
					refFrame.globalTransform.rotation.x,
					refFrame.globalTransform.rotation.y,
					refFrame.globalTransform.rotation.z,
					refFrame.globalTransform.rotation.w);

			}


			if (entityCount < view.size() - 1) {
				ImGui::Separator();
			}
			entityCount++;


			ImGui::PopID();
		}

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


		// Numerical integrator selector
		// TODO: Implement integrator switching
		static std::string currentIntegrator = "Fourth Order Runge-Kutta";
		static float padding = 0.85f;
		ImGui::Text("Numerical integrator");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * padding);

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


		// Slider to change time scale
		static const char* sliderLabel = "Time scale";
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
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * padding);
		ImGui::SliderFloat(sliderID, &timeScale, MIN_VAL, MAX_VAL);
		if (!m_simulationIsPaused) {
			// Edge case: Prevents modifying the time scale when the simulation control panel is open while the simulation is still running
			Time::SetTimeScale(timeScale);
		}

		if (m_simulationIsPaused) {
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		if (timeScale > RECOMMENDED_SCALE_VAL_THRESHOLD)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
			ImGui::TextWrapped(ICON_FA_TRIANGLE_EXCLAMATION " Warning: Higher time scales may cause inaccuracies in the simulation.");
			ImGui::PopStyleColor(); // Don't forget to pop the style color!
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


		ImGui::Text("Sort by log type");
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
		if (ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), true, m_windowFlags)) {
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


void UIPanelManager::renderDebugInput() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_INPUT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		ImGui::Text("Viewport:");
		{
			ImGui::BulletText("Hovered over: %s", BOOLALPHACAP(g_appContext.Input.isViewportHoveredOver));
			ImGui::BulletText("Focused: %s", BOOLALPHACAP(g_appContext.Input.isViewportFocused));
			ImGui::BulletText("Input blocker on: %s", BOOLALPHACAP(m_inputBlockerIsOn));
		}

		ImGui::Separator();

		ImGui::Text("Viewport controls (Input manager)");
		{
			ImGui::BulletText("Input allowed: %s", BOOLALPHACAP(m_inputManager->isViewportInputAllowed()));
			ImGui::BulletText("Focused: %s", BOOLALPHACAP(m_inputManager->isViewportFocused()));
			ImGui::BulletText("Unfocused: %s", BOOLALPHACAP(m_inputManager->isViewportUnfocused()));
		}
	
		ImGui::End();
	}
}
