#include "BufferManager.hpp"

BufferManager::BufferManager(VulkanContext& context, bool autoCleanup):
	vkContext(context), cleanOnDestruction(autoCleanup) {

	Log::print(Log::INFO, __FUNCTION__, "Initializing...");
}

BufferManager::~BufferManager() {
	if (cleanOnDestruction)
		cleanup();
}


void BufferManager::init() {
	loadVertexBuffer();
}


void BufferManager::cleanup() {
	Log::print(Log::INFO, __FUNCTION__, "Cleaning up...");

	if (vkIsValid(vertexBuffer))
		vkDestroyBuffer(vkContext.logicalDevice, vertexBuffer, nullptr);

	if (vkIsValid(vertexBufferMemory))
		vkFreeMemory(vkContext.logicalDevice, vertexBufferMemory, nullptr);
}


VkVertexInputBindingDescription BufferManager::getBindingDescription() {
	// A vertex binding describes at which rate to load data from memory throughout the vertices.
		// It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
	VkVertexInputBindingDescription bindingDescription{};

	// Our data is currently packed together in 1 array, so we're only going to have one binding (whose index is 0).
	bindingDescription.binding = 0;

	// Specifies the number of bytes from one entry to the next (i.e., byte stride between consecutive elements in a buffer)
	bindingDescription.stride = sizeof(Vertex);

	// Specifies how to move to the next entry
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next entry after each vertex (for per-vertex data); for instanced rendering, use INPUT_RATE_INSTANCE

	return bindingDescription;
}


std::array<VkVertexInputAttributeDescription, 2> BufferManager::getAttributeDescriptions() {
	// Attribute descriptions specify the type of the attributes passed to the vertex shader, which binding to load them from (and at which offset)
		// Each vertex attribute (e.g., position, color) must have its own attribute description.
	std::array<VkVertexInputAttributeDescription, 2> attribDescriptions{};

	// Attribute: Position
	attribDescriptions[0].binding = 0;
	attribDescriptions[0].location = 0;
	attribDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // R32G32 because `position` is a vec2. If it were a vec3, its format would need to have 3 channels, e.g., R32G32B32_SFLOAT
	attribDescriptions[0].offset = offsetof(Vertex, position);

	// Attribute: Color
	attribDescriptions[1].binding = 0;
	attribDescriptions[1].location = 1;
	attribDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribDescriptions[1].offset = offsetof(Vertex, color);


	return attribDescriptions;
}


void BufferManager::createBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	/* A note on buffer memory allocation:
	* It should be noted that in a real world application, you’re not supposed to actually call vkAllocateMemory for every individual buffer. The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080. The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a single allocation among many different objects by using the offset parameters that we’ve seen in many functions.
	*/


	// Creates the buffer
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

		// Specifies the size of the buffer (in bytes)
	bufCreateInfo.size = deviceSize;

		// Specifies the purpose of the buffer (It is possible to specify multiple purposes using a bitwise OR)
	bufCreateInfo.usage = usageFlags;

		// Buffers can either be owned by a specific queue family or be shared between multiple queue families.
	bufCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

		// If the sharing mode is CONCURRENT, we must specify queue families
	QueueFamilyIndices familyIndices = VkDeviceManager::getQueueFamilies(vkContext.physicalDevice, vkContext.vkSurface);
	uint32_t queueFamilyIndices[] = {
		familyIndices.graphicsFamily.index.value(),
		familyIndices.transferFamily.index.value()
	};
	bufCreateInfo.queueFamilyIndexCount = 2;
	bufCreateInfo.pQueueFamilyIndices = queueFamilyIndices;

		// Configures sparse buffer memory (which is irrelevant right now, so we'll leave it at the default value of 0)
	bufCreateInfo.flags = 0;

	VkResult bufCreateResult = vkCreateBuffer(vkContext.logicalDevice, &bufCreateInfo, nullptr, &buffer);
	if (bufCreateResult != VK_SUCCESS) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Failed to create buffer!");
	}


	// Allocates memory for the buffer
	
		// Queries the buffer's memory requirements
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vkContext.logicalDevice, buffer, &memoryRequirements);

		// Allocates memory for the buffer
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

	VkResult memAllocResult = vkAllocateMemory(vkContext.logicalDevice, &allocInfo, nullptr, &bufferMemory);
	if (memAllocResult != VK_SUCCESS) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Failed to allocate memory for the buffer!");
	}

		// Binds the buffer memory to the newly allocated memory
			/* Memory offset is essentially the "distance" between the starting point of the allocated memory block and that of the buffer.
			* In other words, it specifies how far into the memory block the buffer's memory starts.
			* In our case, we just have a single buffer with a single memory block allocated specifically for it, so the offset betwen them is 0.
			*
			* However, if we have multiple buffers that share the same memory allocation, we must set the offset betwen the memory allocation and each buffer. Note that a valid non-zero offset must be divisible by `memoryRequirements.alignment`.
			*/
	vkBindBufferMemory(vkContext.logicalDevice, buffer, bufferMemory, 0);
}


