/* PhysicsSystem.hpp - Manages the physics.
*/

#pragma once

#include <Core/ECS.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>

#include <Systems/Time.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/WorldSpaceComponents.hpp>

#include <Simulation/Integrators/SymplecticEuler.hpp>

#include <Utils/SpaceUtils.hpp>


class PhysicsSystem {
public:
	PhysicsSystem();
	~PhysicsSystem() = default;

	void update(const double dt);


	/* Updates all rigid bodies. */
	void updateRigidBodies(const double dt);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
};