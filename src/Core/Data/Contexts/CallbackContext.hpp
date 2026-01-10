#pragma once

class InputManager;

struct CallbackContext {
    InputManager* inputManager;
};

extern CallbackContext g_glfwCallbackCtx;