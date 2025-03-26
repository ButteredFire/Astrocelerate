#include "MemoryManager.hpp"

MemoryManager::MemoryManager(): nextID(0) {

	Log::print(Log::T_INFO, __FUNCTION__, "Initializing...");

	// Initializes the VMA's cleanup task as a placeholder (for modification later when the VMA is actually created)
	CleanupTask vmaCleanupTask{};
	vmaCleanupTask.validTask = false;
	vmaCleanupTask.caller = __FUNCTION__;
	vmaCleanupTask.mainObjectName = "Vulkan Memory Allocator";
	createCleanupTask(vmaCleanupTask);
}

MemoryManager::~MemoryManager() {}


VmaAllocator MemoryManager::createVMAllocator(VkInstance& instance, VkPhysicalDevice& physicalDevice, VkDevice& device) {
	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.physicalDevice = physicalDevice;
	allocatorCreateInfo.device = device;
	allocatorCreateInfo.instance = instance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // If using buffer device addresses (optional)


	VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create Vulkan Memory Allocator!");
	}

	// Updates the VMA's cleanup task with proper data
	CleanupTask& task = modifyCleanupTask(0);
	task.validTask = true;
	task.caller = __FUNCTION__;
	task.mainObjectName = VARIABLE_NAME(vmaAllocator);
	task.vkObjects = { vmaAllocator };
	task.cleanupFunc = [this]() { vmaDestroyAllocator(vmaAllocator); };

	return vmaAllocator;
}


uint32_t MemoryManager::createCleanupTask(CleanupTask task) {
	uint32_t id = nextID++;
	cleanupStack.push_back(task);
	idToIdxLookup[id] = (cleanupStack.size() - 1);

	Log::print(Log::T_INFO, task.caller.c_str(), "Pushed object " + enquote(task.mainObjectName) + " to cleanup stack.");
	return id;
}


CleanupTask& MemoryManager::modifyCleanupTask(uint32_t taskID) {
	try {
		return cleanupStack[idToIdxLookup.at(taskID)];
	}
	catch (const std::exception& e) {
		throw Log::RuntimeException(__FUNCTION__, "Task ID " + enquote(std::to_string(taskID)) + " is invalid!\nOriginal exception message: " + std::string(e.what()));
	}
}


bool MemoryManager::executeCleanupTask(uint32_t taskID) {
	try {
		CleanupTask& task = cleanupStack[idToIdxLookup.at(taskID)];
		return executeTask(task, taskID);
	}
	catch (const std::exception& e) {
		throw Log::RuntimeException(__FUNCTION__, "Task ID " + enquote(std::to_string(taskID)) + " is invalid!\nOriginal exception message: " + std::string(e.what()));
	}
}


void MemoryManager::processCleanupStack() {
	size_t stackSize = cleanupStack.size();

	cleanStack();

	std::string plural = (stackSize != 1)? "s" : "";
	Log::print(Log::T_INFO, __FUNCTION__, "Executing " + std::to_string(stackSize) + " task" + plural + " in the cleanup stack...");

	while (!cleanupStack.empty()) {
		uint32_t id = (cleanupStack.size() - 1);
		CleanupTask task = cleanupStack[id];
		executeTask(task, id);

		cleanupStack.pop_back();
	}
}


bool MemoryManager::executeTask(CleanupTask& task, uint32_t taskID) {
	std::string objectName = enquote(task.caller + " -> " + task.mainObjectName);

	if (!task.validTask) {
		Log::print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup task for object " + objectName + " because it either has already been executed or is invalid.");
		return false;
	}

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

	// Executes the task and invalidates it to prevent future executions
	task.cleanupFunc();
	task.validTask = false;
	invalidTasks++;

	if (invalidTasks >= MAX_INVALID_TASKS)
		cleanStack();


	Log::print(Log::T_INFO, __FUNCTION__, "Executed cleanup task for object " + objectName + ".");
	return true;
}


void MemoryManager::cleanStack() {
	size_t displacement = 0, invalidTaskCount = 0;
	size_t oldSize = cleanupStack.size();
	for (size_t i = 0; i < oldSize; i++) {
		if (!cleanupStack[i].validTask) {
			displacement++;
			invalidTaskCount++;
		}
		else {
			idToIdxLookup[i] -= displacement;
			cleanupStack[i - displacement] = cleanupStack[i];
		}
	}

	nextID = (oldSize - invalidTaskCount);

	for (size_t i = (oldSize - invalidTaskCount); i < oldSize; i++) {
		cleanupStack.erase(cleanupStack.begin() + (oldSize - invalidTaskCount));
	}

	invalidTasks = 0;
}
