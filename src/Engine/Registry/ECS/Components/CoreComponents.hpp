/* CoreComponents.hpp - Components that have no specific tie to any part of the simulation, and thus can be used in all other components.
*/

#pragma once

#include <optional>


#include <Engine/Registry/ECS/ECS.hpp>

#include <Platform/External/GLM.hpp>


namespace CoreComponent {
	struct Transform {
		glm::dvec3 position;						// World or local position.
		glm::dquat rotation = glm::dquat();			// Orientation as a quaternion (Default: Identity (1, 0, 0, 0)).
		double scale;								// Physical scale/radius (m).
	};


	/* Identifiers to identify an entity. This is essential for systems like SPICE. */
	struct Identifiers {
		enum class EntityType {
			UNKNOWN,

			STAR,
			PLANET,
			MOON,
			SPACECRAFT,
			ASTEROID
		};

		EntityType entityType;					// The type of entity.
		std::optional<std::string> spiceID;		// The entity's SPICE ID. This can be optional for custom-defined entities.
	};
}