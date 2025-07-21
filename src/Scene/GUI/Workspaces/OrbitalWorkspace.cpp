#include "OrbitalWorkspace.hpp"


OrbitalWorkspace::OrbitalWorkspace() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	m_windowFlags = ImGuiWindowFlags_NoCollapse;
	m_popupWindowFlags = ImGuiWindowFlags_NoDocking;

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void OrbitalWorkspace::bindEvents() {
	m_eventDispatcher->subscribe<Event::OffscreenResourcesAreRecreated>(
		[this](const Event::OffscreenResourcesAreRecreated &event) {
			for (size_t i = 0; i < m_viewportRenderTextureIDs.size(); i++) {
				ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_viewportRenderTextureIDs[i]);
			}

			initPerFrameTextures();
		}
	);


	m_eventDispatcher->subscribe<Event::InputIsValid>(
		[this](const Event::InputIsValid &event) {
			m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);
		}
	);
}


void OrbitalWorkspace::init() {
	initStaticTextures();
	initPerFrameTextures();

	initPanels();
}


void OrbitalWorkspace::initPanels() {
	// Panel registration
	m_panelViewport =				GUI::RegisterPanel("Viewport");
	m_panelTelemetry =				GUI::RegisterPanel("Telemetry Dashboard");
	m_panelEntityInspector =		GUI::RegisterPanel("Entity Inspector");
	m_panelSimulationControl =		GUI::RegisterPanel("Simulation Settings");
	m_panelRenderSettings =			GUI::RegisterPanel("Render Settings");
	m_panelOrbitalPlanner =			GUI::RegisterPanel("Orbital Planner");
	m_panelDebugConsole =			GUI::RegisterPanel("Console");
	m_panelDebugApp =				GUI::RegisterPanel("Application Info");


	// Panel binding to respective render function
	m_panelCallbacks[m_panelViewport] =				[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderViewportPanel(); };
	m_panelCallbacks[m_panelTelemetry] =			[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderTelemetryPanel(); };
	m_panelCallbacks[m_panelEntityInspector] =		[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderEntityInspectorPanel(); };
	m_panelCallbacks[m_panelSimulationControl] =	[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderSimulationControlPanel(); };
	m_panelCallbacks[m_panelRenderSettings] =		[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderRenderSettingsPanel(); };
	m_panelCallbacks[m_panelOrbitalPlanner] =		[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderOrbitalPlannerPanel(); };
	m_panelCallbacks[m_panelDebugConsole] =			[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderDebugConsole(); };
	m_panelCallbacks[m_panelDebugApp] =				[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderDebugApplication(); };


	// Specify panel visibility
		// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, m_panelViewport, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelTelemetry, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelEntityInspector, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelSimulationControl, GUI::TOGGLE_ON);
		//GUI::TogglePanel(m_panelMask, m_panelRenderSettings, GUI::TOGGLE_ON);
		//GUI::TogglePanel(m_panelMask, m_panelOrbitalPlanner, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelDebugConsole, GUI::TOGGLE_ON);
		//GUI::TogglePanel(m_panelMask, m_panelDebugApp, GUI::TOGGLE_ON);
}


