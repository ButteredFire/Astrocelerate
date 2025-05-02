/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {
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
