#include "SceneLoader.hpp"

SceneLoader::SceneLoader() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_ecsRegistry = ServiceLocator::GetService<ECSRegistry>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void SceneLoader::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<SceneLoader>();

    m_eventDispatcher->subscribe<UpdateEvent::RegistryReset>(selfIndex,
        [this](const UpdateEvent::RegistryReset &event) {
            this->init();
        }
    );
}


void SceneLoader::init() {
    m_renderSpace = m_ecsRegistry->getRenderSpaceEntity();

    loadModels();

    loadDeserialBindings();
}


void SceneLoader::loadDeserialBindings() {
    // ----- HELPER FUNCTIONS -----
    std::function<void(EntityID)> addPointLight = [this](EntityID entityID) {
        static constexpr double SOLAR_LUMINOSITY = 3.828e26;

        RenderComponent::PointLight pointLight{};
        pointLight.color = glm::vec3(1.0f, 0.95f, 0.90f);
        pointLight.radiantFlux = SOLAR_LUMINOSITY / std::pow(SimulationConst::SIMULATION_SCALE, 2.3);

        m_ecsRegistry->addComponent(entityID, pointLight);
    };


    // ----- BINDING: TEMPLATES/PRESETS -----

    m_serialRegistry.setDeserialLogic(
        YAMLScene::Body_Prefix, [this, addPointLight](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            ICelestialBody *celestialBody = Body::GetCelestialBody(ctx->entityName);

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


            m_ecsRegistry->addComponent(ctx->entityID, identifiers);
            m_ecsRegistry->addComponent(ctx->entityID, transform);
            m_ecsRegistry->addComponent(ctx->entityID, rigidBody);
            m_ecsRegistry->addComponent(ctx->entityID, shapeParams);
            m_ecsRegistry->addComponent(ctx->entityID, meshRenderable);


            // Special case: Entity is a star
            if (identifiers.entityType == CoreComponent::Identifiers::EntityType::STAR)
                addPointLight(ctx->entityID);
        }
    );



    // ----- BINDING: INDIVIDUAL COMPONENTS -----
    m_serialRegistry.setDeserialLogic(
        YAMLScene::Core_Identifiers, [this, addPointLight](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            CoreComponent::Identifiers identifiers{};

            YAMLUtils::GetComponentData(&identifiers, *ctx->componentNode);
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, identifiers);


            // SPECIAL CASE: If the entity is a star, add a point light at its center of mass.
            if (identifiers.entityType == CoreComponent::Identifiers::EntityType::STAR)
                addPointLight(ctx->entityID);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Core_Transform, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            CoreComponent::Transform transform{};

            YAMLUtils::GetComponentData(&transform, *ctx->componentNode);
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, transform);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_RigidBody, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            PhysicsComponent::RigidBody rigidBody{};

            YAMLUtils::GetComponentData(&rigidBody, *ctx->componentNode);
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, rigidBody);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_Propagator, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            PhysicsComponent::Propagator propagator{};

            YAMLUtils::GetComponentData(&propagator, *ctx->componentNode);

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

            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, propagator);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_ShapeParameters, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            PhysicsComponent::ShapeParameters shapeParams{};

            YAMLUtils::GetComponentData(&shapeParams, *ctx->componentNode);
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, shapeParams);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Spacecraft_Spacecraft, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            SpacecraftComponent::Spacecraft spacecraft{};

            YAMLUtils::GetComponentData(&spacecraft, *ctx->componentNode);
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, spacecraft);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Spacecraft_Thruster, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            SpacecraftComponent::Thruster thruster{};

            YAMLUtils::GetComponentData(&thruster, *ctx->componentNode);
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, thruster);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Render_MeshRenderable, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            RenderComponent::MeshRenderable meshRenderable{};

            YAMLUtils::GetComponentData(&meshRenderable, *ctx->componentNode);

            auto renderableNode = (*ctx->componentNode)[YAMLScene::Entity_Components_Type_Data];

            if (renderableNode[YAMLData::Render_MeshRenderable_MeshPath]) {
                std::string meshPath = renderableNode[YAMLData::Render_MeshRenderable_MeshPath].as<std::string>();

                std::string fullPath = FilePathUtils::JoinPaths(ROOT_DIR, meshPath);
                Math::Interval<uint32_t> meshRange = m_geometryLoader.loadGeometryFromFile(fullPath);

                meshRenderable.meshRange = meshRange;

                m_ecsRegistry->addOrUpdateComponent(ctx->entityID, meshRenderable);
            }
            //else {
            //    Log::Print(Log::T_ERROR, __FUNCTION__, info(ctx->entityName, componentType) + "Model path is not provided!");
            //}
        }
    );
}


void SceneLoader::loadModels() {
    loadDefaultModels();
}


