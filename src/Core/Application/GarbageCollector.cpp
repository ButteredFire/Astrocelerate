#include "GarbageCollector.hpp"

GarbageCollector::GarbageCollector() :
	m_nextID(0) {

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

GarbageCollector::~GarbageCollector() {}


VmaAllocator GarbageCollector::createVMAllocator(VkInstance &instance, VkPhysicalDevice &physicalDevice, VkDevice &device) {
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

	return m_vmaAllocator;
}


CleanupID GarbageCollector::createCleanupTask(CleanupTask task) {
	//LOG_ASSERT(m_currentNodeID.has_value(),
	//	"Cannot create cleanup task: Root task has not been set! This should not happen, and may indicate a bug.");

	std::lock_guard<std::recursive_mutex> lock(m_cleanupMutex);

	std::optional<NodeID> parentingID = (m_rootNodeID.has_value()) ? m_rootNodeID : m_currentNodeID;

	task._id = m_nextID++;
	m_currentNodeID = m_taskTree.addNode(task, parentingID);

	Log::Print(Log::T_VERBOSE, task.caller.c_str(), "Pushed " + getObjectNamesString(task) + " to cleanup tree.");

	return task._id;
}


CleanupID GarbageCollector::createRootCleanupTask(CleanupTask task) {
	m_rootNodeID = createCleanupTask(task);

	return m_rootNodeID.value();
}


void GarbageCollector::addTaskDependency(CleanupID childTaskID, CleanupID parentTaskID) {
	m_taskTree.attachNodeToParent(childTaskID, parentTaskID);
}


CleanupTask &GarbageCollector::modifyCleanupTask(CleanupID taskID) {
	std::lock_guard<std::recursive_mutex> lock(m_cleanupMutex);
	return m_taskTree.getNode(taskID).data;
}


void GarbageCollector::executeCleanupTask(CleanupID taskID) {
	executeTask(taskID);
}


void GarbageCollector::processCleanupStack() {
	size_t stackSize = m_taskTree.size();

	Log::Print(Log::T_VERBOSE, __FUNCTION__, "Executing " + std::to_string(stackSize) + " " + PLURAL(stackSize, "task", "tasks") + " in the cleanup tree...");

	executeTask(m_rootNodeID.value());
}


void GarbageCollector::executeTask(CleanupID taskID) {
	/* Executes a cleanup task in isolation. */
	static const std::function<void(CleanupTask &)> execute = [this](CleanupTask &task) {
		
	};


	std::lock_guard<std::recursive_mutex> lock(m_cleanupMutex);

	// To safely do clean ups, we must execute tasks from children to parent.
	CleanupTask &task = m_taskTree.getNode(taskID).data;
	auto tasks = m_taskTree.getNodes(task._id);

	for (int i = tasks.size() - 1; i >= 0; i--)
		for (int j = tasks[i].size() - 1; j >= 0; j--)
			execute(tasks[i][j]->data);
}


std::string GarbageCollector::getObjectNamesString(CleanupTask &task) {
	// Prints the address of a Vulkan handle
	static auto printHandleTrueValue = [](const auto &handle) {
		std::stringstream ss;
		ss << std::showbase << std::hex << typeid(handle).name() << " " << (uint64_t)(handle);

		return ss.str();
	};
	

	std::stringstream ss;

	// Representative object names
	if (!task.objectNames.empty()) {
		ss << "(" << task.objectNames[0];
		for (size_t i = 1; i < task.objectNames.size(); i++)
			ss << ", " << task.objectNames[i];
		ss << ")";
	}


	if (!task.vkHandles.empty()) {
		//ss << " (vkHandle" << PLURAL(task.vkHandles.size(), "", "s") << " ";
		
		// We must use std::visit to determine the actual type of any task.vkHandles[i] handle, which is a variant.
		// Determining the actual handle type is crucial for casting the handle to a uint64_t to get its true hex value (e.g., 0x590000000059)
		ss << " (" << std::visit(printHandleTrueValue, task.vkHandles[0]);

		for (size_t i = 1; i < task.vkHandles.size(); i++) {
			ss << ", " << std::visit(printHandleTrueValue, task.vkHandles[i]);
		}

		ss << ")";
	}


	return ss.str();
}
