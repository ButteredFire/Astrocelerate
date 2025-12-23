/* RenderSystem.hpp - Handles the rendering of renderable entities.
*/

#pragma once

#include <barrier>
#include <memory>

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// Local
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Application/ResourceManager.hpp>


#include <Core/Data/Buffer.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Rendering/UIRenderer.hpp>
class UIRenderer;

#include <Utils/SystemUtils.hpp>


class RenderSystem {
public:
	RenderSystem(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr, UIRenderer *uiRenderer);
	~RenderSystem();


	void tick();

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ResourceManager> m_resourceManager;

	VkCoreResourcesManager *m_coreResources;
	VkSwapchainManager *m_swapchainManager;
	UIRenderer *m_uiRenderer;


	// Synchronization to sleep until new data arrives for rendering
	std::mutex m_tickMutex;
	std::atomic<bool> m_hasNewData;
	std::atomic<bool> m_stopTick;
	std::condition_variable m_tickCondVar;

	std::shared_ptr<std::barrier<>> m_renderThreadBarrier;


	// Persistent
	VkDevice m_logicalDevice;
	QueueFamilyIndices m_queueFamilies;

	// Session data
	std::atomic<bool> m_sceneReady;
	VkBuffer m_globalVertexBuffer;
	VkBuffer m_globalIndexBuffer;

	VkExtent2D m_swapchainExtent;

	VkPipeline m_offscreenPipeline;
	VkPipelineLayout m_offscreenPipelineLayout;
	VkRenderPass m_offscreenRenderPass;
	std::vector<VkFramebuffer> m_offscreenFrameBuffers;

	std::vector<VkCommandBuffer> m_sceneSecondaryCmdBufs;	// Per-frame, pre-allocated secondary command buffers for scene rendering

	std::vector<VkDescriptorSet> m_perFrameDescriptorSets;
	VkDescriptorSet m_texArrayDescriptorSet;
	VkDescriptorSet m_pbrDescriptorSet;

	std::atomic<uint32_t> m_currentFrame;


	void bindEvents();

	void init();


	/* Waits for all necessary resources to be ready. */
	void waitForResources(const EventDispatcher::SubscriberIndex &selfIndex);


	/* Processes all meshes. */
	void renderScene(uint32_t currentFrame);


	/* Processes the GUI. */
	void renderGUI(VkCommandBuffer cmdBuffer, uint32_t currentFrame);
};
