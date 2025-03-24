#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stack>
#include <variant>
#include <functional>

#include <ApplicationContext.hpp>
#include <LoggingManager.hpp>


struct CleanupTask {
	std::string caller;						// The caller from which the task was pushed to the cleanup stack (used for logging)
	std::string mainObjectName;				// The variable name of the primary object to be cleaned up later (used for logging)
	std::vector<VulkanHandles> vkObjects;	// A vector of Vulkan objects involved in their cleanup function
	std::function<void()> cleanupFunc;		// The cleanup/destroy callback function
    std::vector<bool> cleanupConditions;    // The conditions required for the callback function to be executed (aside from the default object validity checking)
};


class MemoryManager {
public:
	MemoryManager();
	~MemoryManager();


	/* Pushes a cleanup task to be executed on program exit to the cleanup stack. */
	void createCleanupTask(CleanupTask task);


	/* Executes all cleanup tasks in the cleanup stack. */
	void executeStack();

private:
	std::stack<CleanupTask> cleanupStack;
};