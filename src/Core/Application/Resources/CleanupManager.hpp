#pragma once

#include <deque>
#include <vector>
#include <variant>
#include <iomanip> // std::hex
#include <optional>
#include <functional>

#include <vk_mem_alloc.h>


#include <Core/Data/Tree.hpp>
#include <Core/Application/IO/LoggingManager.hpp>

#include <Platform/External/GLFWVulkan.hpp>


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


using ResourceID = uint32_t;

// A structure specifying the properties of a cleanup task
struct CleanupTask {
	std::string caller;												// The caller from which the task was pushed to the cleanup stack (used for logging).
	std::vector<std::string> objectNames;							// The variable name of objects to be cleaned up later (used for logging).
	std::vector<VulkanHandles> vkHandles;							// A vector of Vulkan handles involved in their cleanup function.
	std::function<void()> cleanupFunc = [](){};						// The cleanup/destroy callback function.
	std::vector<bool> cleanupConditions;							// The conditions required for the callback function to be executed (aside from the default object validity checking).
};


class CleanupManager {
public:
	CleanupManager();
	~CleanupManager();


	/* Creates the Vulkan Memory Allocator. The VMA object is automatically added to the Vulkan context, and its cleanup task created.
		@param instance: The Vulkan instance.
		@param physicalDevice: The selected physical device.
		@param device: The selected logical device.

		@return The VmaAllocator handle.
	*/
	VmaAllocator createVMAllocator(VkInstance &instance, VkPhysicalDevice &physicalDevice, VkDevice &device);


	/* Schedules a new cleanup task.
		@param task: The cleanup task.
		@param parentTaskID (Default: nullopt): The parent task upon which this task depends.

		@return The cleanup task's ID.
	*/
	ResourceID createCleanupTask(CleanupTask task, std::optional<ResourceID> parentTaskID = std::nullopt);


	/* Creates a logical cleanup group (virtual cleanup task) that performs no action but can act as a parent for other resources.
		@param parentTaskID (Default: nullopt): The parent task upon which this task depends.

		@return The cleanup group's ID.
	*/
	ResourceID createCleanupGroup(std::optional<ResourceID> parentTaskID = std::nullopt);


	/* Creates a cleanup task and sets it as the root node for the underlying tree implementation. */
	ResourceID createRootCleanupTask(CleanupTask task);


	/* Makes a cleanup task depend on another cleanup task.
		When a cleanup task has child tasks, upon the task's execution, all of its children will also be executed, in a child-to-parent order.

		@param childTaskID: The to-be-child cleanup task's ID.
		@param parentTaskID: The to-be-parent cleanup task's ID.
	*/
	void addTaskDependency(ResourceID childTaskID, ResourceID parentTaskID);


	/* Modifies an existing cleanup task.
		@param taskID: The task's ID.

		@return A reference to the cleanup task (allowing for method chaining).
	*/
	CleanupTask &modifyCleanupTask(ResourceID taskID);


	/* Executes a cleanup task (recursively if it has child tasks).
		@param taskID: The task's ID.
		@param executeParent (Default: True): If the cleanup task to be executed has children, does it need to be executed as well?

		@return True if the execution was successful, False otherwise.
	*/
	void executeCleanupTask(ResourceID taskID, bool executeParent = true);


	/* Executes all cleanup tasks in the cleanup tree. */
	void cleanupAll();

private:
	VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

	std::recursive_mutex m_cleanupMutex;
	std::deque<CleanupTask> m_cleanupStack;

	Tree<CleanupTask> m_taskTree;
	NodeID m_treeResourceGroup;		// Resource group of the entire tree (i.e., the root node)
	std::optional<NodeID> m_rootNodeID; // This should be set to the root node, e.g., an essential Vulkan resource
	std::optional<NodeID> m_currentNodeID;

	struct _TaskMetadata {
		bool isValid = true;	// Is the cleanup task still valid?
		bool isGroup = false;	// Is the cleanup task created as a virtual grouping node instead of a node storing real data?
	};
	std::unordered_map<NodeID, _TaskMetadata> m_taskMetadata;


	/* Internal bare-bones implementation of createCleanupTask. */
	ResourceID createTask(CleanupTask task, std::optional<ResourceID> parentTaskID = std::nullopt);


	/* Executes a cleanup task (recursively if it has child tasks).
		@param taskID: The ID of the task to be executed.
		@param executeParent: If the cleanup task to be executed has children, does it need to be executed as well?
	*/
	void executeTask(ResourceID taskID, bool executeParent);


	/* Constructs a string of object names involved in a cleanup task.
		@return The string in question.
	*/
	std::string getObjectNamesString(CleanupTask &task);
};