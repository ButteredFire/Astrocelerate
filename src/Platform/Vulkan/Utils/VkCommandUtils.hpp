/* VkCommandUtils - Command buffer utilities. */
#pragma once

#include <vector>
#include <optional>


#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/Vulkan/VkSyncManager.hpp>


// A structure that stores the configuration for single-use command buffers
struct SingleUseCommandBufferInfo {
	VkCommandPool commandPool;		// The command pool from which the command buffer is allocated.
	VkQueue queue;					// The queue to which the command buffer's recorded data is submitted (and for which the command pool is allocated).

	VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;							// The buffer level.
	VkCommandBufferUsageFlags bufferUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;   // The buffer usage flags.
	VkCommandBufferInheritanceInfo *pInheritanceInfo = nullptr;									// The pointer to the buffer's inheritance info

	VkFence fence = VK_NULL_HANDLE;
	std::vector<VkSemaphore> waitSemaphores = {};
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	std::vector<VkSemaphore> signalSemaphores = {};

	bool usingSingleUseFence = false;		// Is the fence being used (if it is) single-use?
	bool autoSubmit = true;					// Automatically submit after ending buffer recording?
	bool freeAfterSubmit = true;			// Automatically free the buffer after submitting?
};


namespace VkCommandUtils {
	/* Begins recording a single-use/anonymous command buffer for single-time commands.
		@param logicalDevice: The current logical device.
		@param commandBufInfo: The command buffer configuration.

		@return The command buffer in question.
	*/
	inline static VkCommandBuffer BeginSingleUseCommandBuffer(VkDevice logicalDevice, SingleUseCommandBufferInfo *commandBufInfo) {
		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.level = commandBufInfo->bufferLevel;
		cmdBufAllocInfo.commandPool = commandBufInfo->commandPool;
		cmdBufAllocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer;
		VkResult bufAllocResult = vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocInfo, &cmdBuffer);
		LOG_ASSERT(bufAllocResult == VK_SUCCESS, "Failed to allocate single-use command buffer!");

		VkCommandBufferBeginInfo cmdBufBeginInfo{};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = commandBufInfo->bufferUsageFlags;
		cmdBufBeginInfo.pInheritanceInfo = commandBufInfo->pInheritanceInfo;

