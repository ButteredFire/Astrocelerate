#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager() {

	m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);


	bindPanelFlags();

	// Set all bits in the panel mask to 1
	// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.set();
	m_callbacks.reserve(FLAG_COUNT);
	initPanelsFromMask(m_panelMask);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void UIPanelManager::initPanelsFromMask(GUI::PanelMask& mask) {
	using namespace GUI;

	for (size_t i = 0; i < FLAG_COUNT; i++) {
		PanelFlag flag = PanelFlagsArray[i];

		if (IsPanelOpen(mask, flag)) {
			auto it = m_panelInitCallbacks.find(flag);

			if (it == m_panelInitCallbacks.end() || 
				(it != m_panelInitCallbacks.end() && it->second == nullptr)
				) {

				Log::print(Log::T_ERROR, __FUNCTION__, "Cannot open panel: Initialization callback for panel flag #" + std::to_string(i) + " is not available!");

				continue;
			}

			m_callbacks.push_back(it->second);
		}
	}
}


void UIPanelManager::updatePanels() {
	renderPanelsMenu();

	for (PanelCallback callback : m_callbacks) {
		(this->*callback)();
	}
}


void UIPanelManager::bindPanelFlags() {
	using namespace GUI;

	m_panelInitCallbacks[PanelFlag::PANEL_TELEMETRY]			= &UIPanelManager::renderTelemetryPanel;
	m_panelInitCallbacks[PanelFlag::PANEL_ENTITY_INSPECTOR]		= &UIPanelManager::renderEntityInspectorPanel;
	m_panelInitCallbacks[PanelFlag::PANEL_SIMULATION_CONTROL]	= &UIPanelManager::renderSimulationControlPanel;
	m_panelInitCallbacks[PanelFlag::PANEL_RENDER_SETTINGS]		= &UIPanelManager::renderRenderSettingsPanel;
	m_panelInitCallbacks[PanelFlag::PANEL_ORBITAL_PLANNER]		= &UIPanelManager::renderOrbitalPlannerPanel;
	m_panelInitCallbacks[PanelFlag::PANEL_DEBUG_CONSOLE]		= &UIPanelManager::renderDebugConsole;
}


void UIPanelManager::renderPanelsMenu() {
	using namespace GUI;

	ImGui::Begin("Panels Menu");

	for (size_t i = 0; i < FLAG_COUNT; ++i) {
		PanelFlag flag = PanelFlagsArray[i];
		bool isOpen = IsPanelOpen(m_panelMask, flag);

		if (ImGui::Checkbox(GetPanelName(flag), &isOpen)) {
			togglePanel(m_panelMask, flag, TOGGLE_OFF);
		}
		else {
			togglePanel(m_panelMask, flag, TOGGLE_ON);
		}
	}

	ImGui::End();
}


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	auto view = m_registry->getView<Component::RigidBody, Component::ReferenceFrame>();

	for (const auto& [entity, rigidBody, refFrame] : view) {
		// --- Rigid-body Entity Debug Info ---
		if (ImGui::CollapsingHeader("Rigid-body Entity Debug Info")) {

			ImGui::Text("Entity ID #%d", entity);

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

			ImGui::Text("Entity ID #%d", entity);

			// Parent ID
			if (refFrame.parentID.has_value()) {
				ImGui::Text("\tParent Entity ID: %d", refFrame.parentID.value());
			}
			else {
				ImGui::Text("\tParent Entity ID: None");
			}

			// Local Transform
			ImGui::Text("\tLocal Transform:");
			ImGui::Text("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
				refFrame.localTransform.position.x,
				refFrame.localTransform.position.y,
				refFrame.localTransform.position.z);
			ImGui::Text("\t\t\tMagnitude: ||vec|| ~= %.2f", glm::length(refFrame.localTransform.position));

			ImGui::Text("\t\tRotation: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)",
				refFrame.localTransform.rotation.x,
				refFrame.localTransform.rotation.y,
				refFrame.localTransform.rotation.z,
				refFrame.localTransform.rotation.w);
			ImGui::Text("\t\tScale: %.10f",
				refFrame.localTransform.scale);

			// Global Transform
			ImGui::Text("\tGlobal Transform (normalized):");
			ImGui::Text("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
				refFrame.globalTransform.position.x,
				refFrame.globalTransform.position.y,
				refFrame.globalTransform.position.z);
			ImGui::Text("\t\t\tMagnitude: ||vec|| ~= %.2f", glm::length(refFrame.globalTransform.position));

			ImGui::Text("\t\tRotation: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)",
				refFrame.globalTransform.rotation.x,
				refFrame.globalTransform.rotation.y,
				refFrame.globalTransform.rotation.z,
				refFrame.globalTransform.rotation.w);
			ImGui::Text("\t\tScale: %.10f",
				refFrame.globalTransform.scale);

		}


		ImGui::Separator();
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
