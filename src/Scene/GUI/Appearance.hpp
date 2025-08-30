/* Appearance.hpp - Defines color themes (e.g., Dark and Light mode themes).
*/

#pragma once

#include <unordered_map>

#include <imgui/imgui.h>

#include <Utils/ColorUtils.hpp>



namespace ImGuiTheme {
    enum class Appearance {
        IMGUI_APPEARANCE_DARK_MODE,
        IMGUI_APPEARANCE_LIGHT_MODE
    };
    inline constexpr Appearance AppearancesArray[] = {
        Appearance::IMGUI_APPEARANCE_DARK_MODE, Appearance::IMGUI_APPEARANCE_LIGHT_MODE
    };
    inline const std::unordered_map<Appearance, std::string> AppearanceNames = {
        {Appearance::IMGUI_APPEARANCE_DARK_MODE,        "Dark Mode"},
        {Appearance::IMGUI_APPEARANCE_LIGHT_MODE,       "Light Mode"}
    };


    // Color Palette Definitions
    namespace DarkPalette {
        // Grays
        inline const ImVec4 DARK_GRAY_100 = ColorUtils::sRGBToLinear(0.08f, 0.08f, 0.08f, 1.0f); // Very dark, almost black
        inline const ImVec4 DARK_GRAY_200 = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f); // Main dark background
        inline const ImVec4 DARK_GRAY_300 = ColorUtils::sRGBToLinear(0.12f, 0.12f, 0.12f, 1.0f); // Slightly lighter background
        inline const ImVec4 DARK_GRAY_400 = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f); // Frame/Child background
        inline const ImVec4 DARK_GRAY_500 = ColorUtils::sRGBToLinear(0.18f, 0.18f, 0.18f, 1.0f); // Frame/Tab background
        inline const ImVec4 DARK_GRAY_600 = ColorUtils::sRGBToLinear(0.20f, 0.20f, 0.20f, 1.0f); // Header/Button base
        inline const ImVec4 DARK_GRAY_700 = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 1.0f); // Hovered states, borders
        inline const ImVec4 DARK_GRAY_800 = ColorUtils::sRGBToLinear(0.28f, 0.28f, 0.28f, 1.0f); // Header hovered
        inline const ImVec4 DARK_GRAY_900 = ColorUtils::sRGBToLinear(0.30f, 0.30f, 0.30f, 1.0f); // Active states, scrollbar grab

        // Accent Blue
        inline const ImVec4 ACCENT_BLUE_DARK            = ColorUtils::sRGBToLinear(0.20f, 0.35f, 0.50f, 1.0f); // Base accent
        inline const ImVec4 ACCENT_BLUE_DARK_HOVER      = ColorUtils::sRGBToLinear(0.25f, 0.40f, 0.55f, 1.0f); // Lighter accent for hover
        inline const ImVec4 ACCENT_BLUE_DARK_ACTIVE     = ColorUtils::sRGBToLinear(0.30f, 0.45f, 0.60f, 1.0f); // Even lighter for active

        // Text Colors
        inline const ImVec4 CODE_EDITOR_KEYWORD     = ColorUtils::sRGBToLinear(0.208f, 0.945f, 1.0f, 1.0f);     // Aqua
        inline const ImVec4 CODE_EDITOR_NUMBER      = ColorUtils::sRGBToLinear(0.835f, 0.208f, 1.0f, 1.0f);     // Purple
        inline const ImVec4 CODE_EDITOR_STRING      = ColorUtils::sRGBToLinear(1.0f, 1.0f, 0.0f, 1.0f);         // Yellow
        inline const ImVec4 CODE_EDITOR_IDENTIFIER  = ColorUtils::sRGBToLinear(0.078f, 0.78f, 0.078f, 1.0f);    // Dark green
        inline const ImVec4 CODE_EDITOR_BREAKPOINT  = ColorUtils::sRGBToLinear(1.0f, 0.5f, 0.0f, 0.25f);        // Red, 50% alpha
        inline const ImVec4 CODE_EDITOR_ERROR_MARKER = ColorUtils::sRGBToLinear(1.0f, 0.0f, 0.0f, 0.5f);        // Orange, 25% alpha


        inline const ImVec4 TEXT_LIGHT = ColorUtils::sRGBToLinear(0.90f, 0.90f, 0.90f, 1.00f);
        inline const ImVec4 TEXT_MUTED = ColorUtils::sRGBToLinear(0.60f, 0.60f, 0.60f, 1.00f);
    }

    namespace LightPalette {
        // Grays/Off-whites
        inline const ImVec4 LIGHT_GRAY_100 = ColorUtils::sRGBToLinear(0.98f, 0.98f, 0.99f, 1.0f); // Very light, almost white
        inline const ImVec4 LIGHT_GRAY_200 = ColorUtils::sRGBToLinear(0.95f, 0.95f, 0.96f, 1.0f); // Child background
        inline const ImVec4 LIGHT_GRAY_300 = ColorUtils::sRGBToLinear(0.90f, 0.90f, 0.92f, 1.0f); // Main light background
        inline const ImVec4 LIGHT_GRAY_400 = ColorUtils::sRGBToLinear(0.85f, 0.85f, 0.88f, 1.0f); // Frame/Title background
        inline const ImVec4 LIGHT_GRAY_500 = ColorUtils::sRGBToLinear(0.80f, 0.80f, 0.82f, 1.0f); // Tab background
        inline const ImVec4 LIGHT_GRAY_600 = ColorUtils::sRGBToLinear(0.78f, 0.78f, 0.80f, 1.0f); // Active title/Unfocused active tab
        inline const ImVec4 LIGHT_GRAY_700 = ColorUtils::sRGBToLinear(0.75f, 0.80f, 0.85f, 1.0f); // Header base
        inline const ImVec4 LIGHT_GRAY_800 = ColorUtils::sRGBToLinear(0.70f, 0.70f, 0.70f, 1.0f); // Scrollbar grab, borders
        inline const ImVec4 LIGHT_GRAY_900 = ColorUtils::sRGBToLinear(0.60f, 0.60f, 0.60f, 1.0f); // Header hover, scrollbar hover

        // Accent Blue
        inline const ImVec4 ACCENT_BLUE_LIGHT           = ColorUtils::sRGBToLinear(0.30f, 0.50f, 0.70f, 1.0f); // Base accent
        inline const ImVec4 ACCENT_BLUE_LIGHT_HOVER     = ColorUtils::sRGBToLinear(0.35f, 0.55f, 0.75f, 1.0f); // Lighter accent for hover
        inline const ImVec4 ACCENT_BLUE_LIGHT_ACTIVE    = ColorUtils::sRGBToLinear(0.40f, 0.60f, 0.80f, 1.0f); // Even lighter for active

        // Text Colors
        inline const ImVec4 CODE_EDITOR_KEYWORD     = ColorUtils::sRGBToLinear(0.208f, 0.945f, 1.0f, 1.0f);     // Aqua
        inline const ImVec4 CODE_EDITOR_NUMBER      = ColorUtils::sRGBToLinear(0.835f, 0.208f, 1.0f, 1.0f);     // Purple
        inline const ImVec4 CODE_EDITOR_STRING      = ColorUtils::sRGBToLinear(1.0f, 1.0f, 0.0f, 1.0f);         // Yellow
        inline const ImVec4 CODE_EDITOR_IDENTIFIER  = ColorUtils::sRGBToLinear(0.078f, 0.78f, 0.078f, 1.0f);    // Dark green
        inline const ImVec4 CODE_EDITOR_BREAKPOINT  = ColorUtils::sRGBToLinear(1.0f, 0.5f, 0.0f, 0.25f);        // Red, 50% alpha
        inline const ImVec4 CODE_EDITOR_ERROR_MARKER = ColorUtils::sRGBToLinear(1.0f, 0.0f, 0.0f, 0.5f);        // Orange, 25% alpha
        
        inline const ImVec4 TEXT_DARK           = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.00f);
        inline const ImVec4 TEXT_MUTED_LIGHT    = ColorUtils::sRGBToLinear(0.50f, 0.50f, 0.50f, 1.00f);
    }


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
        colors.WindowBg = DarkPalette::DARK_GRAY_300;
        colors.MenuBarBg = DarkPalette::DARK_GRAY_300;
        colors.DockingEmptyBg = DarkPalette::DARK_GRAY_300;
        colors.ChildBg = DarkPalette::DARK_GRAY_400;
        colors.PopupBg = DarkPalette::DARK_GRAY_200; // Slightly darker for popups, using 0.94f alpha in ApplyTheme if desired

        // Headers
        colors.Header = DarkPalette::DARK_GRAY_600;
        colors.HeaderHovered = DarkPalette::DARK_GRAY_800;
        colors.HeaderActive = DarkPalette::DARK_GRAY_900;

        // Borders and separators
        colors.Border = ImVec4(DarkPalette::DARK_GRAY_700.x, DarkPalette::DARK_GRAY_700.y, DarkPalette::DARK_GRAY_700.z, 0.50f); // Adjust alpha if needed
        colors.BorderShadow = ColorUtils::sRGBToLinear(0, 0, 0, 0);

        // Text
        colors.Text = DarkPalette::TEXT_LIGHT;
        colors.TextDisabled = DarkPalette::TEXT_MUTED;

        // Frames
        colors.FrameBg = DarkPalette::DARK_GRAY_500;
        colors.FrameBgHovered = DarkPalette::DARK_GRAY_700;
        colors.FrameBgActive = DarkPalette::DARK_GRAY_900;

        // Buttons
        colors.Button = DarkPalette::ACCENT_BLUE_DARK;
        colors.ButtonHovered = DarkPalette::ACCENT_BLUE_DARK_HOVER;
        colors.ButtonActive = DarkPalette::ACCENT_BLUE_DARK_ACTIVE;

        // Scrollbars
        colors.ScrollbarBg = DarkPalette::DARK_GRAY_200;
        colors.ScrollbarGrab = DarkPalette::DARK_GRAY_900;
        colors.ScrollbarGrabHovered = ColorUtils::sRGBToLinear(0.40f, 0.40f, 0.40f, 1.0f); // Slightly unique, not directly from named palette
        colors.ScrollbarGrabActive = ColorUtils::sRGBToLinear(0.50f, 0.50f, 0.50f, 1.0f); // Slightly unique, not directly from named palette

        // Tabs
        colors.Tab = DarkPalette::DARK_GRAY_400;
        colors.TabHovered = DarkPalette::DARK_GRAY_700;
        colors.TabActive = DarkPalette::ACCENT_BLUE_DARK;
        colors.TabUnfocused = DarkPalette::DARK_GRAY_200;
        colors.TabUnfocusedActive = DarkPalette::DARK_GRAY_500;

        // Title bar
        colors.TitleBg = DarkPalette::DARK_GRAY_200;
        colors.TitleBgActive = DarkPalette::DARK_GRAY_400;
        colors.TitleBgCollapsed = ImVec4(DarkPalette::DARK_GRAY_100.x, DarkPalette::DARK_GRAY_100.y, DarkPalette::DARK_GRAY_100.z, 0.75f); // Adjust alpha if needed

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
        colors.WindowBg = LightPalette::LIGHT_GRAY_300;
        colors.MenuBarBg = LightPalette::LIGHT_GRAY_300;
        colors.DockingEmptyBg = LightPalette::LIGHT_GRAY_300;
        colors.ChildBg = LightPalette::LIGHT_GRAY_200;
        colors.PopupBg = LightPalette::LIGHT_GRAY_100; // Using 0.98f alpha in ApplyTheme if desired

        // Headers
        colors.Header = LightPalette::LIGHT_GRAY_700;
        colors.HeaderHovered = LightPalette::LIGHT_GRAY_900;
        colors.HeaderActive = ColorUtils::sRGBToLinear(0.55f, 0.60f, 0.65f, 1.0f); // Using a unique color here for variety

        // Borders and separators
        colors.Border = ImVec4(LightPalette::LIGHT_GRAY_800.x, LightPalette::LIGHT_GRAY_800.y, LightPalette::LIGHT_GRAY_800.z, 0.60f); // Adjust alpha if needed
        colors.BorderShadow = ColorUtils::sRGBToLinear(0, 0, 0, 0);

        // Text
        colors.Text = LightPalette::TEXT_DARK;
        colors.TextDisabled = LightPalette::TEXT_MUTED_LIGHT;

        // Frames
        colors.FrameBg = LightPalette::LIGHT_GRAY_400;
        colors.FrameBgHovered = ColorUtils::sRGBToLinear(0.80f, 0.80f, 0.83f, 1.0f); // Unique, not directly from named palette
        colors.FrameBgActive = ColorUtils::sRGBToLinear(0.75f, 0.75f, 0.78f, 1.0f); // Unique, not directly from named palette

        // Buttons
        colors.Button = LightPalette::ACCENT_BLUE_LIGHT;
        colors.ButtonHovered = LightPalette::ACCENT_BLUE_LIGHT_HOVER;
        colors.ButtonActive = LightPalette::ACCENT_BLUE_LIGHT_ACTIVE;

        // Scrollbars
        colors.ScrollbarBg = LightPalette::LIGHT_GRAY_300;
        colors.ScrollbarGrab = LightPalette::LIGHT_GRAY_800;
        colors.ScrollbarGrabHovered = LightPalette::LIGHT_GRAY_900;
        colors.ScrollbarGrabActive = LightPalette::LIGHT_GRAY_900; // Same as hover for consistency

        // Tabs
        colors.Tab = LightPalette::LIGHT_GRAY_500;
        colors.TabHovered = LightPalette::LIGHT_GRAY_700;
        colors.TabActive = LightPalette::ACCENT_BLUE_LIGHT;
        colors.TabUnfocused = LightPalette::LIGHT_GRAY_400;
        colors.TabUnfocusedActive = LightPalette::LIGHT_GRAY_600;

        // Title bar
        colors.TitleBg = LightPalette::LIGHT_GRAY_400;
        colors.TitleBgActive = LightPalette::LIGHT_GRAY_600;
        colors.TitleBgCollapsed = ImVec4(LightPalette::LIGHT_GRAY_300.x, LightPalette::LIGHT_GRAY_300.y, LightPalette::LIGHT_GRAY_300.z, 0.90f);

        // Resize grip (bottom-right corner)
        colors.ResizeGrip = ColorUtils::sRGBToLinear(0.0f, 0.0f);
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
