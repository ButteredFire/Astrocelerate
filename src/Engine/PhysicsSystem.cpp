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

	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case INITIALIZED:
				this->homogenizeCoordinateSystems();
				this->update(Time::GetDeltaTime());

				m_simulationTime = 0.0;

				break;
			}
		}
	);


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

	LOG_ASSERT(CoordSys::FrameProperties.count(frame), "Cannot configure coordinate system: Cannot retrieve properties for an unknown/unsupported coordinate system!");

	LOG_ASSERT(CoordSys::EpochToSPICEMap.count(epoch), "Cannot configure coordinate system: Cannot retrieve properties for an unknown/unsupported epoch!");

	m_coordSystem = std::make_shared<CoordinateSystem>();
	m_coordSystem->init(kernelPaths, frame, epoch, epochFormat);
}


void PhysicsSystem::update(const double dt) {
	double currentET = m_coordSystem->getEpochET() + m_simulationTime;
	
	updateSPICEBodies(currentET);
	updateGeneralBodies(dt, currentET);

	m_simulationTime += dt;
}


void PhysicsSystem::updateSPICEBodies(const double currentET) {
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Query SPICE data for bodies with available ephemeris data
	for (auto &&[entityID, transform, rigidBody] : view) {
		CoreComponent::Identifiers identifiers = m_registry->getComponent<CoreComponent::Identifiers>(entityID);

		if (identifiers.spiceID.has_value()) {
			const std::string &spiceID = identifiers.spiceID.value();

			// Update position and velocity
			const std::array<double, 6> stateVec = m_coordSystem->getBodyState(spiceID, currentET);

			transform.position = glm::dvec3(
				stateVec[0], stateVec[1], stateVec[2]
			);

			rigidBody.velocity = glm::dvec3(
				stateVec[3], stateVec[4], stateVec[5]
			);


			// Update rotation
			const std::string frameName = "IAU_" + spiceID;
			const glm::mat3 rotMatrix = m_coordSystem->getRotationMatrix(frameName, currentET);

			transform.rotation = glm::dquat(rotMatrix);


			// Write to components
			m_registry->updateComponent(entityID, transform);
			m_registry->updateComponent(entityID, rigidBody);



			// SPECIAL CASE: If the entity is a star, update its point light position to its own.
			if (identifiers.entityType == CoreComponent::Identifiers::EntityType::STAR) {
				RenderComponent::PointLight &pointLight = m_registry->getComponent<RenderComponent::PointLight>(entityID);

				pointLight.position = SpaceUtils::ToRenderSpace_Position(transform.position);

				m_registry->updateComponent(entityID, pointLight);
			}
		}
	}
}


void PhysicsSystem::updateGeneralBodies(const double dt, const double currentET) {
	using namespace PhysicsConsts;
	
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Integration and component updating
	for (size_t i = 0; i < view.size(); i++) {
		auto [targetEntityID, targetTransform, targetRigidBody] = view[i];

		// If the target entity uses SPICE or a propagator, its state vector has already been computed. We must, therefore, skip them.
		{
			CoreComponent::Identifiers targetIDs = m_registry->getComponent<CoreComponent::Identifiers>(targetEntityID);
			if (targetIDs.spiceID.has_value())
				continue;
		}

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


void PhysicsSystem::homogenizeCoordinateSystems() {
	double currentET = m_coordSystem->getEpochET();


	// Convert TLE state vectors (TEME coordinate system)

	auto view = m_registry->getView<PhysicsComponent::Propagator, CoreComponent::Transform, PhysicsComponent::RigidBody>();

	for (auto &&[entityID, propagator, transform, rigidBody] : view) {
		// Get state vector from TLE
		TLE tle(propagator.tleLine1, propagator.tleLine2);

		double minutesSinceEpoch = m_simulationTime / 60.0;
		double position[3], velocity[3];

		tle.getRV(minutesSinceEpoch, position, velocity);

		std::array<double, 6> stateVec = {
			position[0], position[1], position[2],
			velocity[0], velocity[1], velocity[2]
		};


		// Convert propagator output from km and km/s to m and m/s respectively
		if (propagator.propagatorType == PhysicsComponent::Propagator::Type::SGP4)
			for (size_t i = 0; i < stateVec.size(); i++)
				stateVec[i] *= 1e3;


		// Transform the state vector from TEME to this system's frame
		stateVec = m_coordSystem->TEMEToThisFrame(stateVec, currentET);


		// Write new data to the components
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
