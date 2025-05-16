/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {

	m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::getService<EventDispatcher>(__FUNCTION__);

	m_eventDispatcher->subscribe<Event::UpdatePhysics>(
		[this](const Event::UpdatePhysics& event) {
			this->update(event.dt);
		}
	);
	
	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::update(const double dt) {
	updateRigidBodies(dt);
}


void PhysicsSystem::updateRigidBodies(const double dt) {
	using namespace PhysicsConsts;

	auto view = m_registry->getView<Component::RigidBody, Component::Transform, Component::OrbitingBody>();

	for (auto [entityID, rb, t, ob] : view) {
		Component::RigidBody rigidBody = rb;
		Component::Transform transform = t;
		Component::OrbitingBody orbitingBody = ob;


		// Distance to central mass
		double distance = glm::length(orbitingBody.relativePosition);

		glm::dvec3 acceleration = -G * orbitingBody.centralMass / (distance * distance * distance) * orbitingBody.relativePosition;


		SymplecticEulerIntegrator integrator{};
		integrator.integrate(transform.position, rigidBody.velocity, acceleration, dt);

		rigidBody.acceleration = acceleration;
		orbitingBody.relativePosition = transform.position - glm::dvec3(0.0); // Assuming planet is at (0, 0, 0)

		m_registry->updateComponent(entityID, rigidBody);
		m_registry->updateComponent(entityID, transform);
		m_registry->updateComponent(entityID, orbitingBody);
	}
}
