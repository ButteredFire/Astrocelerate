
#pragma once

#include <Platform/External/GLFWVulkan.hpp>


namespace Buffer {

	/* Buffer Usage Enums */
	enum class MemIntent {
		VRAM,			// Device-local memory. Best for mostly/completely static data (e.g., vertex/index buffers, textures).
		VRAMReBAR,		// Device-local memory that the Host can access (ReBAR PCIe technology). Best for GPU data demanding high-frequency access (e.g., uniform buffers). For machines without ReBAR, the allocated memory defaults to System RAM (Host memory).
		RAM_SEQ_ACCESS,	// System memory optimized for sequential writes by the CPU, and reads by the GPU. Best for linear data updates (e.g., staging/constant buffers).
		RAM_RAND_ACCESS	// System memory optimized for random writes by the CPU, and reads by the GPU. Best for non-linear data updates (e.g., readback buffers).
	};


	/* Buffer Allocation Data */
	struct BufferAlloc {
		VkBuffer buffer;				// The VkBuffer handle
		VmaAllocation allocation;		// The VMA Buffer Allocation handle
		VkBufferUsageFlags usageFlags;	// The buffer usage flags
		VkDeviceSize size;				// The buffer size (in bytes)
		MemIntent memType;				// The type of memory allocated for the buffer
		void *mappedData = nullptr;		// The pointer to the allocated data (nullptr if allocated in VRAM)
		uint32_t resourceID;			// The buffer's resource ID (used in resource cleanup and dependency-linking)
	};
}