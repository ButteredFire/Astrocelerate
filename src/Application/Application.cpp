/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boxer/boxer.h>
#include <nlohmann/json.hpp>

#include <Application/Engine.hpp>
#include <Application/Session.hpp>

#include <Core/Data/Constants.h>
#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Data/Contexts/CallbackContext.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>


#if _WIN32
    #include <windows.h>
#endif


using json = nlohmann::json;


void cleanupAll() {
    std::shared_ptr<CleanupManager> cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);
    cleanupManager->cleanupAll();
}


int processAppConfig(int argc, char *argv[]) {
    std::unordered_set<std::string> args;

    for (int i = 0; i < argc; i++)
        args.insert(std::string(argv[i]));
    

    std::function <bool(json, std::string, std::string) > getConfigOrArgVal = [args](json appConfig, std::string k1, std::string k2) {
        return (args.count(k2)) ? true : appConfig[k1][k2].get<bool>();
    };



    try {
        std::ifstream f(ResourcePath::App.CONFIG_APP);
        json appConfig = json::parse(f);

        g_appCtx.Config.appearance_ColorTheme           = appConfig["Appearance"]["ColorTheme"].get<std::string>();

        g_appCtx.Config.debugging_MaxUIConsoleLines     = appConfig["Debugging"]["MaxUIConsoleLines"].get<uint32_t>();
        g_appCtx.Config.debugging_ShowConsole           = getConfigOrArgVal(appConfig, "Debugging", "ShowWindowConsole");//appConfig["Debugging"]["ShowWindowConsole"].get<bool>();
        g_appCtx.Config.debugging_VkValidationLayers    = getConfigOrArgVal(appConfig, "Debugging", "VkValidationLayers");//appConfig["Debugging"]["VkValidationLayers"].get<bool>();
        g_appCtx.Config.debugging_VkAPIDump             = getConfigOrArgVal(appConfig, "Debugging", "VkAPIDump");//appConfig["Debugging"]["VkAPIDump"].get<bool>();
    }
    catch (const json::parse_error &parseErr) {
        boxer::show(("Cannot start Astrocelerate: Unable to parse file " + enquote(ResourcePath::App.CONFIG_APP) + ".\n\nParser error: " + parseErr.what()).c_str(), "Configuration Error", boxer::Style::Error, boxer::Buttons::Quit);

        return EXIT_FAILURE;
    }
    catch (const json::type_error &typeErr) {
        boxer::show(("Cannot start Astrocelerate: Type error present in configuration file " + enquote(ResourcePath::App.CONFIG_APP) + ".\n\nParser error: " + typeErr.what()).c_str(), "Configuration Error", boxer::Style::Error, boxer::Buttons::Quit);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


void tryOpenConsole() {
#if _WIN32
    // If there is no console window yet and `showConsole` is true, allocate a new console window and redirect standard streams to it
    if (GetConsoleWindow() == NULL && g_appCtx.Config.debugging_ShowConsole) {
        if (AllocConsole()) {
            FILE *fDummy;
            freopen_s(&fDummy, "CONOUT$", "w", stdout);
            freopen_s(&fDummy, "CONOUT$", "w", stderr);
            freopen_s(&fDummy, "CONIN$", "r", stdin);

            // Clear the error state of cin/cout/cerr
            std::cout.clear();
            std::cerr.clear();
            std::cin.clear();
        }
    }
#endif
}


void checkVulkanLoader() {
    SystemUtils::VkLoaderDiag vkLoaderDiag = SystemUtils::VulkanLoaderExists();
    std::string vkLoader = VULKAN_LOADER;
    std::string vkLoaderErrMsg;

    switch (vkLoaderDiag.errType) {
    case SystemUtils::VkLoaderDiag::CANNOT_BE_LOCATED:
#if _WIN32
        vkLoaderErrMsg = vkLoader + " could not be located! This file is distributed as part of your GPU driver. Please reinstall your GPU drivers from your manufacturer and ensure that " + vkLoader + " is available in C:/Windows/System32 or in the application's bin/ directory.";
#elif defined(__linux__)
        vkLoaderErrMsg = vkLoader + " could not be located! Please reinstall your GPU driver package and ensure that " + vkLoader + " is available in your runtime linker search path (e.g., /usr/lib, /usr/lib64).";
#elif defined(__APPLE__)
        vkLoaderErrMsg = vkLoader + " could not be located! Please reinstall your Vulkan runtime/MoltenVK package and ensure that " + vkLoader + " is available in your dynamic library search path.";
#endif
        break;

    case SystemUtils::VkLoaderDiag::CANNOT_BE_LOADED:
        vkLoaderErrMsg = vkLoader + " appears to be damaged or incompatible. Please perform a clean reinstallation of your GPU drivers from your manufacturer.";
        break;

    case SystemUtils::VkLoaderDiag::UNSUPPORTED:
    default:
        vkLoaderErrMsg = "Failed to locate " + vkLoader + ": failed to identify operating system!";
        break;
    }

    LOG_ASSERT(vkLoaderDiag.present, vkLoaderErrMsg);
}


int main(int argc, char *argv[]) {
    int processStat = processAppConfig(argc, argv);
    if (processStat != EXIT_SUCCESS) return processStat;

    tryOpenConsole();

    Engine engine;

    try {
        Log::BeginLogging();
        Log::PrintAppInfo();

        checkVulkanLoader();

        engine.init();
        std::this_thread::sleep_for(std::chrono::seconds(3)); // Sleep for enough time for the user to see the splash screen
        engine.loadMainWindow();
        engine.run();
    }

    catch (const Log::RuntimeException &e) {
        Log::Print(e.severity(), e.origin(), e.what());
        
        cleanupAll();
        Log::Print(Log::T_ERROR, APP_NAME, "Program exited with errors.");

        std::string title = "Fatal Exception";

        std::string errOrigin = "Origin: " + STD_STR(e.origin()) + "\n";
        std::string errLine = "Line: " + TO_STR(e.errorLine()) + "\n";

        std::string threadInfo = "Thread: " + STD_STR(e.threadInfo()) + "\n";

        std::string msgType;
        Log::LogColor(e.severity(), msgType, false);
        std::string severity = "Severity: " + msgType + "\n";

        boxer::show((errOrigin + errLine + threadInfo + severity + "\n" + STD_STR(e.what())).c_str(), title.c_str(), boxer::Style::Error, boxer::Buttons::Quit);

        Log::EndLogging();
        return EXIT_FAILURE;
    }

    catch (const Log::EngineExitException &e) {}

#ifdef NDEBUG
    catch (...) {
        boxer::show("Caught an unknown exception!\nThis should not happen. Please raise an issue at https://github.com/ButteredFire/Astrocelerate!", "Unknown Exception", boxer::Style::Error, boxer::Buttons::Quit);

        Log::EndLogging();
        return EXIT_FAILURE;
    }
#endif


    cleanupAll();
    Log::Print(Log::T_SUCCESS, APP_NAME, "Program exited successfully.");


    Log::EndLogging();
    return EXIT_SUCCESS;
}