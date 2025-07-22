/* PhysicsSystem.hpp - Manages the physics.
*/

#pragma once

#include <Core/Engine/ECS.hpp>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/EventDispatcher.hpp>

#include <Core/Data/Physics.hpp>

#include <Simulation/Systems/Time.hpp>

#include <Engine/Components/PhysicsComponents.hpp>

#include <Simulation/ODEs.hpp>

#include <Simulation/Integrators/SymplecticEuler.hpp>
#include <Simulation/Integrators/RK4.hpp>

#include <Utils/SpaceUtils.hpp>


class PhysicsSystem {
public:
	PhysicsSystem();
	~PhysicsSystem() = default;

	void update(const double dt);


	/* Updates all rigid bodies. */
	void updateRigidBodies(const double dt, const double currentSystemTime);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	double m_simulationTime = 0.0;
};