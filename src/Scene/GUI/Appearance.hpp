/* Appearance.hpp - Defines color themes (e.g., Dark and Light mode themes).
*/

#pragma once

#include <imgui/imgui.h>

#include <Utils/ColorUtils.hpp>



namespace ImGuiTheme {
    enum class Appearance {
        IMGUI_APPEARANCE_DARK_MODE,
        IMGUI_APPEARANCE_LIGHT_MODE
    };


    // Holds all colors for a specific theme
    struct ThemeColors {
        ImVec4 WindowBg;
        ImVec4 MenuBarBg;
        ImVec4 DockingEmptyBg;
        ImVec4 ChildBg;
        ImVec4 PopupBg;

        ImVec4 Header;
        ImVec4 HeaderHovered;
        ImVec4 HeaderActive;

        ImVec4 Border;
        ImVec4 BorderShadow;

        ImVec4 Text;
        ImVec4 TextDisabled;

        ImVec4 FrameBg;
        ImVec4 FrameBgHovered;
        ImVec4 FrameBgActive;

        ImVec4 Button;
        ImVec4 ButtonHovered;
        ImVec4 ButtonActive;

        ImVec4 ScrollbarBg;
        ImVec4 ScrollbarGrab;
        ImVec4 ScrollbarGrabHovered;
        ImVec4 ScrollbarGrabActive;

        ImVec4 Tab;
        ImVec4 TabHovered;
        ImVec4 TabActive;
        ImVec4 TabUnfocused;
        ImVec4 TabUnfocusedActive;

        ImVec4 TitleBg;
        ImVec4 TitleBgActive;
        ImVec4 TitleBgCollapsed;

        ImVec4 ResizeGrip;
        ImVec4 ResizeGripHovered;
        ImVec4 ResizeGripActive;
    };


    /* Defines color themes for Dark Mode.
        @return Dark Mode color themes.
    */
    inline ThemeColors GetDarkThemeColors() {
        ThemeColors colors;

        // Backgrounds
        colors.WindowBg = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f);
        colors.MenuBarBg = colors.WindowBg;
        colors.DockingEmptyBg = colors.WindowBg;
        colors.ChildBg = ColorUtils::sRGBToLinear(0.12f, 0.12f, 0.12f, 1.0f);
        colors.PopupBg = ColorUtils::sRGBToLinear(0.08f, 0.08f, 0.08f, 0.94f);

