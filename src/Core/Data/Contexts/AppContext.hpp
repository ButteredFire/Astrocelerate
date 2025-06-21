#pragma once

#include <imgui/imgui.h>

#include <Scene/GUI/Appearance.hpp>


// General application context
struct AppContext {
    struct Input {
        bool isViewportHoveredOver = false;
        bool isViewportFocused = false;
    } Input;

    struct GUI {
        ImGuiTheme::Appearance currentAppearance = ImGuiTheme::Appearance::IMGUI_APPEARANCE_DARK_MODE;
    } GUI;
};

extern AppContext g_appContext;



struct FontContext {
    struct Roboto {
        ImFont* black;
        ImFont* blackItalic;
        ImFont* bold;
        ImFont* boldItalic;
        ImFont* italic;
        ImFont* light;
        ImFont* lightItalic;
        ImFont* medium;
        ImFont* mediumItalic;
        ImFont* regular;
        ImFont* thin;
        ImFont* thinItalic;
    } Roboto;
};

extern FontContext g_fontContext;