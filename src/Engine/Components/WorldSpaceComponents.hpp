/* WorldSpaceComponents.hpp - Components pertaining to world space/orientation.
*/

#pragma once

#include <optional>

#include <glm_config.hpp>

#include <Core/ECS.hpp>


namespace Component {
	struct Transform {
		glm::dvec3 position;						// World or local position.
		glm::dquat rotation;						// Orientation as a quaternion.
		double scale = 1.0;							// Scale.
	};


	struct ReferenceFrame {
		std::optional<EntityID> parentID;			// The parent reference frame's entity ID.

		/* Local transform.
		- position: The frame's position relative to its parent.
		- rotation: The frame's rotation relative to its parent.
		- scale: Local-to-parent scale ratio (e.g., 2.0 means "1 meter in the child = 2 meters in the parent").
		*/
		Transform localTransform;

		// Global transform: The absolute transform of a frame with respect to the global simulation space.
		Transform globalTransform;
	};
}