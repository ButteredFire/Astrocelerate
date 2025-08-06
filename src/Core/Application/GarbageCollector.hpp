#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>
#include <vk_mem_alloc.h>

#include <vector>
#include <deque>
#include <variant>
#include <iomanip> // std::hex
#include <functional>

#include <Core/Application/LoggingManager.hpp>


using VulkanHandles = std::variant<
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
>;

/* Checks whether a Vulkan object is valid/non-null. */
template<typename T> bool vkIsValid(T vulkanObject) { return (vulkanObject != T()); }


using CleanupID = uint32_t;

// A structure specifying the properties of a cleanup task
struct CleanupTask {
	CleanupID _id{};												// [INTERNAL] The task's own cleanup ID. This is used for stack optimization.
	bool _validTask = true;											// [INTERNAL] A special boolean specifying whether this task is executable or not.
	std::string caller = "Unknown caller";							// The caller from which the task was pushed to the cleanup stack (used for logging).
	std::vector<std::string> objectNames = {"Unknown object"};		// The variable name of objects to be cleaned up later (used for logging).
	std::vector<VulkanHandles> vkHandles;							// A vector of Vulkan handles involved in their cleanup function.
	std::function<void()> cleanupFunc;								// The cleanup/destroy callback function.
    std::vector<bool> cleanupConditions;							// The conditions required for the callback function to be executed (aside from the default object validity checking).
};


class GarbageCollector {
public:
	GarbageCollector();
	~GarbageCollector();


	/* Creates the Vulkan Memory Allocator. The VMA object is automatically added to the Vulkan context, and its cleanup task created.
		@param instance: The Vulkan instance.
		@param physicalDevice: The selected physical device.
		@param device: The selected logical device.
		
		@return The VmaAllocator handle.
	*/
	VmaAllocator createVMAllocator(VkInstance& instance, VkPhysicalDevice& physicalDevice, VkDevice& device);


	/* Pushes a cleanup task to be executed on program exit to the cleanup stack (technically a deque, but almost always used like a stack).
		@param task: The cleanup task.
		
		@return The cleanup task's ID.
	*/
	CleanupID createCleanupTask(CleanupTask task);


	/* Modifies an existing cleanup task. 
		@param taskID: The task's ID.
		
		@return A reference to the cleanup task (allowing for method chaining).
	*/
	CleanupTask& modifyCleanupTask(CleanupID taskID);


	/* Executes a cleanup task from anywhere in the cleanup stack. This can be dangerous if the main object of the cleanup task to be executed is still being referenced by other objects or tasks.
		@param taskID: The task's ID.
		
		@return True if the execution was successful, False otherwise.
	*/
	bool executeCleanupTask(CleanupID taskID);


	/* Executes all cleanup tasks in the cleanup stack. */
	void processCleanupStack();

private:
	VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

	std::recursive_mutex m_cleanupMutex;
	std::deque<CleanupTask> m_cleanupStack;

	std::unordered_map<CleanupID, size_t> m_idToIdxLookup;  // A hashmap that maps a cleanup task's ID to its index in the cleanup stack
	std::atomic<CleanupID> m_nextID;  // A counter for generating unique cleanup task IDs

	// Defines the maximum number of invalid tasks (if invalidTasks exceeds the maximum, we can perform a cleanup on the cleanup stack itself, i.e., remove invalid tasks from the stack and update the ID-to-Index hashmap accordingly)
	const uint32_t m_MAX_INVALID_TASKS = 20;
	uint32_t m_invalidTaskCount = 0;


	/* Executes a cleanup task.
		@param task: The task to be executed.
		@param taskID: The task's ID.
		
		@return True if the execution was successful, False otherwise.
	*/
	bool executeTask(CleanupTask& task, CleanupID taskID);


	/* Garbage-collects the cleanup stack. */
	void optimizeStack();


	/* Constructs a string of object names involved in a cleanup task.
		@return The string in question.
	*/
	std::string getObjectNamesString(CleanupTask &task);
};