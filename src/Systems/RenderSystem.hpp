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
#include <CoreStructs/ApplicationContext.hpp>

#include <Engine/Components/RenderComponents.hpp>

class RenderSystem {
public:
	RenderSystem(VulkanContext& context);
	~RenderSystem() = default;

	void processRenderable(const VkCommandBuffer& cmdBuffer, Component::Renderable& renderable);

private:
	VulkanContext& m_vkContext;

	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
};