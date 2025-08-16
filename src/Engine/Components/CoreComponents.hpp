/* CoreComponents.hpp - Components that have no specific tie to any part of the simulation, and thus can be used in all other components.
*/

#pragma once

#include <optional>

#include <External/GLM.hpp>

#include <Core/Engine/ECS.hpp>


namespace CoreComponent {
	struct Transform {
		glm::dvec3 position;						// World or local position.
		glm::dquat rotation = glm::dquat();			// Orientation as a quaternion (Default: Identity (1, 0, 0, 0)).
		double scale;								// Physical scale/radius (m).
	};


	struct Identifiers {
		std::string spiceID;
	};
}