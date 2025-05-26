#include "GarbageCollector.hpp"

GarbageCollector::GarbageCollector(VulkanContext& context): 
	m_vkContext(context),
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
	task.vkObjects = { m_vmaAllocator };
	task.cleanupFunc = [this]() { vmaDestroyAllocator(m_vmaAllocator); };

	createCleanupTask(task);

	m_vkContext.vmaAllocator = m_vmaAllocator;
	return m_vmaAllocator;
}


uint32_t GarbageCollector::createCleanupTask(CleanupTask task) {
	// Constructs a string displaying the object name(s) for logging
	std::string objectNamesStr = enquote(getObjectNamesString(task));

	uint32_t id = m_nextID++;
	m_cleanupStack.push_back(task);
	m_idToIdxLookup[id] = (m_cleanupStack.size() - 1);

	Log::Print(Log::T_VERBOSE, task.caller.c_str(), "Pushed object(s) " + objectNamesStr + " to cleanup stack.");
	return id;
}


CleanupTask& GarbageCollector::modifyCleanupTask(uint32_t taskID) {
	try {
		return m_cleanupStack[m_idToIdxLookup.at(taskID)];
	}
	catch (const std::exception& e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Task ID " + enquote(std::to_string(taskID)) + " is invalid!\nOriginal exception message: " + std::string(e.what()));
	}
}


bool GarbageCollector::executeCleanupTask(uint32_t taskID) {
	try {
		CleanupTask& task = m_cleanupStack[m_idToIdxLookup.at(taskID)];
		return executeTask(task, taskID);
	}
	catch (const std::exception& e) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Task ID " + enquote(std::to_string(taskID)) + " is invalid!\nOriginal exception message: " + std::string(e.what()));
	}
}


void GarbageCollector::processCleanupStack() {
	optimizeStack();
	size_t stackSize = m_cleanupStack.size();

	std::string plural = (stackSize != 1)? "s" : "";
	Log::Print(Log::T_VERBOSE, __FUNCTION__, "Executing " + std::to_string(stackSize) + " task" + plural + " in the cleanup stack...");

	while (!m_cleanupStack.empty()) {
		uint32_t id = (m_cleanupStack.size() - 1);
		CleanupTask task = m_cleanupStack[id];
		executeTask(task, id);

		m_cleanupStack.pop_back();
	}
}


bool GarbageCollector::executeTask(CleanupTask& task, uint32_t taskID) {
	// Constructs a string displaying the object name(s) for logging
	std::string objectNamesStr = enquote(getObjectNamesString(task));


	// Checks whether the task is already invalid
	if (!task.validTask) {
		Log::Print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup task for object(s) " + objectNamesStr + ".");
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
		Log::Print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup task for object(s) " + objectNamesStr + " due to an invalid Vulkan object used in their destroy/free callback function.");
		return false;
	}


	// Executes the task and invalidates it to prevent future executions
	task.cleanupFunc();

	Log::Print(Log::T_VERBOSE, __FUNCTION__, "Executed cleanup task for object(s) " + objectNamesStr + ".");

	task.validTask = false;
	m_invalidTaskCount++;

	if (m_invalidTaskCount >= m_MAX_INVALID_TASKS) {
		optimizeStack();
	}

	return true;
}


void GarbageCollector::optimizeStack() {
	size_t displacement = 0;		 // Cumulative displacement
	size_t locDisplacement = 0;		 // Local displacement (used only for removing redundant key-value pairs in the ID-to-Index hashmap)
	size_t locInvalidTaskCount = 0;	 // Local invalid task count (differs from GarbageCollector::invalidTaskCount because this keeps track of all invalid tasks, while the member variable only serves to trigger a call to optimizeStack on exceeding the maximum constant)
	size_t oldSize = m_cleanupStack.size();

	for (size_t i = 0; i < oldSize; i++) {
		if (!m_cleanupStack[i].validTask) {
			displacement++;
			locDisplacement++;
			locInvalidTaskCount++;
		}
		else {
			m_idToIdxLookup[i] -= displacement;
			m_cleanupStack[i - displacement] = m_cleanupStack[i];

			while (locDisplacement > 0) {
				m_idToIdxLookup.erase(i - locDisplacement);
				locDisplacement--;
			}
		}
	}

	m_nextID = (oldSize - locInvalidTaskCount);

	// Resizing down `invalidTaskCount` elements effectively discards tasks whose indices are greater than (size - locInvalidTaskCount)
	m_cleanupStack.resize(m_nextID);

	m_invalidTaskCount = 0;

	size_t newSize = m_cleanupStack.size();
	if (newSize < oldSize)
		Log::Print(Log::T_SUCCESS, __FUNCTION__, "Shrunk stack size from " + std::to_string(oldSize) + " down to " + std::to_string(newSize) + ".");

	else
		Log::Print(Log::T_INFO, __FUNCTION__, "Cleanup stack cannot be optimized further.");
}
