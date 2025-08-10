/* SceneManager - Manages scene initialization and asset tracking.
*/
#pragma once

#include <atomic>
#include <thread>

#include <Core/Application/EventDispatcher.hpp>
#include <Core/Data/Math.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Geometry.hpp>
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Rendering/Geometry/GeometryLoader.hpp>

#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/SpacecraftComponents.hpp>

#include <Utils/YAMLUtils.hpp>
#include <Utils/FilePathUtils.hpp>


class SceneManager {
public:
	SceneManager();
	~SceneManager() = default;

	void init();


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
	std::shared_ptr<Registry> m_registry;

	Entity m_renderSpace{};

	GeometryLoader m_geometryLoader;
	Geometry::GeometryData *m_geomData = nullptr;
	uint32_t m_meshCount{};

	struct m_WorkerData {
		float entityProcessPercentage;
		std::string entityName;
		std::thread worker;
	};
	std::vector<m_WorkerData> m_geomLoadWorkers;


	void bindEvents();
};