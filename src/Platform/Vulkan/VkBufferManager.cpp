#include "VkBufferManager.hpp"

VkBufferManager::VkBufferManager(const Ctx::VkRenderDevice *renderDeviceCtx) :
	m_renderDeviceCtx(renderDeviceCtx) {

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkBufferManager::~VkBufferManager() {}


Buffer::BufferAlloc VkBufferManager::allocate(VkDeviceSize bufSize, VkBufferUsageFlags usage, Buffer::MemIntent memUsage) {
	LOG_ASSERT(bufSize > 0, "Cannot allocate an empty (zero-byte) buffer!");

	Buffer::BufferAlloc bufferAlloc{};
	bufferAlloc.usageFlags = usage;
	bufferAlloc.size = bufSize;
	bufferAlloc.memType = memUsage;


	// Configure memory allocation
	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	{
		using enum Buffer::MemIntent;

		switch (memUsage) {
		case VRAM:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			break;

		case VRAMReBAR:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
							  VMA_ALLOCATION_CREATE_MAPPED_BIT;  // Auto-maps allocated device memory to host memory without manual vmaMapMemory/vmaUnmapMemory
			break;

		case RAM_SEQ_ACCESS:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.requiredFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;

		case RAM_RAND_ACCESS:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		}
	}


	// Create buffer
	ResourceID bufResource = VkBufferUtils::CreateBuffer(
		m_renderDeviceCtx,
		bufferAlloc.buffer, bufferAlloc.size, bufferAlloc.usageFlags,
		bufferAlloc.allocation, allocInfo
	);

	bufferAlloc.resourceID = bufResource;


	// Get persistent pointer
	VmaAllocationInfo info;
	vmaGetAllocationInfo(m_renderDeviceCtx->vmaAllocator, bufferAlloc.allocation, &info);
	bufferAlloc.mappedData = info.pMappedData;

	return bufferAlloc;
}
