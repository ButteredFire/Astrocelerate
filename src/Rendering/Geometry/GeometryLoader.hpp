/* GeometryLoader.hpp - Manages geometry loading.
*/

#pragma once

#include <iostream>
#include <vector>

#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>

#include <CoreStructs/Geometry.hpp>

#include <Rendering/Geometry/ModelParser.hpp>

#include <Vulkan/VkBufferManager.hpp>


class GeometryLoader {
public:
	GeometryLoader();
	~GeometryLoader() = default;

	/* Loads geometry from an external file.
		@param path: The path to the file.
	*/
	void loadGeometryFromFile(const std::string& path);


	/* Preprocesses loaded geometry data. 
		@return A vector of mesh offsets.
	*/
	std::vector<Geometry::MeshOffset> bakeGeometry();

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	std::vector<Geometry::MeshData> m_meshes;
};