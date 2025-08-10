/* GeometryLoader.hpp - Manages geometry loading.
*/

#pragma once

#include <mutex>
#include <atomic>
#include <vector>
#include <iostream>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Data/Math.hpp>
#include <Core/Data/Geometry.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Rendering/Geometry/ModelParser.hpp>

#include <Vulkan/VkBufferManager.hpp>


class GeometryLoader {
public:
	GeometryLoader();
	~GeometryLoader() = default;

	/* Loads geometry from an external file.
		@param path: The path to the file.
	
		@return The mesh-offset range of the mesh. If the mesh has N child meshes, its mesh-offset interval will be [M+1, N], where M is the right endpoint of the previous range (or 0 if the mesh is the first to be loaded).
	*/
	Math::Interval<uint32_t> loadGeometryFromFile(const std::string& path);


	/* Preprocesses loaded geometry data.
		NOTE: This function internally depends on the data generated from GeometryLoader::loadGeometryFromFile.
	
		@return A pointer to the geometry data.
	*/
	Geometry::GeometryData* bakeGeometry();

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<GarbageCollector> m_garbageCollector;


	std::vector<Geometry::MeshData> m_meshes;
	std::mutex m_meshLoadMutex;

	std::atomic<int32_t> m_leftEndpoint = 0;
	std::atomic<int32_t> m_rightEndpoint = 0;
	bool m_isInitialLoad = false;


	std::vector<CleanupID> m_sessionCleanupIDs;

	void bindEvents();
};