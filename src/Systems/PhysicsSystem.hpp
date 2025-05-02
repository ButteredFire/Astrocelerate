/* PhysicsSystem.hpp - Manages the physics.
*/

#pragma once

#include <Core/ECS.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>

#include <Systems/Time.hpp>

#include <Engine/Components/PhysicsComponents.hpp>


class PhysicsSystem {
public:
	PhysicsSystem();
	~PhysicsSystem() = default;

	/* Updates a rigid body. */
	void updateRigidBody(Component::RigidBody& rigidBody);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
};