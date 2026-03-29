#include "CleanupManager.hpp"


CleanupManager::CleanupManager() {

	m_treeResourceGroup = m_taskTree.addNode(CleanupTask{});

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


CleanupManager::~CleanupManager() {}


VmaAllocator CleanupManager::createVMAllocator(VkInstance &instance, VkPhysicalDevice &physicalDevice, VkDevice &device) {
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


ResourceID CleanupManager::createCleanupTask(CleanupTask task, std::optional<ResourceID> parentTaskID) {
	ResourceID taskID = createTask(task, parentTaskID);
	Log::Print(Log::T_VERBOSE, task.caller.c_str(), "Pushed " + getObjectNamesString(task) + " to cleanup tree.");

	return taskID;
}


ResourceID CleanupManager::createCleanupGroup(std::optional<ResourceID> parentTaskID) {
	ResourceID taskID = createTask(CleanupTask{}, parentTaskID);
	m_taskMetadata[taskID].isGroup = true;

	return taskID;
}


ResourceID CleanupManager::createRootCleanupTask(CleanupTask task) {
	if (m_rootNodeID.has_value()) {
		Log::Print(Log::T_WARNING, __FUNCTION__, "Cannot make cleanup task the root node: it can only be set once.");
		return createCleanupTask(task);
	}

	m_rootNodeID = createCleanupTask(task);

	return m_rootNodeID.value();
}


ResourceID CleanupManager::createTask(CleanupTask task, std::optional<ResourceID> parentTaskID) {
	std::lock_guard<std::recursive_mutex> lock(m_cleanupMutex);

	std::optional<NodeID> parentingID = (m_rootNodeID.has_value()) ? m_rootNodeID : m_treeResourceGroup;

	NodeID taskID = m_taskTree.addNode(task, parentingID);
	m_taskMetadata[taskID] = _TaskMetadata{};

	if (parentTaskID.has_value())
		addTaskDependency(taskID, parentTaskID.value());

	return taskID;
}


void CleanupManager::addTaskDependency(ResourceID childTaskID, ResourceID parentTaskID) {
	m_taskTree.attachNodeToParent(childTaskID, parentTaskID);
}


CleanupTask &CleanupManager::modifyCleanupTask(ResourceID taskID) {
	std::lock_guard<std::recursive_mutex> lock(m_cleanupMutex);
	return m_taskTree.getNode(taskID).data;
}


void CleanupManager::executeCleanupTask(ResourceID taskID, bool executeParent) {
	executeTask(taskID, executeParent);
}


void CleanupManager::cleanupAll() {
	size_t stackSize = m_taskTree.size();

	Log::Print(Log::T_INFO, __FUNCTION__, std::to_string(stackSize) + " task " + PLURAL(stackSize, "node", "nodes") + " total. Cleaning up...");

	executeTask(m_treeResourceGroup, true);
}


void CleanupManager::executeTask(ResourceID taskID, bool executeParent) {
	std::lock_guard<std::recursive_mutex> lock(m_cleanupMutex);


	// To safely do clean ups, we must execute tasks from children to parent.
	auto tasks = m_taskTree.getNodes(taskID);

	int highestLevel = (executeParent) ? 0 : 1;

		// Execute tasks in depth level order (lowest to highest), then chronological order (latest to oldest)
	for (int i = tasks.size() - 1; i >= highestLevel; i--)
		for (int j = tasks[i].size() - 1; j >= 0; j--) {
			ResourceID taskID = tasks[i][j]->id;
			CleanupTask &task = tasks[i][j]->data;


			// Skip task if it is actually a cleanup group/virtual node
			if (m_taskMetadata[taskID].isGroup)
				continue;


			std::string objectNamesStr = getObjectNamesString(task);

			// Checks whether the task is already invalid
			if (!m_taskMetadata[taskID].isValid) {
				Log::Print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup of " + objectNamesStr + ".");
				return;
			}

			// Checks the validity of all Vulkan objects involved in the task
			bool proceedCleanup = true;
			for (const auto &object : task.vkHandles) {
				if (!vkIsValid(object)) {
					proceedCleanup = false;
					break;
				}
			}

			if (!task.cleanupConditions.empty()) {
				for (const auto &cond : task.cleanupConditions) {
					if (!cond) {
						proceedCleanup = false;
						break;
					}
				}
			}

			if (!proceedCleanup) {
				Log::Print(Log::T_WARNING, __FUNCTION__, "Skipped cleanup of " + objectNamesStr + " due to an invalid Vulkan object used in their destroy/free callback function.");
				return;
			}

			// Executes the task and invalidates it to prevent future executions
			try {
				task.cleanupFunc();
			}
			catch (const std::bad_function_call &e) {
				Log::Print(Log::T_ERROR, __FUNCTION__, "Cannot clean up " + objectNamesStr + ": Bad function call!");
			}
#ifdef NDEBUG
			catch (...) {
				Log::Print(Log::T_ERROR, __FUNCTION__, "An unknown exception prevented cleanup task " + objectNamesStr + " from being executed!");
			}
#endif

			Log::Print(Log::T_VERBOSE, __FUNCTION__, "Executed cleanup task for " + objectNamesStr + ".");

			m_taskMetadata[taskID].isValid = false;
		}
}


std::string CleanupManager::getObjectNamesString(CleanupTask &task) {
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
	}
	else
		ss << "(UNKNOWN_TASK";

	if (!task.caller.empty())
		ss << " @ " << task.caller;
	else
		ss << " @ UNKNOWN_CALLER";

	ss << ")";


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
