/* WorldSpaceComponents.hpp - Components pertaining to world space/orientation.
*/

#pragma once

#include <optional>

#include <glm_config.hpp>

#include <Core/ECS.hpp>


namespace WorldSpaceComponent {
	struct Transform {
		glm::dvec3 position;						// World or local position.
		glm::dquat rotation = glm::dquat();			// Orientation as a quaternion (Default: Identity (1, 0, 0, 0)).
	};


	struct ReferenceFrame {
		std::optional<EntityID> parentID;			// The parent reference frame's entity ID.

		/* Local transform.
		- position: The frame's position relative to its parent.
		- rotation: The frame's rotation relative to its parent.
		*/
		Transform localTransform;

		// Global transform: The absolute transform of a frame with respect to the global simulation space.
		// IMPORTANT: The global transform is STRICTLY used for rendering. For physics simulations, use the local transform.
		Transform globalTransform;

		double scale = 1.0;							// The entity's physical scale (radius).
		double relativeScale = 1.0;					// The entity's physical scale relative to its parent.
		double visualScale = 1.0;					// The entity's mesh size in render space (can be used to exaggerate size).
		bool visualScaleAffectsChildren = true;		// Does the entity's visual scale affect the transform of this reference frame's children? Set this to False for scenarios where you only want to see the visual/exaggerated scale without its effect on child transforms.
	};
}