/* ModelParser.hpp - Defines a model loader.
*/

#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Core/LoggingManager.hpp>

#include <CoreStructs/Geometry.hpp>

#include <Engine/Components/ModelComponents.hpp>


class IModelParser {
public:
	IModelParser() = default;
	~IModelParser() = default;
	
	virtual Geometry::MeshData parse(const std::string& path) = 0;
};


class AssimpParser : public IModelParser {
public:
	AssimpParser() = default;
	~AssimpParser() = default;

	/* Parses a model.
		@param path: The path to the model file.

		@return Raw mesh data.
	*/
	Geometry::MeshData parse(const std::string& path) override;

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
	void processNode(aiNode* node, aiScene* scene, Geometry::MeshData& meshData);


	/* Processes a mesh.
		@param scene: The scene owning the mesh.
		@param mesh: The mesh to be processed.
		@param meshData: The mesh data.
	*/
	void processMesh(aiScene* scene, aiMesh* mesh, Geometry::MeshData& meshData);


	/* Processes mesh materials. */
	void processMeshMaterials(aiScene* scene, aiMesh* mesh, Geometry::MeshData& meshData);
};