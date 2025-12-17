/* PhysicsComponents.hpp - Components pertaining to physics.
*/

#pragma once

#include <atomic>

#include <External/GLM.hpp>

#include "CoreComponents.hpp"
#include <Core/Data/Physics.hpp>
#include <Core/Data/Application.hpp>

#include <Simulation/Propagators/SGP4/TLE.hpp>


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


	/* Properties of ellipsoid celestial bodies. */
	struct ShapeParameters {
		double equatRadius;						// Mean equatorial radius (m)
		double flattening;						// Flattening
		double gravParam;						// Gravitational parameter (m^3 / s^(-2))
		glm::dvec3 rotVelocity;					// Angular/Rotational velocity (rad/s)
		double j2;								// J2 oblateness coefficient
	};


	struct NutationAngles {
		double deltaPsi;				// Nutation in longitude (radians). This is the change in the celestial longitude of a body caused by nutation.
		double deltaEpsilon;			// Nutation in obliquity (radians). This is the change in the obliquity of the ecliptic (the tilt of a body's axis) caused by nutation.
		double meanEpsilon;				// Mean obliquity of the ecliptic (radians). This represents the average tilt of a body's axis.
		double epsilon;					// True obliquity of the ecliptic (radians). This is the instantaneous tilt of a body's axis.
		double eqEquinoxes;				// Equation of the equinoxes (radians). This value accounts for the difference between the true and mean sidereal time.
		double greenwichSiderealTime;	// Greenwich sidereal time (radians). This angle defines the rotational orientation of a body at a given moment.
	};


	/* Properties of coordinate systems. */
	struct CoordinateSystem {
		Application::SimulationConfig simulationConfig;
		double epochET;
		std::string currentEpoch;
	};


	struct Propagator {
		enum class Type {
			SGP4
		};

		Type propagatorType;			// The type of propagator used.
		std::string tlePath;			// The path to the TLE file.

		std::string tleLine1;			// First TLE line detailing orbital elements for propagation. This is dynamically populated from the TLE file.
		std::string tleLine2;			// Second TLE line detailing orbital elements for propagation. This is dynamically populated from the TLE file.
		double tleEpochET;				// The TLE's epoch, measured as TDB seconds elapsed since the J2000 epoch.

		TLE tle;						// The TLE instance.
	};
}