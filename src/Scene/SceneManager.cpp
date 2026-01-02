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
    m_renderSpace = m_registry->getRenderSpaceEntity();

    // Model preloading
    loadModels();
}


void SceneManager::loadModels() {
    loadDefaultModels();
}


void SceneManager::loadSceneFromFile(const std::string &filePath) {
    m_geomLoadWorkers.clear();

    m_fileName = FilePathUtils::GetFileName(filePath);


    // Worker thread progress tracking
    m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
        .progress = 0.0f,
        .message = "Preparing scene load from simulation file " + enquote(m_fileName) + "..."
    });

	Log::Print(Log::T_INFO, __FUNCTION__, "Selected simulation file: " + enquote(m_fileName) + ". Loading scene...");


    std::string currentEntity;
    std::string currentComponent;

	try {
        YAML::Node rootNode = YAML::LoadFile(filePath);
        
        // ----- PROCESS FILE & SIMULATION CONFIGURATIONS -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f,
            .message = "Processing scene metadata..."
        });

        Application::YAMLFileConfig fileConfig;
        Application::SimulationConfig simulationConfig;
        processMetadata(&fileConfig, &simulationConfig, rootNode);

        m_eventDispatcher->dispatch(ConfigEvent::SimulationFileParsed{
           .fileConfig = fileConfig,
           .simulationConfig = simulationConfig
        }, false, true);



        // ----- PROCESS SCENE -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f,
            .message = "Processing scene..."
        });

        processScene(rootNode, currentEntity, currentComponent);



        // TODO: Concurrently process meshes
        //for (auto &workerData : m_geomLoadWorkers) {
        //    m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
        //        .progress = 0.1f + (workerData.entityProcessPercentage * 0.75f),
        //        .message = "[" + std::string(workerData.entityName) + "] Loading geometry..."
        //    });
        //
        //    workerData.worker.join();
        //}


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
        std::string msg = e.what();
        msg[0] = std::toupper(msg[0]);

		throw Log::RuntimeException(__FUNCTION__, __LINE__, msg);

        // Re-throw the original exception to the outer try-catch block (i.e., the Session try-catch block, for it to reset its status)
        throw;
	}
	catch (const YAML::ParserException &e) {
        std::string msg = e.what();
        msg[0] = std::toupper(msg[0]);

        std::string entityData;
        if (currentEntity.length() > 0 && currentComponent.length() > 0)
            entityData = "(Entity " + enquote(currentEntity) + ", Component " + currentComponent + ")\n";

        throw Log::RuntimeException(__FUNCTION__, __LINE__, entityData + msg);
        throw;
	}
	catch (const std::exception &e) {
        std::string msg = e.what();
        msg[0] = std::toupper(msg[0]);

        std::string entityData;
        if (currentEntity.length() > 0 && currentComponent.length() > 0)
            entityData = "(Entity " + enquote(currentEntity) + ", Component " + currentComponent + ")\n";

        throw Log::RuntimeException(__FUNCTION__, __LINE__, entityData + msg);
        throw;
	}


	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Successfully loaded scene from simulation file " + enquote(m_fileName) + ".");
}


void SceneManager::saveSceneToFile(const std::string &filePath) {

}


void SceneManager::loadDefaultModels() {
    const std::string SPHERE_MESH = FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/TestModels/Sphere/Sphere.gltf");
    
    m_sphereMesh = m_geometryLoader.loadGeometryFromFile(SPHERE_MESH);
}


