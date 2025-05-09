/* WorldSpaceComponents.hpp - Components pertaining to world space/orientation.
*/

#pragma once

#include <glm_config.hpp>


namespace Component {
	struct Transform {
		glm::vec3 position = glm::vec3(0.0f);							// World or local position.
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);			// Orientation as a quaternion.
		glm::vec3 scale = glm::vec3(1.0f);								// Scale.
	};
}