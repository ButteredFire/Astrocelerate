/* Engine.hpp - Core engine logic.
*
* Defines the Engine class, responsible for managing the simulation loop, updating
* game state, and coordinating subsystems such as rendering.
*/

#pragma once

#include <vector>
#include <unordered_set>

#include <Vulkan/VkInstanceManager.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkSwapchainManager.hpp>
#include <Vulkan/VkCommandManager.hpp>
#include <Vulkan/VkSyncManager.hpp>

#include <Rendering/Renderer.hpp>
#include <Rendering/TextureManager.hpp>
#include <Rendering/GraphicsPipeline.hpp>

#include <Shaders/BufferManager.hpp>

#include <Core/ECS.hpp>
#include <Core/AppWindow.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>

#include <CoreStructs/ApplicationContext.hpp>

#include <Engine/Components/ModelComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>

#include <Systems/Time.hpp>
#include <Systems/RenderSystem.hpp>
#include <Systems/PhysicsSystem.hpp>


class Engine {
public:
	Engine(GLFWwindow *w, VulkanContext &context);
	~Engine();

	void initComponents();

	void run();

private:
	GLFWwindow *window;
	VulkanContext &m_vkContext;

	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<Renderer> m_renderer;

template<typename T>
	static inline bool isPointerValid(T *ptr) { return ptr != nullptr; };
	
	void update();
};
