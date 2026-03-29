/* Buffer.hpp - Defines buffers used in rendering.
*/

#pragma once

#include <vector>

#include <vk_mem_alloc.h>


#include <Core/Data/Math.hpp>

#include <Platform/External/GLM.hpp>
#include <Platform/External/GLFWVulkan.hpp>

#include <Engine/Rendering/Data/Geometry.hpp>


namespace Buffer {
	// Global uniform buffer object
	struct GlobalUBO {
		alignas(16) glm::mat4 viewMatrix;
		alignas(16) glm::mat4 projMatrix;
		alignas(16) glm::vec3 cameraPos;
		float ambientStrength;
		alignas(16) glm::vec3 lightPos;
		float lightRadiantFlux;
		alignas(16) glm::vec3 lightColor;
		float _pad0 = 0.0f;
	};


	// Per-object uniform buffer object
	struct ObjectUBO {
		glm::mat4 modelMatrix;
		glm::mat4 normalMatrix;
	};


	// Transient global data for rendering one frame.
	struct FramePacket {
		uint32_t frameIndex;

		Geometry::GeometryData *geomData;
		glm::dvec3 camFloatingOrigin;
		GlobalUBO globalUBO;
	};
}