/* Application.hpp - Common data pertaining to application states and data.
*/

#pragma once


namespace Application {
	/* Specifies which state in the application pipeline the window/user is in. */
	enum class Stage {
		// Welcome screens
		START_SCREEN,

		// Setup screens
		SETUP_ORBITAL,			// Orbital mechanics

		// Loading
		LOADING_SCREEN,

		// Workspaces
		WORKSPACE_ORBITAL
	};
}