SceneLoader::FileData SceneLoader::loadSceneFromFile(const std::string &filePath) {
    m_fileName = FilePathUtils::GetFileName(filePath);


    // Worker thread progress tracking
    m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
        .progress = 0.0f,
        .message = "Preparing scene load from simulation file " + enquote(m_fileName) + "..."
    });

	Log::Print(Log::T_INFO, __FUNCTION__, "Selected simulation file: " + enquote(m_fileName) + ". Loading scene...");


    std::string currentEntity;
    std::string currentComponent;

    Application::YAMLFileConfig fileConfig;
    Application::SimulationConfig simulationConfig;

	try {
        YAML::Node rootNode = YAML::LoadFile(filePath);
        

        // ----- PROCESS FILE & SIMULATION CONFIGURATIONS -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f,
            .message = "Processing scene metadata..."
        });

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



        // ----- Finalize geometry baking -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.9f,
            .message = "Baking geometry data..."
        });

		m_geomData = m_geometryLoader.bakeGeometry();
		m_meshCount = m_geomData->meshCount;



        // ----- Initialize resources -----
        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.95f,
            .message = "Initializing resources..."
        });


	}

	catch (const YAML::BadFile &e) {
        std::string msg = e.what();

        if (msg.size() > 0)
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


    FileData fileData{};
    fileData.fileConfig = std::move(fileConfig);
    fileData.simulationConfig = std::move(simulationConfig);
    fileData.geometryData = m_geomData;

    return fileData;
}


void SceneLoader::saveSceneToFile(const std::string &filePath) {

}


void SceneLoader::loadDefaultModels() {
    const std::string SPHERE_MESH = FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/TestModels/Sphere/Sphere.gltf");
    
    //m_sphereMesh = m_geometryLoader.loadGeometryFromFile(SPHERE_MESH);
}


void SceneLoader::processMetadata(Application::YAMLFileConfig *fileConfig, Application::SimulationConfig *simConfig, const YAML::Node &rootNode) {
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
    Entity coordSystem = m_ecsRegistry->createEntity(CoordSys::FrameProperties.at(simConfig->frame).displayName);
    m_ecsRegistry->addComponent(coordSystem.id, PhysicsComponent::CoordinateSystem{
        .simulationConfig = *simConfig
    });
}


void SceneLoader::processScene(const YAML::Node &rootNode, std::string &currentEntity, std::string &currentComponent) {
    const auto sceneRoot = rootNode[YAMLScene::ROOT];
    LOG_ASSERT(sceneRoot, "There is nothing to process!");

    size_t processedEntities = 0;
    size_t totalEntities = sceneRoot.size();


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



    // ----- PROCESS ALL ENTITIES IN THE SCENE -----
    std::unordered_set<std::string> existingEntities;
    for (auto entityNode : sceneRoot) {
        // Create entity
        std::string entityName = entityNode[YAMLScene::Entity].as<std::string>();

            // Remove any special prefixes from the display name
        std::string originalEntityName = entityName;
        if (StringUtils::BeginsWith(entityName, YAMLScene::Body_Prefix))
            entityName = entityName.substr(YAMLScene::Body_Prefix.length());
        
            // Register entity
        LOG_ASSERT(existingEntities.count(entityName) == 0, "Found multiple " + enquote(entityName) + " entities! Entity names must be unique.");

        Entity newEntity = m_ecsRegistry->createEntity(entityName);
        currentEntity = entityName;

            // Automatically add to telemetry dashboard (if applicable)
        m_ecsRegistry->addComponent(newEntity.id, TelemetryComponent::RenderTransform{});

        

        // Update progress
        processedEntities++;
        float entityProcessingProgress = static_cast<float>(processedEntities) / totalEntities;

        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f + (entityProcessingProgress * 0.75f),
            .message = "[" + std::string(entityName) + "] Processing components..."
        });



        // If entity is built-in, populate it with default arguments
            // ----- ENTITY IS A BUILT-IN CELESTIAL BODY -----
        if (StringUtils::BeginsWith(originalEntityName, YAMLScene::Body_Prefix)) {
            YAMLParseCtx parseContext{};
            parseContext.entityID = newEntity.id;
            parseContext.entityName = originalEntityName;
            parseContext.componentType = YAMLScene::Body_Prefix;
            parseContext.componentNode = &entityNode;

            m_serialRegistry.deserialize(YAMLScene::Body_Prefix, static_cast<IParseCtx *>(&parseContext));
        }



        // ----- PROCESS ALL COMPONENTS FOR EACH ENTITY -----
        std::vector<YAMLParseCtx> componentContexts;

        for (auto componentNode : entityNode[YAMLScene::Entity_Components]) {
            std::string componentType = componentNode[YAMLScene::Entity_Components_Type].as<std::string>();
            
            YAMLParseCtx parseContext{};
            parseContext.entityID = newEntity.id;
            parseContext.entityName = entityName;
            parseContext.componentType = componentType;
            parseContext.componentNode = &componentNode;

            //componentContexts.emplace_back(parseContext);
            m_serialRegistry.deserialize(componentType, static_cast<IParseCtx *>(&parseContext));
        }

        //for (auto &ctx : componentContexts)
        //    m_serialRegistry.deserialize(ctx.componentType, static_cast<IParseCtx *>(&ctx));
    }
}
