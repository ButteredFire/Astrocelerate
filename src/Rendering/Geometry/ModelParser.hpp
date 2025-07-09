/* ModelParser.hpp - Defines a model loader.
*/

#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/Geometry.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Engine/Components/ModelComponents.hpp>

#include <Rendering/Textures/TextureManager.hpp>
class TextureManager;


class IModelParser {
public:
	IModelParser() = default;
	~IModelParser() = default;
	
	virtual Geometry::MeshData parse(const std::string &modelPath) = 0;
};


class AssimpParser : public IModelParser {
public:
	AssimpParser();
	~AssimpParser() = default;

	/* Parses a model.
		@param path: The path to the model file.

		@return Raw mesh data.
	*/
	Geometry::MeshData parse(const std::string &modelPath) override;

private:
	std::shared_ptr<TextureManager> m_textureManager;

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
		@param currentTransformMat: The current (accumulated) global transformation matrix of the mesh.
	*/
	void processNode(aiNode *node, const aiScene *scene, Geometry::MeshData &meshData, glm::mat4 currentTransformMat);


	/* Processes the geometry of the mesh.
		@param scene: The scene owning the mesh.
		@param mesh: The mesh to be processed.
		@param meshData: The mesh data.
		@param currentTransformMat: The current (accumulated) global transformation matrix of the mesh.
	*/
	void processMeshGeometry(const aiScene *scene, aiMesh *mesh, Geometry::MeshData &meshData, glm::mat4 currentTransformMat);


	/* Processes mesh materials.
		@param scene: The scene owning the mesh.
		@param aiMat: The material to be processed.
		@param meshMat: The mesh material to be filled with data.
		@param parentDir: The parent directory of the model file, used to build absolute texture paths.
	*/
	void processMeshMaterials(const aiScene *scene, const aiMaterial *aiMat, Geometry::Material &meshMat, const std::string &modelPath);
};