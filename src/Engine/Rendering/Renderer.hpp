/* Renderer.hpp - Handles the main application rendering loop.
*/


#pragma once

#include <vector>
#include <barrier>
#include <iostream>
#include <optional>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <imgui/imgui.h>


#include <Core/Data/Constants.h>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/External/GLM.hpp>
#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/VkSyncManager.hpp>
#include <Platform/Vulkan/VkWindowManager.hpp>
#include <Platform/Vulkan/VkBufferManager.hpp>
#include <Platform/Vulkan/VkCommandManager.hpp>

#include <Engine/Scene/Camera.hpp>
#include <Engine/Systems/RenderSystem.hpp>
#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Registry/ECS/Components/RenderComponents.hpp>
#include <Engine/Rendering/UIRenderer.hpp>
#include <Engine/Rendering/Data/Buffer.hpp>


class Renderer {
public:
	Renderer(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, std::shared_ptr<VkWindowManager> windowMgr, std::shared_ptr<VkCommandManager> commandMgr, std::shared_ptr<VkSyncManager> syncMgr, std::shared_ptr<UIRenderer> uiRenderer, std::shared_ptr<RenderSystem> renderSystem);
	~Renderer();

	void tick();

private:
	std::shared_ptr<ECSRegistry> m_ecsRegistry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<CleanupManager> m_cleanupManager;

	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	const Ctx::VkWindow *m_windowCtx;

	std::shared_ptr<VkWindowManager> m_windowManager;
	std::shared_ptr<VkCommandManager> m_commandManager;
	std::shared_ptr<VkSyncManager> m_syncManager;
	std::shared_ptr<UIRenderer> m_uiRenderer;
	std::shared_ptr<RenderSystem> m_renderSystem;


	std::vector<VkSemaphore> m_imageReadySemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

	uint32_t m_currentFrame = 0;

	// Session data
	bool m_sessionReady = false;
	bool m_pauseUpdateLoop = false;

	const uint32_t RENDERER_THREAD_COUNT = 1 + 1; // + 1 to count the main thread, which also participates in rendering
	std::shared_ptr<std::barrier<>> m_renderThreadBarrier;

	std::vector<VkCommandBuffer> m_secondaryCmdBufsStageNONE{};
	std::vector<VkCommandBuffer> m_secondaryCmdBufsStageOFFSCREEN{};
	std::vector<VkCommandBuffer> m_secondaryCmdBufsStagePRESENT{};


	void bindEvents();

	void init();

	void recreateSwapchainResources();


	/* Performs any updates prior to command buffer recording. */
	void preRenderTick();


	/* Renders a frame. 
		At a high level, rendering a frame in Vulkan consists of a common set of steps:
			- Wait for the previous frame to finish
			- Acquire an image from the swap chain
			- Record a command buffer which draws the scene onto that image
			- Submit the recorded command buffer
			- Present the swap chain image
	*/
	void drawFrame();
};