		VkResult bufBeginResult = vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo);
		LOG_ASSERT(bufBeginResult == VK_SUCCESS, "Failed to start recording single-use command buffer!");

		return cmdBuffer;
	}


	/* Stops recording a single-use/anonymous command buffer and submit its data to the GPU.
		@param logicalDevice: The current logical device.
		@param commandBufInfo: The command buffer configuration.
		@param cmdBuffer: The command buffer.
	*/
	inline static void EndSingleUseCommandBuffer(VkDevice logicalDevice, SingleUseCommandBufferInfo *commandBufInfo, VkCommandBuffer &cmdBuffer) {
		VkResult bufEndResult = vkEndCommandBuffer(cmdBuffer);
		LOG_ASSERT(bufEndResult == VK_SUCCESS, "Failed to stop recording single-use command buffer!");


		if (!commandBufInfo->autoSubmit) {
			std::ostringstream stream;
			stream << static_cast<const void *>(&cmdBuffer);

			if (commandBufInfo->usingSingleUseFence && IN_DEBUG_MODE)
				// If fences are accidentally used
				Log::Print(Log::T_WARNING, __FUNCTION__, "Command buffer " + stream.str() + " is not auto-submitted, but uses a single-use fence! Please, depending on your use case, either enable auto-submission or remove the fence.");

			return;
		}


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		if (commandBufInfo->waitSemaphores.size() > 0) {
			submitInfo.waitSemaphoreCount = static_cast<uint32_t>(commandBufInfo->waitSemaphores.size());
			submitInfo.pWaitSemaphores = commandBufInfo->waitSemaphores.data();
			submitInfo.pWaitDstStageMask = &commandBufInfo->waitStageMask;
		}

		if (commandBufInfo->signalSemaphores.size() > 0) {
			submitInfo.signalSemaphoreCount = static_cast<uint32_t>(commandBufInfo->signalSemaphores.size());
			submitInfo.pSignalSemaphores = commandBufInfo->signalSemaphores.data();
		}

		VkResult submitResult = vkQueueSubmit(commandBufInfo->queue, 1, &submitInfo, commandBufInfo->fence);
		LOG_ASSERT(submitResult == VK_SUCCESS, "Failed to submit recorded data from single-use command buffer!");

		if (commandBufInfo->fence != VK_NULL_HANDLE) {
			if (commandBufInfo->usingSingleUseFence) {
				VkSyncManager::WaitForSingleUseFence(logicalDevice, commandBufInfo->fence);
			}
			else {
				vkWaitForFences(logicalDevice, 1, &commandBufInfo->fence, VK_TRUE, UINT64_MAX);
				vkResetFences(logicalDevice, 1, &commandBufInfo->fence);
			}
		}
		else
			vkDeviceWaitIdle(logicalDevice);

		if (commandBufInfo->freeAfterSubmit)
			vkFreeCommandBuffers(logicalDevice, commandBufInfo->commandPool, 1, &cmdBuffer);
	}


	/* Creates a command pool.
	
		SERVICE DEPENDENCIES: CleanupManager

		@param device: The logical device.
		@param queueFamilyIndex: The index of the queue family for which the command pool is to be created.
		@param flags: The command pool flags.
		@param parentResourceID (Default: nullopt): The ID of the parent resource that the command pool is a dependency of.

		@return The allocated command pool.
	*/
	inline static VkCommandPool CreateCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, std::optional<ResourceID> parentResourceID = std::nullopt) {
		std::shared_ptr<CleanupManager> cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);

		VkCommandPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.flags = flags; // Allows command buffers to be re-recorded individually

		// Command buffers are executed by submitting them on a device queue.
		// Each command pool can only allocate command buffers that are submitted on a single type of queue.
		poolCreateInfo.queueFamilyIndex = queueFamilyIndex;

		VkCommandPool commandPool;
		VkResult result = vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
		LOG_ASSERT(result == VK_SUCCESS, "Failed to create command pool!");

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(m_commandPool) };
		task.vkHandles = { commandPool };
		task.cleanupFunc = [device, commandPool]() { vkDestroyCommandPool(device, commandPool, nullptr); };

		ResourceID taskID = cleanupManager->createCleanupTask(task);
		if (parentResourceID.has_value())
			cleanupManager->addTaskDependency(taskID, parentResourceID.value());

		return commandPool;
	}


	/* Allocates a command buffer vector.
	
		SERVICE DEPENDENCIES: CleanupManager

		@param logicalDevice: The logical device.
		@param commandPool: The command pool from which the buffers will be allocated.
		@param commandBuffers: A vector of uninitialized command buffers, each of which will be allocated.
	*/
	inline void AllocCommandBuffers(VkDevice logicalDevice, VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers) {
		std::shared_ptr<CleanupManager> cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);


		commandBuffers.resize(SimulationConst::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo bufferAllocInfo{};
		bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocInfo.commandPool = commandPool;

		// Specifies whether the command buffer is primary or secondary
		// BUFFER_LEVEL_PRIMARY: The buffer can be submitted to a queue for execution, but cannot be directly called from other command buffers.
		// BUFFER_LEVEL_SECONDARY: The buffer cannot be submitted directly, but can be called from primary command buffers.
		bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		bufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(logicalDevice, &bufferAllocInfo, commandBuffers.data());
		LOG_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffers!");

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(commandBuffers) };
		task.vkHandles = { commandPool };
		task.cleanupFunc = [logicalDevice, commandPool, commandBuffers]() {
			vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		};

		cleanupManager->createCleanupTask(task);
	}
}