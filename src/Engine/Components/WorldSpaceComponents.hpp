/* WorldSpaceComponents.hpp - Components pertaining to world space/orientation.
*/

#pragma once

#include <glm_config.hpp>


namespace Component {
	struct Transform {
		glm::dvec3 position;						// World or local position.
		glm::dquat rotation;						// Orientation as a quaternion.
		glm::dvec3 scale = glm::dvec3(1.0);			// Scale.
	};
}