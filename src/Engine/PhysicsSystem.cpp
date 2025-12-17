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
				m_simulationTime = 0.0;

				this->homogenizeCoordinateSystems();
				this->update(Time::GetDeltaTime());

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


void PhysicsSystem::tick(std::shared_ptr<WorkerThread> worker) {
	float timeScale = Time::GetTimeScale();
	Time::UpdateDeltaTime();
	double deltaTime = Time::GetDeltaTime();

	std::lock_guard<std::mutex> lock(m_accumulatorMutex);

	m_accumulator += deltaTime * timeScale;

	// TODO: Implement adaptive timestepping instead of a constant TIME_STEP
	while (m_accumulator >= SimulationConsts::TIME_STEP) {
		if (worker->stopRequested())
			// If the thread in which this function is called is requested to be stopped, immediately stop updating physics
			m_accumulator = 0.0;

		if (Time::GetTimeScale() != timeScale) {
			// If time scale changes while physics is updating, immediately exit update loop to renew time scale
			m_accumulator = 0.0;
			break;
		}

		update(SimulationConsts::TIME_STEP);
		m_accumulator -= SimulationConsts::TIME_STEP;
	}
}


void PhysicsSystem::update(const double dt) {
	double currentET = m_coordSystem->getEpochET() + m_simulationTime;
	m_simulationTime += dt;
	
	updateSPICEBodies(currentET);
	propagateBodies(currentET);
	updateGeneralBodies(dt, currentET);
}


