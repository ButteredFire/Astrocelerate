/* GeometryLoader.hpp - Manages geometry loading.
*/

#pragma once

#include <iostream>
#include <vector>

#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>

#include <CoreStructs/Geometry.hpp>

#include <Utils/ModelParser.hpp>

#include <Shaders/BufferManager.hpp>


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
	std::vector<Geometry::MeshData> m_meshes;

	std::shared_ptr<BufferManager> m_bufferManager;
};