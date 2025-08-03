/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include <Core/Application/AppWindow.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Contexts/AppContext.hpp>

#include <Core/Data/Contexts/CallbackContext.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Orchestration/Engine.hpp>
#include <Orchestration/Session.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <boxer/boxer.h>


const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;


void processCleanupStack() {
    std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
    garbageCollector->processCleanupStack();
}


int main() {
    Log::BeginLogging();
    Log::PrintAppInfo();

    // Creates a window
    Window window(WIN_WIDTH, WIN_HEIGHT, APP_NAME);
    GLFWwindow *windowPtr = window.getGLFWwindowPtr();

    window.initGLFWBindings(&g_callbackContext);

    try {
        Engine engine(windowPtr);

        engine.init();

        engine.run();
    }
    catch (const Log::RuntimeException& e) {
        Log::Print(e.severity(), e.origin(), e.what());
        
        processCleanupStack();
        Log::Print(Log::T_ERROR, APP_NAME, "Program exited with errors.");

        std::string title = "Exception raised from " + STD_STR(e.origin());

        std::string errOrigin = "Origin: " + STD_STR(e.origin()) + "\n";
        std::string errLine = "Line: " + TO_STR(e.errorLine()) + "\n";

        std::string threadInfo = "Thread: " + STD_STR(e.threadInfo()) + "\n";

        std::string msgType;
        Log::LogColor(e.severity(), msgType, false);
        std::string severity = "Exception type: " + msgType + "\n";

        boxer::show((errOrigin + errLine + threadInfo + severity + "\n" + STD_STR(e.what())).c_str(), title.c_str(), boxer::Style::Error, boxer::Buttons::Quit);

        Log::EndLogging();
        return EXIT_FAILURE;
    }
#ifdef NDEBUG
    catch (...) {
        boxer::show("Caught an unknown exception!\nThis should not happen. Please raise an issue at https://github.com/ButteredFire/Astrocelerate!", "Unknown Exception", boxer::Style::Error, boxer::Buttons::Quit);

        Log::EndLogging();
        return EXIT_FAILURE;
    }
#endif

    processCleanupStack();
    Log::Print(Log::T_SUCCESS, APP_NAME, "Program exited successfully.");

    Log::EndLogging();
    return EXIT_SUCCESS;
}