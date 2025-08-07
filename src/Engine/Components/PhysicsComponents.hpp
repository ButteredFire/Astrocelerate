/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <External/GLM.hpp>

#include "CoreComponents.hpp"
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

		Physics::FrameType frameType;					// TODO: Implement reference frame types

		CoreComponent::Transform localTransform;		// Transform relative to parent (meters, inertial frame).
		CoreComponent::Transform globalTransform;		// Absolute transform in simulation space (meters).
		double scale = 1.0;								// The entity's physical scale (radius).
		double visualScale = 1.0;						// The entity's mesh size in render space (can be used to exaggerate size).
		
		std::string _parentID_str;						// [INTERNAL] ReferenceFrame::parentID in YAML files can either be a reference to another entity or null.
		glm::dvec3 _computedGlobalPosition;				// [INTERNAL] The entity's global position that has been scaled to account for its parent's visual scale (see VkBufferManager::updateObjectUBOs). This is primarily used for camera attachment. 
	};


	/* Properties of ellipsoid celestial bodies. */
	struct ShapeParameters {
		double equatRadius;						// Mean equatorial radius (m)
		double flattening;						// Flattening
		double gravParam;						// Gravitational parameter (m^3 / s^(-2))
		glm::dvec3 rotVelocity;					// Angular/Rotational velocity (rad/s)
		double j2;								// J2 oblateness coefficient
	};
}