/* Constants.h: Stores... constants, duh.
*/

#pragma once

#include <string>
#include <filesystem>

#include <External/GLM.hpp>


// Default working directory
inline std::string DEFAULT_WORKING_DIR = std::filesystem::current_path().string();

// Initialize IN_DEBUG_MODE
#ifdef NDEBUG
	const bool IN_DEBUG_MODE = false;
#else
	const bool IN_DEBUG_MODE = true;
#endif


// Vulkan version
#define VULKAN_VERSION VK_API_VERSION_1_2 // If you decide to change VULKAN_VERSION, also change VMA_VULKAN_VERSION defined in CMakeLists.txt


// Definition of a byte
using Byte = uint8_t;


// Application properties
#ifndef APP_NAME
	#define APP_NAME "Astrocelerate"
#endif
#ifndef APP_VERSION
	#define APP_VERSION "default-dev-build"
#endif
#ifndef AUTHOR
	#define AUTHOR "Duong Duy Nhat Minh"
#endif

extern const std::filesystem::path EXEC_DIR;
extern const std::string ROOT_DIR;


// Shader properties
namespace ShaderConsts {
	// Locations
		// Vertex shader
	constexpr int VERT_BIND_GLOBAL_UBO = 0;
	constexpr int VERT_BIND_OBJECT_UBO = 1;

	constexpr int VERT_LOC_IN_INPOSITION = 0;
	constexpr int VERT_LOC_IN_INCOLOR = 1;
	constexpr int VERT_LOC_IN_INTEXTURECOORD_0 = 2;
	constexpr int VERT_LOC_IN_INNORMAL = 3;
	constexpr int VERT_LOC_IN_INTANGENT = 4;

	constexpr int VERT_LOC_OUT_FRAGCOLOR = 0;
	constexpr int VERT_LOC_OUT_FRAGTEXTURECOORD_0 = 1;
	constexpr int VERT_LOC_OUT_FRAGNORMAL = 2;
	constexpr int VERT_LOC_OUT_FRAGTANGENT = 3;
	constexpr int VERT_LOC_OUT_FRAGPOSITION = 4;


		// Fragment shader
	constexpr int FRAG_BIND_MATERIAL_PARAMETERS = 0;
	constexpr int FRAG_BIND_TEXTURE_MAP = 0;

	constexpr int FRAG_LOC_IN_FRAGCOLOR = VERT_LOC_OUT_FRAGCOLOR;
	constexpr int FRAG_LOC_IN_FRAGTEXTURECOORD_0 = VERT_LOC_OUT_FRAGTEXTURECOORD_0;
	constexpr int FRAG_LOC_IN_FRAGNORMAL = VERT_LOC_OUT_FRAGNORMAL;
	constexpr int FRAG_LOC_IN_FRAGTANGENT = VERT_LOC_OUT_FRAGTANGENT;
	constexpr int FRAG_LOC_IN_FRAGPOSITION = VERT_LOC_OUT_FRAGPOSITION;


	constexpr int FRAG_LOC_OUT_OUTCOLOR = 0;


	// Compiled shaders
	extern const std::string VERTEX;
	extern const std::string FRAGMENT;
}


// Subpass properties
namespace SubpassConsts {
	enum Type {
		T_MAIN,
		T_IMGUI
	};
}


// Window properties
namespace WindowConsts {
	constexpr int DEFAULT_WINDOW_WIDTH = 1500;
	constexpr int DEFAULT_WINDOW_HEIGHT = 900;
}


// Configuration properties
namespace ConfigConsts {
	extern const std::string IMGUI_DEFAULT_CONFIG;
}


// Fonts
#define C_STR(stdString)	(stdString).c_str()
#define STD_STR(cStr)		std::string((cStr))
#define TO_STR(any)			std::to_string((any))

namespace FontConsts {
	extern struct NotoSansFonts {
		const std::string BOLD;
		const std::string BOLD_ITALIC;
		const std::string ITALIC;
		const std::string LIGHT;
		const std::string LIGHT_ITALIC;
		const std::string REGULAR;
		const std::string REGULAR_MATH;
		const std::string REGULAR_MONO;
	} NotoSans;
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
	constexpr double G = 6.67430e-11;			// Gravitational constant (m^3 kg^-1 s^-2)
	constexpr double C = 299792458.0;           // Speed of light (m/s)
	constexpr double AU = 149597870700;			// 1 Astronomical Unit (AU) OR 149,597,870,700 meters (average distance from the Earth to the Sun)
}


// Simulation settings
namespace SimulationConsts {
	constexpr int MAX_SIMULATION_STEPS = 10000;
	constexpr int MAX_FRAMES_IN_FLIGHT = 3;							// How many frames should be processed concurrently
	constexpr int MAX_GLOBAL_TEXTURES = 128;						// The maximum number of textures in the global texture array
	constexpr double TIME_STEP = 1.0 / 60.0;						// 60 Hz
	constexpr double SIMULATION_SCALE = 1e6;						// 1 world unit = 1,000,000 meters (1000 km)

	constexpr glm::vec3 UP_AXIS = glm::vec3(0.0f, 0.0f, 1.0f);		// Z-up
}