void SceneManager::processMetadata(Application::YAMLFileConfig *fileConfig, Application::SimulationConfig *simConfig, const YAML::Node &rootNode) {
    const auto fileCfgRoot = rootNode[YAMLFileConfig::ROOT];
    const auto simCfgRoot = rootNode[YAMLSimConfig::ROOT];

    // File configuration
    if (fileCfgRoot) {
        fileConfig->fileName = m_fileName;

        if (!YAMLUtils::TryGetEntryData(&fileConfig->version, YAMLFileConfig::Version, fileCfgRoot))
            Log::Print(Log::T_WARNING, __FUNCTION__, "File configuration does not include simulation version! This simulation may be incompatible.");

        YAMLUtils::TryGetEntryData(&fileConfig->description, YAMLFileConfig::Description, fileCfgRoot);
    }

    else {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to process metadata: File configuration does not exist!");
        throw;
    }


    // Simulation configuration
    if (simCfgRoot) {
        // SPICE kernels
        const auto kernelsNode = simCfgRoot[YAMLSimConfig::Kernels];
        if (!kernelsNode) {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Simulation configuration does not contain SPICE kernel paths!");
            throw;
        }

        else {
            const size_t kernelCount = kernelsNode.size();
            simConfig->kernelPaths.resize(kernelCount);

            for (size_t i = 0; i < kernelCount; i++) {
                YAMLUtils::TryGetEntryData(&simConfig->kernelPaths[i], YAMLSimConfig::Kernel_Path, kernelsNode[i]);
            }
        }


        // Coordinate system
        const auto coordSysNode = simCfgRoot[YAMLSimConfig::CoordSys];
        if (!coordSysNode) {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Simulation configuration does not contain information on the coordinate system!");
            throw;
        }

        else {
            std::string frameStr;
            if (YAMLUtils::TryGetEntryData(&frameStr, YAMLSimConfig::CoordSys_Frame, coordSysNode)) {
                simConfig->frame = CoordSys::FrameYAMLToEnumMap.at(frameStr);
                simConfig->frameType = CoordSys::FrameProperties.at(simConfig->frame).frameType;
            }

            std::string epochStr;
            if (YAMLUtils::TryGetEntryData(&epochStr, YAMLSimConfig::CoordSys_Epoch, coordSysNode)) {
                simConfig->epoch = CoordSys::EpochStrToEnumMap.at(epochStr);
                YAMLUtils::TryGetEntryData(&simConfig->epochFormat, YAMLSimConfig::CoordSys_EpochFormat, coordSysNode);
            }
        }
    }

    else {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to process metadata: Simulation configuration does not exist!");
        throw;
    }


    // Create entities
    Entity coordSystem = m_registry->createEntity(CoordSys::FrameProperties.at(simConfig->frame).displayName);
    m_registry->addComponent(coordSystem.id, PhysicsComponent::CoordinateSystem{
        .simulationConfig = *simConfig
    });
}


