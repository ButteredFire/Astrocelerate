/* VkSyncManager.cpp - Synchronization manager implementation.
*/

#include "VkSyncManager.hpp"


VkSyncManager::VkSyncManager(VulkanContext& context):
	vkContext(context) {

	garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
};


VkSyncManager::~VkSyncManager() {};


void VkSyncManager::init() {
	createSyncObjects();
}


void VkSyncManager::createSyncObjects() {
	/* A note on synchronization:
	* - Since the GPU executes commands in parallel, and since each step in the frame-rendering process depends on the previous step (and the completion thereof), we must explicitly define an order of operations to prevent these steps from being executed concurrently (which results in unintended/undefined behavior). To that effect, we may use various synchronization primitives:
	*
	* - Semaphores: Used to synchronize queue operations within the GPU.
	* - Fences: Used to synchronize the CPU with the GPU.
	*
	* Detailed explanation:
	*
	* 1) SEMAPHORES
		* The semaphore is used to add order between queue operations (which is the work we submit to a queue (e.g., graphics/presentation queue), either in a command buffer or from within a function).
		* Semaphores are both used to order work either within the same queue or between different queues.
		*
		* There are two types of semaphores: binary, and timeline. We will be using binary semaphores.
			- The binary semaphore has a binary state: signaled OR unsignaled. On initialization, it is unsignaled. The binary semaphore can be used to order operations like this: To order 2 operations `opA` and `opB`, we configure `opA` so that it signals the binary semaphore on completion, and configure `opB` so that it only begins when the binary semaphore is signaled (i.e., `opB` will "wait" for that semaphore). After `opB` begins executing, the binary semaphore is reset to unsignaled to allow for future reuse.
		*
	* 2) FENCES
		* The fence, like the semaphore, is used to synchronize execution, but it is for the CPU (a.k.a., the "host").
		* We use the fence in cases where the host needs to know when the GPU has finished something.
		*
		* The fence, like the binary semaphore, is either in a signaled OR unsignaled state. When we want to execute something (i.e., a command buffer), we attach a fence to it and configure the fence to be signaled on its completion. Then, we must make the host wait for the fence to be signaled (i.e., halt/block any execution on the CPU until the fence is signaled) to guarantee that the work is completed before the host continues.
		*
		* In general, it is preferable to not block the host unless necessary. We want to feed the GPU and the host with useful work to do. Waiting on fences to signal is not useful work. Thus we prefer semaphores, or other synchronization primitives not yet covered, to synchronize our work. However, certain operations require the host to wait (e.g., rendering/drawing a frame, to ensure that the CPU waits until the GPU has finished processing the previous frame before starting to process the next one).
		*
		* Fences must be reset manually to put them back into the unsignaled state. This is because fences are used to control the execution of the host, and so the host gets to decide when to reset the fence. Contrast this to semaphores which are used to order work on the GPU without the host being involved.
	*/
	imageReadySemaphores.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	// Specifies the fence to be created already in the signal state.
	// We need to do this, because if the fence is created unsignaled (default), when we call drawFrame() in the Renderer for the first time, `vkWaitForFences` will wait for the fence to be signaled, only to wait indefinitely because the fence can only be signaled after a frame has finished rendering, and we are calling drawFrames() for the first time so there is no frame initially.
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		VkResult imageSemaphoreCreateResult = vkCreateSemaphore(vkContext.logicalDevice, &semaphoreCreateInfo, nullptr, &imageReadySemaphores[i]);
		VkResult renderSemaphoreCreateResult = vkCreateSemaphore(vkContext.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
		VkResult inFlightFenceCreateResult = vkCreateFence(vkContext.logicalDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]);

		if (imageSemaphoreCreateResult != VK_SUCCESS || renderSemaphoreCreateResult != VK_SUCCESS) {
			throw Log::RuntimeException(__FUNCTION__, "Failed to create semaphores for a frame!");
		}

		if (inFlightFenceCreateResult != VK_SUCCESS) {
			throw Log::RuntimeException(__FUNCTION__, "Failed to create in-flight fence for a frame!");
		}

		VkSemaphore imageReadySemaphore = imageReadySemaphores[i];
		VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores[i];
		VkFence inFlightFence = inFlightFences[i];

		CleanupTask syncObjTask{};
		syncObjTask.caller = __FUNCTION__;
		syncObjTask.objectNames = { VARIABLE_NAME(imageReadySemaphore), VARIABLE_NAME(renderFinishedSemaphore), VARIABLE_NAME(inFlightFence) };
		syncObjTask.vkObjects = { vkContext.logicalDevice, imageReadySemaphore, renderFinishedSemaphore, inFlightFence };
		syncObjTask.cleanupFunc = [this, imageReadySemaphore, renderFinishedSemaphore, inFlightFence]() {
			vkDestroySemaphore(vkContext.logicalDevice, imageReadySemaphore, nullptr);
			vkDestroySemaphore(vkContext.logicalDevice, renderFinishedSemaphore, nullptr);
			vkDestroyFence(vkContext.logicalDevice, inFlightFence, nullptr); 
		};

		garbageCollector->createCleanupTask(syncObjTask);
	}

	vkContext.SyncObjects.imageReadySemaphores = imageReadySemaphores;
	vkContext.SyncObjects.renderFinishedSemaphores = renderFinishedSemaphores;
	vkContext.SyncObjects.inFlightFences = inFlightFences;
}