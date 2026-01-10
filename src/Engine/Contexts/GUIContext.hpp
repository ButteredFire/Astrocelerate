#pragma once


#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <Engine/GUI/Data/Appearance.hpp>


struct GUIContext {
    ImFont *primaryFont;

    struct Input {
        bool isViewportHoveredOver = false;
        bool isViewportFocused = false;
    } Input;

    struct GUI {
        ImGuiTheme::Appearance currentAppearance = ImGuiTheme::Appearance::IMGUI_APPEARANCE_DARK_MODE;
    } GUI;

    struct Font {
        ImFont *bold;
        ImFont *boldItalic;
        ImFont *italic;
        ImFont *light;
        ImFont *lightItalic;
        ImFont *regular;
        ImFont *regularMono;
    } Font;   // Noto Sans
};


extern GUIContext g_guiCtx;