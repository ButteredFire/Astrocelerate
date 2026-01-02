#pragma once

#include <mutex>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <condition_variable>

#include <imgui/imgui.h>

#include <Scene/GUI/Appearance.hpp>


// General application context
struct AppContext {
    struct Config {
        std::string appearance_ColorTheme           = "DARK";

        uint32_t    debugging_MaxUIConsoleLines     = 1000;
        bool        debugging_ShowConsole           = false;
        bool        debugging_VkValidationLayers    = false;
        bool        debugging_VkAPIDump             = false;
    } Config;

    struct Input {
        bool isViewportHoveredOver  = false;
        bool isViewportFocused      = false;
    } Input;

    struct GUI {
        ImGuiTheme::Appearance currentAppearance = ImGuiTheme::Appearance::IMGUI_APPEARANCE_DARK_MODE;
    } GUI;

    struct MainThread {
        std::mutex haltMutex;
        std::condition_variable haltCV;
        std::atomic<bool> isHalted = false;

        std::atomic<std::chrono::steady_clock::time_point> heartbeatTimePoint;
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