/* Buffer.hpp - Common data pertaining to buffers.
*/

#pragma once

#include <glm_config.hpp>


namespace Buffer {
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