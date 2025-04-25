/* TextureManager.cpp - Texture manager implementation.
*/
#include "TextureManager.hpp"


TextureManager::TextureManager(VulkanContext& context) :
	m_vkContext(context) {

	garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void TextureManager::createTexture(const char* texSource, VkFormat texImgFormat, int channels) {
	// If the default format is passed, use the default surface format
	if (texImgFormat == VK_FORMAT_UNDEFINED) {
		textureImageFormat = m_vkContext.SwapChain.surfaceFormat.format;
	}
	else {
		textureImageFormat = texImgFormat;
	}

	createTextureImage(texSource, channels);
	createTextureImageView();
	createTextureSampler();
}



void TextureManager::createTextureImage(const char* texSource, int channels) {
	// Get pixel and texture data
	int textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load(texSource, &textureWidth, &textureHeight, &textureChannels, channels);

		// The size of the pixels array is equal to: width * height * bytesPerPixel
	VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth * textureHeight * channels);

	if (!pixels) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create texture image for texture source path " + enquote(texSource) + "!");
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

	uint32_t stagingBufTaskID = BufferManager::createBuffer(m_vkContext, stagingBuffer, imageSize, stagingBufUsageFlags, stagingBufAllocation, bufAllocInfo);

		// Copy pixel data to the buffer
	void* pixelData;
	vmaMapMemory(m_vkContext.vmaAllocator, stagingBufAllocation, &pixelData);
		memcpy(pixelData, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(m_vkContext.vmaAllocator, stagingBufAllocation);


	// Clean up the `pixels` array
	stbi_image_free(pixels);


	// Create texture image objects
		// Image
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsageFlags = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		// Image allocation info
	VmaAllocationCreateInfo imgAllocCreateInfo{};
	imgAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	createImage(textureImage, textureImageAllocation, textureWidth, textureHeight, 1, textureImageFormat, imgTiling, imgUsageFlags, imgAllocCreateInfo);


	// Copy the staging buffer to the texture image
		// Transition the image layout to TRANSFER_DST (staging buffer (TRANSFER_SRC) -> image (TRANSFER_DST))
	switchImageLayout(textureImage, textureImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));


	// Transition the image layout to SHADER_READ_ONLY so that it can be read by the shader for sampling
	switchImageLayout(textureImage, textureImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


	// Cleanup
	CleanupTask imgTask{};
	imgTask.caller = __FUNCTION__;
	imgTask.objectNames = { VARIABLE_NAME(textureImageAllocation) };
	imgTask.vkObjects = { m_vkContext.vmaAllocator, textureImageAllocation };
	imgTask.cleanupFunc = [this]() {
		vmaDestroyImage(m_vkContext.vmaAllocator, textureImage, textureImageAllocation);
	};

	garbageCollector->createCleanupTask(imgTask);

		// Destroy the staging buffer at the end as it has served its purpose
	garbageCollector->executeCleanupTask(stagingBufTaskID);
}


void TextureManager::createTextureImageView() {
	std::pair<VkImageView, uint32_t> imageViewProperties = VkSwapchainManager::createImageView(m_vkContext, textureImage, textureImageFormat);
	textureImageView = imageViewProperties.first;
	m_vkContext.Texture.imageView = textureImageView;
}


void TextureManager::createTextureSampler() {
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Specifies how to interpolate textures if they are magnified/minified (thus solving the oversampling and undersampling problems respectively)
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;


	// Specifies the addressing mode to handle textures when the surface to which they are applied has coordinates that exceed the bounds of the texture (i.e., to handle texture behavior when applied to surfaces with bigger dimensions than its own).
	// The addressing mode is specified per-axis of the axes (U, V, W) instead of their counterparts (X, Y, Z) (because that's a convention).
	/* NOTE: Common addressing modes: VK_SAMPLER_ADDRESS_MODE_...
		- ...REPEAT: The texture repeats itself.
		- ...MIRRORED_REPEAT: Similar to ...REPEAT, but the texture is mirrored every time it repeats.
		- ...CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the texture dimensions.
		- ...MIRROR_CLAMP_TO_EDGE: Similar to ...CLAMP_TO_EDGE, but instead uses the edge opposite to the closest edge.
		- ...CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the texture.
	*/
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;


	// Specifies which color is returned when sampling beyond the image with CLAMP_TO_BORDER addressing mode
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;


	// Specifies whether anisotropy filtering is enabled. 
	// Enabling it makes textures less blurry/distorted especially when viewed at sharp angles, when stretched across a surface, etc..
	// However, it may impact performance (although that depends on the filtering level, e.g., 2x, 4x, 8x, 16x)
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = m_vkContext.Device.deviceProperties.limits.maxSamplerAnisotropy;

	
	// Use normalized texture coordinates <=> coordinates are clamped in the [0, 1) range instead of [0, textureWidth) and [0, textureHeight)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;


	// Specifies whether comparison functions are enabled.
	// Comparison functions are used to compare a sampled value (e.g., depth/stencil value) against a reference value. They are particularly useful in shadow mapping, percentage-closer filtering on shadow maps, depth testing, etc..
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;


	// Specifies mipmapping attributes
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;


	VkResult result = vkCreateSampler(m_vkContext.Device.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create texture sampler!");
	}
	
	m_vkContext.Texture.sampler = textureSampler;


	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(textureSampler) };
	task.vkObjects = { m_vkContext.Device.logicalDevice, textureSampler };
	task.cleanupFunc = [this]() { vkDestroySampler(m_vkContext.Device.logicalDevice, textureSampler, nullptr); };

	garbageCollector->createCleanupTask(task);
}


