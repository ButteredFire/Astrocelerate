#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager(VulkanContext& vkContext, AppContext& appContext):
	m_vkContext(vkContext),
	m_appContext(appContext) {

	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	m_windowFlags = ImGuiWindowFlags_NoCollapse;

	bindPanelFlags();

	// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_VIEWPORT, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_TELEMETRY, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_ENTITY_INSPECTOR, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_SIMULATION_CONTROL, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_RENDER_SETTINGS, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_ORBITAL_PLANNER, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_INPUT, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_DEBUG_CONSOLE, GUI::TOGGLE_ON);

	initPanelsFromMask(m_panelMask);
	
	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void UIPanelManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::GUIContextIsValid>(
		[this](const Event::GUIContextIsValid& event) {
			initViewportTextureDescriptorSet();
		}
	);

	m_eventDispatcher->subscribe<Event::InputIsValid>(
		[this](const Event::InputIsValid& event) {
			m_inputManager = ServiceLocator::GetService<InputManager>(__FUNCTION__);
		}
	);
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


void UIPanelManager::updatePanels() {
	renderPanelsMenu();

	for (auto&& [flag, callback] : m_panelCallbacks) {
		if (GUI::IsPanelOpen(m_panelMask, flag))
			(this->*callback)();
	}


	// This window will capture all input and prevent interaction with other widgets
	if (m_inputManager->isViewportInputAllowed()) {
		m_inputBlockerIsOn = true;
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("##InputBlocker", nullptr,
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground);
		ImGui::End();
	}
	else {
		m_inputBlockerIsOn = false;
	}
}

void UIPanelManager::renderMenuBar() {
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("%s (version %s). Copyright (c) 2024-2025 %s.", APP_NAME, APP_VERSION, AUTHOR);

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
	m_viewportRenderTextureID = (ImTextureID) ImGui_ImplVulkan_AddTexture(
		m_vkContext.OffscreenResources.sampler,
		m_vkContext.OffscreenResources.imageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
}


void UIPanelManager::renderPanelsMenu() {
	using namespace GUI;

	ImGui::Begin("Panels Menu", nullptr, ImGuiWindowFlags_NoCollapse);
	{
		for (size_t i = 0; i < FLAG_COUNT; i++) {
			PanelFlag flag = PanelFlagsArray[i];
			bool isOpen = IsPanelOpen(m_panelMask, flag);

			ImGui::Checkbox(GetPanelName(flag), &isOpen);

			TogglePanel(m_panelMask, flag, isOpen ? TOGGLE_ON : TOGGLE_OFF);
		}
	}
	ImGui::End();
}


void UIPanelManager::renderViewportPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_VIEWPORT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) {
		Log::Print(Log::T_ERROR, __FUNCTION__, "The viewport must not be closed!");
		GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_ON);
	}

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);		// This panel must be docked
	{
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_appContext.Input.isViewportHoveredOver = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) || m_inputBlockerIsOn;
		m_appContext.Input.isViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || m_inputBlockerIsOn;

		//ImGui::Text("Panel size: (%.2f, %.2f)", viewportPanelSize.x, viewportPanelSize.y);


		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = m_vkContext.OffscreenResources.imageView;
		imageInfo.sampler = m_vkContext.OffscreenResources.sampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet imageDescSetWrite{};
		imageDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageDescSetWrite.dstBinding = 0;
		imageDescSetWrite.descriptorCount = 1;
		imageDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescSetWrite.dstSet = (VkDescriptorSet)m_viewportRenderTextureID;
		imageDescSetWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_vkContext.Device.logicalDevice, 1, &imageDescSetWrite, 0, nullptr);

		ImGui::Image(m_viewportRenderTextureID, viewportPanelSize);
	}
	ImGui::End();
}


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
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
				ImGui::Text("\tVelocity:");
				ImGui::Text("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.velocity.x, rigidBody.velocity.y, rigidBody.velocity.z);
				ImGui::Text("\t\tAbsolute: |v| ~= %.4f m/s", velocityAbs);


				float accelerationAbs = glm::length(rigidBody.acceleration);
				ImGui::Text("\tAcceleration:");
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
				if (refFrame.parentID.has_value()) {
					ImGui::Text("\tParent Entity ID: %d", refFrame.parentID.value());
				}
				else {
					ImGui::Text("\tParent Entity ID: None");
				}

				ImGui::Text("\t\tScale (simulation): %.10f m (radius)",
					refFrame.scale);
				ImGui::Text("\t\tScale (render): %.10f m (radius)",
					SpaceUtils::GetRenderableScale(SpaceUtils::ToRenderSpace(refFrame.scale)));

				// Local Transform
				ImGui::Text("\tLocal Transform:");
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
				ImGui::Text("\tGlobal Transform (normalized):");
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
	}
	ImGui::End();
}


