/* GeometryLoader.hpp - Manages geometry loading.
*/

#pragma once

#include <mutex>
#include <atomic>
#include <vector>
#include <iostream>


#include <Core/Data/Math.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>
#include <Core/Application/IO/LoggingManager.hpp>

#include <Platform/Vulkan/VkBufferManager.hpp>

#include <Engine/Registry/Event/EventDispatcher.hpp>
#include <Engine/Rendering/Data/Geometry.hpp>
#include <Engine/Rendering/Geometry/ModelParser.hpp>


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
	std::shared_ptr<CleanupManager> m_cleanupManager;


	std::vector<Geometry::MeshData> m_meshes;
	std::mutex m_meshLoadMutex;

	std::atomic<int32_t> m_leftEndpoint = 0;
	std::atomic<int32_t> m_rightEndpoint = 0;
	bool m_isInitialLoad = false;


	std::vector<CleanupID> m_sessionCleanupIDs;

	void bindEvents();
};