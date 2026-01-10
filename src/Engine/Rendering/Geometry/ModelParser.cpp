#include "ModelParser.hpp"


AssimpParser::AssimpParser() {
	m_textureManager = ServiceLocator::GetService<TextureManager>(__FUNCTION__);
}


Geometry::MeshData AssimpParser::parse(const std::string &modelPath) {
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
		| aiProcess_OptimizeMeshes
		| aiProcess_FlipUVs
	);

	const aiScene* scene = importer.ReadFile(modelPath, postProcessingFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, importer.GetErrorString());
	}

	meshData.materials.reserve(scene->mNumMaterials);

	// Process mesh materials first to get their indices
	for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
		aiMaterial* aiMat = scene->mMaterials[i];
		Geometry::Material material;
		// Pass the directory so processMeshMaterials can build absolute texture paths
		processMeshMaterials(scene, aiMat, material, modelPath);
		meshData.materials.push_back(material);
	}

	processNode(scene->mRootNode, scene, meshData, glm::mat4(1.0f));

	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Successfully parsed model " + enquote(FilePathUtils::GetFileName(modelPath)) + "!");

	return meshData;
}


void AssimpParser::processNode(aiNode *node, const aiScene *scene, Geometry::MeshData &meshData, glm::mat4 currentTransformMat) {
	static const std::function<glm::mat4(aiMatrix4x4 &)> glmMatrix = [](aiMatrix4x4 &aiMat) {
		glm::mat4 glmMat(1.0f);

		glmMat[0][0] = aiMat.a1;
		glmMat[0][1] = aiMat.a2;
		glmMat[0][2] = aiMat.a3;
		glmMat[0][3] = aiMat.a4;

		glmMat[1][0] = aiMat.b1;
		glmMat[1][1] = aiMat.b2;
		glmMat[1][2] = aiMat.b3;
		glmMat[1][3] = aiMat.b4;

		glmMat[2][0] = aiMat.c1;
		glmMat[2][1] = aiMat.c2;
		glmMat[2][2] = aiMat.c3;
		glmMat[2][3] = aiMat.c4;

		glmMat[3][0] = aiMat.d1;
		glmMat[3][1] = aiMat.d2;
		glmMat[3][2] = aiMat.d3;
		glmMat[3][3] = aiMat.d4;

		return glmMat;
	};

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
			processMeshGeometry(scene, mesh, meshData, currentTransformMat);
		}
	}


	// Recursively processes child nodes
	glm::mat4 childTransformMat = glmMatrix(node->mTransformation);
	glm::mat4 newTransformMat = currentTransformMat * childTransformMat;

	size_t childNodeCount = static_cast<size_t>(node->mNumChildren);
	for (size_t i = 0; i < childNodeCount; i++) {
		processNode(node->mChildren[i], scene, meshData, newTransformMat);
	}
}


void AssimpParser::processMeshGeometry(const aiScene *scene, aiMesh *mesh, Geometry::MeshData &meshData, glm::mat4 currentTransformMat) {
	// Current sizes and index count to calculate THIS child mesh's offset from the parent meshData
		// Offsets from the beginning of meshData.vertices and meshData.indices
	size_t currentVertexOffset = meshData.vertices.size();
	size_t currentIndexOffset = meshData.indices.size();

	uint32_t currentMeshIndexCount = 0;

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
			glm::vec4 initialPosition(
				mesh->mVertices[index].x,
				mesh->mVertices[index].y,
				mesh->mVertices[index].z,
				1.0f
			);
			vertex.position = glm::vec3(currentTransformMat * initialPosition);  // Transformed position


			// Normals and tangents should be transformed by the inverse transpose of the model matrix
			
			// Normals (essential for lighting, as they define the direction the vertex is "facing")
			if (mesh->HasNormals()) {
				glm::vec3 initialNormals(
					mesh->mNormals[index].x,
					mesh->mNormals[index].y,
					mesh->mNormals[index].z
				);

				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(currentTransformMat)));
				vertex.normal = glm::normalize(normalMatrix * initialNormals);  // Transformed normal
			}


			// Tangents and bi-tangents
			if (mesh->HasTangentsAndBitangents()) {
				glm::vec3 initialTangents(
					mesh->mTangents[index].x,
					mesh->mTangents[index].y,
					mesh->mTangents[index].z
				);

				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(currentTransformMat)));
				vertex.tangent = glm::normalize(normalMatrix * initialTangents);  // Transformed tangent
			}


			// UV coordinates
				/* Assimp supports up to 8 sets of texture coordinates, but right now, we only care about the first set.
					(PBR textures may necessitate multiple UV channels)
				*/
			const size_t MAX_CHANNELS = 1;
			for (size_t k = 0; k < MAX_CHANNELS; k++) {
				if (mesh->HasTextureCoords(k)) {
					// TODO: Implement multiple texture coordinate slots
					vertex.texCoord0 = glm::vec2(
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
			currentMeshIndexCount++;
		}
	}


	// Creates a mesh offset for THIS child mesh
	Geometry::MeshOffset childOffset{};
	childOffset.vertexOffset = static_cast<uint32_t>(currentVertexOffset);
	childOffset.indexOffset = static_cast<uint32_t>(currentIndexOffset);
	childOffset.indexCount = currentMeshIndexCount;
	childOffset.materialIndex = mesh->mMaterialIndex;

	meshData.childMeshOffsets.push_back(childOffset);
}


