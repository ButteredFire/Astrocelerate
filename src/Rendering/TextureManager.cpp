/* TextureManager.cpp - Texture manager implementation.
*/
#include "TextureManager.hpp"


std::pair<VkImage, VmaAllocation> TextureManager::createTextureImage(VulkanContext& vkContext, const char* imgPath, int channels) {
	std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	// Get pixel and texture data
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
		memcpy(pixelData, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(vkContext.vmaAllocator, stagingBufAllocation);


	// Clean up the `pixels` array
	stbi_image_free(pixels);


	// Create texture image objects
	VkImage textureImage;
	VmaAllocation textureImageAllocation;

		// Image
	VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsageFlags = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		// Image allocation info
	VmaAllocationCreateInfo imgAllocCreateInfo{};
	imgAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	createImage(vkContext, textureImage, textureImageAllocation, textureWidth, textureHeight, 1, imgFormat, imgTiling, imgUsageFlags, imgAllocCreateInfo);


	// Copy the staging buffer to the texture image
		// Transition the image layout to TRANSFER_DST (staging buffer (TRANSFER_SRC) -> image (TRANSFER_DST))
	switchImageLayout(vkContext, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(vkContext, stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));


	// Transition the image layout to SHADER_READ_ONLY so that it can be read by the shader for sampling
	switchImageLayout(vkContext, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


	// Cleanup
	CleanupTask imgTask{};
	imgTask.caller = __FUNCTION__;
	imgTask.objectNames = { VARIABLE_NAME(textureImageAllocation) };
	imgTask.vkObjects = { vkContext.vmaAllocator, textureImageAllocation };
	imgTask.cleanupFunc = [vkContext, textureImage, textureImageAllocation]() {
		vmaDestroyImage(vkContext.vmaAllocator, textureImage, textureImageAllocation);
	};

	garbageCollector->createCleanupTask(imgTask);


	return { textureImage, textureImageAllocation };
}


void TextureManager::createImage(VulkanContext& vkContext, VkImage& image, VmaAllocation& imgAllocation, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VmaAllocationCreateInfo& imgAllocCreateInfo) {
	
	// Image info
	VkImageCreateInfo imgCreateInfo{};
	imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	// Specifies the type of image to be created (including the kind of coordinate system the image's texels are going to be addressed)
	/* It is possible to create 1D, 2D, and 3D images.
		+ A 1D image (width) is an array of texels (texture elements/pixels). It is typically used for linear data storage (e.g., lookup tables, gradients).
		+ A 2D image (width * height) is a rectangular grid of texels. It is typically used for textures in 2D and 3D rendering (e.g., diffuse maps, normal maps).
		+ A 3D image (width * height * depth) is a volumetric grid of texels. It is typically used for volumetric data (e.g., 3D textures, volume rendering, scientific visualization).
	*/
	imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;

	// Specifies image dimensions (i.e., number of texels per axis) 
	imgCreateInfo.extent.width = static_cast<uint32_t>(width);
	imgCreateInfo.extent.height = static_cast<uint32_t>(height);
	imgCreateInfo.extent.depth = static_cast<uint32_t>(depth);

	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;

	imgCreateInfo.format = imgFormat;
	imgCreateInfo.tiling = imgTiling;

	/* NOTE: VK_IMAGE_...
		+ ...LAYOUT_UNDEFINED: The image will not be usable by the GPU and the very first transition will discard the texels.
		+ ...LAYOUT_PREINITIALZED: Same as LAYOUT_UNDEFINED, but the very first transition will preserve the texels.
	*/
	imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	/* NOTE: VK_IMAGE_USAGE_...
		+ ...TRANSFER_DST_BIT: The image will be used as the destination for the staging buffer copy .
		+ ...SAMPLED_BIT: The image is accessible from the shader. We need this accessibility to color meshes. In other words, the image will be used for sampling in shaders.

	*/
	imgCreateInfo.usage = imgUsageFlags;

	imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgCreateInfo.flags = 0; // Currently disabled, but is useful for sparse images

	// The image will only be used by the graphics queue family (which fortunately also supports transfer operations, so there is no need to specify the image to be used both by the graphics and transfer queue families)
	imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


	VkResult imgCreateResult = vmaCreateImage(vkContext.vmaAllocator, &imgCreateInfo, &imgAllocCreateInfo, &image, &imgAllocation, nullptr);
	if (imgCreateResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create image!");
	}
}


void TextureManager::switchImageLayout(VulkanContext& vkContext, VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout) {
	SingleUseCommandBufferInfo cmdInfo{};
	cmdInfo.commandPool = VkCommandManager::createCommandPool(vkContext, vkContext.logicalDevice, vkContext.queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdInfo.fence = VkSyncManager::createSingleUseFence(vkContext);
	cmdInfo.usingSingleUseFence = true;
	cmdInfo.queue = vkContext.queueFamilies.graphicsFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(vkContext, &cmdInfo);

	/* Perform layout transition using an image memory barrier.
		It is part of Vulkan barriers, which are used for processes like:
			- Synchronization (execution barrier): Ensuring sync/order between commands/resources
			- Memory visibility/availability (memory barrier):
				+ Ensuring the visibility of writes (i.e., that writes are flushed to allow for, for instance, subsequent reading of the written data)
				+ Also used to transition image layouts and transfer queue family ownership (if `VK_SHARING_MODE_EXCLUSIVE` is used)
	*/

	VkImageMemoryBarrier imgMemBarrier{};
	imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	// Specifies layout transition
	imgMemBarrier.oldLayout = oldLayout;
	imgMemBarrier.newLayout = newLayout;

	// Specifies queue family ownership transference
	imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Use IGNORED to skip this transference
	imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	// Specifies image properties
	imgMemBarrier.image = image;
	imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		
		// Image does not have multiple mipmapping levels
	imgMemBarrier.subresourceRange.baseMipLevel = 0;
	imgMemBarrier.subresourceRange.levelCount = 1;

		// Image is not an array, i.e., only having one layer
	imgMemBarrier.subresourceRange.baseArrayLayer = 0;
	imgMemBarrier.subresourceRange.layerCount = 1;

	// Resource accessing: Specifies operations involving the resource that...
		// ... must happen before the barrier
	imgMemBarrier.srcAccessMask = 0;

		// ... must wait for the barrier
	imgMemBarrier.dstAccessMask = 0;

	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	defineImageLayoutTransitionStages(&imgMemBarrier.srcAccessMask, &imgMemBarrier.dstAccessMask, &srcStage, &dstStage, oldLayout, newLayout);


	// Creates the barrier
	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);


	VkCommandManager::endSingleUseCommandBuffer(vkContext, &cmdInfo, commandBuffer);
}


void TextureManager::defineImageLayoutTransitionStages(VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage, VkImageLayout oldLayout, VkImageLayout newLayout) {
	// Old layout
	const bool oldLayoutIsUndefined =			(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED);
	const bool oldLayoutIsTransferDst =			(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// New layout
	const bool newLayoutIsTransferDst =			(newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	const bool newLayoutIsShaderReadOnly =		(newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	
	if (oldLayoutIsUndefined && newLayoutIsTransferDst) {
		*srcAccessMask = 0;
		*dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		*srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		*dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		return;
	}


	if (oldLayoutIsTransferDst && newLayoutIsShaderReadOnly) {
		*srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		*dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		*srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		*dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		return;
	}


	throw Log::RuntimeException(__FUNCTION__, "Cannot define stages for image layout transition: Unsupported layout transition!");
}


void TextureManager::copyBufferToImage(VulkanContext& vkContext, VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height) {
	SingleUseCommandBufferInfo cmdInfo{};
	cmdInfo.commandPool = VkCommandManager::createCommandPool(vkContext, vkContext.logicalDevice, vkContext.queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdInfo.fence = VkSyncManager::createSingleUseFence(vkContext);
	cmdInfo.usingSingleUseFence = true;
	cmdInfo.queue = vkContext.queueFamilies.graphicsFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(vkContext, &cmdInfo);


	// Specifies the region of the buffer to copy to the image
	VkBufferImageCopy region{};
	region.bufferOffset = 0;		// Byte offset in the buffer at which the pixel values start
	
		// Specifies the buffer layout in memory.
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

		// Specifies the region of the image to copy image the pixels from the buffer to
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;


	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,	// The image layout is assumed to be an optimal one for pixel transference.
		1, &region
	);


	VkCommandManager::endSingleUseCommandBuffer(vkContext, &cmdInfo, commandBuffer);
}
