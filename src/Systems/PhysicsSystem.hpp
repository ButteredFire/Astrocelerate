/* PhysicsSystem.hpp - Manages the physics.
*/

#pragma once

#include <Core/ECSCore.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>


class PhysicsSystem {
public:
	PhysicsSystem();
	~PhysicsSystem() = default;

	/* Updates rigid bodies. */
	void updateRigidBodies();

private:
	std::shared_ptr<EntityManager> entityManager;
	std::shared_ptr<EventDispatcher> eventDispatcher;
};