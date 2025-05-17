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
#include <Vulkan/VkBufferManager.hpp>

#include <Rendering/Renderer.hpp>
#include <Rendering/Textures/TextureManager.hpp>
#include <Rendering/Pipelines/GraphicsPipeline.hpp>
#include <Rendering/Geometry/GeometryLoader.hpp>

#include <Core/ECS.hpp>
#include <Core/InputManager.hpp>
#include <Core/AppWindow.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/EventDispatcher.hpp>

#include <CoreStructs/Contexts.hpp>

#include <Engine/Components/ModelComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/WorldSpaceComponents.hpp>

#include <Systems/Time.hpp>
#include <Systems/RenderSystem.hpp>
#include <Systems/PhysicsSystem.hpp>

#include <Utils/SubpassBinder.hpp>


class Engine {
public:
	Engine(GLFWwindow *w, VulkanContext &context);
	~Engine();

	void initComponents();

	void run();

private:
	GLFWwindow *window;
	VulkanContext &m_vkContext;

	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<Renderer> m_renderer;

	std::shared_ptr<InputManager> m_appInput;

template<typename T>
	static inline bool isPointerValid(T *ptr) { return ptr != nullptr; };
	
	void update();
};
