/* VkBufferUtils - Manages buffer creation and modification.
*/
#pragma once

#include <memory>
#include <vector>


#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/Data/Device.hpp>
#include <Platform/Vulkan/Utils/VkCommandUtils.hpp>
#include <Platform/External/GLFWVulkan.hpp>


namespace VkBufferUtils {
    /* Creates a buffer.
		@param renderDevice: The current render device context.
        @param &buffer: The buffer to be created.
        @param bufferSize: The size of the buffer (in bytes).
        @param usageFlags: Flags specifying how the buffer will be used.
        @param bufferAllocation: The memory block allocated for the buffer.
        @param bufferAllocationCreateInfo: The buffer allocation create info struct.

        @return The cleanup task ID for the newly created buffer.
    */
    inline ResourceID CreateBuffer(const Ctx::VkRenderDevice *renderDevice, VkBuffer &buffer, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VmaAllocation &bufferAllocation, VmaAllocationCreateInfo bufferAllocationCreateInfo) {
		std::shared_ptr<CleanupManager> cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);

		// Creates the buffer
		VkBufferCreateInfo bufCreateInfo{};
		bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufCreateInfo.size = bufferSize;

		// Specifies the purpose of the buffer (It is possible to specify multiple purposes using a bitwise OR)
		bufCreateInfo.usage = usageFlags;

		// Buffers can either be owned by a specific queue family or be shared between multiple queue families.
		QueueFamilyIndices familyIndices = renderDevice->queueFamilies;
		std::vector<uint32_t> queueFamilyIndices = {
			familyIndices.graphicsFamily.index.value()
		};

		if (familyIndices.familyExists(familyIndices.transferFamily)) {
			bufCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			queueFamilyIndices.push_back(familyIndices.transferFamily.index.value());
		}
		else
			bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		bufCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
		bufCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

		// Configures sparse buffer memory (which is irrelevant right now, so we'll leave it at the default value of 0)
		bufCreateInfo.flags = 0;


		VkResult bufCreateResult = vmaCreateBuffer(renderDevice->vmaAllocator, &bufCreateInfo, &bufferAllocationCreateInfo, &buffer, &bufferAllocation, nullptr);
		LOG_ASSERT(bufCreateResult == VK_SUCCESS, "Failed to create buffer!");


		CleanupTask bufTask{};
		bufTask.caller = __FUNCTION__;
		bufTask.objectNames = { VARIABLE_NAME(buffer) };
		bufTask.vkHandles = { buffer, bufferAllocation };
		bufTask.cleanupFunc = [renderDevice, buffer, bufferAllocation]() { vmaDestroyBuffer(renderDevice->vmaAllocator, buffer, bufferAllocation); };

		ResourceID bufferTaskID = cleanupManager->createCleanupTask(bufTask);