void OrbitalWorkspace::update(uint32_t currentFrame) {
	m_currentFrame = currentFrame;

	// The input blocker serves to capture all input and prevent interaction with other widgets when using the viewport.
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


void OrbitalWorkspace::preRenderUpdate(uint32_t currentFrame) {
	updatePerFrameTextures(currentFrame);
}


void OrbitalWorkspace::loadSimulationConfig(const std::string &configPath) {

}


void OrbitalWorkspace::loadWorkspaceConfig(const std::string &configPath) {

}


void OrbitalWorkspace::saveSimulationConfig(const std::string &configPath) {

}


void OrbitalWorkspace::saveWorkspaceConfig(const std::string &configPath) {

}


void OrbitalWorkspace::initStaticTextures() {
	
}


void OrbitalWorkspace::initPerFrameTextures() {
	// Simulation scene (sampled as a texture for the viewport)
	const size_t OFFSCREEN_RESOURCE_COUNT = g_vkContext.OffscreenResources.images.size();
	m_viewportRenderTextureIDs.resize(OFFSCREEN_RESOURCE_COUNT);

	for (size_t i = 0; i < OFFSCREEN_RESOURCE_COUNT; i++) {
		m_viewportRenderTextureIDs[i] = TextureUtils::GenerateImGuiTextureID(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			g_vkContext.OffscreenResources.imageViews[i],
			g_vkContext.OffscreenResources.samplers[i]
		);
	}
}


void OrbitalWorkspace::updatePerFrameTextures(uint32_t currentFrame) {
	// Simulation scene
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


void OrbitalWorkspace::renderViewportPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelViewport), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();


		// Simulation control group
		ImGui::BeginGroup();
		{
			// Pause/Play button
			static bool initialLoad = true;
			static float lastTimeScale = 1.0f;

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
		ImGui::EndGroup();

		ImGuiUtils::VerticalSeparator();

		// Camera
		static Camera *camera = m_inputManager->getCamera();

		ImGui::BeginGroup();
		{
			static bool inFreeFly = true;
			if (ImGui::Button("Camera")) {
				if (inFreeFly) camera->attachToEntity(3);
				else camera->detachFromEntity();

				inFreeFly = !inFreeFly;
			}
		}
		ImGui::EndGroup();


		ImGui::Separator();


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


void OrbitalWorkspace::renderTelemetryPanel() {
	static const ImVec2 separatorPadding(10.0f, 10.0f);

	if (ImGui::Begin(GUI::GetPanelName(m_panelTelemetry), nullptr, m_windowFlags)) {
		auto view = m_registry->getView<PhysicsComponent::RigidBody, PhysicsComponent::ReferenceFrame, TelemetryComponent::RenderTransform>();
		size_t entityCount = 0;

		for (const auto &[entity, rigidBody, refFrame, renderT] : view) {
			// As the content is dynamically generated, we need each iteration to have its ImGui ID to prevent conflicts.
			// Since entity IDs are always unique, we can use them as ImGui IDs.
			ImGui::PushID(static_cast<int>(entity));

			ImGui::SeparatorText(m_registry->getEntity(entity).name.c_str());

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
				ImGui::Text("\tAbsolute: |v| ≈ %.4f m/s", velocityAbs);

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
				ImGui::Text("\tAbsolute: |a| ≈ %.4f m/s²", accelerationAbs);


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

				ImGuiUtils::BoldText("\tScaling (render)");
				ImGui::Text("\t\tVisual scale: %.10f units", renderT.visualScale);

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
				ImGui::Text("\tMagnitude: ||vec|| ≈ %.2f m", glm::length(refFrame.localTransform.position));


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
				ImGui::Text("\tMagnitude: ||vec|| ≈ %.2f m", glm::length(refFrame.globalTransform.position));


				ImGuiUtils::ComponentField(
					{
						{"X", renderT.position.x},
						{"Y", renderT.position.y},
						{"Z", renderT.position.z}
					},
					"%.2f", "\tPosition (render)"
				);
				ImGui::Text("\tMagnitude: ||vec|| ≈ %.2f units", glm::length(renderT.position));


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


		static Camera *camera = m_inputManager->getCamera();
		CommonComponent::Transform cameraTransform = camera->getGlobalTransform();
		glm::vec3 scaledCameraPosition = SpaceUtils::ToRenderSpace_Position(cameraTransform.position);

		ImGui::SeparatorText("Camera");

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


void OrbitalWorkspace::renderEntityInspectorPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelEntityInspector), nullptr, m_windowFlags)) {
		auto view = m_registry->getView<PhysicsComponent::ShapeParameters>();
		if (view.size() == 0)
			ImGui::SeparatorText("Shape Parameters: None");

		else {
			ImGui::SeparatorText("Shape Parameters");

			for (auto &&[entity, shapeParams] : view) {
				ImGui::PushID(static_cast<int>(entity));

				if (ImGui::CollapsingHeader(m_registry->getEntity(entity).name.c_str())) {
					ImGui::TextWrapped("Eccentricity: e ≈ %.5f", shapeParams.eccentricity);
					ImGui::TextWrapped("Mean equatorial radius: r ≈ %.5f m", shapeParams.equatRadius);
					ImGui::TextWrapped("Gravitational parameter: μ ≈ %.5g m³/s⁻²", shapeParams.gravParam);
					ImGui::TextWrapped("Rotational velocity: ω ≈ %.5g rad/s", shapeParams.rotVelocity);
				}

				ImGui::PopID();
			}
		}

		ImGui::End();
	}
}


void OrbitalWorkspace::renderSimulationControlPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelSimulationControl), nullptr, m_windowFlags)) {
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
			static const char *sliderLabel = "Time Scale:";
			static const char *sliderID = "##TimeScaleSliderFloat";
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
			static Camera *camera = m_inputManager->getCamera();
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


void OrbitalWorkspace::renderRenderSettingsPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelRenderSettings), nullptr, m_windowFlags)) {
		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");

		ImGui::End();
	}
}


void OrbitalWorkspace::renderOrbitalPlannerPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelOrbitalPlanner), nullptr, m_windowFlags)) {
		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");

		ImGui::End();
	}
}


void OrbitalWorkspace::renderDebugConsole() {
	static bool scrolledOnWindowFocus = false;
	static bool notAtBottom = false;
	static size_t prevLogBufSize = Log::LogBuffer.size();

	static bool processedLogTypes = false;
	static std::vector<std::string> logTypes;


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


	if (ImGui::Begin(GUI::GetPanelName(m_panelDebugConsole), nullptr, m_windowFlags)) {
		ImGui::Text("Sort by log type:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f); // Sets a fixed width of 150 pixels
		if (ImGui::BeginCombo("##SortByLogTypeCombo", selectedLogType.c_str())) {
			// NOTE: The "##" prefix tells ImGui to use the label as the internal ID for the widget. This means the label will not be displayed.
			for (const auto &logType : logTypes) {
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

			ImGui::PushFont(g_fontContext.NotoSans.regularMono);
			{
				for (const auto &log : Log::LogBuffer) {
					if (selectedLogType != logTypes[0] && selectedLogType != log.displayType)
						continue;

					ImGui::PushStyleColor(ImGuiCol_Text, ColorUtils::LogMsgTypeToImVec4(log.type));
					{
						ImGui::TextWrapped("[%s] [%s]: %s", log.displayType.c_str(), log.caller.c_str(), log.message.c_str());
					}
					ImGui::PopStyleColor();

					// Auto-scroll to the bottom only if the scroll position is already at the bottom
					if (!notAtBottom)
						ImGui::SetScrollHereY(1.0f);
				}
			}
			ImGui::PopFont();


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


void OrbitalWorkspace::renderDebugApplication() {
	static ImGuiIO &io = ImGui::GetIO();

	if (ImGui::Begin(GUI::GetPanelName(m_panelDebugApp), nullptr, m_windowFlags)) {
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
