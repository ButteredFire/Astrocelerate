/* EventTypes.hpp - Defines event types for the event dispatcher.
*/

#pragma once

#include <External/GLFWVulkan.hpp>

#include <External/GLM.hpp>

#include <Core/Data/Geometry.hpp>
#include <Core/Data/Application.hpp>


enum EventType {
	EVENT_APP_IS_STABLE,

	EVENT_ID_REGISTRY_IS_CLEARED,
	EVENT_ID_SWAPCHAIN_IS_RECREATED,
	EVENT_ID_OFFSCREEN_RESOURCES_ARE_RECREATED,
	EVENT_ID_VIEWPORT_IS_RESIZED,
	EVENT_ID_OFFSCREEN_PIPELINE_INITIALIZED,
	EVENT_ID_PRESENT_PIPELINE_INITIALIZED,
	EVENT_ID_GEOMETRY_INITIALIZED,

	EVENT_ID_UPDATE_APP_STAGE,
	EVENT_ID_UPDATE_INPUT,
	EVENT_ID_UPDATE_RENDERABLES,
	EVENT_ID_UPDATE_SESSION_STATUS,
	EVENT_ID_UPDATE_GUI,
	EVENT_ID_UPDATE_PHYSICS,
	EVENT_ID_UPDATE_UBOS,

	EVENT_ID_GUI_CONTEXT_IS_VALID,
	EVENT_ID_INPUT_IS_VALID,
	EVENT_ID_BUFFER_MANAGER_IS_VALID,

	EVENT_ID_REQUIRE_INIT_SESSION,

	EVENT_SCENE_LOAD_PROGRESS,
	EVENT_SCENE_LOAD_COMPLETE
};


/* Event types for the event dispatcher. */
namespace Event {
	/* Used when all managers/services have been created, and the application is at a stable point.
		Much like VkDeviceWaitIdle, this is a "catch-all" event.
	*/
	struct AppIsStable {
		const EventType eventType = EventType::EVENT_APP_IS_STABLE;
	};


	/* Used when the registry has been reset. */
	struct RegistryReset {
		const EventType eventType = EventType::EVENT_ID_REGISTRY_IS_CLEARED;
	};


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


	/* Used when the offscreen pipeline has been initialized. */
	struct OffscreenPipelineInitialized {
		const EventType eventType = EventType::EVENT_ID_OFFSCREEN_PIPELINE_INITIALIZED;
	};


	/* Used when the presentation pipeline has been initialized. */
	struct PresentPipelineInitialized {
		const EventType eventType = EventType::EVENT_ID_PRESENT_PIPELINE_INITIALIZED;
	};


	/* Used when data required to initialize global vertex and index buffers is available. */
	struct GeometryInitialized {
		const EventType eventType = EventType::EVENT_ID_GEOMETRY_INITIALIZED;
		std::vector<Geometry::Vertex> vertexData;
		std::vector<uint32_t> indexData;
		Geometry::GeometryData *pGeomData;
	};


	/* Used when the application stage is updated. */
	struct UpdateAppStage {
		const EventType eventType = EventType::EVENT_ID_UPDATE_APP_STAGE;
		Application::Stage newStage;
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


	/* Used to update the render status of renderables (e.g., are renderables ready to be rendered?) */
	struct UpdateSessionStatus {
		enum class Status {
			NOT_READY,				// Session is not initialized (meaning that, e.g., scenes and per-scene resources should not be initialized)
			PREPARE_FOR_INIT,		// Session is preparing to be initialized. This allows for managers listening to this event to also prepare.
			INITIALIZED				// Session is initialized. At this stage, scenes and per-scene resources are safe to use.
		};

		const EventType eventType = EventType::EVENT_ID_UPDATE_SESSION_STATUS;
		Status sessionStatus;
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


	/* Used when a manager/service requires initialization of a new user session. */
	struct RequireInitSession {
		const EventType eventType = EventType::EVENT_ID_REQUIRE_INIT_SESSION;
		std::string simulationFilePath;
	};


	/* Used to dispatch progress updates during heavy operations like scene loading. */
	struct SceneLoadProgress {
		const EventType eventType = EventType::EVENT_SCENE_LOAD_PROGRESS;
		float progress; // 0.0 to 1.0
		std::string message;
	};


	/* Used to signal the completion of a heavy operation like scene loading. */
	struct SceneLoadComplete {
		const EventType eventType = EventType::EVENT_SCENE_LOAD_COMPLETE;
		bool loadSuccessful;
		std::string finalMessage;
	};
}