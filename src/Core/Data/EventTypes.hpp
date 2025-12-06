/* EventTypes.hpp - Defines event types for the event dispatcher.
*/

#pragma once

#include <External/GLFWVulkan.hpp>

#include <External/GLM.hpp>

#include <Core/Application/GarbageCollector.hpp>
#include <Core/Data/Device.hpp>
#include <Core/Data/Geometry.hpp>
#include <Core/Data/Application.hpp>


// All events (used as bitfield flags)
using EventFlags = unsigned int;
enum EventFlag {
	EVENT_FLAG_INIT_OFFSCREEN_PIPELINE_BIT				= 1 << 0,
	EVENT_FLAG_INIT_PRESENT_PIPELINE_BIT				= 1 << 1,
	EVENT_FLAG_INIT_GEOMETRY_BIT						= 1 << 2,
	EVENT_FLAG_INIT_SCENE_BIT							= 1 << 3,
	EVENT_FLAG_INIT_IMGUI_BIT							= 1 << 4,
	EVENT_FLAG_INIT_INPUT_MANAGER_BIT					= 1 << 5,
	EVENT_FLAG_INIT_BUFFER_MANAGER_BIT					= 1 << 6,
	EVENT_FLAG_INIT_SWAPCHAIN_MANAGER_BIT				= 1 << 7,

	EVENT_FLAG_RECREATION_SWAPCHAIN_BIT					= 1 << 8,
	EVENT_FLAG_RECREATION_OFFSCREEN_RESOURCES_BIT		= 1 << 9,

	EVENT_FLAG_UPDATE_APPLICATION_STATUS_BIT			= 1 << 10,
	EVENT_FLAG_UPDATE_INPUT_BIT							= 1 << 11,
	EVENT_FLAG_UPDATE_RENDERABLES_BIT					= 1 << 12,
	EVENT_FLAG_UPDATE_SESSION_STATUS_BIT				= 1 << 13,
	EVENT_FLAG_UPDATE_PHYSICS_BIT						= 1 << 14,
	EVENT_FLAG_UPDATE_PER_FRAME_BUFFERS_BIT				= 1 << 15,
	EVENT_FLAG_UPDATE_APP_IS_STABLE_BIT					= 1 << 16,
	EVENT_FLAG_UPDATE_REGISTRY_RESET_BIT				= 1 << 17,
	EVENT_FLAG_UPDATE_SCENE_LOAD_PROGRESS_BIT			= 1 << 18,
	EVENT_FLAG_UPDATE_SCENE_LOAD_COMPLETE_BIT			= 1 << 19,
	EVENT_FLAG_UPDATE_VIEWPORT_SIZE_BIT					= 1 << 20,
	EVENT_FLAG_UPDATE_CORE_RESOURCES_BIT				= 1 << 21,

	EVENT_FLAG_REQUEST_INIT_SESSION_BIT					= 1 << 22,
	EVENT_FLAG_REQUEST_PROCESS_SECONDARY_COMMAND_BUFFERS_BIT = 1 << 23,
	EVENT_FLAG_REQUEST_INIT_SCENE_RESOURCES_BIT			= 1 << 24,

	EVENT_FLAG_CONFIG_SIMULATION_FILE_PARSED			= 1 << 25
};
constexpr size_t EVENT_FLAG_COUNT = 25 + 1; // Highest bit position + 1


namespace InitEvent {
	/* Used when the offscreen pipeline has been initialized. */
	struct OffscreenPipeline {
		const EventFlag eventFlag = EVENT_FLAG_INIT_OFFSCREEN_PIPELINE_BIT;

		VkRenderPass renderPass;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;

		std::vector<VkDescriptorSet> perFrameDescriptorSets;
		VkDescriptorSet pbrDescriptorSet;
		VkDescriptorSet texArrayDescriptorSet;

		std::vector<VkImage> offscreenImages;
		std::vector<VkImageView> offscreenImageViews;
		std::vector<VkSampler> offscreenImageSamplers;
		std::vector<VkFramebuffer> offscreenFrameBuffers;
	};