void TextureManager::defineImageLayoutTransitionStages(VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage, VkImageLayout oldLayout, VkImageLayout newLayout) {
	// Old layout
	const bool oldLayoutIsUndefined = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED);
	const bool oldLayoutIsTransferDst = (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// New layout
	const bool newLayoutIsTransferDst = (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	const bool newLayoutIsShaderReadOnly = (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


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



void TextureManager::createImage(VkImage& image, VmaAllocation& imgAllocation, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VmaAllocationCreateInfo& imgAllocCreateInfo) {
	
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


	VkResult imgCreateResult = vmaCreateImage(m_vkContext.vmaAllocator, &imgCreateInfo, &imgAllocCreateInfo, &image, &imgAllocation, nullptr);
	if (imgCreateResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create image!");
	}
}


void TextureManager::switchImageLayout(VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout) {
	SingleUseCommandBufferInfo cmdInfo{};
	cmdInfo.commandPool = VkCommandManager::createCommandPool(m_vkContext, m_vkContext.Device.logicalDevice, m_vkContext.Device.queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdInfo.fence = VkSyncManager::createSingleUseFence(m_vkContext);
	cmdInfo.usingSingleUseFence = true;
	cmdInfo.queue = m_vkContext.Device.queueFamilies.graphicsFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(m_vkContext, &cmdInfo);

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


	VkCommandManager::endSingleUseCommandBuffer(m_vkContext, &cmdInfo, commandBuffer);
	m_vkContext.Texture.imageLayout = newLayout;
}


void TextureManager::copyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height) {
	SingleUseCommandBufferInfo cmdInfo{};
	cmdInfo.commandPool = VkCommandManager::createCommandPool(m_vkContext, m_vkContext.Device.logicalDevice, m_vkContext.Device.queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdInfo.fence = VkSyncManager::createSingleUseFence(m_vkContext);
	cmdInfo.usingSingleUseFence = true;
	cmdInfo.queue = m_vkContext.Device.queueFamilies.graphicsFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(m_vkContext, &cmdInfo);


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


	VkCommandManager::endSingleUseCommandBuffer(m_vkContext, &cmdInfo, commandBuffer);
}
