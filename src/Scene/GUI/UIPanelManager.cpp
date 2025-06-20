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
	//GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_APP, GUI::TOGGLE_ON);

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

	m_eventDispatcher->subscribe<Event::OffscreenResourcesAreRecreated>(
		[this](const Event::OffscreenResourcesAreRecreated& event) {
			for (size_t i = 0; i < m_viewportRenderTextureIDs.size(); i++) {
				ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet) m_viewportRenderTextureIDs[i]);
			}

			initViewportTextures();
		}
	);
}


void UIPanelManager::onImGuiInit() {
	initViewportTextures();
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


void UIPanelManager::bindPanelFlags() {
	using namespace GUI;

	m_panelCallbacks[PanelFlag::PANEL_VIEWPORT]				= &UIPanelManager::renderViewportPanel;
	m_panelCallbacks[PanelFlag::PANEL_TELEMETRY]			= &UIPanelManager::renderTelemetryPanel;
	m_panelCallbacks[PanelFlag::PANEL_ENTITY_INSPECTOR]		= &UIPanelManager::renderEntityInspectorPanel;
	m_panelCallbacks[PanelFlag::PANEL_SIMULATION_CONTROL]	= &UIPanelManager::renderSimulationControlPanel;
	m_panelCallbacks[PanelFlag::PANEL_RENDER_SETTINGS]		= &UIPanelManager::renderRenderSettingsPanel;
	m_panelCallbacks[PanelFlag::PANEL_ORBITAL_PLANNER]		= &UIPanelManager::renderOrbitalPlannerPanel;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_CONSOLE]		= &UIPanelManager::renderDebugConsole;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_APP]			= &UIPanelManager::renderDebugApplication;
}


