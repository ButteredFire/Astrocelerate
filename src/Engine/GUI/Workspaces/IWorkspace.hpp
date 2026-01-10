/* IWorkspace.hpp - Defines an interface for all derived Workspace managers.
*/

#pragma once

#include <Engine/GUI/Data/GUI.hpp>

class IWorkspace {
public:
	virtual ~IWorkspace() = default;

	virtual void init() = 0;

	virtual void update(uint32_t currentFrame) = 0;
	virtual void preRenderUpdate(uint32_t currentFrame) = 0;

	virtual GUI::PanelMask& getPanelMask() = 0;
	virtual std::unordered_map<GUI::PanelID, GUI::PanelCallback>& getPanelCallbacks() = 0;

	virtual void loadSimulationConfig(const std::string &configPath) = 0;
	virtual void loadWorkspaceConfig(const std::string &configPath) = 0;

	virtual void saveSimulationConfig(const std::string &configPath) = 0;
	virtual void saveWorkspaceConfig(const std::string &configPath) = 0;
};