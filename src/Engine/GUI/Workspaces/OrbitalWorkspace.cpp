#include "OrbitalWorkspace.hpp"


OrbitalWorkspace::OrbitalWorkspace() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_ecsRegistry = ServiceLocator::GetService<ECSRegistry>(__FUNCTION__);

	m_coreResources = ServiceLocator::GetService<VkCoreResourcesManager>(__FUNCTION__);
	m_swapchainManager = ServiceLocator::GetService<VkSwapchainManager>(__FUNCTION__);

	m_windowFlags = ImGuiWindowFlags_NoCollapse;
	m_popupWindowFlags = ImGuiWindowFlags_NoDocking;

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void OrbitalWorkspace::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<OrbitalWorkspace>();

	m_eventDispatcher->subscribe<RecreationEvent::OffscreenResources>(selfIndex,
		[this](const RecreationEvent::OffscreenResources &event) {
			for (size_t i = 0; i < m_viewportRenderTextureIDs.size(); i++) {
				ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_viewportRenderTextureIDs[i]);
			}

			m_offscreenImageViews = event.imageViews;
			m_offscreenSamplers = event.samplers;
			initPerFrameTextures();
		}
	);


	m_eventDispatcher->subscribe<InitEvent::OffscreenPipeline>(selfIndex, 
		[this](const InitEvent::OffscreenPipeline &event) {
			m_offscreenImageViews = event.offscreenImageViews;
			m_offscreenSamplers = event.offscreenImageSamplers;
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_RESET:
				m_sceneSampleInitialized = false;
				m_sceneSampleReady = false;
				break;

			case INITIALIZED:
				m_sceneSampleInitialized = true;
				initPerFrameTextures();
				break;

			case POST_INITIALIZATION:
				m_sceneSampleReady = true;
				ThreadManager::SortThreadMap();		// Sorts the thread map for displaying on the UI
				break;
			}
		}
	);


	m_eventDispatcher->subscribe<InitEvent::InputManager>(selfIndex,
		[this](const InitEvent::InputManager &event) {
			m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);
		}
	);
}


void OrbitalWorkspace::init() {
	initStaticTextures();

	if (m_sceneSampleInitialized)
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
	m_panelDebugApp =				GUI::RegisterPanel("Application Debug Info");
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
	GUI::TogglePanel(m_panelMask, m_panelDebugApp, GUI::TOGGLE_ON);
}


