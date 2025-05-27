/* EventTypes.hpp - Defines event types for the event dispatcher.
*/

#pragma once

#include <glfw_vulkan.hpp>

#include <glm_config.hpp>

#include <CoreStructs/Geometry.hpp>
#include <CoreStructs/Contexts.hpp>


enum EventType {
	EVENT_ID_SWAPCHAIN_RECREATION,

	EVENT_ID_INIT_FRAMEBUFFERS,
	EVENT_ID_INIT_GLOBAL_BUFFERS,

	EVENT_ID_UPDATE_INPUT,
	EVENT_ID_UPDATE_RENDERABLES,
	EVENT_ID_UPDATE_PHYSICS,
	EVENT_ID_UPDATE_UBOS,

	EVENT_ID_GUI_CONTEXT_IS_VALID
};


/* Event types for the event dispatcher. */
namespace Event {

	/* Used when the swapchain is recreated. */
	struct SwapchainRecreation {
		const EventType eventType = EventType::EVENT_ID_SWAPCHAIN_RECREATION;
	};


	/* Used when data required to initialize framebuffers is available (usually after graphics pipeline intiialization). */
	struct InitFrameBuffers {
		const EventType eventType = EventType::EVENT_ID_INIT_FRAMEBUFFERS;
	};


	/* Used when data required to initialize global vertex and index buffers is available. */
	struct InitGlobalBuffers {
		const EventType eventType = EventType::EVENT_ID_INIT_GLOBAL_BUFFERS;
		std::vector<Geometry::Vertex> vertexData;
		std::vector<uint32_t> indexData;
	};


	/* Used when user input needs to be processed. */
	struct UpdateInput {
		const EventType eventType = EventType::EVENT_ID_UPDATE_INPUT;
		double deltaTime;
	};


	/* Used when renderables need to be updated. */
	struct UpdateRenderables {
		const EventType eventType = EventType::EVENT_ID_UPDATE_RENDERABLES;
		VkCommandBuffer commandBuffer;
		VkDescriptorSet descriptorSet;
	};


	/* Used when physics need to be updated. */
	struct UpdatePhysics {
		const EventType eventType = EventType::EVENT_ID_UPDATE_PHYSICS;
		double dt;
	};


	/* Used when uniform buffer objects need to be updated. */
	struct UpdateUBOs {
		const EventType eventType = EventType::EVENT_ID_UPDATE_UBOS;
		uint32_t currentFrame;
		glm::dvec3 renderOrigin;
	};


	/* Used when the ImGui context is available. */
	struct GUIContextIsValid {
		const EventType eventType = EventType::EVENT_ID_GUI_CONTEXT_IS_VALID;
	};
}