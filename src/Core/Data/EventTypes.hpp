/* EventTypes.hpp - Defines event types for the event dispatcher.
*/

#pragma once

#include <External/GLFWVulkan.hpp>

#include <External/GLM.hpp>

#include <Core/Data/Geometry.hpp>


enum EventType {
	EVENT_ID_SWAPCHAIN_IS_RECREATED,
	EVENT_ID_OFFSCREEN_RESOURCES_ARE_RECREATED,
	EVENT_ID_VIEWPORT_IS_RESIZED,
	EVENT_ID_PIPELINES_ARE_INITIALIZED,

	EVENT_ID_INIT_GLOBAL_BUFFERS,

	EVENT_ID_UPDATE_INPUT,
	EVENT_ID_UPDATE_RENDERABLES,
	EVENT_ID_UPDATE_GUI,
	EVENT_ID_UPDATE_PHYSICS,
	EVENT_ID_UPDATE_UBOS,

	EVENT_ID_GUI_CONTEXT_IS_VALID,
	EVENT_ID_INPUT_IS_VALID,
	EVENT_ID_BUFFER_MANAGER_IS_VALID
};


/* Event types for the event dispatcher. */
namespace Event {

	/* Used when the swapchain is recreated. */
	struct SwapchainIsRecreated {
		const EventType eventType = EventType::EVENT_ID_SWAPCHAIN_IS_RECREATED;
		uint32_t imageIndex;
	};


	/* Used when offscreen render targets are recreated. */
	struct OffscreenResourcesAreRecreated {
		const EventType eventType = EventType::EVENT_ID_OFFSCREEN_RESOURCES_ARE_RECREATED;
	};


	/* Used when the simulation viewport is resized. */
	struct ViewportIsResized {
		const EventType eventType = EventType::EVENT_ID_VIEWPORT_IS_RESIZED;
		uint32_t currentFrame;
		uint32_t newWidth;
		uint32_t newHeight;
	};


	/* Used when all pipelines have been initialized. */
	struct PipelinesInitialized {
		const EventType eventType = EventType::EVENT_ID_PIPELINES_ARE_INITIALIZED;
	};


	/* Used when data required to initialize global vertex and index buffers is available. */
	struct InitGlobalBuffers {
		const EventType eventType = EventType::EVENT_ID_INIT_GLOBAL_BUFFERS;
		std::vector<Geometry::Vertex> vertexData;
		std::vector<uint32_t> indexData;
		Geometry::GeometryData *pGeomData;
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


	/* Used when the ImGui GUI needs to be updated. */
	struct UpdateGUI {
		const EventType eventType = EventType::EVENT_ID_UPDATE_GUI;
		VkCommandBuffer commandBuffer;
		uint32_t currentFrame;
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


	/* Used when the input manager is initialized. */
	struct InputIsValid {
		const EventType eventType = EventType::EVENT_ID_INPUT_IS_VALID;
	};


	/* Used when the buffer manager is initialized. */
	struct BufferManagerIsValid {
		const EventType eventType = EventType::EVENT_ID_BUFFER_MANAGER_IS_VALID;
	};
}