void PhysicsSystem::updateSPICEBodies(const double et) {
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Query SPICE data for bodies with available ephemeris data
	for (auto &&[entityID, transform, rigidBody] : view) {
		CoreComponent::Identifiers identifiers = m_registry->getComponent<CoreComponent::Identifiers>(entityID);

		if (identifiers.spiceID.has_value()) {
			const std::string &spiceID = identifiers.spiceID.value();

			// Update position and velocity
			const std::array<double, 6> stateVec = m_coordSystem->getBodyState(spiceID, et);

			transform.position = glm::dvec3(
				stateVec[0], stateVec[1], stateVec[2]
			);

			rigidBody.velocity = glm::dvec3(
				stateVec[3], stateVec[4], stateVec[5]
			);


			// Update rotation
			const std::string frameName = "IAU_" + spiceID;
			const glm::mat3 rotMatrix = m_coordSystem->getRotationMatrix(frameName, et);

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


void PhysicsSystem::updateGeneralBodies(const double dt, const double et) {
	using namespace PhysicsConsts;
	
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Integration and component updating
	for (size_t i = 0; i < view.size(); i++) {
		auto [targetEntityID, targetTransform, targetRigidBody] = view[i];

		{
			// If the target entity uses SPICE, its state vector has already been computed. We must, therefore, skip them.
			CoreComponent::Identifiers targetIDs = m_registry->getComponent<CoreComponent::Identifiers>(targetEntityID);
			if (targetIDs.spiceID.has_value())
				continue;
		
			// TODO: If the target has a propagator, its state vector has already been computed.
			//if (m_registry->hasComponent<PhysicsComponent::Propagator>(targetEntityID))
			//	continue;
		}

		// Prepare initial states and ODE
		Physics::State state{};
		state.position = targetTransform.position;
		state.velocity = targetRigidBody.velocity;

		ODE::NewtonianNBody ode{};
		ode.view = &view;
		ode.entityID = targetEntityID;


		// Integrate!
		RK4Integrator<Physics::State, ODE::NewtonianNBody>::Integrate(state, et, dt, ode);


		// Update components
		targetTransform.position = state.position;

		targetRigidBody.velocity = state.velocity;
		targetRigidBody.acceleration = ode(state, dt).velocity;		// Recompute acceleration again

		m_registry->updateComponent(targetEntityID, targetTransform);
		m_registry->updateComponent(targetEntityID, targetRigidBody);
	}
}


void PhysicsSystem::propagateBodies(const double et) {
	auto view = m_registry->getView<PhysicsComponent::Propagator, CoreComponent::Transform, PhysicsComponent::RigidBody>();

	for (auto &&[entityID, propagator, transform, rigidBody] : view) {
		const double secondsSinceEpoch = et - propagator.tleEpochET;
		const double minutesSinceEpoch = secondsSinceEpoch / 60.0;

		double position[3], velocity[3];
		propagator.tle.getRV(minutesSinceEpoch, position, velocity);

		std::array<double, 6> stateVec = {
			position[0], position[1], position[2],
			velocity[0], velocity[1], velocity[2]
		};


		//std::cout << "Epoch: " << et << " (ET), " << minutesSinceEpoch << " minutes (" << secondsSinceEpoch << " seconds) since epoch.\n";
		//std::cout << "State vector (in km and km/s):\n\tr_TEME  = {" << stateVec[0] << ", " << stateVec[1] << ", " << stateVec[2] << "}, v_TEME  = {" << stateVec[3] << ", " << stateVec[4] << ", " << stateVec[5] << "}";


		glm::dvec3 rT(stateVec[0], stateVec[1], stateVec[2]);
		
		// Transform the state vector from TEME to this system's frame
		stateVec = m_coordSystem->TEMEToThisFrame(stateVec, et);

		glm::dvec3 rJ(stateVec[0], stateVec[1], stateVec[2]);
		
		//std::cout << "\n\tr_J2000 = {" << stateVec[0] << ", " << stateVec[1] << ", " << stateVec[2] << "}, v_J2000 = {" << stateVec[3] << ", " << stateVec[4] << ", " << stateVec[5] << "}\n\n";



		double dr = glm::length(rJ - rT);
		double angular = glm::degrees(acos(glm::dot(glm::normalize(rJ), glm::normalize(rT)))); // deg
		//std::cout << " |delta-r| = " << dr << " km, angular = " << angular << " deg\n";




		// Convert propagator output from km and km/s to m and m/s respectively
		if (propagator.propagatorType == PhysicsComponent::Propagator::Type::SGP4)
			for (size_t i = 0; i < stateVec.size(); i++)
				stateVec[i] *= 1e3;


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


void PhysicsSystem::homogenizeCoordinateSystems() {
	// Convert TLE state vectors (TEME coordinate system)
	auto view = m_registry->getView<PhysicsComponent::Propagator, CoreComponent::Transform, PhysicsComponent::RigidBody>();

	for (auto &&[entityID, propagator, transform, rigidBody] : view) {
		// Compute TLE epoch
		propagator.tleEpochET = SPICEUtils::tleEpochToET(propagator.tleLine1);

		// Get state vector from TLE
		propagator.tle.parseLines(propagator.tleLine1, propagator.tleLine2);

		double position[3], velocity[3];

		propagator.tle.getRV(0.0, position, velocity);

		std::array<double, 6> stateVec = {
			position[0], position[1], position[2],
			velocity[0], velocity[1], velocity[2]
		};


		// Transform the state vector from TEME to this system's frame
		stateVec = m_coordSystem->TEMEToThisFrame(stateVec, m_coordSystem->getEpochET());


		// Convert propagator output from km and km/s to m and m/s respectively
		if (propagator.propagatorType == PhysicsComponent::Propagator::Type::SGP4)
			for (size_t i = 0; i < stateVec.size(); i++)
				stateVec[i] *= 1e3;


		// Write new data to the components
		transform.position = glm::dvec3(
			stateVec[0], stateVec[1], stateVec[2]
		);

		rigidBody.velocity = glm::dvec3(
			stateVec[3], stateVec[4], stateVec[5]
		);

		m_registry->updateComponent(entityID, propagator);
		m_registry->updateComponent(entityID, transform);
		m_registry->updateComponent(entityID, rigidBody);
	}
}
