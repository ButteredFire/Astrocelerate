#include "MemoryManager.hpp"

MemoryManager::MemoryManager() {}

MemoryManager::~MemoryManager() {}

void MemoryManager::createCleanupTask(CleanupTask task) {
	Log::print(Log::T_INFO, task.caller.c_str(), "Pushed object " + enquote(task.mainObjectName) + " to cleanup stack.");
	cleanupStack.push_back(task);
}


bool MemoryManager::executeCleanupTask(std::vector<VulkanHandles> vkObjects) {
	bool execSuccess = false;
	uint32_t taskIdx = 0;
	for (auto& task : cleanupStack) {
		if (task.vkObjects == vkObjects) {
			execSuccess = executeTask(task);
			CleanupTask invalidTask{};
			invalidTask.validTask = false;
			cleanupStack[taskIdx] = invalidTask;

			return execSuccess;
		}

		taskIdx++;
	}

	return execSuccess;
}

void MemoryManager::executeStack() {
	size_t stackSize = cleanupStack.size();
	std::string plural = (stackSize != 1)? "s" : "";
	Log::print(Log::T_INFO, __FUNCTION__, "Executing cleanup task for " + std::to_string(stackSize) + " object" + plural + " in the cleanup stack...");

	while (!cleanupStack.empty()) {
		CleanupTask task = cleanupStack[cleanupStack.size() - 1];
		if (task.validTask)
			executeTask(task);

		cleanupStack.pop_back();
	}
}


bool MemoryManager::executeTask(CleanupTask& task) {
	std::string objectName = enquote(task.caller + " -> " + task.mainObjectName);

	// Checks the validity of all Vulkan objects involved in the task
	bool proceedCleanup = true;
	for (const auto& object : task.vkObjects) {
		if (!vkIsValid(object)) {
			proceedCleanup = false;
			break;
		}
	}

	if (!task.cleanupConditions.empty()) {
		for (const auto& cond : task.cleanupConditions) {
			if (!cond) {
				proceedCleanup = false;
				break;
			}
		}
	}

	if (!proceedCleanup) {
		Log::print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup task for object " + objectName + " due to an invalid Vulkan object used in its destroy/free callback function.");
		return false;
	}

	// Executes the task
	task.cleanupFunc();

	Log::print(Log::T_INFO, __FUNCTION__, "Executed cleanup task for object " + objectName + ".");
	return true;
}
