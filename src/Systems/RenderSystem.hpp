/* RenderSystem.hpp - Handles the rendering of renderable entities.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Local
#include <Core/ECSCore.hpp>
#include <Core/ApplicationContext.hpp>

#include <Engine/Components/RenderComponents.hpp>

class RenderSystem {
public:
	static void processRenderable(VulkanContext& m_vkContext, VkCommandBuffer& cmdBuffer, RenderableComponent& renderable);
};