void SceneManager::processScene(const YAML::Node &rootNode, std::string &currentEntity, std::string &currentComponent) {
    // Logging & other utilities
#define logMissingComponent(componentName) Log::Print(Log::T_WARNING, __FUNCTION__, "In simulation file " + m_fileName + ": Essential component " + enquote((componentName)) + " is missing!")
#define info(entityName, componentType) "Entity " + enquote((entityName)) + ", component " + enquote((componentType)) + ": "

    std::function<void(EntityID)> addPointLight = [this](EntityID entityID) {
        static constexpr double SOLAR_LUMINOSITY = 3.828e26;

        RenderComponent::PointLight pointLight{};
        pointLight.color = glm::vec3(1.0f, 0.95f, 0.90f);
        pointLight.radiantFlux = SOLAR_LUMINOSITY / std::pow(SimulationConst::SIMULATION_SCALE, 2.3);

        m_registry->addComponent(entityID, pointLight);
    };



    const auto sceneRoot = rootNode[YAMLScene::ROOT];
    LOG_ASSERT(sceneRoot, "There is nothing to process!");

    size_t processedEntities = 0;
    size_t totalEntities = sceneRoot.size();

    std::map<std::string, EntityID> entityNameToID; // Map entity names to their runtime IDs



    // Components that MUST be present in any entity: (std::string componentName, bool presentInEntity)
    const std::unordered_set<std::string> coreComponents = {
        YAMLScene::Core_Identifiers,
        YAMLScene::Core_Transform,
        YAMLScene::Physics_RigidBody,
        YAMLScene::Render_MeshRenderable
    };

        // Core components that dynamically vary based on entity type
    using enum CoreComponent::Identifiers::EntityType;
    const std::unordered_map<CoreComponent::Identifiers::EntityType, std::vector<std::string>> coreEntityComponents = {
        { PLANET, {YAMLScene::Physics_ShapeParameters} },
        { SPACECRAFT, {YAMLScene::Spacecraft_Spacecraft} }
    };

        // Vector to keep track of current components an entity has
    std::unordered_set<std::string> currentComponents{};



    // ----- PROCESS ALL ENTITIES IN THE SCENE -----
    for (const auto &entityNode : sceneRoot) {
        // Create entity
        std::string entityName = entityNode[YAMLScene::Entity].as<std::string>();

            // Remove any special prefixes from the display name
        std::string originalEntityName = entityName;
        if (StringUtils::BeginsWith(entityName, YAMLScene::Body_Prefix))
            entityName = entityName.substr(YAMLScene::Body_Prefix.length());
        
            // Register entity
        Entity newEntity = m_registry->createEntity(entityName);
        LOG_ASSERT(entityNameToID.count(entityName) == 0, "Found multiple " + enquote(entityName) + " entities! Please ensure entity names are unique.");

        entityNameToID[entityName] = newEntity.id;
        currentEntity = entityName;

            // Automatically add to telemetry dashboard (if applicable)
        m_registry->addComponent(newEntity.id, TelemetryComponent::RenderTransform{});

        

        // Update progress
        processedEntities++;
        float entityProcessingProgress = static_cast<float>(processedEntities) / totalEntities;

        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f + (entityProcessingProgress * 0.75f),
            .message = "[" + std::string(entityName) + "] Processing components..."
        });



        // If entity is a special entity, automatically create its components, and proceed to the next entity.
            // ----- ENTITY IS A BUILT-IN CELESTIAL BODY -----
        if (StringUtils::BeginsWith(originalEntityName, YAMLScene::Body_Prefix)) {
            ICelestialBody *celestialBody = Body::GetCelestialBody(originalEntityName);

            CoreComponent::Identifiers identifiers = celestialBody->getIdentifiers();

            CoreComponent::Transform transform{};
            transform.scale = celestialBody->getEquatRadius();

            PhysicsComponent::RigidBody rigidBody{};
            rigidBody.mass = celestialBody->getMass();

            PhysicsComponent::ShapeParameters shapeParams{};
            shapeParams.equatRadius = celestialBody->getEquatRadius();
            shapeParams.flattening = celestialBody->getFlattening();
            shapeParams.gravParam = celestialBody->getGravParam();
            shapeParams.rotVelocity = celestialBody->getRotVelocity();
            shapeParams.j2 = celestialBody->getJ2();

            RenderComponent::MeshRenderable meshRenderable{};
            meshRenderable.meshPath = celestialBody->getMeshPath();
            meshRenderable.meshRange = m_geometryLoader.loadGeometryFromFile(meshRenderable.meshPath);
            meshRenderable.visualScale = 1.0;


            m_registry->addComponent(newEntity.id, identifiers);
            m_registry->addComponent(newEntity.id, transform);
            m_registry->addComponent(newEntity.id, rigidBody);
            m_registry->addComponent(newEntity.id, shapeParams);
            m_registry->addComponent(newEntity.id, meshRenderable);


            // Special case: Entity is a star
            if (identifiers.entityType == CoreComponent::Identifiers::EntityType::STAR)
                addPointLight(newEntity.id);
            
            continue;
        }



        // Otherwise,
        // ----- PROCESS ALL COMPONENTS FOR EACH ENTITY -----
        CoreComponent::Identifiers::EntityType entityType = UNKNOWN;
        currentComponents.clear();

        for (const auto &componentNode : entityNode[YAMLScene::Entity_Components]) {
            std::string componentType = componentNode[YAMLScene::Entity_Components_Type].as<std::string>();

            if (currentComponents.count(componentType)) {
                // Duplicate components
                throw Log::RuntimeException(__FUNCTION__, __LINE__, "Entity " + enquote(entityName) + " has duplicate " + componentType + "components!\nEach entity can have only one component for each component type.");
                throw;
            }

            currentComponents.insert(componentType);
            currentComponent = componentType;


            // ----- CORE/PHYSICS -----
            if (componentType == YAMLScene::Core_Identifiers) {
                CoreComponent::Identifiers identifiers{};

                YAMLUtils::GetComponentData(&identifiers, componentNode);

                entityType = identifiers.entityType;
                m_registry->addComponent(newEntity.id, identifiers);


                // SPECIAL CASE: If the entity is a star, add a point light at its center of mass.
                if (entityType == CoreComponent::Identifiers::EntityType::STAR)
                    addPointLight(newEntity.id);
            }


            if (componentType == YAMLScene::Core_Transform) {
                CoreComponent::Transform transform{};

                if (!YAMLUtils::GetComponentData(&transform, componentNode))
                    logMissingComponent(componentType);

                m_registry->addComponent(newEntity.id, transform);
            }


            else if (componentType == YAMLScene::Physics_RigidBody) {
                PhysicsComponent::RigidBody rigidBody{};

                if (!YAMLUtils::GetComponentData(&rigidBody, componentNode))
                    logMissingComponent(componentType);

                m_registry->addComponent(newEntity.id, rigidBody);
            }


            else if (componentType == YAMLScene::Physics_Propagator) {
                PhysicsComponent::Propagator propagator{};

                YAMLUtils::GetComponentData(&propagator, componentNode);

                // Get the last 2 compact lines in the TLE (which I assume to always be the last 2 lines)
                {
                    propagator.tlePath = FilePathUtils::JoinPaths(ROOT_DIR, propagator.tlePath);
                    std::vector<std::string> tleLines = FilePathUtils::GetFileLines(
                        FilePathUtils::ReadFile(propagator.tlePath)
                    );

                    if (tleLines.size() < 2) {
                        throw Log::RuntimeException(__FUNCTION__, __LINE__, "TLE file is invalid or contains too little data!");
                        throw;
                    }

                    propagator.tleLine1 = tleLines[tleLines.size() - 2];
                    propagator.tleLine2 = tleLines[tleLines.size() - 1];
                }

                m_registry->addComponent(newEntity.id, propagator);
            }


            else if (componentType == YAMLScene::Physics_ShapeParameters) {
                PhysicsComponent::ShapeParameters shapeParams{};

                if (!YAMLUtils::GetComponentData(&shapeParams, componentNode))
                    logMissingComponent(componentType);

                m_registry->addComponent(newEntity.id, shapeParams);
            }



            // ----- SPACECRAFT -----
            else if (componentType == YAMLScene::Spacecraft_Spacecraft) {
                SpacecraftComponent::Spacecraft spacecraft{};

                YAMLUtils::GetComponentData(&spacecraft, componentNode);
                m_registry->addComponent(newEntity.id, spacecraft);
            }



            else if (componentType == YAMLScene::Spacecraft_Thruster) {
                SpacecraftComponent::Thruster thruster{};

                YAMLUtils::GetComponentData(&thruster, componentNode);
                m_registry->addComponent(newEntity.id, thruster);
            }



            // ----- RENDERING -----
            else if (componentType == YAMLScene::Render_MeshRenderable) {
                RenderComponent::MeshRenderable meshRenderable{};

                if (!YAMLUtils::GetComponentData(&meshRenderable, componentNode))
                    logMissingComponent(componentType);

                const auto &renderableNode = componentNode[YAMLScene::Entity_Components_Type_Data];
                if (renderableNode[YAMLData::Render_MeshRenderable_MeshPath]) {
                    std::string meshPath = renderableNode[YAMLData::Render_MeshRenderable_MeshPath].as<std::string>();


                    //m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
                    //    .progress = 0.1f + (entityProcessingProgress * 0.75f),
                    //    .message = "[" + std::string(entityName) + "] Loading geometry..."
                    //});

                    std::string fullPath = FilePathUtils::JoinPaths(ROOT_DIR, meshPath);
                    Math::Interval<uint32_t> meshRange = m_geometryLoader.loadGeometryFromFile(fullPath);

                    meshRenderable.meshRange = meshRange;
                    m_registry->addComponent(newEntity.id, meshRenderable);

                    // TODO: Concurrently process meshes
                    //m_geomLoadWorkers.push_back(
                    //    m_WorkerData{
                    //        .entityProcessPercentage = entityProcessingProgress,
                    //        .entityName = entityName,
                    //
                    //        .worker = ThreadManager::CreateThread("GEOMETRY_LOAD",
                    //            [this, meshPath, meshRenderable, newEntity]() {
                    //                RenderComponent::MeshRenderable renderableCopy = meshRenderable; // meshRenderable is const for some reason
                    //
                    //                std::string fullPath = FilePathUtils::JoinPaths(ROOT_DIR, meshPath);
                    //                renderableCopy.meshRange = m_geometryLoader.loadGeometryFromFile(fullPath);
                    //
                    //                m_registry->addComponent(newEntity.id, renderableCopy);
                    //            }
                    //        )
                    //    }
                    //);
                }
                else {
                    Log::Print(Log::T_ERROR, __FUNCTION__, info(entityName, componentType) + "Model path is not provided!");
                    continue;
                }
            }
        }
    
        
        // Check if all core components are present
        std::stringstream stream;
        size_t missingComponentCount = 0;

        for (const auto &component : coreComponents)
            if (!currentComponents.count(component)) {
                stream << "\n-\t" << component;
                missingComponentCount++;
            }

        for (const auto &[entity, components] : coreEntityComponents)
            if (entityType == entity) {
                for (const auto &component : components)
                    if (!currentComponents.count(component)) {
                        stream << "\n-\t" << component;
                        missingComponentCount++;
                    }

                break;
            }

        LOG_ASSERT(missingComponentCount == 0, TO_STR(missingComponentCount) + " core " + PLURAL(missingComponentCount, "component", "components") + " are missing:" + stream.str());
    }
}
