/* Buffer.hpp - Common data pertaining to buffers.
*/

#pragma once

#include <External/GLM.hpp>
#include <vk_mem_alloc.h>


namespace Buffer {
	// "Utility" struct that groups a buffer and its allocation.
	struct BufferAndAlloc {
		VkBuffer buffer;
		VmaAllocation allocation;
	};


	// Global uniform buffer object.
	struct GlobalUBO {
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 projection;
		alignas(16) glm::vec3 cameraPosition;
		float _pad0 = 0.0f;

		alignas(16) glm::vec3 lightDirection;
		float _pad1 = 0.0f;

		alignas(16) glm::vec3 lightColor;
		float _pad2 = 0.0f;
	};


	// Per-object uniform buffer object.
	struct ObjectUBO {
		glm::mat4 model;
		glm::mat4 normalMatrix;
	};
}