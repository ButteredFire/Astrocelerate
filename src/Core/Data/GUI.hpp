/* GUI.hpp - Common data pertaining to the graphical user interface.  
*/  

#pragma once

#include <bitset>  
#include <vector>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <imgui/imgui.h>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/Constants.h>

class IWorkspace;


namespace GUI {
	// Panel callback function types. This allows for mapping between panel IDs and their respective render functions.
	typedef std::function<void(IWorkspace*)> PanelCallback;


	using PanelID = int32_t;
	constexpr PanelID PANEL_NULL = -1;		// The NULL panel is a hypothetical panel, and should NOT exist in any panel mask.
#define NULL_PANEL_CHECK(id, ...) if ((id) == PANEL_NULL) return __VA_ARGS__


	constexpr uint32_t MAX_PANEL_COUNT = 256;
	using PanelMask = std::bitset<MAX_PANEL_COUNT>;


	// Atomic counters are useful for thread-safe ID generation
		// NOTE: The s_ prefix is commonly used to indicate that a variable is static and private or internal.
	static std::atomic<PanelID>	s_nextPanelID = PANEL_NULL + 1;
	static std::unordered_map<std::string, PanelID> s_panelNameToID;
	static std::vector<std::string> s_panelIDToName;	// Indexed by panel ID
	static std::unordered_set<PanelID> s_instancedPanels;


	enum Toggle {  
		TOGGLE_ON,  
		TOGGLE_OFF  
	};



	/* Creates an ID for a new panel.
		By "Register", it is meant that the new panel will be registered into the global panel registry, NOT a panel mask, which is instance-based!

		@param panelName: The panel name.
		@param makeInstanced (Default: False): Whether to treat this panel as instanced (True), or persistent (False). An instanced panel is a conditional panel only accessible through certain events that give it data to render (think the Details panel), and cannot be opened anywhere else. A persistent panel is always accessible, regardless of whether it has data to render or not (think the Console/Telemetry panels).
		
		@return The newly registered panel's ID.
	*/
	inline PanelID RegisterPanel(const std::string &panelName, bool makeInstanced = false) {
		if (s_nextPanelID >= MAX_PANEL_COUNT) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Cannot register panel " + enquote(panelName) + ": Panel count exceeded the maximum of " + TO_STR(MAX_PANEL_COUNT) + "! The default NULL panel (ID: " + TO_STR(PANEL_NULL) + ") will be returned instead.");
			return PANEL_NULL;
		}

		if (s_panelNameToID.count(panelName)) {
			return s_panelNameToID[panelName];
		}

		PanelID newID = s_nextPanelID++;
		s_panelNameToID[panelName] = newID;

		if (s_panelIDToName.size() <= newID) {
			// Resize vector if necessary
			s_panelIDToName.resize(newID + 1);
		}

		s_panelIDToName[newID] = panelName;

		if (makeInstanced)
			s_instancedPanels.insert(newID);

		return newID;
	}


	/* Gets the panel name.
		@param panelID: The ID of the panel to get the name of.

		@return The name of the panel.
	*/
	inline const char *GetPanelName(PanelID panelID) {
		if (panelID > PANEL_NULL && panelID < s_panelIDToName.size()) {
			return s_panelIDToName[panelID].c_str();
		}

		Log::Print(Log::T_WARNING, __FUNCTION__, "Cannot get name for panel ID " + TO_STR(panelID) + ": Panel does not exist! A placeholder name will be returned instead. Please ensure the panel is registered.");
		return "Unknown Panel";
	}


	/* Is a panel currently open?   
		@param mask: The panel bit-field.  
		@param panelID: The ID of the panel to be checked.  

		@return True if the panel is open, False otherwise.  
	*/  
	inline bool IsPanelOpen(PanelMask& mask, PanelID panelID) {
		NULL_PANEL_CHECK(panelID, false);
		return mask.test(static_cast<size_t>(panelID));  
	}


	/* Is a panel an instanced/conditional panel?
		@param panelID: The ID of the panel to be checked.

		@return True if the panel is instanced, False otherwise.
	*/
	inline bool IsPanelInstanced(PanelID panelID) {
		return s_instancedPanels.count(panelID);
	}


	/* Toggles a panel on or off.  
		@param mask: The panel bit-field.  
		@param panelID: The ID of the panel to be toggled.  
		@param toggleMode: The toggle mode (on/off).  
	*/  
	inline void TogglePanel(PanelMask& mask, PanelID panelID, Toggle toggleMode) {  
		NULL_PANEL_CHECK(panelID);

		switch (toggleMode) {
		case TOGGLE_ON:
			mask.set(static_cast<size_t>(panelID));
			break;

		case TOGGLE_OFF:
			mask.reset(static_cast<size_t>(panelID));
			break;
		}  
	}


	// SERIALIZATION
	/* Serializes a panel mask.
		@param mask: The panel mask to be serialized.

		@return A compact string representation of the panel mask.
	*/
	inline std::string SerializePanelMask(const PanelMask &mask) {
		return STD_STR(mask.to_string()); // Compact string like "110010"
	}


	/* Deserializes a panel mask.
		@param str: The string representation of the panel mask.

		@return A PanelMask object.
	*/
	inline PanelMask DeserializePanelMask(const std::string &str) {
		return PanelMask(str);
	}
}