        // Headers
        colors.Header = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 1.0f);
        colors.HeaderHovered = ColorUtils::sRGBToLinear(0.30f, 0.30f, 0.30f, 1.0f);
        colors.HeaderActive = ColorUtils::sRGBToLinear(0.35f, 0.35f, 0.35f, 1.0f);

        // Borders and separators
        colors.Border = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 0.50f);
        colors.BorderShadow = ColorUtils::sRGBToLinear(0, 0, 0, 0);

        // Text
        colors.Text = ColorUtils::sRGBToLinear(0.95f, 0.96f, 0.98f, 1.00f);
        colors.TextDisabled = ColorUtils::sRGBToLinear(0.50f, 0.50f, 0.50f, 1.00f);

        // Frames
        colors.FrameBg = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f);
        colors.FrameBgHovered = ColorUtils::sRGBToLinear(0.20f, 0.20f, 0.20f, 1.0f);
        colors.FrameBgActive = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 1.0f);

        // Buttons
        colors.Button = ColorUtils::sRGBToLinear(0.20f, 0.22f, 0.27f, 1.0f);
        colors.ButtonHovered = ColorUtils::sRGBToLinear(0.30f, 0.33f, 0.38f, 1.0f);
        colors.ButtonActive = ColorUtils::sRGBToLinear(0.35f, 0.40f, 0.45f, 1.0f);

        // Scrollbars
        colors.ScrollbarBg = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f);
        colors.ScrollbarGrab = ColorUtils::sRGBToLinear(0.20f, 0.25f, 0.30f, 1.0f);
        colors.ScrollbarGrabHovered = ColorUtils::sRGBToLinear(0.25f, 0.30f, 0.35f, 1.0f);
        colors.ScrollbarGrabActive = ColorUtils::sRGBToLinear(0.30f, 0.35f, 0.40f, 1.0f);

        // Tabs
        colors.Tab = ColorUtils::sRGBToLinear(0.18f, 0.18f, 0.18f, 1.0f);
        colors.TabHovered = ColorUtils::sRGBToLinear(0.26f, 0.26f, 0.26f, 1.0f);
        colors.TabActive = ColorUtils::sRGBToLinear(0.28f, 0.28f, 0.28f, 1.0f);
        colors.TabUnfocused = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f);
        colors.TabUnfocusedActive = ColorUtils::sRGBToLinear(0.20f, 0.20f, 0.20f, 1.0f);

        // Title bar
        colors.TitleBg = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f);
        colors.TitleBgActive = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f);
        colors.TitleBgCollapsed = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 0.75f);

        // Resize grips (bottom-right corner)
        colors.ResizeGrip = ColorUtils::sRGBToLinear(0.0f, 0.0f);
        colors.ResizeGripHovered = ColorUtils::sRGBToLinear(0.0f, 0.0f);
        colors.ResizeGripActive = ColorUtils::sRGBToLinear(0.0f, 0.0f);


        return colors;
    }

    
    /* Defines color themes for Light Mode.
        @return Light Mode color themes.
    */
    inline ThemeColors GetLightThemeColors() {
        ThemeColors colors;

        // Background
        colors.WindowBg = ColorUtils::sRGBToLinear(0.95f, 0.95f, 0.95f, 1.0f); // Very light gray
        colors.MenuBarBg = colors.WindowBg;
        colors.DockingEmptyBg = colors.WindowBg;
        colors.ChildBg = ColorUtils::sRGBToLinear(0.90f, 0.90f, 0.90f, 1.0f); // Slightly darker light gray
        colors.PopupBg = ColorUtils::sRGBToLinear(0.98f, 0.98f, 0.98f, 0.94f); // Almost white
        
        // Headers
        colors.Header = ColorUtils::sRGBToLinear(0.70f, 0.70f, 0.70f, 1.0f);
        colors.HeaderHovered = ColorUtils::sRGBToLinear(0.60f, 0.60f, 0.60f, 1.0f);
        colors.HeaderActive = ColorUtils::sRGBToLinear(0.50f, 0.50f, 0.50f, 1.0f);

        // Borders and separators
        colors.Border = ColorUtils::sRGBToLinear(0.75f, 0.75f, 0.75f, 0.50f);
        colors.BorderShadow = ColorUtils::sRGBToLinear(0, 0, 0, 0);

        // Text
        colors.Text = ColorUtils::sRGBToLinear(0.0f, 0.0f, 0.0f, 1.00f);
        colors.TextDisabled = ColorUtils::sRGBToLinear(0.50f, 0.50f, 0.50f, 1.00f);

        // Frames
        colors.FrameBg = ColorUtils::sRGBToLinear(0.85f, 0.85f, 0.85f, 1.0f);
        colors.FrameBgHovered = ColorUtils::sRGBToLinear(0.80f, 0.80f, 0.80f, 1.0f);
        colors.FrameBgActive = ColorUtils::sRGBToLinear(0.75f, 0.75f, 0.75f, 1.0f);

        // Buttons
        colors.Button = ColorUtils::sRGBToLinear(0.70f, 0.72f, 0.77f, 1.0f);
        colors.ButtonHovered = ColorUtils::sRGBToLinear(0.60f, 0.63f, 0.68f, 1.0f);
        colors.ButtonActive = ColorUtils::sRGBToLinear(0.55f, 0.60f, 0.65f, 1.0f);

        // Scrollbars
        colors.ScrollbarBg = ColorUtils::sRGBToLinear(0.90f, 0.90f, 0.90f, 1.0f);
        colors.ScrollbarGrab = ColorUtils::sRGBToLinear(0.70f, 0.75f, 0.80f, 1.0f);
        colors.ScrollbarGrabHovered = ColorUtils::sRGBToLinear(0.65f, 0.70f, 0.75f, 1.0f);
        colors.ScrollbarGrabActive = ColorUtils::sRGBToLinear(0.60f, 0.65f, 0.70f, 1.0f);

        // Tabs
        colors.Tab = ColorUtils::sRGBToLinear(0.82f, 0.82f, 0.82f, 1.0f);
        colors.TabHovered = ColorUtils::sRGBToLinear(0.74f, 0.74f, 0.74f, 1.0f);
        colors.TabActive = ColorUtils::sRGBToLinear(0.72f, 0.72f, 0.72f, 1.0f);
        colors.TabUnfocused = ColorUtils::sRGBToLinear(0.85f, 0.85f, 0.85f, 1.0f);
        colors.TabUnfocusedActive = ColorUtils::sRGBToLinear(0.80f, 0.80f, 0.80f, 1.0f);

        // Title bar
        colors.TitleBg = ColorUtils::sRGBToLinear(0.90f, 0.90f, 0.90f, 1.0f);
        colors.TitleBgActive = ColorUtils::sRGBToLinear(0.85f, 0.85f, 0.85f, 1.0f);
        colors.TitleBgCollapsed = ColorUtils::sRGBToLinear(0.90f, 0.90f, 0.90f, 0.75f);

        // Resize grip (bottom-right corner)
        colors.ResizeGrip = ColorUtils::sRGBToLinear(0.0f,0.0f);
        colors.ResizeGripHovered = ColorUtils::sRGBToLinear(0.0f, 0.0f);
        colors.ResizeGripActive = ColorUtils::sRGBToLinear(0.0f, 0.0f);

        return colors;
    }

    
    /* Applies a theme to change the GUI appearance. */
    inline void ApplyTheme(const Appearance& appearance) {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        ThemeColors theme;

        switch (appearance) {
        case Appearance::IMGUI_APPEARANCE_DARK_MODE:
            theme = GetDarkThemeColors();
            break;

        case Appearance::IMGUI_APPEARANCE_LIGHT_MODE:
            theme = GetLightThemeColors();
            break;
        }

        colors[ImGuiCol_WindowBg] = theme.WindowBg;
        colors[ImGuiCol_MenuBarBg] = theme.MenuBarBg;
        style.Colors[ImGuiCol_DockingEmptyBg] = theme.DockingEmptyBg;
        colors[ImGuiCol_ChildBg] = theme.ChildBg;
        colors[ImGuiCol_PopupBg] = theme.PopupBg;

        colors[ImGuiCol_Header] = theme.Header;
        colors[ImGuiCol_HeaderHovered] = theme.HeaderHovered;
        colors[ImGuiCol_HeaderActive] = theme.HeaderActive;

        colors[ImGuiCol_Border] = theme.Border;
        colors[ImGuiCol_BorderShadow] = theme.BorderShadow;

        colors[ImGuiCol_Text] = theme.Text;
        colors[ImGuiCol_TextDisabled] = theme.TextDisabled;

        colors[ImGuiCol_FrameBg] = theme.FrameBg;
        colors[ImGuiCol_FrameBgHovered] = theme.FrameBgHovered;
        colors[ImGuiCol_FrameBgActive] = theme.FrameBgActive;

        colors[ImGuiCol_Button] = theme.Button;
        colors[ImGuiCol_ButtonHovered] = theme.ButtonHovered;
        colors[ImGuiCol_ButtonActive] = theme.ButtonActive;

        colors[ImGuiCol_ScrollbarBg] = theme.ScrollbarBg;
        colors[ImGuiCol_ScrollbarGrab] = theme.ScrollbarGrab;
        colors[ImGuiCol_ScrollbarGrabHovered] = theme.ScrollbarGrabHovered;
        colors[ImGuiCol_ScrollbarGrabActive] = theme.ScrollbarGrabActive;

        colors[ImGuiCol_Tab] = theme.Tab;
        colors[ImGuiCol_TabHovered] = theme.TabHovered;
        colors[ImGuiCol_TabActive] = theme.TabActive;
        colors[ImGuiCol_TabUnfocused] = theme.TabUnfocused;
        colors[ImGuiCol_TabUnfocusedActive] = theme.TabUnfocusedActive;

        colors[ImGuiCol_TitleBg] = theme.TitleBg;
        colors[ImGuiCol_TitleBgActive] = theme.TitleBgActive;
        colors[ImGuiCol_TitleBgCollapsed] = theme.TitleBgCollapsed;

        colors[ImGuiCol_ResizeGrip] = theme.ResizeGrip;
        colors[ImGuiCol_ResizeGripHovered] = theme.ResizeGripHovered;
        colors[ImGuiCol_ResizeGripActive] = theme.ResizeGripActive;
    }
}
