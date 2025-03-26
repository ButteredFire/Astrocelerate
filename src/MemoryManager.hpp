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
	* @param lowestPriority (Default: False): A boolean determining whether the task should be executed last in the queue (True), or not (False). By default, the task will be pushed to the back of the cleanup queue and, if it is the last task in the queue, be executed first on program exit.
	*/
	void createCleanupTask(CleanupTask task, bool lowestPriority = false);


	/* Executes a cleanup task from anywhere in the cleanup stack. This can be computationally costly and/or dangerous!
	* @param vkObjects: The data of the cleanup task's vkObjects member when the task was created. vkObjects is used as a key to look up the cleanup task to be executed. In theory, every task's vkObjects vector is unique, but may still cause undefined behavior.
	* @return True if the execution was successful, otherwise False.
	*/
	bool executeCleanupTask(std::vector<VulkanHandles> vkObjects);


	/* Executes all cleanup tasks in the cleanup stack. */
	void processCleanupStack();

private:
	VmaAllocator vmaAllocator = VK_NULL_HANDLE;
	std::deque<CleanupTask> cleanupStack;

	/* Executes a cleanup task.
	* @param task: The task to be executed.
	* @return True if the execution was successful, otherwise False.
	*/
	bool executeTask(CleanupTask& task);
};