	/* Used when the presentation pipeline has been initialized. */
	struct PresentPipeline {
		const EventFlag eventFlag = EVENT_FLAG_INIT_PRESENT_PIPELINE_BIT;

		VkRenderPass renderPass;
	};


	/* Used when data required to initialize global vertex and index buffers is available. */
	struct Geometry {
		/* NOTE:
			There exists a namespace with the same name as this struct: "Geometry".
			To resolve the ambiguity when we want to access data inside the Geometry namespace, we use the global scope resolution operator (::).
			Example:
				Geometry::Vertex	vertex;		// Conflict!
			->  ::Geometry::Vertex	vertex;		// The compiler understands "Vertex" is in the Geometry namespace and not the Geometry struct.
		*/
		const EventFlag eventFlag = EVENT_FLAG_INIT_GEOMETRY_BIT;

		std::vector<::Geometry::Vertex> vertexData;
		std::vector<uint32_t> indexData;
		::Geometry::GeometryData *pGeomData;
	};


	/* Used when the scene is initialized (most often emitted to signal the end of the scene initialization worker thread).
		This event is just an alias for InitEvent::OffscreenPipeline for the sake of readability.
		Services that don't have a direct tie to the offscreen pipeline should listen to this event.
	*/
	struct Scene {
		const EventFlag eventFlag = EVENT_FLAG_INIT_SCENE_BIT;
	};


	/* Used when the ImGui context is available. */
	struct ImGui {
		const EventFlag eventFlag = EVENT_FLAG_INIT_IMGUI_BIT;
	};


	/* Used when the input manager is initialized. */
	struct InputManager {
		const EventFlag eventFlag = EVENT_FLAG_INIT_INPUT_MANAGER_BIT;
	};


	/* Used when the buffer manager is initialized. */
	struct BufferManager {
		const EventFlag eventFlag = EVENT_FLAG_INIT_BUFFER_MANAGER_BIT;

		VkBuffer globalVertexBuffer;
		VkBuffer globalIndexBuffer;
		std::vector<VkDescriptorSet> perFrameDescriptorSets;
	};


	/* Used when the swapchain manager is ready. */
	struct SwapchainManager {
		const EventFlag eventFlag = EVENT_FLAG_INIT_SWAPCHAIN_MANAGER_BIT;
	};
}



namespace RecreationEvent {
	/* Used AFTER the swapchain has been recreated. */
	struct Swapchain {
		const EventFlag eventFlag = EVENT_FLAG_RECREATION_SWAPCHAIN_BIT;

		uint32_t imageIndex;
		std::vector<VkImageLayout> imageLayouts;
		std::vector<CleanupID> deferredDestructionList;
	};


	/* Used AFTER offscreen render targets have been recreated. */
	struct OffscreenResources {
		const EventFlag eventFlag = EVENT_FLAG_RECREATION_OFFSCREEN_RESOURCES_BIT;

		std::vector<VkImageView> imageViews;
		std::vector<VkSampler> samplers;
		std::vector<VkFramebuffer> framebuffers;
	};
}



namespace UpdateEvent {
	/* Used when the application status is updated. */
	struct ApplicationStatus {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_APPLICATION_STATUS_BIT;

		Application::Stage appStage = Application::Stage::NULL_STAGE;
		Application::State appState = Application::State::NULL_STATE;
	};


	/* Used when user input needs to be processed. */
	struct Input {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_INPUT_BIT;

		double deltaTime = 0.0;
		double timeSinceLastPhysicsUpdate = 0.0;
	};


	/* Used when renderables need to be updated. */
	struct Renderables {
		enum class Type {
			MESHES,			// Meshes.
			GUI				// Dear ImGui quads.
		};
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_RENDERABLES_BIT;

		Type renderableType;

		VkCommandBuffer commandBuffer;
		uint32_t currentFrame;
	};


