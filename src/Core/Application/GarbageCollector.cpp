#include "GarbageCollector.hpp"

GarbageCollector::GarbageCollector():
	m_nextID(0) {

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

GarbageCollector::~GarbageCollector() {}


VmaAllocator GarbageCollector::createVMAllocator(VkInstance& instance, VkPhysicalDevice& physicalDevice, VkDevice& device) {
	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.physicalDevice = physicalDevice;
	allocatorCreateInfo.device = device;
	allocatorCreateInfo.instance = instance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // If using buffer device addresses (optional)


	VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create Vulkan Memory Allocator!");
	}

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_vmaAllocator) };
	task.vkHandles = { m_vmaAllocator };
	task.cleanupFunc = [this]() { vmaDestroyAllocator(m_vmaAllocator); };

	createCleanupTask(task);

	g_vkContext.vmaAllocator = m_vmaAllocator;
	return m_vmaAllocator;
}


CleanupID GarbageCollector::createCleanupTask(CleanupTask task) {
	// Constructs a string displaying the object name(s) for logging
	std::string objectNamesStr = enquote(getObjectNamesString(task));

	CleanupID id = m_nextID++;
    task._id = id;
	m_idToIdxLookup[id] = m_cleanupStack.size();
	m_cleanupStack.push_back(task);

	Log::Print(Log::T_VERBOSE, task.caller.c_str(), "Pushed object(s) " + objectNamesStr + " to cleanup stack.");
	return id;
}


CleanupTask& GarbageCollector::modifyCleanupTask(CleanupID taskID) {
	LOG_ASSERT(m_idToIdxLookup.count(taskID), "Cannot modify cleanup task: Task ID #" + std::to_string(taskID) + " is invalid!");
	return m_cleanupStack[m_idToIdxLookup.at(taskID)];
}


bool GarbageCollector::executeCleanupTask(CleanupID taskID) {
	LOG_ASSERT(m_idToIdxLookup.count(taskID), "Cannot execute cleanup task: Task ID #" + std::to_string(taskID) + " is invalid!");
	LOG_ASSERT(m_idToIdxLookup.at(taskID) < m_cleanupStack.size(), "Cannot execute cleanup task: Cannot retrieve task data for task ID #" + std::to_string(taskID) + "!");
	
	CleanupTask& task = m_cleanupStack[m_idToIdxLookup.at(taskID)];
	return executeTask(task, taskID);
}


void GarbageCollector::processCleanupStack() {
	optimizeStack();
	size_t stackSize = m_cleanupStack.size();

	std::string plural = (stackSize != 1)? "s" : "";
	Log::Print(Log::T_VERBOSE, __FUNCTION__, "Executing " + std::to_string(stackSize) + " task" + plural + " in the cleanup stack...");

	if (g_vkContext.Device.logicalDevice)
		// Makes sure the CPU is idle before executing tasks
		vkDeviceWaitIdle(g_vkContext.Device.logicalDevice);

	while (!m_cleanupStack.empty()) {
		CleanupID id = (m_cleanupStack.size() - 1);
		CleanupTask task = m_cleanupStack[id];
		executeTask(task, id);

		m_cleanupStack.pop_back();
	}
}


bool GarbageCollector::executeTask(CleanupTask& task, CleanupID taskID) {
	// Constructs a string displaying the object name(s) for logging
	std::string objectNamesStr = enquote(getObjectNamesString(task));


	// Checks whether the task is already invalid
	if (!task._validTask) {
		Log::Print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup task for object(s) " + objectNamesStr + ".");
		return false;
	}

	// Checks the validity of all Vulkan objects involved in the task
	bool proceedCleanup = true;
	for (const auto& object : task.vkHandles) {
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
		Log::Print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup task for object(s) " + objectNamesStr + " due to an invalid Vulkan object used in their destroy/free callback function.");
		return false;
	}

	// Executes the task and invalidates it to prevent future executions
	task.cleanupFunc();
	
	Log::Print(Log::T_VERBOSE, __FUNCTION__, "Executed cleanup task for object(s) " + objectNamesStr + ".");

	task._validTask = false;
	m_invalidTaskCount++;

	if (m_invalidTaskCount >= m_MAX_INVALID_TASKS) {
		optimizeStack();
	}

	return true;
}


void GarbageCollector::optimizeStack() {
	// TODO: Fix optimizeStack incorrect ID-to-cleanup index remapping issue
	return;

    // Create a new deque for valid tasks
    std::deque<CleanupTask> newCleanupStack;
    std::unordered_map<CleanupID, size_t> newIdToIdxLookup;

    size_t oldSize = m_cleanupStack.size();

    size_t newIndex = 0;
    for (size_t i = 0; i < m_cleanupStack.size(); ++i) {
        if (m_cleanupStack[i]._validTask) {
            // Only add valid tasks to the new stack
            newCleanupStack.push_back(m_cleanupStack[i]);
            newIdToIdxLookup[m_cleanupStack[i]._id] = newIndex;
            newIndex++;
        }
    }

	// A = std::move(B) efficiently swaps A with B
    m_cleanupStack = std::move(newCleanupStack);
    m_idToIdxLookup = std::move(newIdToIdxLookup);

    // Update m_nextID to reflect the number of remaining valid tasks
    m_nextID = m_cleanupStack.size();

    m_invalidTaskCount = 0;

    size_t newSize = m_cleanupStack.size();
    if (newSize < oldSize) {
        Log::Print(Log::T_SUCCESS, __FUNCTION__, "Shrunk stack size from " + std::to_string(oldSize) + " down to " + std::to_string(newSize) + ".");
    }
    else {
        Log::Print(Log::T_INFO, __FUNCTION__, "Cleanup stack cannot be optimized further.");
    }
}
