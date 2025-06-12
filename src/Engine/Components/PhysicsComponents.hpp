/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <glm_config.hpp>


namespace PhysicsComponent {
	/* Rigid-body */
	struct RigidBody {
		glm::dvec3 velocity;				// Velocity (m/s)
		glm::dvec3 acceleration;			// Acceleration (m/s^2)
		double mass;						// Mass (kg)
	};


	/* Orbiting body (around another celestial body) */
	struct OrbitingBody {
		double centralMass;					// The mass of the body that this body is orbiting around.
	};
}