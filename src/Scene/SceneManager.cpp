#include "SceneManager.hpp"

SceneManager::SceneManager() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void SceneManager::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<SceneManager>();

    m_eventDispatcher->subscribe<UpdateEvent::RegistryReset>(selfIndex,
        [this](const UpdateEvent::RegistryReset &event) {
            this->init();
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


void SceneManager::loadSceneFromFile(const std::string &filePath) {
	const std::string fileName = FilePathUtils::GetFileName(filePath);


#define logMissingComponent(componentName) Log::Print(Log::T_WARNING, __FUNCTION__, "In simulation file " + fileName + ": Essential component " + enquote((componentName)) + " is missing!")
#define info(entityName, componentType) "Entity " + enquote((entityName)) + ", component " + enquote((componentType)) + ": "


    // Worker thread progress tracking
    m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
        .progress = 0.0f,
        .message = "Preparing scene load from simulation file " + enquote(fileName) + "..."
    });

    std::map<std::string, EntityID> entityNameToID; // Map entity names to their runtime IDs
    std::map<std::string, double> entityNameToMass; // Map entity names to their masses

	Log::Print(Log::T_INFO, __FUNCTION__, "Selected simulation file: " + enquote(fileName) + ". Loading scene...");

	try {
        YAML::Node scene = YAML::LoadFile(filePath);

        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f,
            .message = "Acquiring scene data..."
        });

        size_t processedEntities = 0;
        size_t totalEntities = 0;
        if (scene["Entities"])
            totalEntities = scene["Entities"].size();


        // ----- Query each entity -----
        for (const auto &entityNode : scene["Entities"]) {
            std::string entityName = entityNode["Name"].as<std::string>();
            Entity newEntity = m_registry->createEntity(entityName);

            if (entityNameToID.count(entityName) > 0) {
                Log::Print(Log::T_ERROR, __FUNCTION__, "Found duplicate " + enquote(entityName) + " entities! Please ensure entity names are unique.");
                throw;
            }

            entityNameToID[entityName] = newEntity.id;


            processedEntities++;
            float entityProcessingProgress = static_cast<float>(processedEntities) / totalEntities;
            m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
                .progress = 0.1f + (entityProcessingProgress * 0.75f),
                .message = "[" + std::string(entityName) + "] Parsing components..."
            });


            // ----- Scene Data Deserialization -----
            for (const auto &componentNode : entityNode["Components"]) {
                std::string componentType = componentNode["Type"].as<std::string>();


                // ----- PHYSICS -----
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
                            if (parentName.size() <= YAMLKey::Ref.size()) {
                                Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Reference value for parentID cannot be empty!");
                                continue;
                            }
                            

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

                    if (componentNode["Data"]) { // Check if 'Data' node exists
                        shapeParams = componentNode["Data"].as<PhysicsComponent::ShapeParameters>();
                        m_registry->addComponent(newEntity.id, shapeParams);
                    }
                    else {
                        logMissingComponent(componentType);
                    }
                }


                else if (componentType == YAMLKey::Physics_OrbitingBody) {
                    PhysicsComponent::OrbitingBody orbitingBody;

                    if (!YAMLUtils::GetComponentData(componentNode, orbitingBody))
                        logMissingComponent(componentType);

                    bool refIsValid = false;
                    if (componentNode["Data"]["centralMass"]) {
                        std::string centralMassRef = componentNode["Data"]["centralMass"].as<std::string>();

						if (centralMassRef.starts_with(YAMLKey::Ref)) {
                            if (centralMassRef.size() <= YAMLKey::Ref.size()) {
                                Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Reference value for centralMass cannot be empty!");
                                continue;
                            }

							centralMassRef = YAMLUtils::GetReferenceSubstring(centralMassRef);
                            if (entityNameToMass.count(centralMassRef)) {
                                refIsValid = true;
                                orbitingBody.centralMass = entityNameToMass[centralMassRef];
                            }
                            else {
                                Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Unable to resolve reference " + enquote(centralMassRef) + "! Reference must be a valid entity name.");
                                continue;
                            }
						}
                    }

                    if (!refIsValid) {
                        Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Central mass reference value is not provided!");
                        continue;
                    }

                    m_registry->addComponent(newEntity.id, orbitingBody);
                }



                // ----- SPACECRAFT -----
                else if (componentType == YAMLKey::Spacecraft_Spacecraft) {
                    SpacecraftComponent::Spacecraft spacecraft{};

                    YAMLUtils::GetComponentData(componentNode, spacecraft);
                    m_registry->addComponent(newEntity.id, spacecraft);
                }



                else if (componentType == YAMLKey::Spacecraft_Thruster) {
                    SpacecraftComponent::Thruster thruster{};

                    YAMLUtils::GetComponentData(componentNode, thruster);
                    m_registry->addComponent(newEntity.id, thruster);
                }



                // ----- RENDERING -----
                else if (componentType == YAMLKey::Render_MeshRenderable) {
                    RenderComponent::MeshRenderable meshRenderable{};

                    if (!YAMLUtils::GetComponentData(componentNode, meshRenderable))
                        logMissingComponent(componentType);

                    if (componentNode["Data"]["meshPath"]) {
                        std::string meshPath = componentNode["Data"]["meshPath"].as<std::string>();

                        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
                            .progress = 0.1f + (entityProcessingProgress * 0.75f),
                            .message = "[" + std::string(entityName) + "] Loading geometry..."
                        });

                        std::string fullPath = FilePathUtils::JoinPaths(ROOT_DIR, meshPath);
                        Math::Interval<uint32_t> meshRange = m_geometryLoader.loadGeometryFromFile(fullPath);

                        meshRenderable.meshRange = meshRange;
                        m_registry->addComponent(newEntity.id, meshRenderable);
                    }
                    else {
                        Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Model path is not provided!");
                        continue;
                    }
                }


                else if (componentType == YAMLKey::Telemetry_RenderTransform) {
                    // This component has no data, just add it as a tag
                    m_registry->addComponent(newEntity.id, TelemetryComponent::RenderTransform{});
                }
            }
        }


        // ----- Finalize geometry baking -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.9f,
            .message = "Baking geometry data..."
        });

		m_geomData = m_geometryLoader.bakeGeometry();
		m_meshCount = m_geomData->meshCount;

		RenderComponent::SceneData globalSceneData{};
		globalSceneData.pGeomData = m_geomData;
		m_registry->addComponent(m_renderSpace.id, globalSceneData);


        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.95f,
            .message = "Initializing resources..."
        });
	}

	catch (const YAML::BadFile &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to read data from simulation file " + enquote(fileName) + ": " + e.what());

        // Re-throw the original exception to the outer try-catch block (i.e., the Session try-catch block, for it to reset its status)
        throw;
	}
	catch (const YAML::ParserException &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to parse data from simulation file " + enquote(fileName) + ": " + e.what());
        throw;
	}
	catch (const std::exception &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to load scene from simulation file " + enquote(fileName) + ": " + e.what());
        throw;
	}


	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Successfully loaded scene from simulation file " + enquote(fileName) + ".");
}


void SceneManager::saveSceneToFile(const std::string &filePath) {

}
