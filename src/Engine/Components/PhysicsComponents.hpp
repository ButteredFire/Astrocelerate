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
		std::string _centralMass_str;		// [INTERNAL] OrbitingBody::centralMass in YAML files MUST be a reference to another entity.
	};


	/* Inertial frame of reference. */
	struct ReferenceFrame {
		std::optional<EntityID> parentID;				// The parent reference frame's entity ID.
		std::string _parentID_str;						// [INTERNAL] ReferenceFrame::parentID in YAML files can either be a reference to another entity or null.

		Physics::FrameType frameType;					// TODO: Implement reference frame types

		CommonComponent::Transform localTransform;		// Transform relative to parent (meters, inertial frame).
		CommonComponent::Transform globalTransform;		// Absolute transform in simulation space (meters).
		double scale = 1.0;								// The entity's physical scale (radius).
		double visualScale = 1.0;						// The entity's mesh size in render space (can be used to exaggerate size).
	};


	/* Mesh physical properties. */
	struct ShapeParameters {
		double equatRadius;						// Mean equatorial radius (m)
		double eccentricity;					// Eccentricity
		double gravParam;						// Gravitational parameter (m^3 / s^(-2))
		double rotVelocity;						// Rotational velocity (rad/s)
	};
}