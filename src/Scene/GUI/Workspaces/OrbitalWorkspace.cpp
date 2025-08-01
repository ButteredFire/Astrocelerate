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


	m_eventDispatcher->subscribe<Event::UpdateSessionStatus>(
		[this](const Event::UpdateSessionStatus &event) {
			using namespace Event;

			switch (event.sessionStatus) {
			case UpdateSessionStatus::Status::PREPARE_FOR_RESET:
				m_sceneSampleReady = false;
				break;

			case UpdateSessionStatus::Status::INITIALIZED:
				m_sceneSampleReady = true;
				initPerFrameTextures();
				break;
			}
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

	if (m_sceneSampleReady)
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
	m_panelSceneResourceTree =		GUI::RegisterPanel("Scene Resources");
	m_panelSceneResourceDetails =	GUI::RegisterPanel("Configuration", true);	// This will be dynamically updated
	m_panelCodeEditor =				GUI::RegisterPanel("###CodeEditor", true);


	// Panel binding to respective render function
	m_panelCallbacks[m_panelViewport] =				[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderViewportPanel(); };
	m_panelCallbacks[m_panelTelemetry] =			[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderTelemetryPanel(); };
	//m_panelCallbacks[m_panelEntityInspector] =		[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderEntityInspectorPanel(); };
	m_panelCallbacks[m_panelSimulationControl] =	[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderSimulationControlPanel(); };
	//m_panelCallbacks[m_panelRenderSettings] =		[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderRenderSettingsPanel(); };
	//m_panelCallbacks[m_panelOrbitalPlanner] =		[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderOrbitalPlannerPanel(); };
	m_panelCallbacks[m_panelDebugConsole] =			[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderDebugConsole(); };
	m_panelCallbacks[m_panelDebugApp] =				[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderDebugApplication(); };
	m_panelCallbacks[m_panelSceneResourceTree] =	[this](IWorkspace *ws) { static_cast<OrbitalWorkspace *>(ws)->renderSceneResourceTree(); };


	// Specify panel visibility
		// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, m_panelViewport, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelTelemetry, GUI::TOGGLE_ON);
	//GUI::TogglePanel(m_panelMask, m_panelEntityInspector, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelSimulationControl, GUI::TOGGLE_ON);
		//GUI::TogglePanel(m_panelMask, m_panelRenderSettings, GUI::TOGGLE_ON);
		//GUI::TogglePanel(m_panelMask, m_panelOrbitalPlanner, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelDebugConsole, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, m_panelSceneResourceTree, GUI::TOGGLE_ON);
		//GUI::TogglePanel(m_panelMask, m_panelDebugApp, GUI::TOGGLE_ON);
}


void OrbitalWorkspace::update(uint32_t currentFrame) {
	m_currentFrame = currentFrame;

	// Conditionally render instanced panels
	if (GUI::IsPanelOpen(m_panelMask, m_panelSceneResourceDetails))
		renderSceneResourceDetails();
	if (GUI::IsPanelOpen(m_panelMask, m_panelCodeEditor))
		renderCodeEditor();


	// The input blocker serves to capture all input and prevent interaction with other widgets when using the viewport.
	if (m_inputManager->isViewportInputAllowed() && m_sceneSampleReady) {
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
	// Close certain instanced panels
	GUI::TogglePanel(m_panelMask, m_panelSceneResourceDetails, GUI::TOGGLE_OFF);
	GUI::TogglePanel(m_panelMask, m_panelCodeEditor, GUI::TOGGLE_OFF);

	// Load script
	m_simulationConfigChanged = true;
	m_simulationConfigPath = configPath;
	m_simulationScriptData = FilePathUtils::ReadFile(configPath);

	m_eventDispatcher->dispatch(Event::RequestInitSession{
		.simulationFilePath = configPath
	});
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
	if (m_sceneSampleReady) {
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
}


void OrbitalWorkspace::renderViewportPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelViewport), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		ImGuiUtils::PushStyleClearButton();
		{
			ImGui::AlignTextToFramePadding();


			ImGui::TextDisabled("Controls\t");
			if (ImGui::BeginItemTooltip())
			{
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::Text("[Left-click]    Enter viewport");
				ImGui::Text("[ESC]           Exit out of viewport");
				ImGui::Text("[W,A,S,D]       Control camera");
				ImGui::Text("[Scroll]        Control camera zoom");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
			ImGui::SameLine();

			
			// Simulation control group
			ImGui::BeginGroup();
			{
				// Pause/Play button
				static bool initialLoad = true;
				static float lastTimeScale = 1.0f;
				std::string sceneName;
				if (!m_simulationConfigPath.empty())
					sceneName += FilePathUtils::GetFileName(m_simulationConfigPath, false);

				if (m_simulationIsPaused) {
					if (initialLoad) {
						Time::SetTimeScale(0.0f);
						initialLoad = false;
					}

					if (ImGui::Button(ImGuiUtils::IconString(ICON_FA_PLAY, sceneName).c_str())) {
						Time::SetTimeScale(lastTimeScale);
						m_simulationIsPaused = false;
					}
					ImGuiUtils::CursorOnHover();
				}
				else {
					if (ImGui::Button(ImGuiUtils::IconString(ICON_FA_PAUSE, sceneName).c_str())) {
						lastTimeScale = Time::GetTimeScale();
						Time::SetTimeScale(0.0f);
						m_simulationIsPaused = true;
					}
					ImGuiUtils::CursorOnHover();
				}
			}
			ImGui::EndGroup();

			ImGuiUtils::VerticalSeparator();

			// Camera
			static Camera *camera = m_inputManager->getCamera();

			ImGui::BeginGroup();
			{
				//static bool inFreeFly = true;
				//if (ImGui::Button("Camera") && 0) {	// TODO: Implement camera switching
				//	if (inFreeFly) camera->attachToEntity(3);
				//	else camera->detachFromEntity();
				//
				//	inFreeFly = !inFreeFly;
				//}

				ImGui::Text("Camera:");

				ImGui::SameLine();
				ImGui::SetNextItemWidth(150.0f);

				ImGuiUtils::PushStyleDisabled();
				{
					if (ImGui::BeginCombo("##CameraSwitchCombo", "Free-fly", ImGuiComboFlags_NoArrowButton)) {
						// TODO: Implement camera switching
						ImGui::EndCombo();
					}
					ImGuiUtils::CursorOnHover();
				}
				ImGuiUtils::PopStyleDisabled();

				ImGuiUtils::CursorOnHover();
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
					ImGuiUtils::TextTooltip(ImGuiHoveredFlags_AllowWhenDisabled, "Camera-switching is currently unstable, so it has been disabled.");
				ImGui::PopStyleColor();
			}
			ImGui::EndGroup();

		}
		ImGuiUtils::PopStyleClearButton();


		ImGui::Separator();


		if (m_sceneSampleReady) {
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
			ImGuiUtils::CursorOnHover();
		}

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
			ImGuiUtils::CursorOnHover();


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


				glm::dvec3 localRotationEuler = SpaceUtils::QuatToEulerAngles(refFrame.localTransform.rotation);
				ImGuiUtils::ComponentField(
					{
						{"X", localRotationEuler.x},
						{"Y", localRotationEuler.y},
						{"Z", localRotationEuler.z}
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


				glm::dvec3 globalRotationEuler = SpaceUtils::QuatToEulerAngles(refFrame.globalTransform.rotation);
				ImGuiUtils::ComponentField(
					{
						{"X", globalRotationEuler.x},
						{"Y", globalRotationEuler.y},
						{"Z", globalRotationEuler.z}
					},
					"%.2f", "\tRotation"
				);
			}
			ImGuiUtils::CursorOnHover();


			if (entityCount < view.size() - 1) {
				ImGui::Dummy(separatorPadding);
			}
			entityCount++;

			ImGui::PopID();
		}

		ImGui::Dummy(separatorPadding);


		static Camera *camera = m_inputManager->getCamera();
		CoreComponent::Transform cameraTransform = camera->getGlobalTransform();
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

		glm::dvec3 camRotationEuler = SpaceUtils::QuatToEulerAngles(cameraTransform.rotation);
		ImGuiUtils::ComponentField(
			{
				{"X", camRotationEuler.x},
				{"Y", camRotationEuler.y},
				{"Z", camRotationEuler.z}
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
					ImGui::TextWrapped("Flattening: e ≈ %.5f", shapeParams.flattening);
					ImGui::TextWrapped("Mean equatorial radius: r ≈ %.5f m", shapeParams.equatRadius);
					ImGui::TextWrapped("Gravitational parameter: μ ≈ %.5g m³/s⁻²", shapeParams.gravParam);
					ImGui::TextWrapped("Rotational velocity (scalar): ω ≈ %.5g rad/s", glm::length(shapeParams.rotVelocity));
					ImGui::TextWrapped("J2 oblateness coefficient: ω ≈ %.5g", shapeParams.j2);
				}
				ImGuiUtils::CursorOnHover();

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

			ImGuiUtils::PushStyleDisabled();
			{
				if (ImGui::BeginCombo("##NumericalIntegratorCombo", currentIntegrator.c_str(), ImGuiComboFlags_NoArrowButton)) {
					// TODO
					ImGui::EndCombo();
				}
				ImGuiUtils::CursorOnHover();
			}
			ImGuiUtils::PopStyleDisabled();

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGuiUtils::TextTooltip(ImGuiHoveredFlags_AllowWhenDisabled, "Numerical integrator switching is not currently supported.");
			ImGui::PopStyleColor();
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
			ImGuiUtils::CursorOnHover();
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
				ImGui::TextWrapped(ImGuiUtils::IconString(ICON_FA_TRIANGLE_EXCLAMATION, "Warning: Higher time scales may cause inaccuracies in the simulation.").c_str());
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
			ImGuiUtils::CursorOnHover();
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
		// Filtering
			// Filter by log type
		ImGui::BeginGroup();
		{
			ImGui::AlignTextToFramePadding();

			ImGui::Text("Filter by log type:");

			ImGui::SameLine();
			ImGui::SetNextItemWidth(150.0f); // Sets a fixed width of 150 pixels

			if (ImGui::BeginCombo("##FilterByLogTypeCombo", selectedLogType.c_str(), ImGuiComboFlags_NoArrowButton)) {
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
			ImGuiUtils::CursorOnHover();
		}
		ImGui::EndGroup();


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
						ImGui::TextWrapped(log.message.c_str());
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


void OrbitalWorkspace::renderSceneResourceTree() {
	static ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_DrawLinesToNodes;

	static std::function<void(m_ResourceType, EntityID, const std::string&)> renderTreeNode = [this](m_ResourceType resourceType, EntityID entityID, const std::string &nodeName) {
		using enum m_ResourceType;

		if (ImGui::Button(C_STR(nodeName))) {
			if (resourceType == SCRIPTS)	// Redirect to code editor
				GUI::TogglePanel(m_panelMask, m_panelCodeEditor, GUI::TOGGLE_ON);

			else {
				m_currentSceneResourceType = resourceType;
				m_currentSceneResourceEntityID = entityID;
				GUI::TogglePanel(m_panelMask, m_panelSceneResourceDetails, GUI::TOGGLE_ON);
			}
		}
		ImGuiUtils::CursorOnHover();
	};


	if (ImGui::Begin(GUI::GetPanelName(m_panelSceneResourceTree), nullptr, m_windowFlags)) {

		ImGuiUtils::PushStyleClearButton();
		{
			// Spacecraft
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Spacecraft & Satellites").c_str(), treeFlags)) {
				auto view = m_registry->getView<SpacecraftComponent::Spacecraft>();

				ImGui::Indent();
				{
					for (const auto &[entity, sc] : view) {
						renderTreeNode(m_ResourceType::SPACECRAFT, entity, ImGuiUtils::IconString(ICON_FA_SATELLITE, m_registry->getEntity(entity).name));
					}
				}
				ImGui::Unindent();

				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();


			// Celestial bodies
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Celestial bodies").c_str(), treeFlags)) {
				auto view = m_registry->getView<PhysicsComponent::ShapeParameters>();

				ImGui::Indent();
				{
					for (const auto &[entity, bodies] : view) {
						renderTreeNode(m_ResourceType::CELESTIAL_BODIES, entity, ImGuiUtils::IconString(ICON_FA_CIRCLE, m_registry->getEntity(entity).name));
					}
				}
				ImGui::Unindent();
				
				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();


			// Propagators
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Propagators").c_str(), treeFlags)) {
				ImGui::Indent();
				{

				}
				ImGui::Unindent();

				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();


			// Solvers
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Solvers").c_str(), treeFlags)) {
				ImGui::Indent();
				{

				}
				ImGui::Unindent();

				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();


			// Scripts
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Scripts").c_str(), treeFlags)) {
				ImGui::Indent();
				{
					if (!m_simulationConfigPath.empty())
						renderTreeNode(m_ResourceType::SCRIPTS, INVALID_ENTITY, ImGuiUtils::IconString(ICON_FA_FILE_CODE, FilePathUtils::GetFileName(m_simulationConfigPath)));
				}
				ImGui::Unindent();
		
				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();


			// Coordinate systems
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Coordinate systems").c_str(), treeFlags)) {
				ImGui::Indent();
				{
					// TODO: Make coordinate systems entities
					renderTreeNode(m_ResourceType::COORDINATE_SYSTEMS, INVALID_ENTITY, ImGuiUtils::IconString(ICON_FA_VECTOR_SQUARE, "Earth-Fixed Inertial"));
				}
				ImGui::Unindent();
	
				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();
		}
		ImGuiUtils::PopStyleClearButton();
		

		ImGui::End();
	}
}


void OrbitalWorkspace::renderSceneResourceDetails() {
	using enum m_ResourceType;

	EntityID currentEntity = m_currentSceneResourceEntityID;

	std::stringstream ss;	// Panel name

	if (currentEntity != INVALID_ENTITY)
		ss << m_registry->getEntity(m_currentSceneResourceEntityID).name;
	else {
		// Currently, the only exception is the scripts. In such a case, we should open a text editor.
	}
	ss << " " << GUI::GetPanelName(m_panelSceneResourceDetails) << "###SceneResourceDetailsPanel"; 	// We must use a persistent ID to avoid ImGui treating instances with different titles as separate panels.

	static bool panelOpen = GUI::IsPanelOpen(m_panelMask, m_panelSceneResourceDetails);
	if (ImGui::Begin(C_STR(ss.str()), &panelOpen, ImGuiWindowFlags_NoDecoration)) {	// ImGuiWindowFlags_NoCollapse
		
		// ----- SPACECRAFT -----
		if (m_currentSceneResourceType == SPACECRAFT) {
			SpacecraftComponent::Spacecraft sc = m_registry->getComponent<SpacecraftComponent::Spacecraft>(currentEntity);

			ImGui::SeparatorText("Spacecraft Configuration");
			ImGui::Indent();
			{
				ImGui::SeparatorText("Perturbation");

				ImGui::Indent();
				{
					ImGui::Text("Drag coefficient: cₓ ≈ %.5g", sc.dragCoefficient);
					ImGui::Text("Reference area (for drag/SRP): A ≈ %.5g m²", sc.referenceArea);
					ImGui::Text("Reflectivity coefficient: Γ ≈ %.5g", sc.reflectivityCoefficient);
				}
				ImGui::Unindent();
			}
			ImGui::Unindent();


			if (m_registry->hasComponent<SpacecraftComponent::Thruster>(currentEntity)) {
				SpacecraftComponent::Thruster thruster = m_registry->getComponent<SpacecraftComponent::Thruster>(currentEntity);

				ImGui::SeparatorText("Thruster Configuration");
				ImGui::Indent();
				{
					ImGui::Text("Thrust magnitude: T ≈ %.5g N", thruster.thrustMagnitude);
					ImGui::Text("Specific impulse: Iₛₚ ≈ %.5g s", thruster.specificImpulse);
					ImGui::Text("Current fuel mass: %.0f kg", thruster.currentFuelMass);
					ImGui::Text("Max. fuel mass: %.0f kg", thruster.maxFuelMass);
				}
				ImGui::Unindent();
			}
		}



		// ----- CELESTIAL BODIES -----
		else if (m_currentSceneResourceType == CELESTIAL_BODIES) {
			PhysicsComponent::ShapeParameters shape = m_registry->getComponent<PhysicsComponent::ShapeParameters>(currentEntity);

			ImGui::SeparatorText("Shape Configuration");
			ImGui::Indent();
			{
				ImGui::Text("Flattening: e ≈ %.5f", shape.flattening);
				ImGui::Text("Mean equatorial radius: r ≈ %.5f m", shape.equatRadius);
				ImGui::Text("Gravitational parameter: μ ≈ %.5g m³/s⁻²", shape.gravParam);
				ImGui::Text("Rotational velocity (scalar): ω ≈ %.5g rad/s", glm::length(shape.rotVelocity));
				ImGui::Text("J2 oblateness coefficient: ω ≈ %.5g", shape.j2);
			}
			ImGui::Unindent();
		}



		// ----- PROPAGATORS -----
		else if (m_currentSceneResourceType == PROPAGATORS) {
			ImGui::Text("Current information on this propagator is not currently available.");
		}



		// ----- SOLVERS -----
		else if (m_currentSceneResourceType == SOLVERS) {
			ImGui::Text("Current information on this solver is not currently available.");
		}



		// ----- SCRIPTS -----
		//else if (m_currentSceneResourceType == SCRIPTS) {
		//	ImGui::Text("Current information on this script is not currently available.");
		//}



		// ----- COORDINATE SYSTEMS -----
		else if (m_currentSceneResourceType == COORDINATE_SYSTEMS) {
			ImGui::Text("Current information on this coordinate system is not currently available.");
		}

		ImGui::End();
	}
}


void OrbitalWorkspace::renderCodeEditor() {
	// Editor settings
	if (m_simulationConfigChanged) {
		m_simulationConfigChanged = false;

		// Populate with data
		if (!m_simulationScriptData.empty()) {
			std::string scriptData(m_simulationScriptData.begin(), m_simulationScriptData.end());
			m_codeEditor.SetText(scriptData);
		}
		else {
			m_codeEditor.SetText("# Welcome to Astrocelerate's code editor!");
		}
	}

		// Customization
	using enum ImGuiTheme::Appearance;
	switch (g_appContext.GUI.currentAppearance) {
	case IMGUI_APPEARANCE_DARK_MODE:
		m_codeEditor.SetPalette(CodeEditor::GetDarkPalette());
		break;

	case IMGUI_APPEARANCE_LIGHT_MODE:
		m_codeEditor.SetPalette(CodeEditor::GetLightPalette());
		break;
	}


	std::stringstream ss;
	if (!m_simulationScriptData.empty())
		ss << FilePathUtils::GetFileName(m_simulationConfigPath);
	else
		ss << "New Script";
	ss << GUI::GetPanelName(m_panelCodeEditor);


	static bool panelOpen = GUI::IsPanelOpen(m_panelMask, m_panelCodeEditor);
	if (ImGui::Begin(C_STR(ss.str()), &panelOpen, ImGuiWindowFlags_NoCollapse)) {

		ImGui::AlignTextToFramePadding();

		// Editor controls
		// TODO: Implement functionality
		ImGuiUtils::PushStyleClearButton();
		{
			// Editing actions
			ImGui::BeginGroup();
			{
				if (ImGui::Button(ICON_FA_EXCLAMATION)) {}
				ImGuiUtils::TextTooltip(0, "Script editing and reloading is currently disabled due to instability.");
				ImGui::SameLine();


				if (ImGui::Button(ICON_FA_ARROW_ROTATE_LEFT)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Undo");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_ARROW_ROTATE_RIGHT)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Redo");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_SCISSORS)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Cut");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_COPY)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Copy");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_CLIPBOARD)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Paste");
			}
			ImGui::EndGroup();


			ImGuiUtils::VerticalSeparator();


			// Navigation & Search
			ImGui::BeginGroup();
			{
				if (ImGui::Button(ICON_FA_MAGNIFYING_GLASS)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Find & Replace");

			}
			ImGui::EndGroup();


			ImGuiUtils::VerticalSeparator();


			// Formatting
			ImGui::BeginGroup();
			{
				if (ImGui::Button(ICON_FA_INDENT)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Indent");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_OUTDENT)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Outdent");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_HASHTAG)) {

				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Comment");
			}
			ImGui::EndGroup();
		}
		ImGuiUtils::PopStyleClearButton();


		static const float bottomStatsPadding = 70.0f;
		ImGui::PushFont(g_fontContext.NotoSans.regularMono);
		{
			m_codeEditor.Render("###CodeEditorSpace", ImVec2(0, ImGui::GetContentRegionAvail().y - bottomStatsPadding));
		}
		ImGui::PopFont();


		ImGui::BeginGroup();
		{
			auto cpos = m_codeEditor.GetCursorPosition();
			ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_RIGHT,
				"Ln: %d  Col: %d  |  %d lines  | %s | %s",
				cpos.mLine + 1, cpos.mColumn + 1,
				m_codeEditor.GetTotalLines(),
				m_codeEditor.IsOverwrite() ? "Ovr" : "Ins",
				FilePathUtils::GetFileExtension(m_simulationConfigPath).c_str()
			);
		}
		ImGui::EndGroup();

		ImGui::End();
	}
}
