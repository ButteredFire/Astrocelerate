/* ModelParser.hpp - Defines a model loader.
*/

#pragma once

#include <tinyobjloader/tiny_obj_loader.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Core/LoggingManager.hpp>

#include <Engine/Components/ModelComponents.hpp>


struct MeshData {
	std::vector<Geometry::Vertex> vertices;
	std::vector<uint32_t> indices;
	glm::vec3 normal;
	glm::vec3 tangent;
};


class IModelParser {
public:
	IModelParser() = default;
	~IModelParser() = default;
	
	virtual MeshData parse(const std::string& path) = 0;
};


class AssimpParser : public IModelParser {
public:
	AssimpParser() = default;
	~AssimpParser() = default;

	/* Parses a model.
		@param path: The path to the model file.

		@return Raw mesh data.
	*/
	MeshData parse(const std::string& path) override;

private:
	/* Processes a node.
		This is a recursive function intended to process a scene hierarchically, starting from the root node.
		This is necessary, because the file might contain:
			+ Nested objects (e.g., a satellite with sub-parts like panels/antennae)
			+ Shared mesh instances (via mesh indices)
			+ Hierarchical transformations (TODO: Handle this)

		These nodes help represent the scene graph (a hierarchy of objects).

		@param node: The node to be processed.
		@param scene: The scene owning the node to be processed.
		@param meshData: The mesh data.
	*/
	void processNode(aiNode* node, const aiScene* scene, MeshData& meshData);


	/* Processes a mesh.
	
	*/
	void processMesh(aiMesh* mesh, MeshData& meshData);
};