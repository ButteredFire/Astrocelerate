/* Buffer.hpp - Common data pertaining to buffers.
*/

#pragma once

#include <vk_mem_alloc.h>


#include <Platform/External/GLM.hpp>


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

		alignas(16) glm::vec3 lightPosition;
		float _pad1 = 0.0f;

		alignas(16) glm::vec3 lightColor;
		float lightRadiantFlux;
	};


	// Per-object uniform buffer object.
	struct ObjectUBO {
		glm::mat4 model;
		glm::mat4 normalMatrix;
	};
}