/* Renderer.hpp - Handles Vulkan-based rendering.
*
* Defines the Renderer class, which manages Vulkan rendering logic.
*/


#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#include <glm_config.hpp>

// C++ STLs
#include <iostream>
#include <optional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// Local
#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkCommandManager.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Rendering/UIRenderer.hpp>
#include <Rendering/Pipelines/GraphicsPipeline.hpp>

#include <Vulkan/VkBufferManager.hpp>

#include <Core/ECS.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>

// Font
#include <../assets/DefaultFont.hpp>


class Renderer {
public:
	Renderer(VulkanContext &context);
	~Renderer();

	void init();

	/* Updates the rendering. */
	void update(glm::dvec3& renderOrigin);

private:
	VkInstance& m_vulkInst;
	VulkanContext& m_vkContext;

	std::shared_ptr<Registry> m_globalRegistry;

	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<VkSwapchainManager> m_swapchainManager;
	std::shared_ptr<VkBufferManager> m_bufferManager;
	std::shared_ptr<VkCommandManager> m_commandManager;

	std::shared_ptr<UIRenderer> m_imguiRenderer;

	uint32_t m_currentFrame = 0;

	Entity m_vertexRenderable{};
	Entity m_guiRenderable{};

	Component::MeshRenderable m_vertexRenderComponent{};
	Component::GUIRenderable m_guiRenderComponent{};

	/* Creates renderable entities. */
	void initializeRenderables();


	/* Renders a frame. 
		At a high level, rendering a frame in Vulkan consists of a common set of steps:
			- Wait for the previous frame to finish
			- Acquire an image from the swap chain
			- Record a command buffer which draws the scene onto that image
			- Submit the recorded command buffer
			- Present the swap chain image
	*/
	void drawFrame(glm::dvec3& renderOrigin);
};
