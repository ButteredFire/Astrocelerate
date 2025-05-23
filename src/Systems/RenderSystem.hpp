/* RenderSystem.hpp - Handles the rendering of renderable entities.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Local
#include <Core/ECS.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>

#include <CoreStructs/Contexts.hpp>
#include <CoreStructs/Buffer.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Utils/SubpassBinder.hpp>

#include <Vulkan/VkBufferManager.hpp>
class VkBufferManager;


class RenderSystem {
public:
	RenderSystem(VulkanContext& context);
	~RenderSystem() = default;

private:
	VulkanContext& m_vkContext;

	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<VkBufferManager> m_bufferManager;

	std::shared_ptr<SubpassBinder> m_subpassBinder;

	size_t m_dynamicAlignment = 0;


	/* Processes a mesh. */
	void processMeshRenderable(const VkCommandBuffer& cmdBuffer, const Component::MeshRenderable& renderable, const VkDescriptorSet& descriptorSet);


	/* Processes the GUI. */
	void processGUIRenderable(const VkCommandBuffer& cmdBuffer, const Component::GUIRenderable& renderable);

	/* Processes a renderable. */
	//template<typename Renderable>
	//void processRenderable(const VkCommandBuffer& cmdBuffer, Renderable& renderable, SubpassConsts::Type subpassType);
};
