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


	try {
        YAML::Node rootNode = YAML::LoadFile(filePath);
        
        // ----- PROCESS FILE & SIMULATION CONFIGURATIONS -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f,
            .message = "Processing scene metadata..."
        });

        processMetadata(&m_fileConfig, &m_simulationConfig, rootNode);

        m_eventDispatcher->dispatch(ConfigEvent::SimulationFileParsed{
           .fileConfig = m_fileConfig,
           .simulationConfig = m_simulationConfig
        });



        // ----- PROCESS SCENE -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f,
            .message = "Processing scene..."
        });

        processScene(rootNode);



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
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to read data from simulation file " + enquote(m_fileName) + ": " + e.what());

        // Re-throw the original exception to the outer try-catch block (i.e., the Session try-catch block, for it to reset its status)
        throw;
	}
	catch (const YAML::ParserException &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to parse data from simulation file " + enquote(m_fileName) + ": " + e.what());
        throw;
	}
	catch (const std::exception &e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to load scene from simulation file " + enquote(m_fileName) + ": " + e.what());
        throw;
	}


	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Successfully loaded scene from simulation file " + enquote(m_fileName) + ".");
}


void SceneManager::saveSceneToFile(const std::string &filePath) {

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
        if (!kernelsNode)
            Log::Print(Log::T_ERROR, __FUNCTION__, "Simulation configuration does not contain SPICE kernel paths!");

        else {
            const size_t kernelCount = kernelsNode.size();
            simConfig->kernelPaths.resize(kernelCount);

            for (size_t i = 0; i < kernelCount; i++) {
                YAMLUtils::TryGetEntryData(&simConfig->kernelPaths[i], YAMLSimConfig::Kernel_Path, kernelsNode[i]);
            }
        }


        // Coordinate system
        const auto coordSysNode = simCfgRoot[YAMLSimConfig::CoordSys];
        if (!coordSysNode)
            Log::Print(Log::T_ERROR, __FUNCTION__, "Simulation configuration does not contain information on the coordinate system!");

        else {
            std::string frameStr;
            if (YAMLUtils::TryGetEntryData(&frameStr, YAMLSimConfig::CoordSys_Frame, coordSysNode)) {
                simConfig->frame = CoordSys::FrameStrToEnumMap.at(frameStr);
                simConfig->frameType = CoordSys::FrameToTypeMap.at(simConfig->frame);
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
    Entity coordSystem = m_registry->createEntity(CoordSys::FrameEnumToDisplayStrMap.at(simConfig->frame));
    m_registry->addComponent(coordSystem.id, PhysicsComponent::CoordinateSystem{
        .simulationConfig = *simConfig
    });
}


void SceneManager::processScene(const YAML::Node &rootNode) {

#define logMissingComponent(componentName) Log::Print(Log::T_WARNING, __FUNCTION__, "In simulation file " + m_fileName + ": Essential component " + enquote((componentName)) + " is missing!")
#define info(entityName, componentType) "Entity " + enquote((entityName)) + ", component " + enquote((componentType)) + ": "


    const auto sceneRoot = rootNode[YAMLScene::ROOT];

    if (!sceneRoot) {
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Scene is empty!");
        throw;
    }

    size_t processedEntities = 0;
    size_t totalEntities = sceneRoot.size();

    std::map<std::string, EntityID> entityNameToID; // Map entity names to their runtime IDs



    // ----- PROCESS ALL ENTITIES IN THE SCENE -----
    for (const auto &entityNode : sceneRoot) {
        std::string entityName = entityNode[YAMLScene::Entity].as<std::string>();
        Entity newEntity = m_registry->createEntity(entityName);

        if (entityNameToID.count(entityName) > 0) {
            Log::Print(Log::T_ERROR, __FUNCTION__, "Found duplicate " + enquote(entityName) + " entities! Please ensure entity names are unique.");
            throw;
        }

        entityNameToID[entityName] = newEntity.id;

        m_registry->addComponent(newEntity.id, TelemetryComponent::RenderTransform{});  // Automatically add to telemetry dashboard (if applicable)


        processedEntities++;
        float entityProcessingProgress = static_cast<float>(processedEntities) / totalEntities;
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f + (entityProcessingProgress * 0.75f),
            .message = "[" + std::string(entityName) + "] Parsing components..."
        });



        // ----- PROCESS ALL COMPONENTS FOR EACH ENTITY -----
        for (const auto &componentNode : entityNode[YAMLScene::Entity_Components]) {
            std::string componentType = componentNode[YAMLScene::Entity_Components_Type].as<std::string>();

            // ----- CORE/PHYSICS -----
            if (componentType == YAMLScene::Core_Identifiers) {
                CoreComponent::Identifiers identifiers{};

                if (!YAMLUtils::GetComponentData(&identifiers, componentNode))
                    logMissingComponent(componentType);

                m_registry->addComponent(newEntity.id, identifiers);
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
                if (renderableNode["meshPath"]) {
                    std::string meshPath = renderableNode["meshPath"].as<std::string>();


                    m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
                        .progress = 0.1f + (entityProcessingProgress * 0.75f),
                        .message = "[" + std::string(entityName) + "] Loading geometry..."
                    });

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
    }
}
