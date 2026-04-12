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


            // If body is orbiting around another body, configure COEs
            PhysicsComponent::OrbitalElements orbitalElems{};
            using enum CoordSys::Frame;
            switch (m_simulationConfig.frame) {
            case ECI:
                if (ctx->entityName != YAMLScene::Body_Earth) {
                    orbitalElems.parentBody = ctx->sceneEntities->at(YAMLScene::Body_Earth);
                    m_ecsRegistry->addComponent(ctx->entityID, orbitalElems);
                }
                break;

            case HCI:
                if (ctx->entityName != YAMLScene::Body_Sun) {
                    orbitalElems.parentBody = ctx->sceneEntities->at(YAMLScene::Body_Sun);
                    m_ecsRegistry->addComponent(ctx->entityID, orbitalElems);
                }
                break;

            case SSB:
                // IMPORTANT: The actual central orbiting body of the simulation is NOT the Sun in SSB, but the Solar System Barycenter. Setting the parent body to the Sun is a temporary fix, and does not reflect physical reality.
                // TODO: IMPLEMENT SOLAR SYSTEM BARYCENTER AS CENTRAL ORBITING BODY FOR SSB-FRAME SIMULATIONS

                if (ctx->entityName != YAMLScene::Body_Sun) {
                    orbitalElems.parentBody = ctx->sceneEntities->at(YAMLScene::Body_Sun);
                    m_ecsRegistry->addComponent(ctx->entityID, orbitalElems);
                }
                break;

            case ECEF:
                break;
            }


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

            YAMLUtils::GetComponentData(&identifiers, ctx->selfComponents->at(ctx->currentComponentType));
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

            YAMLUtils::GetComponentData(&transform, ctx->selfComponents->at(ctx->currentComponentType));
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, transform);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_RigidBody, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            PhysicsComponent::RigidBody rigidBody{};

            YAMLUtils::GetComponentData(&rigidBody, ctx->selfComponents->at(ctx->currentComponentType));
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, rigidBody);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_Propagator, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            PhysicsComponent::Propagator propagator{};

            YAMLUtils::GetComponentData(&propagator, ctx->selfComponents->at(ctx->currentComponentType));

            // Get the last 2 compact lines in the TLE
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

            // If propagator is SGP4 (Earth-centric), the entity must be orbiting around Earth
            if (propagator.propagatorType == PhysicsComponent::Propagator::Type::SGP4) {
                PhysicsComponent::OrbitalElements orbitalElems{};
                orbitalElems.parentBody = ctx->sceneEntities->at(YAMLScene::Body_Earth);
                
                m_ecsRegistry->addOrUpdateComponent(ctx->entityID, orbitalElems);
            }


            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, propagator);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_ShapeParameters, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            PhysicsComponent::ShapeParameters shapeParams{};

            YAMLUtils::GetComponentData(&shapeParams, ctx->selfComponents->at(ctx->currentComponentType));
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, shapeParams);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Physics_OrbitalElements, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            if (ctx->selfComponents->count(YAMLScene::Physics_Propagator)) {
                throw Log::RuntimeException(__FUNCTION__, __LINE__, "If a Propagator is specified, OrbitalElements cannot be overridden, and must be derived from it.");
                throw;
            }

            PhysicsComponent::OrbitalElements orbitalElems{};
            YAMLUtils::GetComponentData(&orbitalElems, ctx->selfComponents->at(ctx->currentComponentType));

            if (!ctx->sceneEntities->count(orbitalElems._parentBody_str)) {
                throw Log::RuntimeException(__FUNCTION__, __LINE__, "Unknown reference to parent body: " + orbitalElems._parentBody_str);
                throw;
            }

            if (orbitalElems._parentBody_str == ctx->entityName) {
                throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot reference self as the parent body: " + orbitalElems._parentBody_str);
                throw;
            }

            orbitalElems.parentBody = ctx->sceneEntities->at(orbitalElems._parentBody_str);

            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, orbitalElems);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Spacecraft_Spacecraft, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            SpacecraftComponent::Spacecraft spacecraft{};

            YAMLUtils::GetComponentData(&spacecraft, ctx->selfComponents->at(ctx->currentComponentType));
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, spacecraft);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Spacecraft_Thruster, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            SpacecraftComponent::Thruster thruster{};

            YAMLUtils::GetComponentData(&thruster, ctx->selfComponents->at(ctx->currentComponentType));
            m_ecsRegistry->addOrUpdateComponent(ctx->entityID, thruster);
        }
    );



    m_serialRegistry.setDeserialLogic(
        YAMLScene::Render_MeshRenderable, [this](IParseCtx *context) {
            auto *ctx = static_cast<YAMLParseCtx *>(context);

            RenderComponent::MeshRenderable meshRenderable{};

            YAMLUtils::GetComponentData(&meshRenderable, ctx->selfComponents->at(ctx->currentComponentType));

            auto renderableNode = ctx->selfComponents->at(ctx->currentComponentType)[YAMLScene::Entity_Components_Type_Data];

            if (renderableNode[YAMLData::Render_MeshRenderable_MeshPath]) {
                std::string meshPath = renderableNode[YAMLData::Render_MeshRenderable_MeshPath].as<std::string>();

                std::string fullPath = FilePathUtils::JoinPaths(ROOT_DIR, meshPath);
                Math::Interval<uint32_t> meshRange = m_geometryLoader.loadGeometryFromFile(fullPath);

                meshRenderable.meshRange = meshRange;

                m_ecsRegistry->addOrUpdateComponent(ctx->entityID, meshRenderable);
            }
            //else {
            //    Log::Print(Log::T_ERROR, __FUNCTION__, info(ctx->displayEntityName, componentType) + "Model path is not provided!");
            //}
        }
    );
}


