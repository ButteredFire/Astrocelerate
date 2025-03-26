#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <deque>
#include <variant>
#include <functional>

#include <ApplicationContext.hpp>
#include <LoggingManager.hpp>


// A structure specifying the properties of a cleanup task
struct CleanupTask {
	bool validTask = true;								// A special boolean specifying whether this task is executable or not
	std::string caller = "Unknown caller";				// The caller from which the task was pushed to the cleanup stack (used for logging)
	std::string mainObjectName = "Unknown object";		// The variable name of the primary object to be cleaned up later (used for logging)
	std::vector<VulkanHandles> vkObjects;				// A vector of Vulkan objects involved in their cleanup function
	std::function<void()> cleanupFunc;					// The cleanup/destroy callback function
    std::vector<bool> cleanupConditions;				// The conditions required for the callback function to be executed (aside from the default object validity checking)
};


// A structure specifying the properties of a memory block
struct MemoryBlock {
	VkDeviceMemory memory;			// The handle to the device memory object
	VkDeviceSize size;				// The size of the memory block (in bytes)
	VkDeviceSize currentOffset;		// The offset between the start of the actual memory block and the start of the usable sub-block
};


class MemoryManager {
public:
	MemoryManager();
	~MemoryManager();


	/* Creates the Vulkan Memory Allocator. The VMA object is automatically added to the application context, and its cleanup task created.
	* @param instance: The Vulkan instance.
	* @param physicalDevice: The selected physical device.
	* @param device: The selected logical device.
	* @return The VmaAllocator handle.
	*/
	VmaAllocator createVMAllocator(VkInstance& instance, VkPhysicalDevice& physicalDevice, VkDevice& device);


	/* Pushes a cleanup task to be executed on program exit to the cleanup stack (technically a deque, but almost always used like a stack).
	* @param task: The cleanup task.
	* @return The cleanup task's ID.
	*/
	uint32_t createCleanupTask(CleanupTask task);


	/* Modifies an existing cleanup task. 
	* @param taskID: The task's ID.
	* @return A reference to the cleanup task (allowing for method chaining).
	*/
	CleanupTask& modifyCleanupTask(uint32_t taskID);


	/* Executes a cleanup task from anywhere in the cleanup stack. This can be dangerous if the main object of the cleanup task to be executed is still being referenced by other objects or tasks.
	* @param taskID: The task's ID.
	* @return True if the execution was successful, otherwise False.
	*/
	bool executeCleanupTask(uint32_t taskID);


	/* Executes all cleanup tasks in the cleanup stack. */
	void processCleanupStack();

private:
	VmaAllocator vmaAllocator = VK_NULL_HANDLE;
	std::deque<CleanupTask> cleanupStack;

	std::unordered_map<uint32_t, size_t> idToIdxLookup;  // A hashmap that maps a cleanup task's ID to its index in the cleanup stack
	uint32_t nextID;  // A counter for generating unique cleanup task IDs

	// Defines the maximum number of invalid tasks (if invalidTasks exceeds the maximum, we can perform a cleanup on the cleanup stack itself, i.e., remove invalid tasks from the stack and update the ID-to-Index hashmap accordingly)
	const uint32_t MAX_INVALID_TASKS = 10;
	uint32_t invalidTasks = 0;


	/* Executes a cleanup task.
	* @param task: The task to be executed.
	* @param taskID: The task's ID.
	* @return True if the execution was successful, otherwise False.
	*/
	bool executeTask(CleanupTask& task, uint32_t taskID);


	/* Garbage-collects the cleanup stack. */
	void cleanStack();
};