/* ModelParser.hpp - Defines a model loader.
*/

#pragma once

#include <tinyobjloader/tiny_obj_loader.h>

#include <Core/LoggingManager.hpp>

#include <Engine/Components/ModelComponents.hpp>


struct RawMeshData {
	std::vector<Geometry::Vertex> vertices;
	std::vector<uint32_t> indices;
	glm::vec3 normal;
	glm::vec3 tangent;
};


class IModelParser {
public:
	IModelParser() = default;
	~IModelParser() = default;
	
	/* Parses a model.
		@param path: The path to the model file.

		@return Raw mesh data.
	*/
	virtual RawMeshData parse(const std::string& path) = 0;
};


class OBJParser : public IModelParser {
public:
	OBJParser() = default;
	~OBJParser() = default;

	RawMeshData parse(const std::string& path) override;
};