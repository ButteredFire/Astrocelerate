/* ModelComponents.hpp - Defines components pertaining to models: Meshes, textures, etc..
*/

#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include <glm_config.hpp>

#include <Engine/Components/ComponentTypes.hpp>

#include <CoreStructs/Geometry.hpp>


namespace Component {
	struct Mesh {
		ComponentType::Model type = ComponentType::Model::T_MODEL_MESH;		// The component type.
		uint32_t meshID;													// The mesh's ID.

        std::vector<Geometry::Vertex> vertices = {};						// The mesh's vertices.
        std::vector<uint32_t> indices = {};									// The mesh's vertex indices.
	};


	struct Material {
		ComponentType::Model type = ComponentType::Model::T_MODEL_MATERIAL;		// The component type.
		glm::vec4 baseColor = glm::vec4();										// The material's base color.
	};
}