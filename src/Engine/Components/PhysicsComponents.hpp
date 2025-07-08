/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <External/GLM.hpp>

#include "CommonComponents.hpp"
#include <Core/Data/Physics.hpp>


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


	/* Inertial frame of reference. */
	struct ReferenceFrame {
		std::optional<EntityID> parentID;				// The parent reference frame's entity ID.
		Physics::FrameType frameType;					// TODO: Implement reference frame types

		CommonComponent::Transform localTransform;		// Transform relative to parent (meters, inertial frame)
		double radius;									// Radius (meters)
		double radiusToParentRatio = 1.0;				// The radius-to-parent-radius ratio



		// ----- DEPRECATED -----
		CommonComponent::Transform globalTransform;

		double scale = 1.0;							// The entity's physical scale (radius).
		double relativeScale = 1.0;					// The entity's physical scale relative to its parent.
		double visualScale = 1.0;					// The entity's mesh size in render space (can be used to exaggerate size).
		bool visualScaleAffectsChildren = true;
	};


	/* Mesh physical properties. */
	struct ShapeParameters {
		double equatRadius;						// Mean equatorial radius (m)
		double eccentricity;					// Eccentricity
		double gravParam;						// Gravitational parameter (m^3 / s^(-2))
		double rotVelocity;						// Rotational velocity (rad/s)
	};
}