	/* Used to update the status of the current session. */
	struct SessionStatus {
		enum class Status {
			PREPARE_FOR_RESET,		// Session is preparing to be reset. This allows for any manager using per-session resources to immediately stop accessing them
			RESET,					// Session has been reset. This allows for per-session managers to destroy all outdated resources in preparation for new ones.
			PREPARE_FOR_INIT,		// Session is preparing to be initialized. This allows for per-session managers to also prepare (e.g., by allowing certain processes to be run).
			INITIALIZED,			// Session is initialized. At this stage, scenes and per-scene resources are safe to be used for creating dynamic resources that may depend on them.
			POST_INITIALIZATION		// Session is online. At this stage, all default per-scene resources, as well as dynamic ones created during the INITIALIZED stage, are ready for access.
		};
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_SESSION_STATUS_BIT;

		Status sessionStatus;
	};


	/* Used when physics need to be updated. */
	struct Physics {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_PHYSICS_BIT;

		double dt;
	};


	/* Used when uniform buffer objects need to be updated. */
	struct PerFrameBuffers {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_PER_FRAME_BUFFERS_BIT;

		uint32_t currentFrame;
		glm::dvec3 renderOrigin;
	};



	/* Used when all managers/services have been created, and the application is at a stable point.
		Much like VkDeviceWaitIdle, this is a "catch-all" event.
	*/
	struct AppIsStable {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_APP_IS_STABLE_BIT;
	};


	/* Used when the registry has been reset. */
	struct RegistryReset {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_REGISTRY_RESET_BIT;
	};


	/* Used to dispatch progress updates during heavy operations like scene loading. */
	struct SceneLoadProgress {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_SCENE_LOAD_PROGRESS_BIT;

		float progress; // 0.0 to 1.0
		std::string message;
	};


	/* Used to signal the completion of a heavy operation like scene loading. */
	struct SceneLoadComplete {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_SCENE_LOAD_COMPLETE_BIT;

		bool loadSuccessful;
		std::string finalMessage;
	};


	/* Used when the GUI viewport's available region to render the scene onto has changed in size. */
	struct ViewportSize {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_VIEWPORT_SIZE_BIT;

		glm::vec2 sceneDimensions;
	};


	/* Used when any core resource is recreated/updated. Core resources are defined in VkCoreResourcesManager. */
	struct CoreResources {
		const EventFlag eventFlag = EVENT_FLAG_UPDATE_CORE_RESOURCES_BIT;

		GLFWwindow *window;
		VkInstance instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT dbgMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		PhysicalDeviceProperties chosenDevice{};
		std::vector<PhysicalDeviceProperties> availableDevices;

		VkDevice logicalDevice = VK_NULL_HANDLE;
		QueueFamilyIndices familyIndices{};

		VmaAllocator vmaAllocator = VK_NULL_HANDLE;
	};
}


namespace RequestEvent {
	/* Used when a manager/service requests initialization of a new user session. */
	struct InitSession {
		const EventFlag eventFlag = EVENT_FLAG_REQUEST_INIT_SESSION_BIT;

		std::string simulationFilePath;
	};


	/* Used when any secondary command buffers have finished recording, and need to be recorded into the primary command buffer. */
	struct ProcessSecondaryCommandBuffers {
		const EventFlag eventFlag = EVENT_FLAG_REQUEST_PROCESS_SECONDARY_COMMAND_BUFFERS_BIT;

		std::vector<VkCommandBuffer> buffers;
	};


	/* Used when the scene processing is complete, and its Vulkan resources (e.g., offscreen pipeline) need to be initialized. */
	struct InitSceneResources {
		const EventFlag eventFlag = EVENT_FLAG_REQUEST_INIT_SCENE_RESOURCES_BIT;
	};
}



namespace ConfigEvent {
	/* Used when a simulation file has been successfully read. */
	struct SimulationFileParsed {
		const EventFlag eventFlag = EVENT_FLAG_CONFIG_SIMULATION_FILE_PARSED;

		Application::YAMLFileConfig fileConfig;
		Application::SimulationConfig simulationConfig;
	};
}