void SceneLoader::loadModels() {
    loadDefaultModels();
}


SceneLoader::FileData SceneLoader::loadSceneFromFile(const std::string &filePath) {
    m_fileName = FilePathUtils::GetFileName(filePath);


    // Exception message creation helpers
    static std::function<std::string(const std::string &, const std::string &)> getExceptionHeader = [](const std::string &faultyEntity, const std::string &faultyComponent) {
        std::string entityData;
        if (!faultyEntity.empty() && !faultyComponent.empty())
            entityData = "Error processing " + faultyComponent + " @ " + faultyEntity + "\n\n";

        return entityData;
    };
    static std::function<std::string(const YAML::Exception &, const std::string &)> getYAMLExceptionMsg = [](const YAML::Exception &e, const std::string &customMsg) {
        std::string msg;
        std::string excMsg = e.msg;
        if (!e.mark.is_null())
            msg = "Parsing error at line " + std::to_string(e.mark.line + 1) + ", column " + std::to_string(e.mark.column + 1) + ": ";
        if (!excMsg.empty())
            excMsg[0] = std::toupper(excMsg[0]);
        msg += excMsg;

        if (!customMsg.empty())
            msg += "\n\nDetails:\n" + customMsg;

        return msg;
    };



    // Worker thread progress tracking
    m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
        .progress = 0.0f,
        .message = "Preparing scene load from simulation file " + enquote(m_fileName) + "..."
    });

	Log::Print(Log::T_INFO, __FUNCTION__, "Selected simulation file: " + enquote(m_fileName) + ". Loading scene...");


    std::string currentEntity;
    std::string currentComponent;

    m_fileConfig = Application::YAMLFileConfig{};
    m_simulationConfig = Application::SimulationConfig{};

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
        const std::string msg = "Simulation file " + m_fileName + " is either not found or unreadable.";
		throw Log::RuntimeException(__FUNCTION__, __LINE__, getYAMLExceptionMsg(e, msg));

        // Re-throw the original exception to the outer try-catch block (i.e., the Session try-catch block, for it to reset its status)
        throw;
	}
	catch (const YAML::Exception &e) {
        const std::string header = getExceptionHeader(currentEntity, currentComponent);
        throw Log::RuntimeException(__FUNCTION__, __LINE__, header + getYAMLExceptionMsg(e, ""));
        throw;
	}
    catch (const std::exception &e) {
        const std::string header = getExceptionHeader(currentEntity, currentComponent);
        throw Log::RuntimeException(__FUNCTION__, __LINE__, header + e.what());
        throw;
    }


	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Successfully loaded scene from simulation file " + enquote(m_fileName) + ".");


    FileData fileData{};
    fileData.fileConfig = std::move(m_fileConfig);
    fileData.simulationConfig = std::move(m_simulationConfig);
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
    using EntityName = std::string;
    using ComponentName = std::string;

    std::unordered_map<EntityName, EntityID> sceneEntities;

    std::vector<YAMLParseCtx> entityParseContexts;

    std::unordered_map<EntityName,
        std::unordered_map<ComponentName, YAML::Node>
    > componentNodes;

        // ----- LOAD ENTITIES & COMPONENTS -----
    for (auto entityNode : sceneRoot) {
        // Create entity
        EntityName entityName = entityNode[YAMLScene::Entity].as<std::string>();

            // Remove any special prefixes from the display name
        EntityName displayEntityName = entityName;
        if (StringUtils::BeginsWith(displayEntityName, YAMLScene::Body_Prefix))
            displayEntityName = displayEntityName.substr(YAMLScene::Body_Prefix.length());
        
            // Register entity
        LOG_ASSERT(sceneEntities.count(entityName) == 0, "Found multiple " + enquote(displayEntityName) + " entities! Entity names must be unique.");

        Entity newEntity = m_ecsRegistry->createEntity(displayEntityName);
        sceneEntities[entityName] = newEntity.id;

            // Automatically add to telemetry dashboard (if applicable)
        m_ecsRegistry->addComponent(newEntity.id, TelemetryComponent::RenderTransform{});


        // Load entities & components
        YAMLParseCtx parseContext{};
        parseContext.entityID = newEntity.id;
        parseContext.entityName = entityName;
        parseContext.sceneEntities = &sceneEntities;

        sceneEntities[entityName] = parseContext.entityID;  // Use original entityName (e.g., `Body::Earth` instead of `Earth`) to ensure correct mappings for entity names referenced in components in the simulation file

            // ----- ENTITY IS A BUILT-IN CELESTIAL BODY => LOAD DEFAULTS -----
        if (StringUtils::BeginsWith(entityName, YAMLScene::Body_Prefix)) {
            parseContext.currentComponentType = YAMLScene::Body_Prefix;

            entityParseContexts.emplace_back(parseContext);
        }

            // ----- ENTITY IS CUSTOM-DEFINED => LOAD SPECIFIED COMPONENTS -----
        std::vector<YAMLParseCtx> components{};
        for (YAML::Node componentNode : entityNode[YAMLScene::Entity_Components]) {
            ComponentName componentType = componentNode[YAMLScene::Entity_Components_Type].as<ComponentName>();
            
            parseContext.selfComponents = &componentNodes[entityName];
            parseContext.currentComponentType = componentType;

            componentNodes[entityName][componentType] = componentNode;
            entityParseContexts.emplace_back(parseContext);
        }
    }


        // ----- DESERIALIZE ENTITIES & COMPONENTS -----
    for (auto &ctx : entityParseContexts) {
        Log::Print(Log::T_DEBUG, __FUNCTION__, "Deserializing " + ctx.currentComponentType + " @ " + ctx.entityName);

        currentEntity = ctx.entityName;
        currentComponent = ctx.currentComponentType;

        // Update progress
        processedEntities++;
        float entityProcessingProgress = static_cast<float>(processedEntities) / totalEntities;

        m_eventDispatcher->dispatch(UpdateEvent::SceneLoadProgress{
            .progress = 0.1f + (entityProcessingProgress * 0.75f),
            .message = "[" + std::string(ctx.entityName) + "] Processing components..."
        });


        // Deserialize!
        m_serialRegistry.deserialize(ctx.currentComponentType, static_cast<IParseCtx *>(&ctx));
    }
}