void AssimpParser::processMeshMaterials(const aiScene *scene, const aiMaterial *aiMat, Geometry::Material &meshMat, const std::string &modelPath) {
	const std::string parentDir = FilePathUtils::GetParentDirectory(modelPath);
	const std::string fileName = FilePathUtils::GetFileName(modelPath);
	
	std::string textureAbsPath;

	const std::string fallbackPlaceholder = FilePathUtils::JoinPaths(ROOT_DIR, "assets/Textures", "Fallback/PlaceholderTexture.png");
	const std::string fallbackWhite = FilePathUtils::JoinPaths(ROOT_DIR, "assets/Textures", "Fallback/1x1_White.png");
	const std::string fallbackBlack = FilePathUtils::JoinPaths(ROOT_DIR, "assets/Textures", "Fallback/1x1_Black.png");
	const std::string fallbackFlatNormal = FilePathUtils::JoinPaths(ROOT_DIR, "assets/Textures", "Fallback/1x1_Flat_Normal.png");

	// Albedo (base color)
	aiColor3D albedoBaseColor(1.0f, 1.0f, 1.0f);
	aiString texturePath;
	
		// Scalar value
	if (
		aiMat->Get(AI_MATKEY_BASE_COLOR, albedoBaseColor) == AI_SUCCESS || 
		aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, albedoBaseColor) == AI_SUCCESS
		) {
		meshMat.albedoColor = glm::vec3(albedoBaseColor.r, albedoBaseColor.g, albedoBaseColor.b);
	}
	
		// Map index
	if (
		aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS ||
		aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS
		) {
		textureAbsPath = FilePathUtils::JoinPaths(parentDir, texturePath.C_Str());
		meshMat.albedoMapIndex = m_textureManager->createIndexedTexture(textureAbsPath, VK_FORMAT_R8G8B8A8_SRGB);
	}
	else {
		meshMat.albedoMapIndex = m_textureManager->createIndexedTexture(fallbackPlaceholder, VK_FORMAT_R8G8B8A8_SRGB);
	}


	// Metallic & Roughness
	float metallic = 0.0f;
	float roughness = 1.0f;

		// Scalar value
	if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
		meshMat.metallicFactor = metallic;
	}
	if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
		meshMat.roughnessFactor = roughness;
	}

		// Map index
	if (
		aiMat->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS ||
		aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS
		) {
		textureAbsPath = FilePathUtils::JoinPaths(parentDir, texturePath.C_Str());
		meshMat.metallicRoughnessMapIndex = m_textureManager->createIndexedTexture(textureAbsPath, VK_FORMAT_R8G8B8A8_UNORM);
	}


	// Height/Topography map index
	if (aiMat->GetTexture(aiTextureType_DISPLACEMENT, 0, &texturePath) == AI_SUCCESS) {
		textureAbsPath = FilePathUtils::JoinPaths(parentDir, texturePath.C_Str());
		meshMat.heightMapIndex = m_textureManager->createIndexedTexture(textureAbsPath, VK_FORMAT_R8G8B8A8_UNORM);
	}


	// Normal Map
		// NOTE: Tangent generation via the `aiProcess_CalcTangentSpace` flag must be enabled for this
		// Map index
	if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
		textureAbsPath = FilePathUtils::JoinPaths(parentDir, texturePath.C_Str());
		meshMat.normalMapIndex = m_textureManager->createIndexedTexture(textureAbsPath, VK_FORMAT_R8G8B8A8_UNORM);
	}


	// Ambient Occlusion
		// Map index
	if (aiMat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texturePath) == AI_SUCCESS) {
		textureAbsPath = FilePathUtils::JoinPaths(parentDir, texturePath.C_Str());
		meshMat.aoMapIndex = m_textureManager->createIndexedTexture(textureAbsPath, VK_FORMAT_R8G8B8A8_SRGB);
	}


	// Emission
	aiColor3D emissionColor(1.0f, 1.0f, 1.0f);

		// Scalar value
	if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissionColor) == AI_SUCCESS) {
		//meshMat.emissiveColor = glm::vec3(emissionColor.r, emissionColor.g, emissionColor.b);
	}

		// Map index
	if (aiMat->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath) == AI_SUCCESS) {
		textureAbsPath = FilePathUtils::JoinPaths(parentDir, texturePath.C_Str());
		meshMat.emissiveMapIndex = m_textureManager->createIndexedTexture(textureAbsPath, VK_FORMAT_R8G8B8A8_SRGB);
	}


	// Opacity
		// Scalar value
	float opacity = 1.0f;
	if (aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
		meshMat.opacity = opacity;
	}
	//if (aiMat->GetTexture(aiTextureType_OPACITY, 0, &texturePath) == AI_SUCCESS) {
	//	// If the mesh has an opacity map, or if the alpha channel of the base color is used (AI_MATKEY_BASE_COLOR has 4 components),
	//	// consider it over the scalar value (Format: VK_FORMAT_R8G8B8A8_UNORM)
	//}

	// TODO: Handle other texture types (e.g., specular, normal maps)
}
