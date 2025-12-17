#pragma once

// GLFW & Vulkan
#include <vk_mem_alloc.h>
#include <External/GLFWVulkan.hpp>

#include <deque>
#include <vector>
#include <variant>
#include <iomanip> // std::hex
#include <functional>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/Tree.hpp>


using VulkanHandles = std::variant <
	VmaAllocator,
	VmaAllocation,
	VkDebugUtilsMessengerEXT,
	VkInstance,
	VkPhysicalDevice,   // Usually not destroyed explicitly, but listed for completeness  
	VkDevice,
	VkQueue,            // Generally managed by Vulkan and not destroyed manually  
	VkCommandPool,
	VkCommandBuffer,
	VkBuffer,
	VkBufferView,
	VkImage,
	VkImageView,
	VkFramebuffer,
	VkRenderPass,
	VkShaderModule,
	VkPipeline,
	VkPipelineLayout,
	VkDescriptorSetLayout,
	VkDescriptorPool,
	VkDescriptorSet,    // Generally managed by `VkDescriptorPool`  
	VkSampler,
	VkFence,
	VkSemaphore,
	VkEvent,
	VkQueryPool,
	VkSwapchainKHR,     // Requires `VK_KHR_swapchain` extension  
	VkSurfaceKHR,       // Requires `VK_KHR_surface`  
	VkDeviceMemory
> ;

/* Checks whether a Vulkan object is valid/non-null. */
template<typename T> bool vkIsValid(T vulkanObject) { return (vulkanObject != T()); }


using CleanupID = uint32_t;

// A structure specifying the properties of a cleanup task
struct CleanupTask {
	CleanupID _id{};												// [INTERNAL] The task's own cleanup ID. This is used for stack optimization.
	bool _validTask = true;											// [INTERNAL] A special boolean specifying whether this task is executable or not.
	std::string caller = "Unknown caller";							// The caller from which the task was pushed to the cleanup stack (used for logging).
	std::vector<std::string> objectNames = { "Unknown object" };	// The variable name of objects to be cleaned up later (used for logging).
	std::vector<VulkanHandles> vkHandles;							// A vector of Vulkan handles involved in their cleanup function.
	std::function<void()> cleanupFunc;								// The cleanup/destroy callback function.
	std::vector<bool> cleanupConditions;							// The conditions required for the callback function to be executed (aside from the default object validity checking).
};


class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();


	/* Creates the Vulkan Memory Allocator. The VMA object is automatically added to the Vulkan context, and its cleanup task created.
		@param instance: The Vulkan instance.
		@param physicalDevice: The selected physical device.
		@param device: The selected logical device.

		@return The VmaAllocator handle.
	*/
	VmaAllocator createVMAllocator(VkInstance &instance, VkPhysicalDevice &physicalDevice, VkDevice &device);


	/* Schedules a new cleanup task.
		@param task: The cleanup task.
		@param parentTaskID (overload): The parent task upon which this task depends.

		@return The cleanup task's ID.
	*/
	CleanupID createCleanupTask(CleanupTask task);
	CleanupID createCleanupTask(CleanupTask childTask, CleanupID parentTaskID);


	/* Creates a cleanup task and sets it as the root node for the underlying tree implementation. */
	CleanupID createRootCleanupTask(CleanupTask task);


	/* Makes a cleanup task depend on another cleanup task.
		When a cleanup task has child tasks, upon the task's execution, all of its children will be executed first.

		@param childTaskID: The to-be-child cleanup task's ID.
		@param parentTaskID: The to-be-parent cleanup task's ID.
	*/
	void addTaskDependency(CleanupID childTaskID, CleanupID parentTaskID);


	/* Modifies an existing cleanup task.
		@param taskID: The task's ID.

		@return A reference to the cleanup task (allowing for method chaining).
	*/
	CleanupTask &modifyCleanupTask(CleanupID taskID);


	/* Executes a cleanup task (recursively if it has child tasks).
		@param taskID: The task's ID.
		@param executeParent (Default: True): If the cleanup task to be executed has children, does it need to be executed as well?

		@return True if the execution was successful, False otherwise.
	*/
	void executeCleanupTask(CleanupID taskID, bool executeParent = true);


	/* Executes all cleanup tasks in the cleanup stack. */
	void processCleanupStack();

private:
	VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

	std::recursive_mutex m_cleanupMutex;
	std::deque<CleanupTask> m_cleanupStack;

	Tree<CleanupTask> m_taskTree;
	std::optional<NodeID> m_rootNodeID; // This should be set to the root node, e.g., an essential Vulkan resource
	std::optional<NodeID> m_currentNodeID;

	std::atomic<CleanupID> m_nextID;


	/* Executes a cleanup task (recursively if it has child tasks).
		@param taskID: The ID of the task to be executed.
		@param executeParent: If the cleanup task to be executed has children, does it need to be executed as well?
	*/
	void executeTask(CleanupID taskID, bool executeParent);


	/* Constructs a string of object names involved in a cleanup task.
		@return The string in question.
	*/
	std::string getObjectNamesString(CleanupTask &task);
};