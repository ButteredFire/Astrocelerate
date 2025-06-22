/* ImGuiUtils.hpp - Utilities pertaining to the ImGui GUI.
*/

#pragma once

#include <map>

#include <imgui/imgui.h>

#include <Core/Application/LoggingManager.hpp>

#include <Utils/SystemUtils.hpp>



namespace ImGuiUtils {
	// ----- COMPUTATION -----

	/* Gets the available width of a line.
		@param includePadding (Default: True): Should padding/spacing be included in the final width value?

		@return The available width.
	*/
	inline float GetAvailableWidth(bool includePadding = true) {
		const float totalWidth = ImGui::GetContentRegionAvail().x;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;

		return (includePadding) ? (totalWidth - spacing) : (totalWidth);
	}


	/* Calculates the minimum area height needed for buttons at the bottom of the panel.
		@param rowCount (Default: 1): The expected number of button rows at the bottom.
	
		@return The area height.
	*/
	inline float GetBottomButtonAreaHeight(uint32_t rowCount = 1) {
		const float buttonHeight = ImGui::GetFrameHeight();
		const float verticalPadding = ImGui::GetStyle().ItemSpacing.y * 2.0f;

		return (buttonHeight + verticalPadding) * rowCount;
	}



	/* Resizes an image relative to its parent's size so as to preserve its aspect ratio.
		@param imgSize: The size of the image to be resized.
		@param viewportSize: The size of the viewport (parent) to which the image is resized.

		@return The resized image size.
	*/
	inline ImVec2 ResizeImagePreserveAspectRatio(const ImVec2& imgSize, ImVec2& viewportSize) {
		ImVec2 textureSize{};

		const float renderAspect = static_cast<float>(imgSize.x) / imgSize.y;
		const float panelAspect = viewportSize.x / viewportSize.y;

		if (panelAspect > renderAspect) {
			// Panel is wider than the render target
			textureSize.x = viewportSize.y * renderAspect;
			textureSize.y = viewportSize.y;
		}
		else {
			// Panel is taller than the render target
			textureSize.x = viewportSize.x;
			textureSize.y = viewportSize.x / renderAspect;
		}

		return textureSize;
	};



	// ----- TEXT FORMATTING -----

	/* Emboldens text.
		@param fmt: The (formatted) text to be emboldened.
		@param ...: The arguments for the formatted text.
	*/
    inline void BoldText(const char* fmt, ...) {
		LOG_ASSERT(g_fontContext.Roboto.bold, "Cannot embolden text " + enquote(fmt) + ": The bold font has not been loaded!");
        ImGui::PushFont(g_fontContext.Roboto.bold);
        va_list args;  // Handles variadic arguments
        va_start(args, fmt);
        ImGui::TextWrappedV(fmt, args);   // ImGui::TextWrappedV is the variadic version of ImGui::TextWrapped
        va_end(args);
        ImGui::PopFont();
    }


	/* Italicizes text.
		@param fmt: The (formatted) text to be italicized.
		@param ...: The arguments for the formatted text.
	*/
	inline void ItalicText(const char* fmt, ...) {
		LOG_ASSERT(g_fontContext.Roboto.italic, "Cannot italicize text " + enquote(fmt) + ": The italic font has not been loaded!");
		ImGui::PushFont(g_fontContext.Roboto.italic);
		va_list args;
		va_start(args, fmt);
		ImGui::TextWrappedV(fmt, args);
		va_end(args);
		ImGui::PopFont();
	}


	/* Underlines text.
		@param fmt: The (formatted) text to be underlined.
		@param ...: The arguments for the formatted text.
	*/
	inline void UnderlinedText(const char* fmt, ...) {
		LOG_ASSERT(g_fontContext.Roboto.lightItalic, "Cannot underline text " + enquote(fmt) + ": The light-italic font has not been loaded!");
		ImGui::PushFont(g_fontContext.Roboto.lightItalic);
		va_list args;
		va_start(args, fmt);
		ImGui::TextWrappedV(fmt, args);
		va_end(args);
		ImGui::PopFont();
	}


	/* Renders light text.
		@param fmt: The (formatted) text to be emboldened.
		@param ...: The arguments for the formatted text.
	*/
	inline void LightText(const char* fmt, ...) {
		LOG_ASSERT(g_fontContext.Roboto.light, "Cannot render light text " + enquote(fmt) + ": The bold font has not been loaded!");
		ImGui::PushFont(g_fontContext.Roboto.light);
		va_list args;
		va_start(args, fmt);
		ImGui::TextWrappedV(fmt, args);
		va_end(args);
		ImGui::PopFont();
	}


