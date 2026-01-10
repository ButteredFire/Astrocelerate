/* ICelestialBody - Implementation of the interface for a celestial body.
*/

#pragma once

#include <Platform/External/GLM.hpp>

#include <Engine/Registry/ECS/Components/CoreComponents.hpp>
#include <Engine/Registry/ECS/Components/PhysicsComponents.hpp>

#include <Core/Utils/FilePathUtils.hpp>


class ICelestialBody {
public:
	virtual ~ICelestialBody() = default;
	
	/* Gets this body's identifiers. */
	virtual CoreComponent::Identifiers getIdentifiers() const = 0;
	
	/* Gravitational parameter (m^3 s^-2) */
	virtual double getGravParam() const = 0;

	/* Equatorial radius (m) */
	virtual double getEquatRadius() const = 0;

	/* Rotational velocity (rad/s) */
	virtual glm::dvec3 getRotVelocity() const = 0;

	/* J2 oblateness coefficient */
	virtual double getJ2() const = 0;

	/* Flattening */
	virtual double getFlattening() const = 0;

	/* Mass (kg) */
	virtual double getMass() const = 0;

	/* Gets the path to this body's mesh. */
	virtual std::string getMeshPath() const = 0;
};