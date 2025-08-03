/* Application.hpp - Common data pertaining to application states and data.
*/

#pragma once


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
}