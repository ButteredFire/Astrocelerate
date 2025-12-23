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

				cacheRegistryData();

				this->homogenizeCoordinateSystems();
				this->update(Time::GetDeltaTime());

				syncRegistryData();

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

	auto view = m_registry->getView<PhysicsComponent::CoordinateSystem>();
	LOG_ASSERT(view.size() == 1, "Cannot configure coordinate system: Corrupt registry!");
	auto [id, coordSys] = view[0];
	coordSys.epochET = m_coordSystem->getEpochET();
	m_registry->updateComponent(id, coordSys);
}


void PhysicsSystem::tick(std::shared_ptr<WorkerThread> worker) {
	// Cache physics data
	cacheRegistryData();


	// Compute accumulator
	float timeScale = Time::GetTimeScale();
	Time::UpdateDeltaTime();
	double deltaTime = Time::GetDeltaTime();

	double localAccumulator;
	{
		std::lock_guard<std::mutex> lock(m_accumulatorMutex);
		m_accumulator += deltaTime * timeScale;
		localAccumulator = m_accumulator;
	}

	// TODO: Implement adaptive timestepping instead of a constant TIME_STEP
	uint32_t iterations = 0;
	static constexpr uint32_t SYNC_FREQUENCY = 100;

	while (localAccumulator >= SimulationConsts::TIME_STEP) {
		if (worker->stopRequested() || Time::GetTimeScale() != timeScale) {
			// If the thread in which this function is called is requested to be stopped,
			// OR If time scale changes while physics is updating (e.g., when the simulation is stopped in the middle of the updates),
			// immediately stop updating physics
			localAccumulator = 0.0;
			break;
		}

		update(SimulationConsts::TIME_STEP);
		localAccumulator -= SimulationConsts::TIME_STEP;

		// Sync registry data every [SYNC_FREQUENCY] iterations to see frequent visual progress on high time scales
		if (iterations++ % SYNC_FREQUENCY == 0)
			syncRegistryData();
	}

	{
		std::lock_guard<std::mutex> lock(m_accumulatorMutex);
		m_accumulator = localAccumulator;
	}


	// Write cache to ECS registry
	syncRegistryData();
}


void PhysicsSystem::cacheRegistryData() {
	m_generalData.clear();
	m_propData.clear();
	m_identifierData.clear();

	// General data
	auto generalView = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();
	auto propView = m_registry->getView<PhysicsComponent::Propagator, CoreComponent::Transform, PhysicsComponent::RigidBody>();

	m_generalData = generalView.getData();
	m_propData = propView.getData();

	
	// Specific data
	m_identifierData.resize(m_generalData.size());

	for (int i = 0; i < m_generalData.size(); i++) {
		EntityID id = std::get<EntityID>(m_generalData[i]);
		
		// Identifiers
		std::get<EntityID>(m_identifierData[i]) = id;
		std::get<CoreComponent::Identifiers>(m_identifierData[i]) = m_registry->getComponent<CoreComponent::Identifiers>(id);
	}
}


void PhysicsSystem::syncRegistryData() {
	// General data
	for (auto &&[entityID, transform, rigidBody] : m_generalData) {
		m_registry->updateComponent(entityID, transform);
		m_registry->updateComponent(entityID, rigidBody);
	}

	for (auto &&[entityID, propagator, transform, rigidBody] : m_propData) {
		m_registry->updateComponent(entityID, propagator);
		m_registry->updateComponent(entityID, transform);
		m_registry->updateComponent(entityID, rigidBody);
	}


	// Specific data
		// Identifiers (NONE - they should be constant and read-only)
		// Point lights: If an entity is a star, update its point light position to its own
	for (int i = 0; i < m_identifierData.size(); i++) {
		auto &&[entityID, identifier] = m_identifierData[i];

		if (identifier.entityType == CoreComponent::Identifiers::EntityType::STAR) {
			RenderComponent::PointLight &pointLight = m_registry->getComponent<RenderComponent::PointLight>(entityID);

			auto &&[_, transform, _1] = m_generalData[i];
			pointLight.position = SpaceUtils::ToRenderSpace_Position(transform.position);

			m_registry->updateComponent(entityID, pointLight);
		}
	}


	// Update epoch
	auto [id, coordSys] = m_registry->getView<PhysicsComponent::CoordinateSystem>()[0];
	{
		// Convert current seconds past J2000 => string
		static const char *FMT = "C"; // Calendar format, UTC (https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/et2utc_c.html)
		static constexpr int PREC = 5;
		static constexpr int LENOUT = 35;
		static char buf[LENOUT];

		et2utc_c(m_currentEpoch, FMT, PREC, LENOUT, buf);

		coordSys.currentEpoch = std::string(buf);

		// Perform thread-safe writes to a non-atomic variable (NOTE: We can't have std::atomic types in components since atomics are non-copyable and non-movable)
		//std::atomic_ref<std::string> atomic(coordSys.currentEpoch);
		//atomic.store(std::string(buf));
	}
	m_registry->updateComponent(id, coordSys);
}


