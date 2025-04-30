#include "ModelLoader.hpp"

Component::Mesh ModelLoader::loadModel(const std::string& filePath, FileType fileType)
{
	Component::Mesh mesh{};

	switch (fileType) {
	case FileType::T_OBJ: {
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		tinyobj::attrib_t attributes;                   // Object attributes (e.g., vertices, normals, UV coordinates)
		std::vector<tinyobj::shape_t> shapes;           // Contains all separate objects and their faces
		std::vector<tinyobj::material_t> materials;
		std::string warnings, errors;

		bool loadSuccessful = tinyobj::LoadObj(&attributes, &shapes, &materials, &warnings, &errors, filePath.c_str());

		if (!loadSuccessful) {
			throw Log::RuntimeException(__FUNCTION__, (warnings + errors));
		}

		// Combines all faces in the file into a single model
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.position = glm::vec3(
					attributes.vertices[3 * index.vertex_index + 0],
					attributes.vertices[3 * index.vertex_index + 1],
					attributes.vertices[3 * index.vertex_index + 2]
				);

				vertex.texCoord = glm::vec2(
					attributes.texcoords[2 * index.texcoord_index + 0],
					1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
				);

				vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);



				if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
					uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
					mesh.vertices.push_back(vertex);
				}

				mesh.indices.push_back(uniqueVertices[vertex]);
			}
		}

		break;
	}
	
	default:
		throw Log::RuntimeException(__FUNCTION__, "Failed to load model: Invalid file type!");
		break;
	}

	return mesh;
}
