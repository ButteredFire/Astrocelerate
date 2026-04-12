/* Buffer.hpp - Defines buffers used in rendering.
*/

#pragma once

#include <vector>
#include <optional>

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
		alignas(16) glm::vec3 floatingOrigin;
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


	// Transient handoff data between PhysicsSystem and RenderSystem for rendering one frame
	struct PhysRendFramePacket {
		struct EntityData {
			uint32_t entityID;

			// Transform
			glm::dvec3 position;
			glm::dquat orientation;
			double scale;

			// RigidBody
			glm::dvec3 velocity;
			glm::dvec3 acceleration;
			double mass;

			// Orbit trajectory vertex data
			std::optional <std::vector<glm::dvec3>> orbitVertices = std::nullopt;	// The entity's orbit vertices (nullopt if entity orbit is still valid/not dirty, otherwise populated)
		};

		std::vector<EntityData> entities;
		double epoch;			// Absolute time
		double simulationTime;	// Physics simulation time elapsed since epoch
	};


	// Transient global data for rendering one frame
	struct FramePacket {
		uint32_t frameIndex;

		Geometry::GeometryData *geomData;
		PhysRendFramePacket *physRendFrame;

		glm::dvec3 camFloatingOrigin;
		GlobalUBO globalUBO;
	};
}