void PhysicsSystem::update(const double dt) {
	m_currentEpoch = m_coordSystem->getEpochET() + m_simulationTime;
	
	updateSPICEBodies(m_currentEpoch);
	propagateBodies(m_currentEpoch);
	updateGeneralBodies(dt, m_currentEpoch);

	m_simulationTime += dt;
}


void PhysicsSystem::updateSPICEBodies(const double et) {
	//auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();
	

	// Query SPICE data for bodies with available ephemeris data
	for (int i = 0; i < m_generalData.size(); i++) {
		auto &&[entityID, transform, rigidBody] = m_generalData[i];

		auto &&[_, identifiers] = m_identifierData[i];

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


			// Update cache data
			std::get<CoreComponent::Transform>(m_generalData[i]) = transform;
			std::get<PhysicsComponent::RigidBody>(m_generalData[i]) = rigidBody;
			//m_registry->updateComponent(entityID, transform);
			//m_registry->updateComponent(entityID, rigidBody);
		}
	}
}


void PhysicsSystem::updateGeneralBodies(const double dt, const double et) {
	using namespace PhysicsConsts;
	
	//auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody>();


	// Integration and component updating
	for (size_t i = 0; i < m_generalData.size(); i++) {
		auto &&[targetEntityID, targetTransform, targetRigidBody] = m_generalData[i];

		{
			// If the target entity uses SPICE, its state vector has already been computed. We must, therefore, skip them.
			auto &&[_, targetIDs] = m_identifierData[i];
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
		//ode.view = &view;
		ode.bodies = &m_generalData;
		ode.entityID = targetEntityID;


		// Integrate!
		RK4Integrator<Physics::State, ODE::NewtonianNBody>::Integrate(state, et, dt, ode);


		// Update cache data
		targetTransform.position = state.position;

		targetRigidBody.velocity = state.velocity;
		targetRigidBody.acceleration = ode(state, dt).velocity;		// Recompute acceleration again

		std::get<CoreComponent::Transform>(m_generalData[i]) = targetTransform;
		std::get<PhysicsComponent::RigidBody>(m_generalData[i]) = targetRigidBody;
		//m_registry->updateComponent(targetEntityID, targetTransform);
		//m_registry->updateComponent(targetEntityID, targetRigidBody);
	}
}


void PhysicsSystem::propagateBodies(const double et) {
	//auto view = m_registry->getView<PhysicsComponent::Propagator, CoreComponent::Transform, PhysicsComponent::RigidBody>();

	for (int i = 0; i < m_propData.size(); i++) {
		auto &&[entityID, propagator, transform, rigidBody] = m_propData[i];

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


		// Write new data to the cache
		transform.position = glm::dvec3(
			stateVec[0], stateVec[1], stateVec[2]
		);

		rigidBody.velocity = glm::dvec3(
			stateVec[3], stateVec[4], stateVec[5]
		);


		std::get<CoreComponent::Transform>(m_propData[i]) = transform;
		std::get<PhysicsComponent::RigidBody>(m_propData[i]) = rigidBody;
		//m_registry->updateComponent(entityID, transform);
		//m_registry->updateComponent(entityID, rigidBody);
	}
}


void PhysicsSystem::homogenizeCoordinateSystems() {
	// Convert TLE state vectors (TEME coordinate system)
	//auto view = m_registry->getView<PhysicsComponent::Propagator, CoreComponent::Transform, PhysicsComponent::RigidBody>();

	for (int i = 0; i < m_propData.size(); i++) {
		auto &&[entityID, propagator, transform, rigidBody] = m_propData[i];

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


		std::get<PhysicsComponent::Propagator>(m_propData[i]) = propagator;
		std::get<CoreComponent::Transform>(m_propData[i]) = transform;
		std::get<PhysicsComponent::RigidBody>(m_propData[i]) = rigidBody;
		//m_registry->updateComponent(entityID, propagator);
		//m_registry->updateComponent(entityID, transform);
		//m_registry->updateComponent(entityID, rigidBody);
	}
}
