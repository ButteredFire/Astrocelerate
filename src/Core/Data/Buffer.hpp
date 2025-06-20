/* Buffer.hpp - Common data pertaining to buffers.
*/

#pragma once

#include <External/GLM.hpp>


namespace Buffer {
	// "Utility" struct that groups a buffer and its allocation.
	struct BufferAndAlloc {
		VkBuffer buffer;
		VmaAllocation allocation;
	};


	// Global uniform buffer object.
	struct GlobalUBO {
		glm::mat4 view;
		glm::mat4 projection;
	};


	// Per-object uniform buffer object.
	struct ObjectUBO {
		glm::mat4 model;
	};
}