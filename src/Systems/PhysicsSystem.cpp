/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {

	m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::getService<EventDispatcher>(__FUNCTION__);

	m_eventDispatcher->subscribe<Event::UpdateRigidBodies>(
		[this](const Event::UpdateRigidBodies& event) {
			auto view = m_registry->getView<Component::RigidBody, Component::Transform>();

			for (auto [entityID, rigidBody, transform] : view) {
				Component::RigidBody copyRigidBody = rigidBody;
				Component::Transform copyTransform = transform;
				this->updateRigidBody(copyRigidBody, copyTransform);

				m_registry->updateComponent(entityID, copyRigidBody);
				m_registry->updateComponent(entityID, copyTransform);
			}
		}
	);
	
	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::updateRigidBody(Component::RigidBody& rigidBody, Component::Transform& transform) {
	float dt = Time::GetDeltaTime();

	transform.position += rigidBody.velocity * dt;
	rigidBody.velocity += rigidBody.acceleration * dt;
}
