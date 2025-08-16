/* PhysicsSystem.cpp - Physics system implementation.
*/

#include "PhysicsSystem.hpp"


PhysicsSystem::PhysicsSystem() {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void PhysicsSystem::bindEvents() {
	const EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<PhysicsSystem>();

	m_eventDispatcher->subscribe<ConfigEvent::SimulationFileParsed>(selfIndex, 
		[this](const ConfigEvent::SimulationFileParsed &event) {
			// Create absolute kernel paths and furnish SPICE
			std::vector<std::string> kernelPaths = event.simulationConfig.kernelPaths;
			for (size_t i = 0; i < kernelPaths.size(); i++)
				kernelPaths[i] = FilePathUtils::JoinPaths(ROOT_DIR, kernelPaths[i]);
			

			this->configureCoordSys(
				event.simulationConfig.frameType, event.simulationConfig.frame,
				kernelPaths,
				event.simulationConfig.epoch, event.simulationConfig.epochFormat
			);
		}
	);
}


void PhysicsSystem::configureCoordSys(CoordSys::FrameType frameType, CoordSys::Frame frame, const std::vector<std::string> &kernelPaths, CoordSys::Epoch epoch, const std::string &epochFormat) {
	using namespace CoordSys;
	using enum Frame;


	if (frameType == FrameType::INERTIAL) {
		switch (frame) {
		case ECI:
			m_coordSystem = std::make_shared<ECIFrame>();
			break;

		case HCI:
			break;
		}
	}
	else if (frameType == FrameType::NON_INERTIAL) {
		switch (frame) {
		case ECEF:
			break;
		}
	}
	else throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot configure coordinate system: Frame type (inertial/non-inertial) is unknown!");


	m_coordSystem->init(kernelPaths, epoch, epochFormat);
}


void PhysicsSystem::update(const double dt) {
	double currentET = m_coordSystem->getEpochET() + m_simulationTime;

	updateSPICEBodies(currentET);
	updateRigidBodies(dt, currentET);

	m_simulationTime += dt;
}


void PhysicsSystem::updateSPICEBodies(const double currentET) {
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Query SPICE data for bodies with available ephemeris data
	for (auto &&[entityID, transform, rigidBody] : view)
		if (m_registry->hasComponent<CoreComponent::Identifiers>(entityID)) {
			CoreComponent::Identifiers identifiers = m_registry->getComponent<CoreComponent::Identifiers>(entityID);

			const std::array<double, 6> stateVec = m_coordSystem->getBodyState(identifiers.spiceID, currentET);

			transform.position = glm::dvec3(
				stateVec[0], stateVec[1], stateVec[2]
			);

			rigidBody.velocity = glm::dvec3(
				stateVec[3], stateVec[4], stateVec[5]
			);

			m_registry->updateComponent(entityID, transform);
			m_registry->updateComponent(entityID, rigidBody);
		}
}


void PhysicsSystem::updateRigidBodies(const double dt, const double currentET) {
	using namespace PhysicsConsts;
	
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Integration and component updating
	for (size_t i = 0; i < view.size(); i++) {
		auto [targetEntityID, targetTransform, targetRigidBody] = view[i];

		// Prepare initial states and ODE
		Physics::State state{};
		state.position = targetTransform.position;
		state.velocity = targetRigidBody.velocity;

		ODE::NewtonianNBody ode{};
		ode.view = &view;
		ode.entityID = targetEntityID;


		// Integrate!
		RK4Integrator<Physics::State, ODE::NewtonianNBody>::Integrate(state, currentET, dt, ode);


		// Update components
		targetTransform.position = state.position;

		targetRigidBody.velocity = state.velocity;
		targetRigidBody.acceleration = ode(state, dt).velocity;		// Recompute acceleration again

		m_registry->updateComponent(targetEntityID, targetTransform);
		m_registry->updateComponent(targetEntityID, targetRigidBody);
	}
}
