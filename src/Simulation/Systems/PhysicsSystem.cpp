/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::update(const double dt) {
	updateRigidBodies(dt, m_simulationTime);
	m_simulationTime += dt;
}


void PhysicsSystem::updateRigidBodies(const double dt, const double currentSystemTime) {
	using namespace PhysicsConsts;

	auto view = m_registry->getView<PhysicsComponent::RigidBody, PhysicsComponent::ReferenceFrame, PhysicsComponent::OrbitingBody>();

	for (auto [entityID, rigidBody, refFrame, orbitingBody] : view) {
		Physics::State state{};
		state.position = refFrame.localTransform.position;
		state.velocity = rigidBody.velocity;
		

		ODE::NewtonianTwoBody ode{};
		ode.centralMass = orbitingBody.centralMass;


		RK4Integrator<Physics::State, ODE::NewtonianTwoBody>::Integrate(state, currentSystemTime, dt, ode);

		refFrame.localTransform.position = state.position;
		rigidBody.velocity = state.velocity;

		// Recalculates acceleration for telemetry display
		double distance = glm::length(refFrame.localTransform.position);
		rigidBody.acceleration = -G * (orbitingBody.centralMass * refFrame.localTransform.position) / (distance * distance * distance);

		//SymplecticEulerIntegrator::Integrate(refFrame.localTransform.position, rigidBody.velocity, acceleration, dt);
		

		m_registry->updateComponent(entityID, rigidBody);
		m_registry->updateComponent(entityID, refFrame);
		m_registry->updateComponent(entityID, orbitingBody);
	}
}
