/* SceneManager - Manages scene initialization and asset tracking.
*/
#pragma once

#include <Core/Application/EventDispatcher.hpp>
#include <Core/Data/Math.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Geometry.hpp>
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Rendering/Geometry/GeometryLoader.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>

#include <Utils/FilePathUtils.hpp>


class SceneManager {
public:
	SceneManager();
	~SceneManager() = default;

	void init();

	/* Deserializes and loads scene data. */
	void loadScene();

	/* Initializes the default scene, with default mesh parameters in physics and rendering. */
	void loadDefaultScene();

	/* Gets the number of meshes in the current scene. */
	inline uint32_t GetMeshCount() const { return m_meshCount; }

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<Registry> m_registry;
	Entity m_renderSpace{};

	GeometryLoader m_geometryLoader;
	Geometry::GeometryData *m_geomData = nullptr;
	uint32_t m_meshCount{};


	void bindEvents();
};