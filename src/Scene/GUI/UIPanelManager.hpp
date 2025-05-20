/* UIPanelManager.hpp - Manages the UI panels.
*/

#pragma once

#include <unordered_map>

#include <imgui/imgui.h>

#include <Core/ECS.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/LoggingManager.hpp>

#include <CoreStructs/GUI.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/WorldSpaceComponents.hpp>


class UIPanelManager {
public:
	UIPanelManager();
	~UIPanelManager() = default;

	/* Initializes panels from a panel mask bit-field. */
	void initPanelsFromMask(GUI::PanelMask& mask);


	/* Updates panels. */
	void updatePanels();


	const size_t FLAG_COUNT = sizeof(GUI::PanelFlagsArray) / sizeof(GUI::PanelFlagsArray[0]);

private:
	std::shared_ptr<Registry> m_registry;

	GUI::PanelMask m_panelMask;

	typedef void(UIPanelManager::*PanelCallback)();  // void(void) function pointer
	std::unordered_map<GUI::PanelFlag, PanelCallback> m_panelInitCallbacks;
	std::vector<PanelCallback> m_callbacks;


	/* Binds the panel flags to their respective initalization callback functions. */
	void bindPanelFlags();

	void renderPanelsMenu();

	void renderTelemetryPanel();
	void renderEntityInspectorPanel();
	void renderSimulationControlPanel();
	void renderRenderSettingsPanel();
	void renderOrbitalPlannerPanel();
	void renderDebugConsole();
};
