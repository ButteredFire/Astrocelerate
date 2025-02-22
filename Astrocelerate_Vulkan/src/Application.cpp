/* Application.cpp: The entry point for the Astrocelerate engine.
*/

#include "AppWindow.hpp"
#include "Engine/Engine.hpp"
#include "Constants.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const int WIN_WIDTH = WindowConsts::DEFAULT_WINDOW_WIDTH;
const int WIN_HEIGHT = WindowConsts::DEFAULT_WINDOW_HEIGHT;
const std::string WIN_NAME = APP::APP_NAME;

int main() {
    Window window(WIN_WIDTH, WIN_HEIGHT, WIN_NAME);
    GLFWwindow *windowPtr = window.getGLFWwindowPtr();
    Renderer renderer;
    Engine engine(windowPtr, &renderer);

    try {
        engine.run();
    }
    catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}