void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize) {
	// Allocates a temporary command buffer to perform memory transfer operations on
	QueueFamilyIndices familyIndices = vkContext.queueFamilies;

	// Uses the transfer queue by default, but if it does not exist, switch to the graphics queue
	QueueFamilyIndices::QueueFamily queueFamily = familyIndices.transferFamily;
	if (queueFamily.deviceQueue == VK_NULL_HANDLE || !queueFamily.index.has_value()) {
		Log::print(Log::WARNING, __FUNCTION__, "Transfer queue family is not valid. Switching to graphics queue family...");
		queueFamily = familyIndices.graphicsFamily;
	}


	VkCommandPool commandPool = RenderPipeline::createCommandPool(vkContext.logicalDevice, queueFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

	VkCommandBufferAllocateInfo bufAllocInfo{};
	bufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	bufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	bufAllocInfo.commandPool = commandPool;
	bufAllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vkContext.logicalDevice, &bufAllocInfo, &commandBuffer);

	// Start recording the command buffer
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Use the command buffer once

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	// Copies the data
	VkBufferCopy copyRegion{};
		// Sets the source and destination buffer offsets to 0 (since we want to copy everything, not just a portion)
	copyRegion.srcOffset = copyRegion.dstOffset = 0;
	copyRegion.size = deviceSize;
		// We can actually transfer multiple regions (that is why there is `regionCount` - to specify how many regions to transfer). To copy multiple regions, we can pass in a region array for `pRegions`, and its size for `regionCount`.
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	// Stops recording
	vkEndCommandBuffer(commandBuffer);


	// Executes the command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queueFamily.deviceQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queueFamily.deviceQueue);

	// Free command buffer
	vkFreeCommandBuffers(vkContext.logicalDevice, commandPool, 1, &commandBuffer);
}


void BufferManager::loadVertexBuffer() {
	/* NOTE: The driver may not immediately copy the data into the buffer memory due to various reasons, like caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. 
	* There are two ways to deal with that problem:
	* - Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT (the method we currently use; see allocBufferMemory)
	* - Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
	*/

	/* NOTE on buffer usage flags: VK_BUFFER_USAGE_...
	* - ...TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
	* - ...TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
	*/


	// Creates a staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufMemory;

	VkDeviceSize bufferSize = (sizeof(vertices[0]) * vertices.size());
	VkBufferUsageFlags stagingBufUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// HOST_COHERENT_BIT ensures that the contents of the mapped buffer memory are coherent/matching with those of the allocated memory
	VkMemoryPropertyFlags stagingBufPropertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	createBuffer(bufferSize, stagingBufUsage, stagingBufPropertyFlags, stagingBuffer, stagingBufMemory);


	// Maps the buffer memory into CPU-accessible memory so that we can write data to it
	void* data;
		// vkMapMemory lets us access a specified memory resource defined by an offset and size (size <= VK_WHOLE_SIZE, a special value used to map all of the memory)
	vkMapMemory(vkContext.logicalDevice, stagingBufMemory, 0, bufferSize, 0, &data);

	// Copies the vertex data to the mapped buffer memory
		// Note: we already defined `bufCreateInfo.size` in `createVertexBuffer`, so we can use that instead of `sizeof(vertices[0]) * vertices.size())`
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));

	// Unmaps the mapped buffer memory
	vkUnmapMemory(vkContext.logicalDevice, stagingBufMemory);


	// Creates the actual vertex buffer
	VkBufferUsageFlags vertBufUsage = (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	VkMemoryPropertyFlags vertBufPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	createBuffer(bufferSize, vertBufUsage, vertBufPropertyFlags, vertexBuffer, vertexBufferMemory);

	// Copies the contents from the staging buffer to the vertex buffer
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(vkContext.logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkContext.logicalDevice, stagingBufMemory, nullptr);
}


uint32_t BufferManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	// Queries info about available memory types on the GPU
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(vkContext.physicalDevice, &memoryProperties);
	
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

	throw Log::RuntimeException(__FUNCTION__, "Failed to find suitable memory type!");
}
