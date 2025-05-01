/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <glm_config.hpp>

namespace Component {
	struct RigidBody {
		glm::vec3 position = glm::vec3();				// Position
		glm::vec3 velocity = glm::vec3();				// Velocity (m/s)
		glm::vec3 acceleration = glm::vec3();			// Acceleration (m/s^2)
		double mass = 0;								// Mass (kg)
	};
}