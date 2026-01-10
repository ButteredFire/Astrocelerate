/* ModelComponents.hpp - Defines components pertaining to models: Meshes, textures, etc..
*/

#pragma once

#include <vk_mem_alloc.h>


#include <Platform/External/GLM.hpp>
#include <Platform/External/GLFWVulkan.hpp>

#include <Engine/Rendering/Data/Geometry.hpp>


namespace ModelComponent {
	struct Mesh {
		uint32_t meshID;											// The mesh's ID.

        std::vector<Geometry::Vertex> vertices = {};				// The mesh's vertices.
        std::vector<uint32_t> indices = {};							// The mesh's vertex indices.
	};


	struct Material {
		size_t textureIndex;										// Index into a texture atlas or descriptor array.
		glm::vec4 baseColor = glm::vec4();							// The material's base color.
	};
}