		return bufferTaskID;
    }


	/* Copies the contents from a source buffer to a destination buffer.
		@param renderDevice: The current render device context.
		@param srcBuffer: The source buffer that stores the contents to be transferred.
		@param dstBuffer: The destination buffer to receive the contents from the source buffer.
		@param deviceSize: The size of either the source or destination buffer (in bytes).
	*/
	inline void CopyBuffer(const Ctx::VkRenderDevice *renderDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize) {
		// Uses the transfer queue by default, but if it does not exist, switch to the graphics queue
		QueueFamilyIndices queueFamilies = renderDevice->queueFamilies;
		QueueFamilyIndices::QueueFamily selectedFamily = queueFamilies.transferFamily;

		if (!queueFamilies.familyExists(selectedFamily)) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Transfer queue family is not valid. Switching to graphics queue family...");
			selectedFamily = queueFamilies.graphicsFamily;
		}


		// Begins recording a command buffer to send data to the GPU
		SingleUseCommandBufferInfo cmdBufInfo{};
		cmdBufInfo.commandPool = VkCommandUtils::CreateCommandPool(renderDevice->logicalDevice, selectedFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		cmdBufInfo.fence = VkSyncManager::CreateSingleUseFence(renderDevice->logicalDevice);
		cmdBufInfo.usingSingleUseFence = true;
		cmdBufInfo.queue = selectedFamily.deviceQueue;

		VkCommandBuffer commandBuffer = VkCommandUtils::BeginSingleUseCommandBuffer(renderDevice->logicalDevice, &cmdBufInfo);


		// Copies the data
		VkBufferCopy copyRegion{};
		// Sets the source and destination buffer offsets to 0 (since we want to copy everything, not just a portion)
		copyRegion.srcOffset = copyRegion.dstOffset = 0;
		copyRegion.size = deviceSize;
		// We can actually transfer multiple regions (that is why there is `regionCount` - to specify how many regions to transfer). To copy multiple regions, we can pass in a region array for `pRegions`, and its size for `regionCount`.
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);


		// Stops recording the command buffer and submits recorded data to the GPU
		VkCommandUtils::EndSingleUseCommandBuffer(renderDevice->logicalDevice, &cmdBufInfo, commandBuffer);
	}


	/* Writes data to a buffer that is allocated in GPU (device-local) memory.
		@param renderDevice: The current render device context.
		@param data: The data to write to the buffer.
		@param buffer: The device-local buffer that the data will be written to.
		@param bufferSize: The size of the buffer (in bytes).
	*/
	inline void WriteToGPUBuffer(const Ctx::VkRenderDevice *renderDevice, const void *data, VkBuffer &buffer, VkDeviceSize bufferSize) {
		/* How data is written into a device-local-memory allocated buffer:
		*
		* - We want the CPU to access and write data to a buffer in GPU memory (or device-local memory). It is in GPU memory because its memory usage flag is `VMA_MEMORY_USAGE_GPU_ONLY`. However, such buffers are not always directly accessible from the CPU.
		*
		* - Therefore, we will need to use a third buffer: the staging buffer. It acts as a medium through which the CPU can write data into GPU-memory-allocated buffers. The staging buffer is specified to be created in CPU memory (or host-visible memory) via the memory usage flag `VMA_MEMORY_USAGE_CPU_ONLY`.
		*
		* - STEP 1: Allocate a staging buffer in host-visible memory.
		* - STEP 2: Copy/Load the data onto the staging buffer.
		*	+ STEP 2.1: Map the staging buffer's memory block onto the CPU address space so that the CPU can access and write data to it
		*	+ STEP 2.2: Copy the data to the memory block via `memcpy` (which is a CPU operation)
		*	+ STEP 2.3: Unmaps the staging buffer's memory block from the CPU address space to ensure the CPU can no longer access it
		*
		* - STEP 3: Copy the data from the staging buffer to the destination/target buffer. This has already been handled in `copyBuffer`.
		*
		* NOTE: `VMA_MEMORY_USAGE_CPU_ONLY` and `VMA_MEMORY_USAGE_GPU_ONLY` are deprecated.
		* Use `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` and `VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE` respectively.
		*/
		std::shared_ptr<CleanupManager> cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);


		// Creates a staging buffer
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufAllocation;

		VkBufferUsageFlags stagingBufUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo stagingBufAllocInfo{};
		stagingBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;				// Use VMA_MEMORY_USAGE_AUTO for general usage
		stagingBufAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;	// Host-visible memory


		/*
		Since the staging buffer's allocation is going to be mapped to CPU memory below (vmaMapMemory), we must specify the expected patern of CPU memory access.

		[VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT]
		This flag indicates that the host will access the memory in a sequential write pattern. This is typically used when the host writes data to the memory in a linear order, such as when uploading a large block of data to a buffer.

		[VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT]
		This flag indicates that the host will access the memory in a random access pattern. This is typically used when the host reads or writes data to the memory in a non-linear order, such as when updating individual elements in a buffer.
		*/
		stagingBufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		uint32_t stagingBufTaskID = CreateBuffer(renderDevice, stagingBuffer, bufferSize, stagingBufUsage, stagingBufAllocation, stagingBufAllocInfo);

		// Copies data to the staging buffer
		void *mappedData;
		vmaMapMemory(renderDevice->vmaAllocator, stagingBufAllocation, &mappedData);	// Maps the buffer memory into CPU-accessible memory so that we can write data to it
			memcpy(mappedData, data, static_cast<size_t>(bufferSize));					// Copies the data to the mapped buffer memory
		vmaUnmapMemory(renderDevice->vmaAllocator, stagingBufAllocation);				// Unmaps the mapped buffer memory


		// Copies the contents from the staging buffer to the destination buffer
		CopyBuffer(renderDevice, stagingBuffer, buffer, bufferSize);


		// The staging buffer has done its job, so we can safely destroy it afterwards
		cleanupManager->executeCleanupTask(stagingBufTaskID);
	}


	/* Finds the memory type suitable for buffer and application requirements.
        GPUs offer different types of memory to allocate from, each differs in allowed operations and performance characteristics.

		@param physicalDevice: The physical device handle.
	    @param typeFilter: A bitfield specifying the memory types that are suitable.
	    @param properties: The properties of the memory.

	    @return The index of the sutiable memory type.
    */
	inline uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		// Queries info about available memory types on the GPU
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		/* The VkPhysicalDeviceMemoryProperties struct has two arrays:
		* - memoryHeaps: An array of structures, each describing a memory heap (i.e., distinct memory resources, e.g., VRAM, RAM) from which memory can be allocated. This is useful if we want to know what heap a memory type comes from.
		* - memoryTypes: An array of structures, each describing a memory type that can be used with a given memory heap in memoryHeaps.
		*/
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			// Checks if the memory type is suitable for the buffer (i.e., if its bit in the typeFilter bitfield is 1)
			const bool MEMTYPE_SUITABLE = (typeFilter & (1 << i));

			// Checks if the memory type supports all features specified in the `properties` bitfield
			const bool FEATURE_SUPPORTED = ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties);

			if (MEMTYPE_SUITABLE && FEATURE_SUPPORTED) {
				return i;
			}
		}

		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to find suitable memory type!");
	}
}
