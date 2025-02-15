/* Constants.h: Stores constants.
*/
#pragma once

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Error/Exit codes
namespace Error {

}

// Window properties
namespace Window {
	constexpr int DEFAULT_WINDOW_WIDTH = 950;
	constexpr int DEFAULT_WINDOW_HEIGHT = 950;
	constexpr char WINDOW_NAME[] = "Astrocelerate (Alpha)";

	// Version will be auto-injected via CMake
	#ifndef APP_VERSION // If APP_VERSION is not defined, default it to "dev-build"
		#define APP_VERSION "dev-build"
	#endif
}


// File Directories
namespace FilePath {
	constexpr char WINDOW_ICONS_PREFIX[] = "";
	constexpr char BASIC_SHADERS_DIR[] = "";
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
