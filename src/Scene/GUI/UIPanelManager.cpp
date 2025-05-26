#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager() {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);


	bindPanelFlags();

	// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_VIEWPORT, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_TELEMETRY, GUI::TOGGLE_ON);

	initPanelsFromMask(m_panelMask);

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
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
}


void UIPanelManager::renderPanelsMenu() {
	using namespace GUI;

	ImGui::Begin("Panels Menu");

	for (size_t i = 0; i < FLAG_COUNT; i++) {
		PanelFlag flag = PanelFlagsArray[i];
		bool isOpen = IsPanelOpen(m_panelMask, flag);

		ImGui::Checkbox(GetPanelName(flag), &isOpen);

		TogglePanel(m_panelMask, flag, isOpen ? TOGGLE_ON : TOGGLE_OFF);
	}

	ImGui::End();
}


void UIPanelManager::renderViewportPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_VIEWPORT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) {
		Log::Print(Log::T_ERROR, __FUNCTION__, "The viewport must not be closed!");
		GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_ON);
	}

	ImGui::Begin("Viewport");		// This panel must be docked

	ImGui::Text("Simulation viewport (TBD)");

	ImGui::End();
}


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

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

	ImGui::End();
}


void UIPanelManager::renderEntityInspectorPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ENTITY_INSPECTOR;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));
	
	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");
	
	ImGui::End();
}


void UIPanelManager::renderSimulationControlPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_SIMULATION_CONTROL;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}


void UIPanelManager::renderRenderSettingsPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_RENDER_SETTINGS;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}


void UIPanelManager::renderOrbitalPlannerPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ORBITAL_PLANNER;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}


void UIPanelManager::renderDebugConsole() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_CONSOLE;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}
