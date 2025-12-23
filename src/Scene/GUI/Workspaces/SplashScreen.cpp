#include "SplashScreen.hpp"


void SplashScreen::init() {
	std::shared_ptr<TextureManager> textureManager = ServiceLocator::GetService<TextureManager>(__FUNCTION__);

	// Splash image
	Geometry::Texture texture = textureManager->createIndependentTexture(ResourcePath::App.SPLASH, VK_FORMAT_R8G8B8A8_SRGB);

	m_splashTexture.size = ImVec2(texture.size.x, texture.size.y);
	m_splashTexture.textureID = TextureUtils::GenerateImGuiTextureID(texture.imageLayout, texture.imageView, texture.sampler);


	// Splash panel
	m_panelSplash = GUI::RegisterPanel("Splash");
	m_panelCallbacks[m_panelSplash] = [this](IWorkspace *ws) { static_cast<SplashScreen*>(ws)->renderSplash(); };
	
	m_panelMask.reset();
	GUI::TogglePanel(m_panelMask, m_panelSplash, GUI::TOGGLE_ON);
}


void SplashScreen::renderSplash() {
	// Ensure splash panel takes up the entire viewport
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);


	// Set all padding/rounding to zero so the image starts exactly at the top-left of the window
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	{
		ImGuiWindowFlags splashWindowFlags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse;


		if (ImGui::Begin(GUI::GetPanelName(m_panelSplash), nullptr, splashWindowFlags)) {
			ImVec2 availableSpace = ImGui::GetContentRegionAvail();
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();

			// Splash image
			ImGui::Image(m_splashTexture.textureID, m_splashTexture.size);

			// Splash text
			static constexpr float PADDING_X = 50.0f, PADDING_Y = 50.0f, FONT_SCALE = 1.25f;

			static const std::vector<std::string> text = {
				"Version " + std::string(APP_VERSION),
				"Open-sourced under Apache License 2.0"
				// TODO: Set loading text here...
			};

			ImGui::SetWindowFontScale(FONT_SCALE);
			const float lineHeight = ImGui::GetFontSize();
			{
				for (int i = 0; i < text.size(); i++)
					ImGuiUtils::FloatingText(
						ImVec2(cursorPos.x + (availableSpace.x - PADDING_X - ImGui::CalcTextSize(text[i].c_str()).x),
							cursorPos.y + PADDING_Y + lineHeight * i
						),
						text[i]
					);
			}
			ImGui::SetWindowFontScale(1.0f);


			ImGui::End();
		}
	}
	ImGui::PopStyleVar(3);
}