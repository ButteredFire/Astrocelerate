#include "ModelParser.hpp"


Geometry::MeshData AssimpParser::parse(const std::string& path) {
	Geometry::MeshData meshData{};

	Assimp::Importer importer;

	// Creates scene

		/* Post-processing flags: aiProcess_...
			+ ...Triangulate: Converts all polygons into triangles.
			+ ...GenSmoothNormals: Generates vertex normals (if missing). This is essential for lighting.
			+ ...CalcTangentSpace: Computes tangents/bi-tangents. This is essential for normal maps.
		*/
	unsigned int postProcessingFlags = (
		  aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_CalcTangentSpace
	);

	aiScene* scene = const_cast<aiScene*>(importer.ReadFile(path, postProcessingFlags));

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, importer.GetErrorString());
	}


	processNode(scene->mRootNode, scene, meshData);

	Log::print(Log::T_SUCCESS, __FUNCTION__, "Successfully loaded model! Vertices: " + std::to_string(meshData.vertices.size())
		+ ";\tindices: " + std::to_string(meshData.indices.size()));

	return meshData;
}


void AssimpParser::processNode(aiNode* node, aiScene* scene, Geometry::MeshData& meshData) {
	// Only process meshes if they exist
	if (node->mNumMeshes > 0) {
		size_t meshCount = static_cast<size_t>(node->mNumMeshes);

		// Processes each mesh in the node
		for (size_t i = 0; i < meshCount; i++) {
			/* Each node stores the indices of the meshes it contains. It is index-based because:
				+ A mesh can be reused by multiple nodes (e.g., instancing)
				+ It keeps the data compact and normalized.
			*/
			size_t nodeIndices = static_cast<size_t>(node->mMeshes[i]);

			aiMesh* mesh = scene->mMeshes[nodeIndices];
			processMesh(scene, mesh, meshData);
		}

		//Log::print(Log::T_WARNING, __FUNCTION__, "Mesh data vertex count for node " + enquote(node->mName.C_Str()) + ": " + std::to_string(meshData.vertices.size()));
	}


	// Recursively processes child nodes
	size_t childNodeCount = static_cast<size_t>(node->mNumChildren);

	for (size_t i = 0; i < childNodeCount; i++) {
		processNode(node->mChildren[i], scene, meshData);
	}
}


void AssimpParser::processMesh(aiScene* scene, aiMesh* mesh, Geometry::MeshData& meshData) {
	// Processes each vertex in mesh
	std::unordered_map<Geometry::Vertex, uint32_t> uniqueVertices{};
	size_t faceCount = static_cast<size_t>(mesh->mNumFaces);

	for (size_t i = 0; i < faceCount; i++) {
		aiFace& face = mesh->mFaces[i];

		size_t indicesCount = static_cast<size_t>(face.mNumIndices);

		for (size_t j = 0; j < indicesCount; j++) {
			size_t index = static_cast<size_t>(face.mIndices[j]);
			Geometry::Vertex vertex{};

			// Position
			vertex.position = glm::vec3(
				mesh->mVertices[index].x,
				mesh->mVertices[index].y,
				mesh->mVertices[index].z
			);


			// Normals (essential for lighting, as they define the direction the vertex is "facing")
			if (mesh->HasNormals()) {
				vertex.normal = glm::vec3(
					mesh->mNormals[index].x,
					mesh->mNormals[index].y,
					mesh->mNormals[index].z
				);
			}


			// Tangents and bi-tangents
			if (mesh->HasTangentsAndBitangents()) {
				vertex.tangent = glm::vec3(
					mesh->mTangents[index].x,
					mesh->mTangents[index].y,
					mesh->mTangents[index].z
				);
			}


			// UV coordinates
				/* Assimp supports up to 8 sets of texture coordinates, but right now, we only care about the first set.
					(PBR textures may necessitate multiple UV channels)
				*/
			const size_t MAX_CHANNELS = 1;
			for (size_t k = 0; k < MAX_CHANNELS; k++) {
				if (mesh->HasTextureCoords(k)) {
					// TODO: Modify `vertex.texCoord` to be an `std::vector<glm::vec2>`
					vertex.texCoord = glm::vec2(
						mesh->mTextureCoords[k][index].x,
						mesh->mTextureCoords[k][index].y
					);
				}
			}


			// TODO: Add color support
			vertex.color = glm::vec3(1.0f);


			if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
				uniqueVertices[vertex] = static_cast<uint32_t>(meshData.vertices.size());
				meshData.vertices.push_back(vertex);
			}

			meshData.indices.push_back(uniqueVertices[vertex]);
		}
	}

	//processMeshMaterials(scene, mesh, meshData);
}


void AssimpParser::processMeshMaterials(aiScene* scene, aiMesh* mesh, Geometry::MeshData& meshData) {
	// Extract material
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	Geometry::Material mat;
	aiColor3D color(0.0f, 0.0f, 0.0f);

	// Diffuse color
	if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
		mat.diffuseColor = glm::vec3(color.r, color.g, color.b);
	}

	// Specular color
	if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
		mat.specularColor = glm::vec3(color.r, color.g, color.b);
	}

	// Ambient color
	if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
		mat.ambientColor = glm::vec3(color.r, color.g, color.b);
	}

	// Shininess
	float shininess = 0.0f;
	if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess)) {
		mat.shininess = shininess;
	}

	// Textures (Diffuse)
	aiString texturePath;
	if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath)) {
		mat.diffuseTexture = texturePath.C_Str();
	}

	// Optionally handle other texture types, like specular, normal maps, etc.

	meshData.materials.push_back(mat);  // Add the material to meshData
}
