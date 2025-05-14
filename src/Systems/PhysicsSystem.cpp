/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {

	m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::getService<EventDispatcher>(__FUNCTION__);

	m_eventDispatcher->subscribe<Event::UpdateRigidBodies>(
		[this](const Event::UpdateRigidBodies& event) {
			auto view = m_registry->getView<Component::RigidBody>();

			for (auto [entityID, rb] : view) {
				Component::RigidBody copy = rb;
				this->updateRigidBody(copy);

				m_registry->updateComponent(entityID, copy);
			}
		}
	);
	
	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::updateRigidBody(Component::RigidBody& rigidBody) {
	float dt = Time::GetDeltaTime();

	rigidBody.transform.position += rigidBody.velocity * dt;
	rigidBody.velocity += rigidBody.acceleration * dt;
}
