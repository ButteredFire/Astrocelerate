#include "SceneManager.hpp"

SceneManager::SceneManager() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void SceneManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::BufferManagerIsValid>(
		[this](const Event::BufferManagerIsValid &event) {
			loadScene();
		}
	);
}


void SceneManager::init() {
	// Initialize global reference frame
	m_renderSpace = m_registry->createEntity("Scene");

	PhysicsComponent::ReferenceFrame globalRefFrame{};
	globalRefFrame.parentID = std::nullopt;
	globalRefFrame.scale = 1.0;
	globalRefFrame.visualScale = 1.0;
	globalRefFrame.relativeScale = 1.0;
	globalRefFrame.localTransform.position = glm::dvec3(0.0);
	globalRefFrame.localTransform.rotation = glm::dquat(1.0, 0.0, 0.0, 0.0);

	m_registry->addComponent(m_renderSpace.id, globalRefFrame);
}


void SceneManager::loadScene() {
	// TODO: Deserialize scene data and load scene. Right now, we can only load the default, hardcoded scene.
	
	loadDefaultScene();
}


void SceneManager::loadDefaultScene() {
	// Rigid bodies
	Entity planet = m_registry->createEntity("Earth");
	Entity satellite = m_registry->createEntity("Satellite");


	// Common mesh physical data
	constexpr double sunRadius = 6.9634e+8;
	constexpr double earthRadius = 6.378e+6;
	constexpr double earthMass = 5.972e+24;


	// Load geometry and mesh physical and render data
	std::string genericCubeSat = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/Models/TestModels", "GenericCubeSat/GenericCubeSat.gltf");
	std::string chandraObservatory = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/Models/Satellites", "Chandra/Chandra.gltf");
	std::string VNREDSat = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/Models/Satellites", "VNREDSat/VNREDSat.gltf");

	std::string earth = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/Models/CelestialBodies", "Earth/Earth.gltf");

	std::string cube = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/Models/TestModels", "Cube/Cube.gltf");

	std::string planetPath = earth;
	std::string satellitePath = VNREDSat;


	// Earth configuration (Fact sheet: https://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html)
	Math::Interval<uint32_t> planetRange = m_geometryLoader.loadGeometryFromFile(planetPath);
	{
		PhysicsComponent::ReferenceFrame planetRefFrame{};
		planetRefFrame.parentID = m_renderSpace.id;
		planetRefFrame.scale = earthRadius;
		planetRefFrame.visualScale = 10.0;
		planetRefFrame.relativeScale = 1.0;
		//planetRefFrame.visualScaleAffectsChildren = false;
		planetRefFrame.localTransform.position = glm::dvec3(0.0, 0.0, 0.0);
		planetRefFrame.localTransform.rotation = glm::dquat(1, 0, 0, 0);


		PhysicsComponent::RigidBody planetRB{};
		planetRB.velocity = glm::dvec3(0.0, 0.0, 0.0);
		planetRB.acceleration = glm::dvec3(0.0);
		planetRB.mass = earthMass;


		PhysicsComponent::ShapeParameters planetShapeParams{};
		planetShapeParams.equatRadius = earthRadius;
		planetShapeParams.eccentricity = 0.0167;
		planetShapeParams.gravParam = PhysicsConsts::G * planetRB.mass;
		planetShapeParams.rotVelocity = 7.2921159e-5;


		RenderComponent::MeshRenderable planetRenderable{};
		planetRenderable.meshRange = planetRange;


		m_registry->addComponent(planet.id, planetRefFrame);
		m_registry->addComponent(planet.id, planetRB);
		m_registry->addComponent(planet.id, planetShapeParams);
		m_registry->addComponent(planet.id, planetRenderable);
		m_registry->addComponent(planet.id, TelemetryComponent::RenderTransform{});
	}


	// Satellite configuration
	Math::Interval<uint32_t> satelliteRange = m_geometryLoader.loadGeometryFromFile(satellitePath);
	{
		PhysicsComponent::ReferenceFrame satelliteRefFrame{};
		satelliteRefFrame.parentID = planet.id;
		satelliteRefFrame.scale = 100;
		satelliteRefFrame.visualScale = 1.0;
		satelliteRefFrame.relativeScale = satelliteRefFrame.scale / earthRadius;
		satelliteRefFrame.localTransform.position = glm::dvec3(0.0, (earthRadius + 2e6), 0.0);
		satelliteRefFrame.localTransform.rotation = glm::dquat(1, 0, 0, 0);


		double earthOrbitalSpeed = sqrt(PhysicsConsts::G * earthMass / glm::length(satelliteRefFrame.localTransform.position));

		PhysicsComponent::RigidBody satelliteRB{};
		satelliteRB.velocity = glm::dvec3(-earthOrbitalSpeed, 0.0, 1080.0);
		satelliteRB.acceleration = glm::dvec3(0.0);
		satelliteRB.mass = 20;


		PhysicsComponent::OrbitingBody satelliteOB{};
		satelliteOB.centralMass = earthMass;


		RenderComponent::MeshRenderable satelliteRenderable{};
		satelliteRenderable.meshRange = satelliteRange;


		m_registry->addComponent(satellite.id, satelliteRB);
		m_registry->addComponent(satellite.id, satelliteRefFrame);
		m_registry->addComponent(satellite.id, satelliteOB);
		m_registry->addComponent(satellite.id, satelliteRenderable);
		m_registry->addComponent(satellite.id, TelemetryComponent::RenderTransform{});
	}


	m_geomData = m_geometryLoader.bakeGeometry();
	m_meshCount = m_geomData->meshCount;

	RenderComponent::SceneData globalSceneData{};
	globalSceneData.pGeomData = m_geomData;
	m_registry->addComponent(m_renderSpace.id, globalSceneData);
}
