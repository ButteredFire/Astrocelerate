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


	void bindEvents();

	/* Processes a mesh. */
	void renderScene(const Event::UpdateRenderables &event);


	/* Processes the GUI. */
	void renderGUI(const Event::UpdateGUI &event);
};
