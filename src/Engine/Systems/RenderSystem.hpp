/* RenderSystem - Handles scene rendering.
*/

#pragma once

#include <barrier>
#include <memory>


#include <Core/Utils/SystemUtils.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/VkBufferManager.hpp>
#include <Platform/Vulkan/Utils/VkCommandUtils.hpp>

#include <Engine/Scene/Camera.hpp>
#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Registry/ECS/Components/RenderComponents.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>
#include <Engine/Rendering/UIRenderer.hpp>
#include <Engine/Rendering/Data/Buffer.hpp>
#include <Engine/Rendering/Data/Geometry.hpp>
#include <Engine/Rendering/Visualizers/GeometryVisualizer.hpp>


class UIRenderer;


class RenderSystem {
public:
	RenderSystem(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, std::shared_ptr<VkBufferManager> bufferMgr, std::shared_ptr<UIRenderer> uiRenderer, std::shared_ptr<Camera> camera);
	~RenderSystem();


	void init(const Geometry::GeometryData *geomData, const Ctx::OffscreenPipeline *offscreenData);

	void tick(std::stop_token stopToken);

	/* Signals the RenderSystem worker thread to begin rendering work.
		@param curentFrame: The index of the frame for which rendering will be done.
		@param barrier: The barrier used to synchronize between the worker thread and the main thread.
	*/
	void beginWork(uint32_t currentFrame, std::weak_ptr<std::barrier<>> barrier);


	/* Processes the GUI. */
	void renderGUI(uint32_t currentFrame, uint32_t imageIndex);


	/* Gets the processed secondary command buffers containing draw calls for the scene.
		@param currentFrame: The index of the frame for which the scene will be rendered.
	*/
	std::vector<VkCommandBuffer> getSceneCommandBuffers(uint32_t currentFrame);

	/* Gets the processed secondary command buffers containing draw calls for the GUI.
		@param currentFrame: The index of the frame for which the GUI will be rendered.
	*/
	VkCommandBuffer getGUICommandBuffer(uint32_t currentFrame);

private:
	std::shared_ptr<ECSRegistry> m_ecsRegistry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<CleanupManager> m_cleanupManager;

	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	const Ctx::VkWindow *m_windowCtx;

	std::shared_ptr<VkBufferManager> m_bufferManager;
	std::shared_ptr<UIRenderer> m_uiRenderer;
	std::shared_ptr<Camera> m_camera;


	// Synchronization to sleep until new data arrives for rendering
	std::mutex m_tickMutex;
	std::atomic<bool> m_hasNewData;
	std::condition_variable_any m_tickCondVar;

	std::weak_ptr<std::barrier<>> m_renderThreadBarrier;

		// Command buffers
	std::array<bool, SimulationConst::MAX_FRAMES_IN_FLIGHT> m_finishedRecording;
	std::mutex m_cmdBufProcessMutex;
	std::condition_variable m_cmdBufProcessCV;


	// Persistent
	VkDevice m_logicalDevice;
	QueueFamilyIndices m_queueFamilies;

	// Session data
	std::atomic<bool> m_sceneReady;
	std::atomic<uint32_t> m_currentFrame;
	ResourceID m_sessionResourceID;

	const Geometry::GeometryData *m_geomData;
	const Ctx::OffscreenPipeline *m_offscreenData;

	Buffer::BufferAlloc m_globalVertBufAlloc;
	Buffer::BufferAlloc m_globalIdxBufAlloc;

	struct FrameMemResource {
		Buffer::BufferAlloc bufAlloc;
		VkDescriptorSet descriptorSet;
	};
	std::array<FrameMemResource, SimulationConst::MAX_FRAMES_IN_FLIGHT> m_globalUBOs;

		// Visualizers
	uint32_t m_visualizerCount;
	std::vector<std::unique_ptr<IVisualizer>> m_visualizers;
	std::vector<std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT>> m_visualizerCmdBufs;	// Secondary command buffer sets for Visualizers
	std::vector<VkCommandPool> m_visualizerCommandPools; // Command pools storing secondary command buffer sets for Visualizers

	VkCommandPool m_guiCommandPool;
	std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> m_guiCmdBufs;						// Secondary command buffer sets for the GUI


	void bindEvents();

	void initGlobalBuffers();

	void createVisualizers();

	void initVisualizers();

	/* Creates sets of secondary command buffers for the visualizers. */
	void initVisualizerCmdBufSets();

	/* Creates sets of secondary command buffers for the GUI. */
	void initGUICmdBufSets();


	/* Constructs the data necessary to render a single frame.
		@param packet: The pointer to the frame packet to be populated.
	*/
	void buildFramePacket(Buffer::FramePacket *packet);
	
	
	/* Renders the scene by transforming raw mesh data into Vulkan commands. */
	void renderScene(Buffer::FramePacket *packet);
};
