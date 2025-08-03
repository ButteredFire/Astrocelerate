/* RenderSystem.hpp - Handles the rendering of renderable entities.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// Local
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/EventDispatcher.hpp>


#include <Core/Data/Buffer.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Rendering/UIRenderer.hpp>
class UIRenderer;

#include <Utils/SystemUtils.hpp>


class RenderSystem {
public:
	RenderSystem(VkCoreResourcesManager *coreResources, UIRenderer *uiRenderer);
	~RenderSystem() = default;

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	VkCoreResourcesManager *m_coreResources;
	UIRenderer *m_uiRenderer;


	// Session data
	bool m_sceneReady = false;
	VkBuffer m_globalVertexBuffer;
	VkBuffer m_globalIndexBuffer;

	VkPipelineLayout m_offscreenPipelineLayout;

	std::vector<VkDescriptorSet> m_perFrameDescriptorSets;
	VkDescriptorSet m_texArrayDescriptorSet;
	VkDescriptorSet m_pbrDescriptorSet;


	void bindEvents();

	/* Waits for all necessary resources to be ready. */
	void waitForResources(const EventDispatcher::SubscriberIndex &selfIndex);

	/* Processes a mesh. */
	void renderScene(VkCommandBuffer cmdBuffer, uint32_t currentFrame);


	/* Processes the GUI. */
	void renderGUI(VkCommandBuffer cmdBuffer, uint32_t currentFrame);
};
