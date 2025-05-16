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


class PhysicsSystem {
public:
	PhysicsSystem();
	~PhysicsSystem() = default;

	/* Updates a rigid body. */
	void updateRigidBody(Component::RigidBody& rigidBody, Component::Transform& transform);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
};