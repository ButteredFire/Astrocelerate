/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <glm_config.hpp>

#include <Engine/Components/WorldSpaceComponents.hpp>


namespace Component {
	struct RigidBody {
		Transform transform = Transform{};				// Transform
		glm::vec3 velocity = glm::vec3();				// Velocity (m/s)
		glm::vec3 acceleration = glm::vec3();			// Acceleration (m/s^2)
		double mass = 0;								// Mass (kg)
	};
}