void UIPanelManager::renderEntityInspectorPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ENTITY_INSPECTOR;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");
	}
	ImGui::End();
}


void UIPanelManager::renderSimulationControlPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_SIMULATION_CONTROL;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");
	}
	ImGui::End();
}


void UIPanelManager::renderRenderSettingsPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_RENDER_SETTINGS;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");
	}
	ImGui::End();
}


void UIPanelManager::renderOrbitalPlannerPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ORBITAL_PLANNER;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
		ImGui::TextWrapped("Pushing the boundaries of space exploration, one line of code at a time.");
	}
	ImGui::End();
}


void UIPanelManager::renderDebugConsole() {
	static bool scrolledOnWindowFocus = false;
	static bool scrollToBottom = false;
	static bool notAtBottom = false;
	static size_t prevLogBufSize = Log::LogBuffer.size();

	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_CONSOLE;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
		const float buttonHeight = ImGui::GetFrameHeight();
		const float spacing = ImGui::GetStyle().ItemSpacing.y;
		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImVec2 scrollRegionSize = ImVec2(avail.x, avail.y - buttonHeight - spacing);


		ImGui::BeginChild("ConsoleScrollRegion", scrollRegionSize, true, ImGuiWindowFlags_HorizontalScrollbar);
		{
			notAtBottom = (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 1.0f);

			for (const auto& log : Log::LogBuffer) {
				ImGui::PushStyleColor(ImGuiCol_Text, ColorUtils::LogMsgTypeToImVec4(log.type));
				ImGui::TextWrapped("[%s] [%s]: %s", log.displayType.c_str(), log.caller.c_str(), log.message.c_str());
				ImGui::PopStyleColor();

				// Auto-scroll to the bottom only if the scroll position is already at the bottom
				if (!notAtBottom)
					ImGui::SetScrollHereY(1.0f);
			}


			// Auto-scroll to bottom once on switching to this panel
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && !scrolledOnWindowFocus) {
				std::cout << "Scrolled once\n";
				ImGui::SetScrollHereY(1.0f);
				scrolledOnWindowFocus = true;
			}

			if (scrollToBottom) {
				ImGui::SetScrollHereY(1.0f);
				scrollToBottom = false;
			}


			prevLogBufSize = Log::LogBuffer.size();

			// Reset the flag if the window is not focused
			if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow))
				scrolledOnWindowFocus = false;

		}
		ImGui::EndChild();

		// Show scroll-to-bottom button if not at the bottom
		if (notAtBottom) {
			if (ImGui::Button("\\/", ImVec2(avail.x, buttonHeight))) {
				scrollToBottom = true;
			}
		}
		else
			ImGui::Dummy(ImVec2(avail.x, buttonHeight));
	}
	ImGui::End();
}


void UIPanelManager::renderDebugInput() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_INPUT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag), nullptr, m_windowFlags);
	{
		ImGui::Text("Viewport:");
		{
			ImGui::BulletText("Hovered over: %s", BOOLALPHACAP(m_appContext.Input.isViewportHoveredOver));
			ImGui::BulletText("Focused: %s", BOOLALPHACAP(m_appContext.Input.isViewportFocused));
			ImGui::BulletText("Input blocker on: %s", BOOLALPHACAP(m_inputBlockerIsOn));
		}

		ImGui::Separator();

		ImGui::Text("Viewport controls (Input manager)");
		{
			ImGui::BulletText("Input allowed: %s", BOOLALPHACAP(m_inputManager->isViewportInputAllowed()));
			ImGui::BulletText("Focused: %s", BOOLALPHACAP(m_inputManager->isViewportFocused()));
			ImGui::BulletText("Unfocused: %s", BOOLALPHACAP(m_inputManager->isViewportUnfocused()));
		}
	}
	ImGui::End();
}
