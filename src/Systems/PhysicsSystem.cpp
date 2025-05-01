/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {
	entityManager = ServiceLocator::getService<EntityManager>(__FUNCTION__);
	eventDispatcher = ServiceLocator::getService<EventDispatcher>(__FUNCTION__);

	eventDispatcher->subscribe<Event::UpdateRigidBodies>(
		[this](const Event::UpdateRigidBodies& event) {
			this->updateRigidBodies();
		}
	);
	
	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::updateRigidBodies() {
	//auto view = View::getView(entityManager, );
}
