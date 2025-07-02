/* Geometry.hpp - Common data pertaining to geometry: world, models, etc..
*/

#pragma once

#include <External/GLFWVulkan.hpp>

#include <External/GLM.hpp>

#include <vector>
#include <Core/Data/Constants.h>

#include <Utils/SystemUtils.hpp>


namespace Geometry {
	// Properties of a vertex.
	struct Vertex {
		alignas(16) glm::vec3 position;     // Vertex position.
		alignas(16) glm::vec3 color;        // Vertex color.
		alignas(8) glm::vec2 texCoord0;     // Texture coordinates (a.k.a., UV coordinates) for mapping textures

		alignas(16) glm::vec3 normal;		// Normals
		alignas(16) glm::vec3 tangent;		// Tangents


		bool operator==(const Vertex& other) const {
			return (
				(position == other.position)
				&& (color == other.color)
				&& (texCoord0 == other.texCoord0)
				&& (normal == other.normal)
				&& (tangent == other.tangent)
				);
		}


		/* Gets the vertex input binding description. */
		inline static VkVertexInputBindingDescription getVertexInputBindingDescription() {
			// A vertex binding describes at which rate to load data from memory throughout the m_vertices.
				// It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
			VkVertexInputBindingDescription bindingDescription{};

			// Our data is currently packed together in 1 array, so we're only going to have one binding (whose index is 0).
			// If we had multiple vertex buffers (e.g., one for position, one for color), each buffer would have its own binding index.
			bindingDescription.binding = 0;

			// Specifies the number of bytes from one entry to the next (i.e., byte stride between consecutive elements in a buffer)
			bindingDescription.stride = sizeof(Vertex);

			// Specifies how to move to the next entry
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next entry after each vertex (for per-vertex data); for instanced rendering, use INPUT_RATE_INSTANCE

			return bindingDescription;
		}


		/* Gets the vertex attribute descriptions. */
		inline static std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() {
			// Attribute descriptions specify the type of the attributes passed to the vertex shader, which binding to load them from (and at which offset)
				// Each vertex attribute (e.g., position, color) must have its own attribute description.
				// Each vertex attribute's binding must source its value from the vertex's bindingDescription binding.
			std::vector<VkVertexInputAttributeDescription> attribDescriptions{};

			size_t vertexAttribCount = 5;
			attribDescriptions.reserve(vertexAttribCount);

			// Attribute: Position
			VkVertexInputAttributeDescription positionAttribDesc{};
			positionAttribDesc.binding = 0;
			positionAttribDesc.location = ShaderConsts::VERT_LOC_IN_INPOSITION;
			positionAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
			positionAttribDesc.offset = offsetof(Vertex, position);

			attribDescriptions.push_back(positionAttribDesc);


			// Attribute: Color
			VkVertexInputAttributeDescription colorAttribDesc{};
			colorAttribDesc.binding = 0;
			colorAttribDesc.location = ShaderConsts::VERT_LOC_IN_INCOLOR;
			colorAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttribDesc.offset = offsetof(Vertex, color);

			attribDescriptions.push_back(colorAttribDesc);


			// Attribute: Texture/UV coordinates
			VkVertexInputAttributeDescription texCoordAttribDesc{};
			texCoordAttribDesc.binding = 0;
			texCoordAttribDesc.location = ShaderConsts::VERT_LOC_IN_INTEXTURECOORD_0;
			texCoordAttribDesc.format = VK_FORMAT_R32G32_SFLOAT;
			texCoordAttribDesc.offset = offsetof(Vertex, texCoord0);

			attribDescriptions.push_back(texCoordAttribDesc);


			// Attribute: Normal
			VkVertexInputAttributeDescription normalAttribDesc{};
			normalAttribDesc.binding = 0;
			normalAttribDesc.location = ShaderConsts::VERT_LOC_IN_INNORMAL;
			normalAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
			normalAttribDesc.offset = offsetof(Vertex, normal);

			attribDescriptions.push_back(normalAttribDesc);

			// Attribute: Tangent
			VkVertexInputAttributeDescription tangentAttribDesc{};
			tangentAttribDesc.binding = 0;
			tangentAttribDesc.location = ShaderConsts::VERT_LOC_IN_INTANGENT;
			tangentAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
			tangentAttribDesc.offset = offsetof(Vertex, tangent);

			attribDescriptions.push_back(tangentAttribDesc);


			return attribDescriptions;
		}
	};


	struct Material {
		// Albedo
		alignas(16) glm::vec3 albedoColor = glm::vec3(1.0f);			// Albedo (color)
		int32_t albedoMapIndex = -1;									// Albedo (map): Base color texture

		// Metallic & Roughness
		float metallicFactor = 0.0f;									// Metallic factor
		float roughnessFactor = 1.0f;									// Roughness factor
		int32_t metallicRoughnessMapIndex = -1;							// Metallic and Roughness packed into one texture (e.g., R=metallic, G=roughness, B=AO). NOTE: Alternatively, the map can be split into metallicMap and roughnessMap indices.

		// Normals
		int32_t normalMapIndex = -1;									// Tangent-space normal map

		// Ambient occlusion
		int32_t aoMapIndex = -1;										// Ambient occlusion texture

		// Emissive
		alignas(16) glm::vec3 emissiveColor = glm::vec3(0.0f);			// Emissive color

		// For glowing objects
		int32_t emissiveMapIndex = -1;									// Emissive texture

		// Opacity/Transparency (Alpha blending/masking)
			// NOTE: If different transparency types are needed, consider adding a `MaterialAlphaMode` enum (OPAQUE, MASK, BLEND)
		float opacity = 1.0f;											// Opacity/Transparency


		float _pad0[3] = {};
	};


	// Vertex and index buffer offsets.
	struct MeshOffset {
		uint32_t vertexOffset;			// Vertex buffer offset.
		uint32_t indexOffset;			// Index buffer offset.
		uint32_t materialIndex;			// Material index into the global texture array.
		uint32_t indexCount;			// Index count (index data from the offset index buffer).
	};


	// Raw mesh data.
	struct MeshData {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Material> materials;
		std::vector<MeshOffset> childMeshOffsets;
	};


	// Processed geometry data.
	struct GeometryData {
		size_t meshCount;
		std::vector<MeshOffset> meshOffsets;
		std::vector<Material> meshMaterials;
	};


	// Processed texture.
	struct Texture {
		glm::vec2 size;												// The texture's dimensions.
		VkImageLayout imageLayout;									// The texture's image layout.
		VkImageView imageView;										// The texture's image view.
		VkSampler sampler;											// The texture's image sampler.
	};
}

namespace std {
	template<>
	struct hash<Geometry::Vertex> {
		inline std::size_t operator()(const Geometry::Vertex& vertex) const noexcept {
			size_t seed = 0;
			SystemUtils::CombineHash(seed, vertex.position);
			SystemUtils::CombineHash(seed, vertex.color);
			SystemUtils::CombineHash(seed, vertex.texCoord0);
			SystemUtils::CombineHash(seed, vertex.normal);
			SystemUtils::CombineHash(seed, vertex.tangent);
			return seed;
		}
	};
}

