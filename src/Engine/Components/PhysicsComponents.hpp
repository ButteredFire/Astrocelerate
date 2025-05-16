/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <glm_config.hpp>


namespace Component {
	/* Rigid-body */
	struct RigidBody {
		glm::dvec3 velocity;				// Velocity (m/s)
		glm::dvec3 acceleration;			// Acceleration (m/s^2)
		double mass;						// Mass (kg)
	};


	/* Orbiting body (around another celestial body) */
	struct OrbitingBody {
		glm::dvec3 relativePosition;		// Position of this body relative to the body that it is orbiting.
		double centralMass;					// The mass of the body that this body is orbiting.
	};
}