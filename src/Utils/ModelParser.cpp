#include "ModelParser.hpp"


MeshData AssimpParser::parse(const std::string& path) {
	MeshData meshData{};

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

	const aiScene* scene = importer.ReadFile(path, postProcessingFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, importer.GetErrorString());
	}


	processNode(scene->mRootNode, scene, meshData);

	//std::unordered_map<Geometry::Vertex, uint32_t> uniqueVertices{};

	//tinyobj::attrib_t attributes;                   // Object attributes (e.g., vertices, normals, UV coordinates)
	//std::vector<tinyobj::shape_t> shapes;           // Contains all separate objects and their faces
	//std::vector<tinyobj::material_t> materials;
	//std::string warnings, errors;

	//bool loadSuccessful = tinyobj::LoadObj(&attributes, &shapes, &materials, &warnings, &errors, path.c_str());

	//if (!loadSuccessful) {
	//	throw Log::RuntimeException(__FUNCTION__, __LINE__, (warnings + errors));
	//}

	//// Combines all faces in the file into a single model
	//for (const auto& shape : shapes) {
	//	for (const auto& index : shape.mesh.indices) {
	//		Geometry::Vertex vertex{};

	//		vertex.position = glm::vec3(
	//			attributes.vertices[3 * index.vertex_index + 0],
	//			attributes.vertices[3 * index.vertex_index + 1],
	//			attributes.vertices[3 * index.vertex_index + 2]
	//		);

	//		vertex.texCoord = glm::vec2(
	//			attributes.texcoords[2 * index.texcoord_index + 0],
	//			1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
	//		);

	//		vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);



	//		if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
	//			uniqueVertices[vertex] = static_cast<uint32_t>(meshData.vertices.size());
	//			meshData.vertices.push_back(vertex);
	//		}

	//		meshData.indices.push_back(uniqueVertices[vertex]);
	//	}
	//}


	return meshData;
}


void AssimpParser::processNode(aiNode* node, const aiScene* scene, MeshData& meshData) {
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
			processMesh(mesh, meshData);
		}

		Log::print(Log::T_WARNING, __FUNCTION__, "Mesh data vertex count for node " + enquote(node->mName.C_Str()) + ": " + std::to_string(meshData.vertices.size()));
	}

	// Recursively processes child nodes
	size_t childNodeCount = static_cast<size_t>(node->mNumChildren);

	for (size_t i = 0; i < childNodeCount; i++) {
		processNode(node->mChildren[i], scene, meshData);
	}
}


void AssimpParser::processMesh(aiMesh* mesh, MeshData& meshData) {
	// Processes each vertex in mesh
	size_t vertexCount = static_cast<size_t>(mesh->mNumVertices);
	std::unordered_map<Geometry::Vertex, uint32_t> uniqueVertices{};

	for (size_t i = 0; i < vertexCount; i++) {
		Geometry::Vertex vertex{};

		// Position
		vertex.position = glm::vec3(
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		);

		// Normals
		if (mesh->HasNormals()) {
			vertex.normal = glm::vec3(
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z
			);
		}

		// UV coordinates
		if (mesh->HasTextureCoords(0)) {
			vertex.texCoord = glm::vec2(
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			);
		}

		// TODO: Add color support
		vertex.color = glm::vec3(1.0f);


		// Index processing
		if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
			// Add the vertex to the vertex buffer and store its index
			uniqueVertices[vertex] = static_cast<uint32_t>(meshData.vertices.size());
			meshData.vertices.push_back(vertex);
		}
	}


	// TODO: Process indices for index buffer
	// Extracts indices
	size_t faceCount = static_cast<size_t>(mesh->mNumFaces);

	for (size_t i = 0; i < faceCount; i++) {
		aiFace& face = mesh->mFaces[i];
		size_t indicesCount = static_cast<size_t>(face.mNumIndices);

		for (size_t j = 0; j < indicesCount; j++) {
			Geometry::Vertex& vertex = meshData.vertices[face.mIndices[j]];
			meshData.indices.push_back(uniqueVertices[vertex]);
		}
	}

}
