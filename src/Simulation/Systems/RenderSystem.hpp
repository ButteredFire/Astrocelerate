/* RenderSystem.hpp - Handles the rendering of renderable entities.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// Local
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/EventDispatcher.hpp>

#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Data/Buffer.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Rendering/UIRenderer.hpp>
class UIRenderer;

#include <Utils/SystemUtils.hpp>

#include <Vulkan/VkBufferManager.hpp>
class VkBufferManager;


class RenderSystem {
public:
	RenderSystem();
	~RenderSystem() = default;

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<VkBufferManager> m_bufferManager;
	std::shared_ptr<UIRenderer> m_imguiRenderer;

	size_t m_dynamicAlignment = 0;


	void bindEvents();

	/* Processes a mesh. */
	void processMeshRenderable(const VkCommandBuffer& cmdBuffer, const RenderComponent::MeshRenderable& renderable, const VkDescriptorSet& descriptorSet);


	/* Processes the GUI. */
	void processGUIRenderable(const VkCommandBuffer& cmdBuffer, const RenderComponent::GUIRenderable& renderable, uint32_t currentFrame);
};
