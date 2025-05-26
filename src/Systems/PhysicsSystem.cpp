/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::update(const double dt) {
	updateRigidBodies(dt);
}


void PhysicsSystem::updateRigidBodies(const double dt) {
	using namespace PhysicsConsts;

	auto view = m_registry->getView<Component::RigidBody, Component::ReferenceFrame, Component::OrbitingBody>();

	for (auto [entityID, rigidBody, refFrame, orbitingBody] : view) {
		// Distance to central mass
		glm::dvec3 relativePosition = refFrame.localTransform.position;
		double distance = glm::length(relativePosition);

		glm::dvec3 acceleration = -G * orbitingBody.centralMass / (distance * distance * distance) * relativePosition;
		rigidBody.acceleration = acceleration; // For telemetry display only (not used in simulation)

		SymplecticEulerIntegrator::Integrate(refFrame.localTransform.position, rigidBody.velocity, acceleration, dt);

		m_registry->updateComponent(entityID, rigidBody);
		m_registry->updateComponent(entityID, refFrame);
		m_registry->updateComponent(entityID, orbitingBody);
	}
}
