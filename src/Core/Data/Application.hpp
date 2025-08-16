/* Application.hpp - Common data pertaining to application states and other high-level data.
*/

#pragma once

#include <Core/Data/CoordSys.hpp>


namespace Application {
	/* Specifies which stage in the application pipeline the window/user is in. */
	enum class Stage {
		NULL_STAGE,

		// Welcome screens
		START_SCREEN,

		// Setup screens
		SETUP_ORBITAL,			// Orbital mechanics

		// Loading
		LOADING_SCREEN,

		// Workspaces
		WORKSPACE_ORBITAL
	};


	/* Specifies the current application state. */
	enum class State {
		NULL_STATE,

		IDLE,
		RECREATING_SWAPCHAIN
	};


	// YAML Simulation File Configuration
	struct YAMLFileConfig {
		std::string fileName;			// The name of the simulation file.
		uint32_t version;				// The version of the simulation. This is NOT the file's YAML version.
		std::string description;		// The simulation's description.
	};


	// Simulation Configuration
	struct SimulationConfig {
		std::vector<std::string> kernelPaths;				// The path to the SPICE kernels.
		CoordSys::FrameType frameType;						// The type of coordinate system (inertial/non-inertial frame).
		CoordSys::Frame frame = CoordSys::Frame::NONE;		// The frame used by the simulation.
		CoordSys::Epoch epoch;								// The simulation's epoch.
		std::string epochFormat;							// The epoch's SPICE format ("YYYY-MM-DD HH:MM:SS TZ")
	};
}