void UIPanelManager::initViewportTextures() {
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
	/* Scales the simulation render from the original render dimensions down to viewport dimensions while preserving the original aspect ratio. */
	static std::function<ImVec2(ImVec2&)> resizeSimulationRender = [](ImVec2& viewportSize) {
		ImVec2 textureSize{};

		float renderAspect = static_cast<float>(g_vkContext.SwapChain.extent.width) / g_vkContext.SwapChain.extent.height;
		float panelAspect = viewportSize.x / viewportSize.y;

		if (panelAspect > renderAspect) {
			// Panel is wider than the render target
			textureSize.x = viewportSize.y * renderAspect;
			textureSize.y = viewportSize.y;
		}
		else {
			// Panel is taller than the render target
			textureSize.x = viewportSize.x;
			textureSize.y = viewportSize.x / renderAspect;
		}

		return textureSize;
	};


	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_VIEWPORT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) {
		Log::Print(Log::T_ERROR, __FUNCTION__, "The viewport must not be closed!");
		GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_ON);
	}


	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
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
		ImVec2 textureSize = resizeSimulationRender(viewportPanelSize);

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


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	//if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	
	if (ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags)) {
		performBackgroundChecks(flag);

		auto view = m_registry->getView<PhysicsComponent::RigidBody, PhysicsComponent::ReferenceFrame, TelemetryComponent::RenderTransform>();
		size_t entityCount = 0;

		for (const auto& [entity, rigidBody, refFrame, renderT] : view) {
			// As the content is dynamically generated, we need each iteration to have its ImGui ID to prevent conflicts.
			// Since entity IDs are always unique, we can use them as ImGui IDs.
			ImGui::PushID(static_cast<int>(entity));


			ImGui::TextWrapped("%s (ID: %d)", m_registry->getEntity(entity).name.c_str(), entity);


			// --- Rigid-body Debug Info ---
			if (ImGui::CollapsingHeader("Rigid-body Data")) {
				float velocityAbs = glm::length(rigidBody.velocity);
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::TextWrapped("\tVelocity:");
				ImGui::PopFont();

				ImGui::TextWrapped("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.velocity.x, rigidBody.velocity.y, rigidBody.velocity.z);
				ImGui::TextWrapped("\t\tAbsolute: |v| ~= %.4f m/s", velocityAbs);

				ImGui::Dummy(ImVec2(0.5f, 0.5f));

				float accelerationAbs = glm::length(rigidBody.acceleration);
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::TextWrapped("\tAcceleration:");
				ImGui::PopFont();

				ImGui::TextWrapped("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.acceleration.x, rigidBody.acceleration.y, rigidBody.acceleration.z);
				ImGui::TextWrapped("\t\tAbsolute: |a| ~= %.4f m/s^2", accelerationAbs);


				// Display mass: scientific notation for large values, fixed for small
				if (std::abs(rigidBody.mass) >= 1e6f) {
					ImGui::TextWrapped("\tMass: %.2e kg", rigidBody.mass);
				}
				else {
					ImGui::TextWrapped("\tMass: %.2f kg", rigidBody.mass);
				}

			}


			// --- Reference Frame Debug Info ---
			if (ImGui::CollapsingHeader("Reference Frame Data")) {
				// Parent ID
				ImGui::PushFont(g_fontContext.Roboto.bold);
				if (refFrame.parentID.has_value()) {
					ImGui::TextWrapped("\tParent: %s (ID: %d)", m_registry->getEntity(refFrame.parentID.value()).name.c_str(), refFrame.parentID.value());
				}
				else {
					ImGui::TextWrapped("\tParent: None");
				}
				ImGui::PopFont();

				ImGui::TextWrapped("\t\tScaling (simulation):");
				ImGui::TextWrapped("\t\t\tPhysical radius: %.10f m", refFrame.scale);
				ImGui::TextWrapped("\t\t\tIntended ratio to parent: %.10f (relative scale)", refFrame.relativeScale);

				ImGui::TextWrapped("\t\tScaling (render):");
				ImGui::TextWrapped("\t\t\tVisual scale: %.10f units", renderT.visualScale);
				
				if (m_registry->hasComponent<TelemetryComponent::RenderTransform>(refFrame.parentID.value())) {
					const TelemetryComponent::RenderTransform& parentRenderT = m_registry->getComponent<TelemetryComponent::RenderTransform>(refFrame.parentID.value());
					ImGui::TextWrapped("\t\t\tActual ratio to parent: %.10f units", (renderT.visualScale / parentRenderT.visualScale));
				}

				// Local Transform
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::TextWrapped("\tLocal Transform:");
				ImGui::PopFont();

				ImGui::TextWrapped("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
					refFrame.localTransform.position.x,
					refFrame.localTransform.position.y,
					refFrame.localTransform.position.z);
				ImGui::TextWrapped("\t\t\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.localTransform.position));

				ImGui::TextWrapped("\t\tRotation: (w: %.2f, x: %.2f, y: %.2f, z: %.2f)",
					refFrame.localTransform.rotation.w,
					refFrame.localTransform.rotation.x,
					refFrame.localTransform.rotation.y,
					refFrame.localTransform.rotation.z);

				// Global Transform
				ImGui::PushFont(g_fontContext.Roboto.bold);
				ImGui::TextWrapped("\tGlobal Transform:");
				ImGui::PopFont();

				ImGui::TextWrapped("\t\tPosition (simulation): (x: %.2f, y: %.2f, z: %.2f)",
					refFrame.globalTransform.position.x,
					refFrame.globalTransform.position.y,
					refFrame.globalTransform.position.z);
				ImGui::TextWrapped("\t\t\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.globalTransform.position));

				ImGui::TextWrapped("\t\tPosition (render): (x: %.2f, y: %.2f, z: %.2f)",
					renderT.position.x, renderT.position.y, renderT.position.z);
				ImGui::TextWrapped("\t\t\tMagnitude: ||vec|| ~= %.2f units", glm::length(renderT.position));

				ImGui::TextWrapped("\t\tRotation: (w: %.2f, x: %.2f, y: %.2f, z: %.2f)",
					refFrame.globalTransform.rotation.w,
					refFrame.globalTransform.rotation.x,
					refFrame.globalTransform.rotation.y,
					refFrame.globalTransform.rotation.z);

			}


			if (entityCount < view.size() - 1) {
				ImGui::Separator();
			}
			entityCount++;

			ImGui::PopID();
		}

		ImGui::Separator();

		ImGui::PushFont(g_fontContext.Roboto.bold);
		ImGui::TextWrapped("Camera");
		ImGui::PopFont();

		static Camera* camera = m_inputManager->getCamera();
		CommonComponent::Transform cameraTransform = camera->getGlobalTransform();
		glm::vec3 scaledCameraPosition = SpaceUtils::ToRenderSpace_Position(cameraTransform.position);

		ImGui::TextWrapped("\tSpeed: %.0e", camera->movementSpeed);

		ImGui::TextWrapped("\tGlobal transform:");

		ImGui::TextWrapped("\t\tPosition (simulation): (x: %.1e, y: %.1e, z: %.1e)",
			cameraTransform.position.x, cameraTransform.position.y, cameraTransform.position.z);
		ImGui::TextWrapped("\t\tPosition (render): (x: %.2f, y: %.2f, z: %.2f)",
			scaledCameraPosition.x, scaledCameraPosition.y, scaledCameraPosition.z);

		ImGui::TextWrapped("\t\tRotation: (w: %.2f, x: %.2f, y: %.2f, z: %.2f)",
			cameraTransform.rotation.w, cameraTransform.rotation.x, cameraTransform.rotation.y, cameraTransform.rotation.z);

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
			ImGui::Text("Numerical Integrator");
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
		}


		// Slider to change time scale
		{
			static const char* sliderLabel = "Time Scale";
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

			ImGui::Text("Camera Speed Magnitude");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * padding);
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
