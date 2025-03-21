/* Constants.h: Stores... constants, duh.
*/

#pragma once

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <filesystem>
#include <string>

// Default working directory
inline std::string DEFAULT_WORKING_DIR = std::filesystem::current_path().string();

// Initialize inDebugMode
#ifdef NDEBUG
	const bool inDebugMode = false;
#else
	const bool inDebugMode = true;
#endif


// Application properties
namespace APP {
	constexpr char APP_NAME[] = "Astrocelerate (Alpha)";
	constexpr char ENGINE_NAME[] = "astrocelerate";

		// Version will be auto-injected via CMake
	#ifndef APP_VERSION // If APP_VERSION is not defined, default it to "default-dev-build"
		#define APP_VERSION "default-dev-build"
	#endif
}

// Compiled shader file paths
namespace ShaderConsts {
	constexpr char VERTEX[] = "compiled_shaders/VertexShader.spv";
	constexpr char FRAGMENT[] = "compiled_shaders/FragmentShader.spv";
}

// Window properties
namespace WindowConsts {
	constexpr int DEFAULT_WINDOW_WIDTH = 1020;
	constexpr int DEFAULT_WINDOW_HEIGHT = 800;
}


// Physics constants
namespace PhysicsConsts {
	constexpr double GRAVITATIONAL_CONSTANT = 6.67430e-11;  // m^3 kg^-1 s^-2
	constexpr double SPEED_OF_LIGHT = 299792458.0;         // m/s
}


// Simulation settings
namespace SimulationConsts {
	constexpr int MAX_SIMULATION_STEPS = 10000;
	constexpr int MAX_FRAMES_IN_FLIGHT = 2; // How many frames should be processed concurrently
	constexpr double TIME_STEP = 0.01;  // seconds
}

#endif
