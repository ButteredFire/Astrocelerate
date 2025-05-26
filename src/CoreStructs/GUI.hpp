/* GUI.hpp - Common data pertaining to the graphical user interface.  
*/  

#pragma once

#include <bitset>  
#include <unordered_map>


namespace GUI {  
	constexpr uint32_t MAX_PANEL_COUNT = 32;  
	using PanelMask = std::bitset<MAX_PANEL_COUNT>;  

	enum class PanelFlag {
		PANEL_VIEWPORT = 0,
		PANEL_TELEMETRY,  
		PANEL_ENTITY_INSPECTOR,  
		PANEL_SIMULATION_CONTROL,  
		PANEL_RENDER_SETTINGS,  
		PANEL_ORBITAL_PLANNER,  
		PANEL_DEBUG_CONSOLE  
	};

	inline constexpr PanelFlag PanelFlagsArray[] = {
		PanelFlag::PANEL_VIEWPORT,
		PanelFlag::PANEL_TELEMETRY,
		PanelFlag::PANEL_ENTITY_INSPECTOR,
		PanelFlag::PANEL_SIMULATION_CONTROL,
		PanelFlag::PANEL_RENDER_SETTINGS,
		PanelFlag::PANEL_ORBITAL_PLANNER,
		PanelFlag::PANEL_DEBUG_CONSOLE
	};


	const std::unordered_map<PanelFlag, std::string> PanelNames = {
		{PanelFlag::PANEL_VIEWPORT,					"Viewport"},
		{PanelFlag::PANEL_TELEMETRY,				"Telemetry Data"},
		{PanelFlag::PANEL_ENTITY_INSPECTOR,			"Entity Inspector"},
		{PanelFlag::PANEL_SIMULATION_CONTROL,		"Simulation Control Panel"},
		{PanelFlag::PANEL_RENDER_SETTINGS,			"Render Settings"},
		{PanelFlag::PANEL_ORBITAL_PLANNER,			"Orbital Planner"},
		{PanelFlag::PANEL_DEBUG_CONSOLE	,			"Debug Console"}
	};


	enum Toggle {  
		TOGGLE_ON,  
		TOGGLE_OFF  
	};



	/* Is a panel currently open?   
		@param mask: The panel bit-field.  
		@param panel: The panel to be checked.  

		@return True if the panel is open, otherwise False.  
	*/  
	inline bool IsPanelOpen(PanelMask& mask, PanelFlag panel) {  
		return mask.test(static_cast<size_t>(panel));  
	}  


	/* Toggles a panel on or off.  
		@param mask: The panel bit-field.  
		@param panel: The panel to be toggled.  
		@param toggleMode: The toggle mode (on/off).  
	*/  
	inline void TogglePanel(PanelMask& mask, PanelFlag panel, Toggle toggleMode) {  
		switch (toggleMode) {  
		case TOGGLE_ON:  
			mask.set(static_cast<size_t>(panel));  
			break;  

		case TOGGLE_OFF:  
			mask.reset(static_cast<size_t>(panel));  
			break;  
		}  
	}


	/* Gets the panel name.
		@param panel: The panel to get the name of.

		@return The name of the panel.
	*/
	inline const char* GetPanelName(PanelFlag panel) {
		auto it = PanelNames.find(panel);
		if (it != PanelNames.end()) {
			return it->second.c_str();
		}

		return "Unknown Panel";
	}


	// SERIALIZATION
	/* Serializes a panel mask.
		@param mask: The panel mask to be serialized.

		@return A compact string representation of the panel mask.
	*/
	inline std::string SerializePanelMask(const PanelMask& mask) {
		return mask.to_string(); // Compact string like "110010"
	}


	/* Deserializes a panel mask.
		@param str: The string representation of the panel mask.

		@return A PanelMask object.
	*/
	inline PanelMask DeserializePanelMask(const std::string& str) {
		return PanelMask(str);
	}
}