	/* Displays a tooltip with the given text.
		@param fmt: The (formatted) text to be displayed in the tooltip.
		@param ...: The arguments for the formatted text.
	*/
	inline void Tooltip(const char* fmt, ...) {
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			va_list args;  // Handles variadic arguments
			va_start(args, fmt);
			ImGui::TextWrappedV(fmt, args);
			va_end(args);
			ImGui::EndTooltip();
		}
	}



	// ----- CUSTOM ELEMENTS -----

	/* A component field for a multi-component container (e.g., 3-component vector, quaternion).
		@param components: A map of component labels and their values (e.g., { {"X", 0.0f}, {"Y", 5.0f} }).
		@param componentFormat: The format string for the component values (e.g., "%.1f").
		@param fmt: The formatted text to be displayed above the component fields. It can be left empty if only the component field itself is desired.
		@param ...: The arguments for the formatted text.
	*/
	inline void ComponentField(const std::map<const char*, float> components, const char *componentFormat, const char* fmt = "", ...) {
		LOG_ASSERT(components.size() > 0, "Cannot render component field: There are no components to be rendered!");
		
		// Since the baselines of ImGui::Text* and ImGui::InputFloat are not always aligned, this can cause text misalignment issues.
		// Therefore, we need to align everything to the frame padding beforehand so that everyhing is rendered at the same baseline.
		ImGui::AlignTextToFramePadding();
		
		// Only renders the formatted text if it is not empty.
		bool mainTextIsNotEmpty = (fmt && fmt[0] != '\0');
		if (mainTextIsNotEmpty) {
			std::string finalText = fmt + std::string(": ");

			va_list args;
			va_start(args, finalText.c_str());
			ImGui::TextWrappedV(finalText.c_str(), args);
			va_end(args);
			ImGui::SameLine();
		}


		// Generates a random throwaway string to be used as the ID
		// TODO: Implement a more robust ID generation method. This is currently very hacky, and there are still risks of ID conflicts, however small statistically.
		std::string fieldID = SystemUtils::GenerateRandomString(100) + componentFormat;
		if (mainTextIsNotEmpty) fieldID += fmt;


		// Calculates available width to fit all components in one line
		static float totalLabelSize = 0.0f;
		static bool computedLabelSize = false;
		if (!computedLabelSize) {
			computedLabelSize = true;

			for (const auto& [label, _] : components) {
				totalLabelSize += ImGui::CalcTextSize(label).x;
				fieldID += label;
			}
		}

		float componentWidth = GetAvailableWidth() / static_cast<float>(components.size()) - totalLabelSize;


		size_t counter = 0;
		size_t hashValue = std::hash<std::string>{}(fieldID); // TODO
		ImGui::PushID(hashValue);
		for (const auto& [label, value] : components) {
			if (counter > 0) ImGui::SameLine();

			ImGui::Text(label);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(componentWidth);

			char buffer[64];
			snprintf(buffer, sizeof(buffer), componentFormat, value);
			ImGui::InputText(("##" + std::to_string(counter)).c_str(), buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);

			// If the text is too long, use a tooltip to show the full text
			bool textIsTooLong = (ImGui::CalcTextSize(buffer).x > componentWidth);
			if (textIsTooLong && ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(buffer);
				ImGui::EndTooltip();
			}

			counter++;
		}
		ImGui::PopID();
	}


	/* A separator with top- and bottom-padding. 
		@param padding (Default: 10.0f): The padding to use for the separator.
	*/
	inline void PaddedSeparator(float padding = 10.0f) {
		const ImVec2 paddingVec = { padding, padding };

		ImGui::Dummy(paddingVec);
		ImGui::Separator();
		ImGui::Dummy(paddingVec);
	}


	/* Padding.
		@param padding (Default: 15.0f): The padding scalar value.
	*/
	inline void Padding(float padding = 15.0f) {
		ImGui::Dummy({ padding, padding });
	}


	/* Initial padding to push buttons at the end of a panel to the bottom-right corner.
		@param btnWidth: The expected width of the button(s), which should be homogenous. If the buttons have different widths, it is safest to use the largest-width button's width for this parameter.
		@param btnCount: The expected number of buttons.
		@param paddingRight (Default: 30.0f): The padding to the right of the rightmost button (i.e., the space between the rightmost button and its parent panel's border).
	*/
	inline void BottomButtonPadding(float btnWidth, uint32_t btnCount, float paddingRight = 30.0f) {
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y); // Adds a little vertical space
		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - paddingRight - btnWidth * btnCount, 0.0f)); // Pushes buttons to the right
		ImGui::SameLine();
	}
}