void OrbitalWorkspace::update(uint32_t currentFrame) {
	m_currentFrame = currentFrame;

	// Conditionally render instanced panels
	if (GUI::IsPanelOpen(m_panelMask, m_panelSceneResourceDetails))
		renderSceneResourceDetails();
	if (GUI::IsPanelOpen(m_panelMask, m_panelCodeEditor))
		renderCodeEditor();


	// The input blocker serves to capture all input and prevent interaction with other widgets when using the viewport.
	if (m_sceneSampleInitialized && m_inputManager->isViewportInputAllowed()) {
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
	// Reset per-session data
	m_simulationIsPaused = true;
	Time::SetTimeScale(0.0f);
	if (m_lastTimeScale <= 0.0f)
		m_lastTimeScale = 1.0f;

	m_sceneResourceEntityData.clear();


	// Close certain instanced panels
	GUI::TogglePanel(m_panelMask, m_panelSceneResourceDetails, GUI::TOGGLE_OFF);
	GUI::TogglePanel(m_panelMask, m_panelCodeEditor, GUI::TOGGLE_OFF);


	// Load script
	m_simulationConfigChanged = true;
	m_simulationConfigPath = configPath;
	m_simulationScriptData = FilePathUtils::ReadFile(configPath);

	m_eventDispatcher->dispatch(RequestEvent::InitSession{
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
	const size_t OFFSCREEN_RESOURCE_COUNT = m_offscreenImageViews.size();
	m_viewportRenderTextureIDs.resize(OFFSCREEN_RESOURCE_COUNT);

	for (size_t i = 0; i < OFFSCREEN_RESOURCE_COUNT; i++) {
		m_viewportRenderTextureIDs[i] = TextureUtils::GenerateImGuiTextureID(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			m_offscreenImageViews[i],
			m_offscreenSamplers[i]
		);
	}
}


void OrbitalWorkspace::updatePerFrameTextures(uint32_t currentFrame) {
	// Simulation scene
	if (m_sceneSampleReady) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = m_offscreenImageViews[currentFrame];
		imageInfo.sampler = m_offscreenSamplers[currentFrame];
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet imageDescSetWrite{};
		imageDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageDescSetWrite.dstBinding = 0;
		imageDescSetWrite.descriptorCount = 1;
		imageDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescSetWrite.dstSet = (VkDescriptorSet)m_viewportRenderTextureIDs[currentFrame];
		imageDescSetWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_coreResources->getLogicalDevice(), 1, &imageDescSetWrite, 0, nullptr);
	}
}


void OrbitalWorkspace::renderViewportPanel() {
	if (ImGui::Begin(GUI::GetPanelName(m_panelViewport), nullptr, m_windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

		ImGuiUtils::PushStyleClearButton();
		{
			ImGui::AlignTextToFramePadding();


			std::string sceneName{};
			if (!m_simulationConfigPath.empty())
				sceneName += FilePathUtils::GetFileName(m_simulationConfigPath, false);


			// ----- RELOAD SIMULATION BUTTON -----
			if (!m_simulationConfigPath.empty()) {
				if (ImGui::Button(ICON_FA_ARROW_ROTATE_RIGHT)) {
					loadSimulationConfig(m_simulationConfigPath);
				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(ImGuiHoveredFlags_None, ("Reload " + sceneName).c_str());

				ImGui::SameLine();
			}


			
			// ----- PAUSE/PLAY BUTTON & TIME SCALE -----
			ImGui::BeginGroup();
			{
				// Pause/Play button
				static bool initialLoad = true;

				if (m_simulationIsPaused) {
					if (ImGui::Button(ImGuiUtils::IconString(ICON_FA_PLAY, sceneName).c_str())) {
						Time::SetTimeScale(m_lastTimeScale);
						m_simulationIsPaused = false;
					}
					ImGuiUtils::CursorOnHover();
				}
				else {
					if (ImGui::Button(ImGuiUtils::IconString(ICON_FA_PAUSE, sceneName).c_str())) {
						Time::SetTimeScale(0.0f);
						m_simulationIsPaused = true;
					}
					ImGuiUtils::CursorOnHover();
				}


				ImGuiUtils::VerticalSeparator();


				// Time scale
				static float timeScale = (Time::GetTimeScale() <= 0.0f) ? 1.0f : Time::GetTimeScale();
				static const float MIN_VAL = 1.0f, MAX_VAL = 1000.0f;

				std::stringstream textSS;
				textSS << std::fixed << std::setprecision(1) << timeScale << "x"; // NOTE: std::fixed ensures floating-point numbers are displayed in floating-point notation (e.g., 123.45 instead of 1.2345e+02)
				std::string timeScaleText = textSS.str();
				float timeScaleTextWidth = ImGui::CalcTextSize(timeScaleText.c_str()).x + 10.0f;

				ImGui::Text("Time scale:");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(timeScaleTextWidth);
				ImGui::InputFloat("##TimeScaleInputFloat", &timeScale, 0.0f, 0.0f, timeScaleText.c_str(), ImGuiInputTextFlags_AlwaysOverwrite);  // NOTE: Setting step and step_fast to 0 disables the -/+ buttons

				if (ImGui::IsItemDeactivatedAfterEdit()) {
					// Only change time scale when input is complete (see legacy ImGuiInputTextFlags_EnterReturnsTrue description)
					timeScale = std::clamp(timeScale, MIN_VAL, MAX_VAL);

					m_lastTimeScale = timeScale;
					
					if (!m_simulationIsPaused)
						Time::SetTimeScale(timeScale);
				}
			}
			ImGui::EndGroup();



			ImGuiUtils::VerticalSeparator();



			// ----- CAMERA PERSPECTIVE SELECTOR -----
			static Camera *camera = m_inputManager->getCamera();
			ImGui::BeginGroup();
			{
				// Data Querying
				static std::vector<std::pair<std::string, EntityID>> entityList;
				static std::pair<std::string, EntityID> selectedEntity, prevSelectedEntity;
			
				if (m_sceneSampleInitialized && entityList.empty()) {
					// Do only once on scene load: construct/refresh the view and update entity names list
					auto view = m_ecsRegistry->getView<CoreComponent::Transform>();		// All entities with transforms are camera-attachable

					entityList.reserve(view.size());


						// Push camera as first option
					Entity camEntity = camera->getEntity();
					entityList.push_back({
						"Free-fly",
						camEntity.id
					});
					selectedEntity = prevSelectedEntity = entityList.back();

						// Populate with other options
					for (auto &entityID : view.getMatchingEntities()) {
						if (entityID == m_ecsRegistry->getRenderSpaceEntity().id)
							// DO NOT include the render space entity
							continue;

						entityList.push_back({
							m_ecsRegistry->getEntity(entityID).name,
							entityID
						});
					}
				}
				else if (!m_sceneSampleInitialized && !entityList.empty())
					// Else if another scene is being loaded
					entityList.clear();


				// Perspective Selector
				ImGui::Text("Camera:");

				ImGui::SameLine();
				ImGui::SetNextItemWidth(200.0f);

				if (!m_sceneSampleInitialized) ImGuiUtils::PushStyleDisabled();
				{
					if (ImGui::BeginCombo("##CameraSwitchCombo", selectedEntity.first.c_str(), ImGuiComboFlags_NoArrowButton)) {
						for (const auto &entityPair : entityList) {
							bool isSelected = (selectedEntity == entityPair);
							if (ImGui::Selectable(entityPair.first.c_str(), isSelected))
								selectedEntity = entityPair;

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}


						ImGui::EndCombo();
					}
					ImGuiUtils::CursorOnHover();
				}
				if (!m_sceneSampleInitialized) ImGuiUtils::PopStyleDisabled();


				if (prevSelectedEntity != selectedEntity) {
					prevSelectedEntity = selectedEntity;
					camera->attachToEntity(selectedEntity.second);
				}
			}
			ImGui::EndGroup();



			ImGuiUtils::VerticalSeparator();



			// ----- INTEGRATOR SELECTOR -----
			// TODO: Implement integrator switching
			ImGui::BeginGroup();
			{
				static std::string currentIntegrator = "Fourth Order Runge-Kutta";
				ImGui::Text("Integrator:");
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
			ImGui::EndGroup();
		}
		ImGuiUtils::PopStyleClearButton();



		// ----- LARGE TIME SCALE WARNING -----
		if (Time::GetTimeScale() >= 500.0f) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
				ImGuiUtils::AlignedText(ImGuiUtils::TEXT_ALIGN_MIDDLE, ImGuiUtils::IconString(ICON_FA_TRIANGLE_EXCLAMATION, "High time scales may cause numerical and visual instability.").c_str());
			ImGui::PopStyleColor();
		}



		ImGui::Separator();



		// ----- VIEWPORT RENDERING -----
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		if (ImGui::BeginChild("##ViewportSceneRegion")) {
			if (m_sceneSampleInitialized) {
				// Viewport Hovering/Focusing Trackers
					// NOTE: Here, the actual viewport is the child ##ViewportSceneRegion window. The parent window contains this and other UI elements (e.g., the top bar), and we don't want to interfere with input flags solely by interacting with the UI elements outside the viewport scene region.
				ImGuiFocusedFlags focusFlags = ImGuiFocusedFlags_ChildWindows;

				g_guiCtx.Input.isViewportHoveredOver	= ImGui::IsWindowHovered(focusFlags) || m_inputBlockerIsOn;
				g_guiCtx.Input.isViewportFocused		= ImGui::IsWindowFocused(focusFlags) || m_inputBlockerIsOn;


				// Viewport Resize Mechanism
				static ImVec2 viewportSceneRegion;
				static ImVec2 lastViewportSceneRegion(0, 0);

				viewportSceneRegion = ImGui::GetContentRegionAvail();

					// If viewport size changes, dispatch an update event
				if (!ImGuiUtils::CompImVec2(viewportSceneRegion, lastViewportSceneRegion)) {
					m_eventDispatcher->dispatch(UpdateEvent::ViewportSize{
						.sceneDimensions = glm::vec2(viewportSceneRegion.x, viewportSceneRegion.y)
						}, true);

					lastViewportSceneRegion = viewportSceneRegion;
				}


				// Viewport Rendering
				ImGui::Image(m_viewportRenderTextureIDs[m_currentFrame], viewportSceneRegion);

					// Telemetry (vertically aligned)
				{
					auto view = m_ecsRegistry->getView<PhysicsComponent::CoordinateSystem>();
					static constexpr float
						PADDING_X = 20.0f,
						PADDING_Y = 20.0f,
						FONT_SCALE = 1.25f;

					if (view.size() > 0) {
						auto [_, coordSys] = view[0];

						const std::vector<std::string> telemetry = {
							"Coordinate System: " + CoordSys::EpochToSPICEMap.at(coordSys.simulationConfig.epoch)
									+ " (Observer: " + CoordSys::FrameProperties.at(coordSys.simulationConfig.frame).spiceName + ")",
							"Epoch: " + coordSys.currentEpoch,
							"Frame: " + CoordSys::FrameProperties.at(coordSys.simulationConfig.frame).displayName
						};

						ImGui::SetWindowFontScale(FONT_SCALE);
						const float lineHeight = ImGui::GetFontSize();
						{
							for (int i = 0; i < telemetry.size(); i++)
								ImGuiUtils::FloatingText(
									ImVec2(cursorPos.x + PADDING_X,
										cursorPos.y + PADDING_Y + lineHeight * i
									),
									telemetry[i]
								);
						}
					}
				}

					// Controls (horizontally aligned)
				{
					static constexpr float
						PADDING_PER_LABEL = 40.0f,  // Padding between labels
						PADDING_BOTTOM = 40.0f,
						FONT_SCALE = 1.0f;

					const std::vector<std::string> controlLabels = {
						(m_inputManager->isCameraOrbiting()) ? "[LMB-Hold] Move Camera" : "[LMB]/[ESC] Pilot/Release Camera",
						(m_inputManager->isCameraOrbiting()) ? ""						: "[W,A,S,D | Q,E] Move Camera",
						"[Scroll] Change Camera Zoom"
					};
					std::vector<float> labelWidths;

					ImGui::SetWindowFontScale(FONT_SCALE);
					{
						// Calculate the total width of all labels + padding between each
						float totalTextWidth = (controlLabels.size() - 1) * PADDING_PER_LABEL;

						for (int i = 0; i < controlLabels.size(); i++) {
							ImVec2 textSize = ImGui::CalcTextSize(controlLabels[i].c_str());
							labelWidths.push_back(textSize.x);
							totalTextWidth += textSize.x;
						}

						const float startX = cursorPos.x + (viewportSceneRegion.x - totalTextWidth) / 2;
						const float startY = cursorPos.y + viewportSceneRegion.y - PADDING_BOTTOM;

						// Render
						ImVec2 start(startX, startY);

						for (int i = 0, sz = controlLabels.size(); i < sz; i++) {
							if (i > 0)
								start.x += labelWidths[i-1] + PADDING_PER_LABEL;

							ImGuiUtils::FloatingText(start, controlLabels[i]);
						}
					}
				}

				ImGui::SetWindowFontScale(1.0f);
			}

			else {
				// If the scene has not been initialized, show a prompt to load the scene
				ImVec2 viewportSceneRegion = ImGui::GetContentRegionAvail();

				static constexpr float FONT_SIZE_HEADING = 3.0f, FONT_SIZE_SUBHEADING = 2.0f, PADDING_Y = 20.0f;

				const std::string heading = "No Simulations Loaded";
				const std::string subheading = "Load a simulation to get started!";


				ImGui::SetWindowFontScale(FONT_SIZE_HEADING);
					ImVec2 headingSize = ImGui::CalcTextSize(heading.c_str());
					float lhHeading = ImGui::GetFontSize();

				ImGui::SetWindowFontScale(FONT_SIZE_SUBHEADING);
					ImVec2 subheadingSize = ImGui::CalcTextSize(subheading.c_str());
					float lhSubheading = ImGui::GetFontSize();


				float totalHeight = headingSize.y + PADDING_Y + subheadingSize.y;
				float startY = cursorPos.y + (viewportSceneRegion.y - totalHeight) / 2;


				ImVec2 headingTextStart(
					cursorPos.x + (viewportSceneRegion.x - headingSize.x) / 2,
					startY
				);

				ImVec2 subheadingTextStart(
					cursorPos.x + (viewportSceneRegion.x - subheadingSize.x) / 2,
					startY + lhHeading + PADDING_Y
				);


				ImGui::SetWindowFontScale(FONT_SIZE_HEADING);
					ImGuiUtils::FloatingText(headingTextStart, heading.c_str());
				ImGui::SetWindowFontScale(FONT_SIZE_SUBHEADING);
					ImGuiUtils::FloatingText(subheadingTextStart, subheading.c_str());
				ImGui::SetWindowFontScale(1.0f);
			}



			ImGui::EndChild();
		}



		ImGui::End();
	}
}


void OrbitalWorkspace::renderTelemetryPanel() {
	static const ImVec2 separatorPadding(10.0f, 10.0f);

	if (ImGui::Begin(GUI::GetPanelName(m_panelTelemetry), nullptr, m_windowFlags)) {
		if (!m_sceneSampleReady) {
			ImGui::End();
			return;
		}
		
		
		auto view = m_ecsRegistry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();
		size_t entityCount = 0;

		for (const auto &[entity, transform, rigidBody] : view) {
			// As the content is dynamically generated, we need each iteration to have its ImGui ID to prevent conflicts.
			// Since entity IDs are always unique, we can use them as ImGui IDs.
			ImGui::PushID(static_cast<int>(entity));

			ImGui::SeparatorText(m_ecsRegistry->getEntity(entity).name.c_str());

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
			if (ImGui::CollapsingHeader("Transform Data")) {
				ImGuiUtils::ComponentField(
					{
						{"X", transform.position.x},
						{"Y", transform.position.y},
						{"Z", transform.position.z}
					},
					"%.2f", "\tPosition"
				);
				ImGui::Text("\tMagnitude: ||vec|| ≈ %.2f m", glm::length(transform.position));


				//ImGuiUtils::ComponentField(
				//	{
				//		{"X", renderT.position.x},
				//		{"Y", renderT.position.y},
				//		{"Z", renderT.position.z}
				//	},
				//	"%.2f", "\tPosition (render)"
				//);
				//ImGui::Text("\tMagnitude: ||vec|| ≈ %.2f units", glm::length(renderT.position));


				glm::dvec3 globalRotationEuler = SpaceUtils::QuatToEulerAngles(transform.rotation);
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
		CoreComponent::Transform cameraTransform = camera->getAbsoluteTransform();
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
		auto view = m_ecsRegistry->getView<PhysicsComponent::ShapeParameters>();
		if (view.size() == 0)
			ImGui::SeparatorText("Shape Parameters: None");

		else {
			ImGui::SeparatorText("Shape Parameters");

			for (auto &&[entity, shapeParams] : view) {
				ImGui::PushID(static_cast<int>(entity));

				if (ImGui::CollapsingHeader(m_ecsRegistry->getEntity(entity).name.c_str())) {
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
		// Camera settings
		{
			ImGui::SeparatorText("Camera");

			static Camera *camera = m_inputManager->getCamera();

			// Speed magnitude
			{
				static float speedMagnitude = 8.0f;

				static bool initialCameraLoad = true;
				if (initialCameraLoad) {
					camera->movementSpeed = std::powf(10.0f, speedMagnitude);
					initialCameraLoad = false;
				}

				ImGui::Text("Speed (Magnitude):");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGuiUtils::GetAvailableWidth());
				if (ImGui::DragFloat("##CameraSpeedDragFloat", &speedMagnitude, 0.25f, 1.0f, 12.0f, "1e+%.0f", ImGuiSliderFlags_AlwaysClamp)) {
					camera->movementSpeed = std::powf(10.0f, speedMagnitude);
				}
				ImGuiUtils::CursorOnHover();
			}
		
			
			// Revert position checkbox
			{
				static bool revertPosition = false;

				if (ImGui::Checkbox("Revert to last Free-fly position on switching back", &revertPosition)) {
					camera->revertPositionOnFreeFlySwitch(revertPosition);
				}

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

	static bool essentialLogsOnly = true;
	static std::unordered_set<Log::MsgType> essentialLogTypes = { Log::T_SUCCESS, Log::T_INFO, Log::T_WARNING, Log::T_ERROR, Log::T_FATAL };


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
		ImGui::BeginGroup();
		{
			// Filter by log type
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


			ImGuiUtils::VerticalSeparator();


			// Show only essential logs checkbox
			ImGui::Checkbox("Only display essential logs", &essentialLogsOnly);
		}
		ImGui::EndGroup();


		// Optional flag: ImGuiWindowFlags_HorizontalScrollbar
		if (ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), ImGuiChildFlags_Borders, m_windowFlags)) {
			notAtBottom = (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 1.0f);

			ImGui::PushFont(g_guiCtx.Font.regularMono);
			{
				for (const auto &log : Log::LogBuffer) {
					if (selectedLogType != logTypes[0] && selectedLogType != log.displayType)
						continue;

					if (essentialLogsOnly && essentialLogTypes.count(log.type) == 0)
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
		ImGui::SeparatorText("Application");
		ImGui::BeginGroup();
		{
			// FPS
			{
				io = ImGui::GetIO();
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

				// FPS must be >= (2 * PHYS_UPDATE_FREQ) for linear interpolation to properly work and prevent jittering
				int recommendedFPS = std::floor(2 * (1 / SimulationConst::TIME_STEP));
				if (io.Framerate < recommendedFPS) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
					ImGui::TextWrapped(ImGuiUtils::IconString(ICON_FA_TRIANGLE_EXCLAMATION, "This framerate does not meet the recommended " + std::to_string(recommendedFPS) + " FPS threshold, below which jittering may occur. Alternatively, you can lower the physics time step to lower the threshold.").c_str());
					ImGui::PopStyleColor();
				}
			}


			// Threads
			ImGui::Text("Threads:");
			if (ImGui::BeginTable("MyTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Thread");
				ImGui::TableSetupColumn("ID");
				ImGui::TableSetupColumn("Status");


				ImGui::TableHeadersRow();
				ImGui::TableNextRow();


				// Row for the main thread
				ImGui::TableSetColumnIndex(0);
					ImGui::Text("Main thread");

				ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", std::this_thread::get_id());

				ImGui::TableSetColumnIndex(2);
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
						ImGui::Text("Active");
					ImGui::PopStyleColor();


				// Rows for worker threads
				for (const auto &[id, worker] : ThreadManager::GetThreadMap()) {
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text(worker->getName().c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", worker->getID());

					ImGui::TableSetColumnIndex(2);
					{
						if (worker->isDetached()) {
							ImGui::BeginDisabled();
							ImGui::Text("Detached");
							ImGui::EndDisabled();
						}
						else {
							if (worker->isRunning()) {
								ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
								ImGui::Text("Active");
								ImGui::PopStyleColor();
							}
							else {
								ImGui::BeginDisabled();
								ImGui::Text("Inactive");
								ImGui::EndDisabled();
							}
						}
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndGroup();
		

		ImGui::Dummy(ImVec2(2.0f, 2.0f));

		ImGui::SeparatorText("Input");
		ImGui::BeginGroup();
		{
			ImGui::Text("Viewport");
			ImGui::Indent();
			{
				ImGui::Text("Hovered over: %s", BOOLALPHACAP(g_guiCtx.Input.isViewportHoveredOver));
				ImGui::Text("Focused: %s", BOOLALPHACAP(g_guiCtx.Input.isViewportFocused));
				ImGui::Text("Input blocker on: %s", BOOLALPHACAP(m_inputBlockerIsOn));
			}
			ImGui::Unindent();


			ImGuiUtils::Padding(5.0f);


			ImGui::Text("Viewport controls (Input manager)");
			ImGui::Indent();
			{
				ImGui::Text("Input allowed: %s", BOOLALPHACAP(m_inputManager->isViewportInputAllowed()));
				ImGui::Text("Focused: %s", BOOLALPHACAP(m_inputManager->isViewportFocused()));
				ImGui::Text("Unfocused: %s", BOOLALPHACAP(m_inputManager->isViewportUnfocused()));
			}
			ImGui::Unindent();
		}
		ImGui::EndGroup();



		ImGui::SeparatorText("Time & Coordinate Systems");
		ImGui::BeginGroup();
		{
			ImGui::Text("Time scale: %.1fx", Time::GetTimeScale());
		}
		ImGui::EndGroup();


		ImGui::End();
	}
}


void OrbitalWorkspace::renderSceneResourceTree() {
	static ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_DefaultOpen;

	static std::function<void(m_ResourceType, EntityID, const std::string&)> renderTreeNode = [this](m_ResourceType resourceType, EntityID entityID, const std::string &nodeName) {
		using enum m_ResourceType;

		if (ImGui::Button(C_STR(nodeName))) {
			if (resourceType == SCRIPTS)	// Redirect to code editor
				GUI::TogglePanel(m_panelMask, m_panelCodeEditor, GUI::TOGGLE_ON);

			else {
				m_sceneResourceEntityData.insert({ entityID, resourceType });
				GUI::TogglePanel(m_panelMask, m_panelSceneResourceDetails, GUI::TOGGLE_ON);
			}
		}
		ImGuiUtils::CursorOnHover();
	};


	if (ImGui::Begin(GUI::GetPanelName(m_panelSceneResourceTree), nullptr, m_windowFlags)) {

		ImGuiUtils::PushStyleClearButton();
		{
			auto view = m_ecsRegistry->getView<CoreComponent::Identifiers>();
			using enum CoreComponent::Identifiers::EntityType;

			// Spacecraft
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Spacecraft & Satellites").c_str(), treeFlags)) {
				ImGui::Indent();
				{
					for (const auto &[entity, id] : view) {
						if (id.entityType == SPACECRAFT)
							renderTreeNode(m_ResourceType::SPACECRAFT, entity, ImGuiUtils::IconString(ICON_FA_SATELLITE, m_ecsRegistry->getEntity(entity).name));
					}
				}
				ImGui::Unindent();

				ImGui::TreePop();
			}
			ImGuiUtils::CursorOnHover();


			// Celestial bodies
			if (ImGui::TreeNodeEx(ImGuiUtils::IconString(ICON_FA_FOLDER, "Celestial bodies").c_str(), treeFlags)) {
				ImGui::Indent();
				{
					for (const auto &[entity, id] : view) {
						using enum CoreComponent::Identifiers::EntityType;
						static const std::unordered_set<CoreComponent::Identifiers::EntityType> celestialBodyTypes = {
							STAR, PLANET, MOON, ASTEROID
						};
						
						if (celestialBodyTypes.count(id.entityType)) {
							const char *icon = ICON_FA_CIRCLE;

							switch (id.entityType) {
							case STAR:
								icon = ICON_FA_STAR;
								break;

							case MOON:
								icon = ICON_FA_MOON;
								break;

							case ASTEROID:
								icon = ICON_FA_METEOR;
								break;
							}
							
							renderTreeNode(m_ResourceType::CELESTIAL_BODIES, entity, ImGuiUtils::IconString(icon, m_ecsRegistry->getEntity(entity).name));
						}
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
					auto view = m_ecsRegistry->getView<PhysicsComponent::Propagator>();

					for (const auto &[entity, propagator] : view) {
						std::string propagatorName = "Unknown";

						using enum PhysicsComponent::Propagator::Type;
						switch (propagator.propagatorType) {
						case SGP4:
							propagatorName = "SGP4";
							break;
						}


						ImGuiUtils::PushStyleDisabled();
							ImGui::Button(ImGuiUtils::IconString(ICON_FA_HEXAGON_NODES, propagatorName).c_str());
						ImGuiUtils::PopStyleDisabled();
						//renderTreeNode(m_ResourceType::PROPAGATORS, entity, ImGuiUtils::IconString(ICON_FA_HEXAGON_NODES, propagatorName));
					}
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
				auto view = m_ecsRegistry->getView<PhysicsComponent::CoordinateSystem>();

				ImGui::Indent();
				{
					for (const auto &[entity, coordSystem] : view) {
						renderTreeNode(m_ResourceType::COORDINATE_SYSTEMS, entity, ImGuiUtils::IconString(ICON_FA_VECTOR_SQUARE, m_ecsRegistry->getEntity(entity).name));
					}
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
	// Containers to track certain data of entity detail panels pending to be rendered
	static std::unordered_map<EntityID, m_ResourceType> entityResourceTypes{};	// Map of entity resource types
	static std::unordered_map<EntityID, bool> entityPanelBools{};				// Map of booleans to track the open state of entity panels

	static int windowCount = 0; // Number of open windows
	windowCount = 0; // Resets count each frame



	using enum m_ResourceType;
	for (const auto &[entityID, resourceType] : m_sceneResourceEntityData) {
		if (entityPanelBools.count(entityID) == 0)
			entityPanelBools[entityID] = true;


		if (entityPanelBools[entityID]) {
			// Offset window position
			const float offsetFromFirstWindow = 30.0f * windowCount;
			
			ImVec2 pos = ImGui::GetWindowPos();
			pos.x += offsetFromFirstWindow;
			pos.y += offsetFromFirstWindow;

			ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
			windowCount++;


			// Only allow max window size to be content size
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX));


			// Create panel name (which will act as the panel ID)
			std::stringstream ss;
			if (entityID != INVALID_ENTITY)
				ss << m_ecsRegistry->getEntity(entityID).name;

			ss << " | " << GUI::GetPanelName(m_panelSceneResourceDetails);

		
			if (ImGui::Begin(C_STR(ss.str()), &entityPanelBools[entityID], ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {

				// ----- SPACECRAFT -----
				if (resourceType == SPACECRAFT) {
					SpacecraftComponent::Spacecraft sc = m_ecsRegistry->getComponent<SpacecraftComponent::Spacecraft>(entityID);

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


					if (m_ecsRegistry->hasComponent<SpacecraftComponent::Thruster>(entityID)) {
						ImGuiUtils::Padding();

						SpacecraftComponent::Thruster thruster = m_ecsRegistry->getComponent<SpacecraftComponent::Thruster>(entityID);

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
				else if (resourceType == CELESTIAL_BODIES) {
					if (m_ecsRegistry->hasComponent<PhysicsComponent::ShapeParameters>(entityID)) {
						PhysicsComponent::ShapeParameters shape = m_ecsRegistry->getComponent<PhysicsComponent::ShapeParameters>(entityID);

						ImGui::SeparatorText("Shape Configuration");
						ImGui::Indent();
						{
							ImGui::Text("Flattening: e ≈ %.5f", shape.flattening);
							ImGui::Text("Mean equatorial radius: r ≈ %.5f m", shape.equatRadius);
							ImGui::Text("Gravitational parameter: μ ≈ %.5g m³/s⁻²", shape.gravParam);
							ImGui::Text("Rotational velocity (scalar): ω ≈ %.5g rad/s", glm::length(shape.rotVelocity));
							ImGui::Text("J2 oblateness coefficient: %.5g", shape.j2);
						}
						ImGui::Unindent();

						ImGuiUtils::Padding();
					}


					CoreComponent::Identifiers identifiers = m_ecsRegistry->getComponent<CoreComponent::Identifiers>(entityID);
					ImGui::SeparatorText("Miscellaneous");
					ImGui::Indent();
					{
						if (identifiers.spiceID.has_value())
							ImGui::Text("SPICE Identifier: %s", identifiers.spiceID.value().c_str());
					}
					ImGui::Unindent();
				}



				// ----- PROPAGATORS -----
				else if (resourceType == PROPAGATORS) {
					ImGui::Text("Current information on this propagator is not currently available.");
				}



				// ----- SOLVERS -----
				else if (resourceType == SOLVERS) {
					ImGui::Text("Current information on this solver is not currently available.");
				}



				// ----- SCRIPTS -----
				//else if (resourceType == SCRIPTS) {
				//	ImGui::Text("Current information on this script is not currently available.");
				//}



				// ----- COORDINATE SYSTEMS -----
				else if (resourceType == COORDINATE_SYSTEMS) {
					const PhysicsComponent::CoordinateSystem &coordSystem = m_ecsRegistry->getComponent<PhysicsComponent::CoordinateSystem>(entityID);

					ImGui::SeparatorText(STD_STR(m_ecsRegistry->getEntity(entityID).name + " Configuration").c_str());
					ImGui::Indent();
					{
						ImGui::Text("Coordinate System: %s (%s)",
							CoordSys::FrameProperties.at(coordSystem.simulationConfig.frame).displayName.c_str(),
							CoordSys::FrameTypeToDisplayStrMap.at(coordSystem.simulationConfig.frameType).c_str()
						);
						ImGui::Text("Epoch: %s", CoordSys::EpochToSPICEMap.at(coordSystem.simulationConfig.epoch).c_str());
						ImGui::Text("Epoch format: %s", coordSystem.simulationConfig.epochFormat.c_str());
					}
					ImGui::Unindent();


					ImGuiUtils::Padding();


					ImGui::SeparatorText("SPICE Kernels Loaded");
					ImGui::Indent();
					{
						for (const auto &kernel : coordSystem.simulationConfig.kernelPaths)
							ImGui::Text(kernel.c_str());
					}
					ImGui::Unindent();
				}

				ImGui::End();
			}
		}
		

		// If the entity panel is closed, remove it from being tracked.
		if (!entityPanelBools[entityID]) {
			entityPanelBools.erase(entityID);
			entityResourceTypes.erase(entityID);
			m_sceneResourceEntityData.erase({ entityID, resourceType });
		}
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
	switch (g_guiCtx.GUI.currentAppearance) {
	case IMGUI_APPEARANCE_DARK_MODE:
		m_codeEditor.SetPalette(CodeEditor::GetDarkPalette());
		break;

	case IMGUI_APPEARANCE_LIGHT_MODE:
		m_codeEditor.SetPalette(CodeEditor::GetLightPalette());
		break;
	}

	m_codeEditor.SetLanguageDefinition(CodeEditor::LanguageDefinition::YAML());
	m_codeEditor.SetShowWhitespaces(false);


	std::stringstream ss;
	if (!m_simulationScriptData.empty())
		ss << FilePathUtils::GetFileName(m_simulationConfigPath);
	else
		ss << "New Script";
	ss << " (Read-only)" << GUI::GetPanelName(m_panelCodeEditor);


	bool panelOpen = GUI::IsPanelOpen(m_panelMask, m_panelCodeEditor);
	ImGuiWindowFlags documentEditedFlag = (m_codeEditor.IsTextChanged()) ? ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

	if (ImGui::Begin(C_STR(ss.str()), &panelOpen, ImGuiWindowFlags_NoCollapse | documentEditedFlag)) {

		ImGui::AlignTextToFramePadding();

		// Editor controls
		// TODO: Implement functionality
		ImGuiUtils::PushStyleClearButton();
		{
			// Editing actions
			ImGui::BeginGroup();
			{
				if (ImGui::Button(ICON_FA_ARROW_ROTATE_LEFT)) {
					m_codeEditor.Undo();
				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Undo");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_ARROW_ROTATE_RIGHT)) {
					m_codeEditor.Redo();
				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Redo");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_SCISSORS)) {
					m_codeEditor.Cut();
				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Cut");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_COPY)) {
					m_codeEditor.Copy();
				}
				ImGuiUtils::CursorOnHover();
				ImGuiUtils::TextTooltip(0, "Copy");

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_CLIPBOARD)) {
					m_codeEditor.Paste();
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
				ImGuiUtils::TextTooltip(0, "Find & Replace (currently unavailable)");

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
		ImGui::PushFont(g_guiCtx.Font.regularMono);
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

	if (!panelOpen)
		GUI::TogglePanel(m_panelMask, m_panelCodeEditor, GUI::TOGGLE_OFF);
}
