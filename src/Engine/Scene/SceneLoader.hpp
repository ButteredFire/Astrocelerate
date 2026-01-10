/* SceneLoader - Manages scene initialization and asset tracking.
*/
#pragma once

#include <atomic>
#include <thread>


#include <Core/Data/Math.hpp>
#include <Core/Data/Constants.h>
#include <Core/Utils/YAMLUtils.hpp>
#include <Core/Utils/StringUtils.hpp>
#include <Core/Utils/FilePathUtils.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Registry/ECS/Components/RenderComponents.hpp>
#include <Engine/Registry/ECS/Components/PhysicsComponents.hpp>
#include <Engine/Registry/ECS/Components/SpacecraftComponents.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>
#include <Engine/Rendering/Data/Geometry.hpp>
#include <Engine/Rendering/Geometry/GeometryLoader.hpp>

#include <Simulation/Data/Bodies.hpp>


class SceneLoader {
public:
	SceneLoader();
	~SceneLoader() = default;

	void init();


	/* (Re)loads models.
	*/
	void loadModels();


	/* Loads the scene from a YAML simulation configuration file.
		@param filePath: The path to the YAML file.
	*/
	void loadSceneFromFile(const std::string &filePath);


	/* Saves the current scene to a YAML simulation configuration file.
		@param filePath: The path to the YAML file.
	*/
	void saveSceneToFile(const std::string &filePath);


	/* Gets the number of meshes in the current scene. */
	inline uint32_t GetMeshCount() const { return m_meshCount; }

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ECSRegistry> m_ecsRegistry;


	std::string m_fileName;


	Entity m_renderSpace{};


	GeometryLoader m_geometryLoader;
	Geometry::GeometryData *m_geomData = nullptr;
	uint32_t m_meshCount{};

	Math::Interval<uint32_t> m_sphereMesh{};

	struct m_WorkerData {
		float entityProcessPercentage;
		std::string entityName;
		std::thread worker;
	};
	std::vector<m_WorkerData> m_geomLoadWorkers;


	void bindEvents();


	/* Loads default models. */
	void loadDefaultModels();


	/* Processes file and simulation configurations. */
	void processMetadata(Application::YAMLFileConfig *fileConfig, Application::SimulationConfig *simConfig, const YAML::Node &rootNode);


	/* Processes the simulation initial states.
		@params currentEntity, currentComponent: The current entity/component being processed. If, during this function's execution, a YAML-CPP exception is thrown, the caller will know precisely where in which component belonging to which entity the error occurred.
	*/
	void processScene(const YAML::Node &rootNode, std::string &currentEntity, std::string &currentComponent);
};