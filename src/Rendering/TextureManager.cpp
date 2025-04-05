/* TextureManager.cpp - Texture manager implementation.
*/
#include "TextureManager.hpp"


void TextureManager::createTextureImage(VulkanContext& vkContext, MemoryManager& memoryManager, const char* imgPath, int channels) {
	int textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load(imgPath, &textureWidth, &textureHeight, &textureChannels, channels);

	// The size of the pixels array is equal to: width * height * bytesPerPixel
	VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth * textureHeight * channels);

	if (!pixels) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create texture image for image path " + enquote(imgPath) + "!");
	}


	// Copy the pixels to a temporary buffer
		// Create the buffer and its allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufAllocation;

	VkBufferUsageFlags stagingBufUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo bufAllocInfo{};
	bufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	bufAllocInfo.requiredFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	bufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; // Specify CPU access since we will be mapping the buffer allocation to CPU memory

	BufferManager::createBuffer(vkContext, stagingBuffer, imageSize, stagingBufUsageFlags, stagingBufAllocation, bufAllocInfo);


		// Copy pixel data to the buffer
	void* pixelData;
	vmaMapMemory(vkContext.vmaAllocator, stagingBufAllocation, &pixelData);
		memcpy(&pixelData, &pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(vkContext.vmaAllocator, stagingBufAllocation);


	// Clean up the `pixels` array
	stbi_image_free(pixels);
}
