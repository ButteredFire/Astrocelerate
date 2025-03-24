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


struct CleanupTask {
	bool validTask = true;								// A special boolean specifying whether this task is executable or not
	std::string caller = "Unknown caller";				// The caller from which the task was pushed to the cleanup stack (used for logging)
	std::string mainObjectName = "Unknown object";		// The variable name of the primary object to be cleaned up later (used for logging)
	std::vector<VulkanHandles> vkObjects;				// A vector of Vulkan objects involved in their cleanup function
	std::function<void()> cleanupFunc;					// The cleanup/destroy callback function
    std::vector<bool> cleanupConditions;				// The conditions required for the callback function to be executed (aside from the default object validity checking)
};


class MemoryManager {
public:
	MemoryManager();
	~MemoryManager();


	/* Pushes a cleanup task to be executed on program exit to the cleanup stack (technically a deque, but almost always used like a stack). */
	void createCleanupTask(CleanupTask task);


	/* Executes a cleanup task from anywhere in the cleanup stack. This can be computationally costly and/or dangerous!
	* @param vkObjects: The data of the cleanup task's vkObjects member when the task was created. vkObjects is used as a key to look up the cleanup task to be executed. In theory, every task's vkObjects vector is unique, but may still cause undefined behavior.
	* @return True if the execution was successful, otherwise False.
	*/
	bool executeCleanupTask(std::vector<VulkanHandles> vkObjects);


	/* Executes all cleanup tasks in the cleanup stack. */
	void executeStack();

private:
	std::deque<CleanupTask> cleanupStack;

	/* Executes a cleanup task.
	* @param task: The task to be executed.
	* @return True if the execution was successful, otherwise False.
	*/
	bool executeTask(CleanupTask& task);
};