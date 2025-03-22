#include "VertexBuffer.hpp"

VertexBuffer::VertexBuffer(VulkanContext& context):
	vkContext(context) {}

VertexBuffer::~VertexBuffer() {}


void VertexBuffer::init() {
	createVertexBuffer();
	allocBufferMemory();
	loadVertexBuffer();
}


void VertexBuffer::cleanup() {
	if (vkIsValid(vertexBuffer))
		vkDestroyBuffer(vkContext.logicalDevice, vertexBuffer, nullptr);

	if (vkIsValid(vertexBufferMemory))
		vkFreeMemory(vkContext.logicalDevice, vertexBufferMemory, nullptr);
}


VkVertexInputBindingDescription VertexBuffer::getBindingDescription() {
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


std::array<VkVertexInputAttributeDescription, 2> VertexBuffer::getAttributeDescriptions() {
	// Attribute descriptions specify the type of the attributes passed to the vertex shader, which binding to load them from (and at which offset)
		// Each vertex attribute (e.g., position, color) must have its own attribute description.
	std::array<VkVertexInputAttributeDescription, 2> attribDescriptions{};

	// Attribute: Position
	attribDescriptions[0].binding = 0;
	attribDescriptions[0].location = 0;
	attribDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // R32G32 because `position` is a vec2. If it were a vec3, its format would be R32G32B32_SFLOAT or similar
	attribDescriptions[0].offset = offsetof(Vertex, position);

	// Attribute: Color
	attribDescriptions[1].binding = 0;
	attribDescriptions[1].location = 1;
	attribDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribDescriptions[1].offset = offsetof(Vertex, color);


	return attribDescriptions;
}


void VertexBuffer::createVertexBuffer() {
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	// Specifies the size of the buffer (in bytes)
		// Note: The size is not `sizeof(vertices)` because the size of `vertices` also includes the vector itself and its internal pointers, methods, etc.
	bufCreateInfo.size = sizeof(vertices[0]) * vertices.size();

	// Specifies the purpose of the buffer (It is possible to specify multiple purposes using a bitwise OR)
	bufCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	// Buffers can either be owned by a specific queue family or be shared between multiple queue families.
	// The vertex buffer is only owned by the graphics queue family, so we set the sharing mode to EXCLUSIVE.
	bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// Configures sparse buffer memory (which is irrelevant right now, so we'll leave it at the default value of 0)
	bufCreateInfo.flags = 0;

	VkResult result = vkCreateBuffer(vkContext.logicalDevice, &bufCreateInfo, nullptr, &vertexBuffer);
	if (result != VK_SUCCESS) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Failed to create vertex buffer!");
	}
}


void VertexBuffer::allocBufferMemory() {
	// Queries the buffer's memory requirements
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vkContext.logicalDevice, vertexBuffer, &memoryRequirements);

		// Specifies properties that the buffer's memory type must support
			// HOST_COHERENT_BIT ensures that the contents of the mapped buffer memory are coherent/matching with those of the allocated memory
	VkMemoryPropertyFlags propertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Allocates memory for the buffer
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);

	VkResult result = vkAllocateMemory(vkContext.logicalDevice, &allocInfo, nullptr, &vertexBufferMemory);
	if (result != VK_SUCCESS) {
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
	vkBindBufferMemory(vkContext.logicalDevice, vertexBuffer, vertexBufferMemory, 0);
}


void VertexBuffer::loadVertexBuffer() {
	// Maps the buffer memory into CPU-accessible memory so that we can write data to it
	void* data;
		// vkMapMemory lets us access a specified memory resource defined by an offset and size (size <= VK_WHOLE_SIZE, a special value used to map all of the memory)
	vkMapMemory(vkContext.logicalDevice, vertexBufferMemory, 0, bufCreateInfo.size, 0, &data);

	// Copies the vertex data to the mapped buffer memory
		// Note: we already defined `bufCreateInfo.size` in `createVertexBuffer`, so we can use that instead of `sizeof(vertices[0]) * vertices.size())`
	memcpy(data, vertices.data(), static_cast<size_t>(bufCreateInfo.size));

	// Unmaps the mapped buffer memory
	vkUnmapMemory(vkContext.logicalDevice, vertexBufferMemory);

	/* NOTE: The driver may not immediately copy the data into the buffer memory due to various reasons, like caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. 
	* There are two ways to deal with that problem:
	* - Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT (the method we currently use; see allocBufferMemory)
	* - Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
	*/
}


uint32_t VertexBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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
