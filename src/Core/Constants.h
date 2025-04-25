/* Constants.h: Stores... constants, duh.
*/

#pragma once

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


// Vulkan version
#define VULKAN_VERSION VK_API_VERSION_1_2 // If you decide to change VULKAN_VERSION, also change VMA_VULKAN_VERSION defined in CMakeLists.txt


// Application properties
namespace APP {
	constexpr char APP_NAME[] = "Astrocelerate (Alpha)";
	constexpr char ENGINE_NAME[] = "astrocelerate";

		// Version will be auto-injected via CMake
	#ifndef APP_VERSION // If APP_VERSION is not defined, default it to "default-dev-build"
		#define APP_VERSION "default-dev-build"
	#endif
}

// Shader properties
namespace ShaderConsts {
	// Locations
		// Vertex shader
	constexpr int VERT_BIND_UNIFORM_UBO = 0;

	constexpr int VERT_LOC_IN_INPOSITION = 0;
	constexpr int VERT_LOC_IN_INCOLOR = 1;
	constexpr int VERT_LOC_IN_INTEXTURECOORD = 2;

	constexpr int VERT_LOC_OUT_FRAGCOLOR = 0;
	constexpr int VERT_LOC_OUT_FRAGTEXTURECOORD = 1;


		// Fragment shader
	constexpr int FRAG_BIND_UNIFORM_TEXURE_SAMPLER = 1;

	constexpr int FRAG_LOC_IN_FRAGCOLOR = VERT_LOC_OUT_FRAGCOLOR;
	constexpr int FRAG_LOC_IN_FRAGTEXTURECOORD = VERT_LOC_OUT_FRAGTEXTURECOORD;

	constexpr int FRAG_LOC_OUT_OUTCOLOR = 0;


	// Compiled shaders
	inline const std::string VERTEX = std::string(APP_BINARY_DIR) + std::string("/compiled_shaders/VertexShader.spv");
	inline const std::string FRAGMENT = std::string(APP_BINARY_DIR) + std::string("/compiled_shaders/FragmentShader.spv");
}

// Window properties
namespace WindowConsts {
	constexpr int DEFAULT_WINDOW_WIDTH = 1200;
	constexpr int DEFAULT_WINDOW_HEIGHT = 900;
}


// Gamma correction constants
namespace Gamma {
	constexpr float THRESHOLD = 0.04045f;
	constexpr float DIVISOR = 12.92f;
	constexpr float OFFSET = 0.055f;
	constexpr float SCALE = 1.055f;
	constexpr float EXPONENT = 2.4f;
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
