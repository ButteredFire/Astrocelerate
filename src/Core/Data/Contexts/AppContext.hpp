#pragma once

#include <mutex>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <condition_variable>


// General application context
struct AppContext {
    struct Config {
        std::string appearance_ColorTheme           = "DARK";

        uint32_t    debugging_MaxUIConsoleLines     = 1000;
        bool        debugging_ShowConsole           = false;
        bool        debugging_VkValidationLayers    = false;
        bool        debugging_VkAPIDump             = false;
    } Config;

    struct MainThread {
        std::mutex haltMutex;
        std::condition_variable haltCV;
        std::atomic<bool> isHalted = false;

        std::atomic<std::chrono::steady_clock::time_point> heartbeatTimePoint;
    } MainThread;
};

extern AppContext g_appCtx;
