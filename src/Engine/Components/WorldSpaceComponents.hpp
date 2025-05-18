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


	struct ReferenceFrame {
		ReferenceFrame* parent;						// A pointer to the parent reference frame (or null).

		glm::dvec3 localPosition;					// The frame's position relative to its parent.
		glm::dquat localRotation;					// The frame's rotation relative to its parent.
		double localScale = 1.0;					// Local-to-parent scale ratio (e.g., 1.0 for 1:1, 1e-6 for orbital to planetary).

		// Global transform: The absolute transform of a frame with respect to the global simulation space.
		glm::dvec3 globalPosition;					// Global position.
		glm::dquat globalRotation;					// Global rotation.
		double globalScale;							// Global scale.
	};
}