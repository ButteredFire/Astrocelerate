#pragma once

#include <mutex>
#include <atomic>
#include <condition_variable>

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

    struct MainThread {
        std::mutex haltMutex;
        std::condition_variable haltCV;
        std::atomic<bool> isHalted = false;
    } MainThread;
};

extern AppContext g_appContext;



struct FontContext {
    ImFont *primaryFont;

    struct NotoSans {
        ImFont *bold;
        ImFont *boldItalic;
        ImFont *italic;
        ImFont *light;
        ImFont *lightItalic;
        ImFont *regular;
        ImFont *regularMono;
    } NotoSans;
};

extern FontContext g_fontContext;