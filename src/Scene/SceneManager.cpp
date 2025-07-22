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
	globalRefFrame.localTransform.position = glm::dvec3(0.0);
	globalRefFrame.localTransform.rotation = glm::dquat(1.0, 0.0, 0.0, 0.0);

	m_registry->addComponent(m_renderSpace.id, globalRefFrame);
}


void SceneManager::loadScene() {
	loadSceneFromFile(
		FilePathUtils::JoinPaths(APP_SOURCE_DIR, "samples", "SatelliteInLEO.yaml")
	);
}


void SceneManager::loadSceneFromFile(const std::string &filePath) {
	const std::string fileName = FilePathUtils::GetFileName(filePath);


#define logMissingComponent(componentName) Log::Print(Log::T_WARNING, __FUNCTION__, "In simulation file " + fileName + ": Essential component " + enquote((componentName)) + " is missing!")
#define info(entityName, componentType) "Entity " + enquote((entityName)) + ", component " + enquote((componentType)) + ": "


    std::map<std::string, EntityID> entityNameToID; // Map entity names to their runtime IDs
    std::map<std::string, double> entityNameToMass; // Map entity names to their masses

	Log::Print(Log::T_INFO, __FUNCTION__, "Selected simulation file: " + enquote(fileName) + ". Loading scene...");

	try {
        YAML::Node scene = YAML::LoadFile(filePath);

        // ----- Query each entity -----
        for (const auto &entityNode : scene["Entities"]) {
            std::string entityName = entityNode["Name"].as<std::string>();
            Entity newEntity = m_registry->createEntity(entityName);

			LOG_ASSERT(entityNameToID.count(entityName) == 0, "Found duplicate " + enquote(entityName) + " entities! Please ensure entity names are unique.");

            entityNameToID[entityName] = newEntity.id;


            // ----- Load Geometry Data -----
            for (const auto &componentNode : entityNode["Components"]) {
                std::string componentType = componentNode["Type"].as<std::string>();

                if (componentType == YAMLKey::Physics_ReferenceFrame) {
                    PhysicsComponent::ReferenceFrame refFrame{};

                    if (!YAMLUtils::GetComponentData(componentNode, refFrame)) {
                        logMissingComponent(componentType);
                        continue;
                    }

                    bool validID = false;
					if (componentNode["Data"]["parentID"]) {
						std::string parentName = componentNode["Data"]["parentID"].as<std::string>();

						if (parentName.starts_with(YAMLKey::Ref)) {
							// The reference frame references an entity name as their parent
							LOG_ASSERT(parentName.size() > YAMLKey::Ref.size(), info(entityName, componentType) + "Reference value for parentID cannot be empty!");

							parentName = YAMLUtils::GetReferenceSubstring(parentName);
							if (entityNameToID.count(parentName) && m_registry->hasEntity(entityNameToID[parentName])); {
								validID = true;
								refFrame.parentID = entityNameToID[parentName];
							}
						}
						else {
							// The reference frame is null
							validID = true;
							refFrame.parentID = m_renderSpace.id;
						}
					}

                    if (!validID) {
                        Log::Print(Log::T_WARNING, __FUNCTION__, info(entityName, componentType) + "Cannot find parent. Defaulting to render space as the parent!");
                        refFrame.parentID = m_renderSpace.id;
                    }


                    m_registry->addComponent(newEntity.id, refFrame);
                }


                else if (componentType == YAMLKey::Physics_RigidBody) {
                    PhysicsComponent::RigidBody rigidBody{};

                    if (!YAMLUtils::GetComponentData(componentNode, rigidBody))
                        logMissingComponent(componentType);

                    m_registry->addComponent(newEntity.id, rigidBody);
                    entityNameToMass[entityName] = rigidBody.mass;  // Store mass for later lookups
                }


                else if (componentType == YAMLKey::Physics_ShapeParameters) {
                    PhysicsComponent::ShapeParameters shapeParams{};

                    YAMLUtils::GetComponentData(componentNode, shapeParams);
                    m_registry->addComponent(newEntity.id, shapeParams);
                }


                else if (componentType == YAMLKey::Physics_OrbitingBody) {
                    PhysicsComponent::OrbitingBody orbitingBody;

                    if (!YAMLUtils::GetComponentData(componentNode, orbitingBody))
                        logMissingComponent(componentType);

                    bool refIsValid = false;
                    if (componentNode["Data"]["centralMass"]) {
                        std::string centralMassRef = componentNode["Data"]["centralMass"].as<std::string>();

						if (centralMassRef.starts_with(YAMLKey::Ref)) {
							LOG_ASSERT(centralMassRef.size() > YAMLKey::Ref.size(), info(entityName, componentType) + "Reference value for centralMass cannot be empty!");

							centralMassRef = YAMLUtils::GetReferenceSubstring(centralMassRef);
							if (entityNameToMass.count(centralMassRef)) {
								refIsValid = true;
								orbitingBody.centralMass = entityNameToMass[centralMassRef];
							}
						}
                    }

                    if (!refIsValid) {
                        Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Central mass reference value is not provided!");
                    }

                    m_registry->addComponent(newEntity.id, orbitingBody);
                }


                else if (componentType == YAMLKey::Render_MeshRenderable) {
                    RenderComponent::MeshRenderable meshRenderable{};

                    if (!YAMLUtils::GetComponentData(componentNode, meshRenderable))
                        logMissingComponent(componentType);

                    if (componentNode["Data"]["meshPath"]) {
                        std::string meshPath = componentNode["Data"]["meshPath"].as<std::string>();

                        std::string fullPath = FilePathUtils::JoinPaths(APP_SOURCE_DIR, meshPath);
                        Math::Interval<uint32_t> meshRange = m_geometryLoader.loadGeometryFromFile(fullPath);

                        meshRenderable.meshRange = meshRange;
                        m_registry->addComponent(newEntity.id, meshRenderable);
                    }
                    else {
                        Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Model path is not provided!");
                    }
                }


                else if (componentType == YAMLKey::Telemetry_RenderTransform) {
                    // This component has no data, just add it as a tag
                    m_registry->addComponent(newEntity.id, TelemetryComponent::RenderTransform{});
                }
            }
        }


        // ----- Finalize geometry baking -----
		m_geomData = m_geometryLoader.bakeGeometry();
		m_meshCount = m_geomData->meshCount;

		RenderComponent::SceneData globalSceneData{};
		globalSceneData.pGeomData = m_geomData;
		m_registry->addComponent(m_renderSpace.id, globalSceneData);
	}

	catch (const YAML::BadFile &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to read data from simulation file " + enquote(fileName) + ": " + e.what());
	}
	catch (const YAML::ParserException &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to parse data from simulation file " + enquote(fileName) + ": " + e.what());
	}
	catch (const std::exception &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to load scene from simulation file " + enquote(fileName) + ": " + e.what());
	}


	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Successfully loaded scene from simulation file " + enquote(fileName) + ".");
}


void SceneManager::saveSceneToFile(const std::string &filePath) {

}
