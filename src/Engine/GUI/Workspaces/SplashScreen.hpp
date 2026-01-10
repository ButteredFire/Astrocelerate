/* SplashScreen - Implementation of a splash screen.
*/

#pragma once

#include "IWorkspace.hpp"

#include <Engine/GUI/Data/GUI.hpp>
#include <Engine/Utils/ImGuiUtils.hpp>
#include <Engine/Utils/TextureUtils.hpp>
#include <Engine/Rendering/Textures/TextureManager.hpp>


class SplashScreen : public IWorkspace {
public:
	~SplashScreen() override = default;

	void init() override;

	inline void update(uint32_t currentFrame) override {};
	inline void preRenderUpdate(uint32_t currentFrame) override {};

	inline GUI::PanelMask &getPanelMask() override { return m_panelMask; }
	inline std::unordered_map<GUI::PanelID, GUI::PanelCallback> &getPanelCallbacks() override { return m_panelCallbacks; }

	// We don't need these functions for splash screens
	inline void loadSimulationConfig(const std::string &configPath) override {};
	inline void loadWorkspaceConfig(const std::string &configPath) override {};
	inline void saveSimulationConfig(const std::string &configPath) override {};
	inline void saveWorkspaceConfig(const std::string &configPath) override {};

private:
	GUI::PanelMask m_panelMask;
	GUI::PanelID m_panelSplash;

	std::unordered_map<GUI::PanelID, GUI::PanelCallback> m_panelCallbacks;

	TextureUtils::TextureProps m_splashTexture;

	void renderSplash();
};