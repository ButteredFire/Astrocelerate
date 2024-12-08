#pragma once

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Error/Exit codes
namespace Error {
	constexpr int CANNOT_INIT_GLFW = -1;
	constexpr int CANNOT_INIT_GLEW = -2;
	constexpr int CANNOT_INIT_WINDOW = -3;
	constexpr int CANNOT_COMPILE_SHADER = -4;
	constexpr int CANNOT_PARSE_SHADER_FILE = -5;
	constexpr int UNKNOWN_SHADER = -6;
	constexpr int UNSUPPORTED_VBO_ELEM_TYPE = -7;
}

// Window properties
namespace Window {
	constexpr int WINDOW_WIDTH = 1000;
	constexpr int WINDOW_HEIGHT = 800;
	constexpr char WINDOW_NAME[] = "OpenGL Test Environment";
}


// File Directories
namespace FilePath {
	constexpr char WINDOW_ICONS_PREFIX[] = "assets/app/AppIcon-";
	constexpr char BASIC_SHADERS_DIR[] = "shaders/basic.shd";
}


// Physics constants
namespace Physics {
	constexpr double GRAVITATIONAL_CONSTANT = 6.67430e-11;  // m^3 kg^-1 s^-2
	constexpr double SPEED_OF_LIGHT = 299792458.0;         // m/s
}


// Simulation settings
namespace Simulation {
	constexpr int MAX_SIMULATION_STEPS = 10000;
	constexpr double TIME_STEP = 0.01;  // seconds
}

#endif
