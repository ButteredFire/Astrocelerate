/* WorldSpaceComponents.hpp - Components pertaining to world space/orientation.
*/

#pragma once

#include <optional>

#include <glm_config.hpp>

#include <Core/ECS.hpp>


namespace Component {
	struct Transform {
		glm::dvec3 position;						// World or local position.
		glm::dquat rotation = glm::dquat();			// Orientation as a quaternion (Default: Identity (1, 0, 0, 0)).
		//double scale = 1.0;							// Scale (Default: 1.0).
	};


	struct ReferenceFrame {
		std::optional<EntityID> parentID;			// The parent reference frame's entity ID.
		double scale = 1.0;							// The frame's absolute scale.

		/* Local transform.
		- position: The frame's position relative to its parent.
		- rotation: The frame's rotation relative to its parent.
		*/
		Transform localTransform;

		// Global transform: The absolute transform of a frame with respect to the global simulation space.
		Transform globalTransform;
	};
}