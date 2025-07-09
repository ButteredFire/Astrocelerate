/* TextureManager.cpp - Texture manager implementation.
*/
#include "TextureManager.hpp"


// Defines hashing for VkSamplerCreateInfo. This is necessary for the ability to maintain unique samplers and reuse them when new textures are added.
namespace std {
	template<> struct hash<VkSamplerCreateInfo> {
		size_t operator()(const VkSamplerCreateInfo& createInfo) const {
			size_t seed = 0;

			SystemUtils::CombineHash(seed, createInfo.magFilter);
			SystemUtils::CombineHash(seed, createInfo.minFilter);
			SystemUtils::CombineHash(seed, createInfo.addressModeU);
			SystemUtils::CombineHash(seed, createInfo.addressModeV);
			SystemUtils::CombineHash(seed, createInfo.addressModeW);
			SystemUtils::CombineHash(seed, createInfo.borderColor);
			SystemUtils::CombineHash(seed, createInfo.anisotropyEnable);
			SystemUtils::CombineHash(seed, createInfo.maxAnisotropy);
			SystemUtils::CombineHash(seed, createInfo.unnormalizedCoordinates);
			SystemUtils::CombineHash(seed, createInfo.compareEnable);
			SystemUtils::CombineHash(seed, createInfo.compareOp);
			SystemUtils::CombineHash(seed, createInfo.mipmapMode);
			SystemUtils::CombineHash(seed, createInfo.mipLodBias);
			SystemUtils::CombineHash(seed, createInfo.minLod);
			SystemUtils::CombineHash(seed, createInfo.maxLod);

			return seed;
		}
	};

	template<> struct equal_to<VkSamplerCreateInfo> {
		bool operator()(const VkSamplerCreateInfo& a, const VkSamplerCreateInfo& b) const {
			return a.magFilter == b.magFilter &&
				a.minFilter == b.minFilter &&
				a.addressModeU == b.addressModeU &&
				a.addressModeV == b.addressModeV &&
				a.addressModeW == b.addressModeW &&
				a.borderColor == b.borderColor &&
				a.anisotropyEnable == b.anisotropyEnable &&
				a.maxAnisotropy == b.maxAnisotropy &&
				a.unnormalizedCoordinates == b.unnormalizedCoordinates &&
				a.compareEnable == b.compareEnable &&
				a.compareOp == b.compareOp &&
				a.mipmapMode == b.mipmapMode &&
				a.mipLodBias == b.mipLodBias &&
				a.minLod == b.minLod &&
				a.maxLod == b.maxLod;
		}
	};
}


TextureManager::TextureManager() {
	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void TextureManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::BufferManagerIsValid>(
		[this](const Event::BufferManagerIsValid &event) {
			// Create a placeholder texture (index 0).
			const std::string texturePath = FilePathUtils::JoinPaths(APP_SOURCE_DIR, "assets/Textures", "Fallback/PlaceholderTexture.png");
			//m_placeholderTextureIndex = createIndexedTexture(texturePath, VK_FORMAT_R8G8B8A8_SRGB);
		}
	);


	m_eventDispatcher->subscribe<Event::PipelinesInitialized>(
		[this](const Event::PipelinesInitialized &event) {
			m_textureArrayDescSetIsValid = true;

			for (size_t i = 0; i < m_textureDescriptorInfos.size(); i++)
				updateTextureArrayDescriptorSet(i, m_textureDescriptorInfos[i]);
		}
	);
}


Geometry::Texture TextureManager::createIndependentTexture(const std::string &texSource, VkFormat texImgFormat, int channels) {
	// If the default format is passed, use the default surface format
	VkFormat imgFormat = (texImgFormat == VK_FORMAT_UNDEFINED) ? g_vkContext.SwapChain.surfaceFormat.format : texImgFormat;

	TextureInfo imageProperties = createTextureImage(imgFormat, texSource.c_str(), channels);
	VkImageView imageView = createTextureImageView(imageProperties.image, imgFormat);
	VkSampler sampler = createTextureSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_TRUE, FLT_MAX,
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, 0.0f, 0.0f
	);

	return Geometry::Texture{
		.size = { imageProperties.width, imageProperties.height },
		.imageLayout = imageProperties.imageLayout,
		.imageView = imageView,
		.sampler = sampler
	};
}


uint32_t TextureManager::createIndexedTexture(const std::string& texSource, VkFormat texImgFormat, int channels) {
	auto it = m_texturePathToIndexMap.find(texSource);
	if (it != m_texturePathToIndexMap.end())
		return it->second;


	// If the default format is passed, use the default surface format
	VkFormat imgFormat = (texImgFormat == VK_FORMAT_UNDEFINED) ? g_vkContext.SwapChain.surfaceFormat.format : texImgFormat;


	// Creates texture
	TextureInfo imageProperties = createTextureImage(imgFormat, texSource.c_str(), channels);
	VkImageView imageView = createTextureImageView(imageProperties.image, imgFormat);
	VkSampler sampler = createTextureSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_BORDER_COLOR_INT_OPAQUE_BLACK, 
		VK_TRUE, FLT_MAX,
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, 0.0f, 0.0f
	);


	// Creates descriptor image info and binds it to the global texture array
	VkDescriptorImageInfo descInfo{};
	descInfo.imageLayout = imageProperties.imageLayout;
	descInfo.imageView = imageView;
	descInfo.sampler = sampler;

	uint32_t newIndex = static_cast<uint32_t>(m_textureDescriptorInfos.size());
	m_textureDescriptorInfos.push_back(descInfo);
	m_texturePathToIndexMap[texSource] = newIndex;

	// Only update texture array descriptor set if it is valid. Else, defer the textures until it is (already done with m_textureDescriptorInfos).
	if (m_textureArrayDescSetIsValid)
		// TODO: Support dynamic texture loading
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot create indexed texture: Dynamic texture loading is not currently supported!");
		//updateTextureArrayDescriptorSet(newIndex, m_textureDescriptorInfos.back());

	return newIndex;
}


void TextureManager::updateTextureArrayDescriptorSet(uint32_t texIndex, const VkDescriptorImageInfo &texImageInfo) {
	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = g_vkContext.Textures.texArrayDescriptorSet;
	descriptorWrite.dstBinding = ShaderConsts::FRAG_BIND_TEXTURE_MAP;
	descriptorWrite.dstArrayElement = texIndex; // The specific index in the array where this texture belongs
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &texImageInfo;

	vkUpdateDescriptorSets(g_vkContext.Device.logicalDevice, 1, &descriptorWrite, 0, nullptr);
}


TextureInfo TextureManager::createTextureImage(VkFormat imgFormat, const char* texSource, int channels) {
	// See documentation in `stb_image.h`
	static std::unordered_set<std::string> supportedTextureFormats = {
		".jpeg", ".jpg", ".png", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic", ".pnm"
	};


	// Get pixel and texture data
	int textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load(texSource, &textureWidth, &textureHeight, &textureChannels, channels);

		// The size of the pixels array is equal to: width * height * bytesPerPixel
	VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth * textureHeight * channels);

	if (!pixels) {
		if (supportedTextureFormats.find(FilePathUtils::GetFileExtension(texSource)) == supportedTextureFormats.end())
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create texture image for texture source path " + enquote(texSource) + "!"
				+ "\nThe extension of the file (" + FilePathUtils::GetFileName(texSource) + ") is currently not supported.");

		else
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create texture image for texture source path " + enquote(texSource) + "!");
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

	uint32_t stagingBufTaskID = VkBufferManager::createBuffer(stagingBuffer, imageSize, stagingBufUsageFlags, stagingBufAllocation, bufAllocInfo);

		// Copy pixel data to the buffer
	void* pixelData;
	vmaMapMemory(g_vkContext.vmaAllocator, stagingBufAllocation, &pixelData);
		memcpy(pixelData, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(g_vkContext.vmaAllocator, stagingBufAllocation);


	// Clean up the `pixels` array
	stbi_image_free(pixels);


	// Create texture image objects
		// Image
	VkImage image;
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsageFlags = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		// Image allocation info
	VmaAllocation imgAllocation;
	VmaAllocationCreateInfo imgAllocCreateInfo{};
	imgAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkImageManager::CreateImage(image, imgAllocation, imgAllocCreateInfo, textureWidth, textureHeight, 1, imgFormat, imgTiling, imgUsageFlags, VK_IMAGE_TYPE_2D);

	// Copy the staging buffer to the texture image
		// Transition the image layout to TRANSFER_DST (staging buffer (TRANSFER_SRC) -> image (TRANSFER_DST))
	switchImageLayout(image, imgFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));


	// Transition the image layout to the final SHADER_READ_ONLY layout so that it can be read by the shader for sampling
	switchImageLayout(image, imgFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Destroy the staging buffer at the end as it has served its purpose
	m_garbageCollector->executeCleanupTask(stagingBufTaskID);


	return TextureInfo{
		.width = textureWidth,
		.height = textureHeight,
		.image = image,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
}


VkImageView TextureManager::createTextureImageView(VkImage image, VkFormat imgFormat) {
	VkImageView imageView;
	VkImageManager::CreateImageView(imageView, image, imgFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1);

	return imageView;
}


VkSampler TextureManager::createTextureSampler(
	VkFilter magFilter, VkFilter minFilter,
	VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
	VkBorderColor borderColor,
	VkBool32 anisotropyEnable, float maxAnisotropy,
	VkBool32 unnormalizedCoordinates,
	VkBool32 compareEnable, VkCompareOp compareOp,
	VkSamplerMipmapMode mipmapMode, float mipLodBias, float minLod, float maxLod
) {
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Specifies how to interpolate textures if they are magnified/minified (thus solving the oversampling and undersampling problems respectively)
	samplerCreateInfo.magFilter = magFilter;
	samplerCreateInfo.minFilter = minFilter;


	// Specifies the addressing mode to handle textures when the surface to which they are applied has coordinates that exceed the bounds of the texture (i.e., to handle texture behavior when applied to surfaces with bigger dimensions than its own).
	// The addressing mode is specified per-axis of the axes (U, V, W) instead of their counterparts (X, Y, Z) (because that's a convention).
	/* NOTE: Common addressing modes: VK_SAMPLER_ADDRESS_MODE_...
		- ...REPEAT: The texture repeats itself.
		- ...MIRRORED_REPEAT: Similar to ...REPEAT, but the texture is mirrored every time it repeats.
		- ...CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the texture dimensions.
		- ...MIRROR_CLAMP_TO_EDGE: Similar to ...CLAMP_TO_EDGE, but instead uses the edge opposite to the closest edge.
		- ...CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the texture.
	*/
	samplerCreateInfo.addressModeU = addressModeU;
	samplerCreateInfo.addressModeV = addressModeV;
	samplerCreateInfo.addressModeW = addressModeW;


	// Specifies which color is returned when sampling beyond the image with CLAMP_TO_BORDER addressing mode
	samplerCreateInfo.borderColor = borderColor;


	// Specifies whether anisotropy filtering is enabled. 
	// Enabling it makes textures less blurry/distorted especially when viewed at sharp angles, when stretched across a surface, etc..
	// However, it may impact performance (although that depends on the filtering level, e.g., 2x, 4x, 8x, 16x)
	samplerCreateInfo.anisotropyEnable = anisotropyEnable;

	samplerCreateInfo.maxAnisotropy = (maxAnisotropy == FLT_MAX)? g_vkContext.Device.deviceProperties.limits.maxSamplerAnisotropy : maxAnisotropy;

	
	// Use normalized texture coordinates <=> coordinates are clamped in the [0, 1) range instead of [0, textureWidth) and [0, textureHeight)
	samplerCreateInfo.unnormalizedCoordinates = unnormalizedCoordinates;


	// Specifies whether comparison functions are enabled.
	// Comparison functions are used to compare a sampled value (e.g., depth/stencil value) against a reference value. They are particularly useful in shadow mapping, percentage-closer filtering on shadow maps, depth testing, etc..
	samplerCreateInfo.compareEnable = compareEnable;
	samplerCreateInfo.compareOp = compareOp;


	// Specifies mipmapping attributes
	samplerCreateInfo.mipmapMode = mipmapMode;
	samplerCreateInfo.mipLodBias = mipLodBias;
	samplerCreateInfo.minLod = minLod;
	samplerCreateInfo.maxLod = maxLod;


	// If the sampler already exists, use the existing sampler.
	size_t samplerInfoHash = std::hash<VkSamplerCreateInfo>{}(samplerCreateInfo);
	
	auto it = m_uniqueSamplers.find(samplerInfoHash);
	if (it != m_uniqueSamplers.end()) {
		return it->second;
	}

	// If not, create it, and add it to the list of unique samplers.
	VkSampler textureSampler;
	VkResult result = vkCreateSampler(g_vkContext.Device.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
	
	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(textureSampler) };
	task.vkObjects = { g_vkContext.Device.logicalDevice, textureSampler };
	task.cleanupFunc = [this, textureSampler]() { vkDestroySampler(g_vkContext.Device.logicalDevice, textureSampler, nullptr); };
	m_garbageCollector->createCleanupTask(task);

	m_uniqueSamplers[samplerInfoHash] = textureSampler;
	
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create texture sampler!");
	}

	return textureSampler;
}


void TextureManager::createImage(VkImage& image, VmaAllocation& imgAllocation, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VmaAllocationCreateInfo& imgAllocCreateInfo) {
	std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

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


	VkResult imgCreateResult = vmaCreateImage(g_vkContext.vmaAllocator, &imgCreateInfo, &imgAllocCreateInfo, &image, &imgAllocation, nullptr);
	if (imgCreateResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create image!");
	}


	CleanupTask imgTask{};
	imgTask.caller = __FUNCTION__;
	imgTask.objectNames = { VARIABLE_NAME(imgAllocation) };
	imgTask.vkObjects = { g_vkContext.vmaAllocator, imgAllocation };
	imgTask.cleanupFunc = [image, imgAllocation]() {
		vmaDestroyImage(g_vkContext.vmaAllocator, image, imgAllocation);
	};

	garbageCollector->createCleanupTask(imgTask);
}


void TextureManager::switchImageLayout(VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout) {
	SingleUseCommandBufferInfo cmdInfo{};
	cmdInfo.commandPool = VkCommandManager::createCommandPool(g_vkContext.Device.logicalDevice, g_vkContext.Device.queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdInfo.fence = VkSyncManager::createSingleUseFence();
	cmdInfo.usingSingleUseFence = true;
	cmdInfo.queue = g_vkContext.Device.queueFamilies.graphicsFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(&cmdInfo);

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

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (VkFormatUtils::FormatHasStencilComponent(imgFormat)) {
			imgMemBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}


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


	VkCommandManager::endSingleUseCommandBuffer(&cmdInfo, commandBuffer);
}


void TextureManager::defineImageLayoutTransitionStages(VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage, VkImageLayout oldLayout, VkImageLayout newLayout) {
	// Old layout
	const bool oldLayoutIsUndefined = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED);
	const bool oldLayoutIsTransferDst = (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// New layout
	const bool newLayoutIsTransferDst = (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	const bool newLayoutIsShaderReadOnly = (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	const bool newLayoutIsDepthStencilAttachment = (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


	if (oldLayoutIsUndefined && newLayoutIsTransferDst) {
		*srcAccessMask = 0;
		*dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		*srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		*dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}


	else if (oldLayoutIsUndefined && newLayoutIsDepthStencilAttachment) {
		*srcAccessMask = 0;
		*dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		*srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		*dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		/* NOTE:
			- The reading happens in the EARLY_FRAGMENT_TESTS stage and the writing happens in the LATE_FRAGMENT_TESTS stage.
			- Advice: Pick the earliest pipeline stage specified in the access mask (in this case, it's the reading stage) so that the resource is available as early as possible.
		*/
	}


	else if (oldLayoutIsTransferDst && newLayoutIsShaderReadOnly) {
		*srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		*dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		*srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		*dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}


	else
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot define stages for image layout transition: Unsupported layout transition!");
}


void TextureManager::copyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height) {
	SingleUseCommandBufferInfo cmdInfo{};
	cmdInfo.commandPool = VkCommandManager::createCommandPool(g_vkContext.Device.logicalDevice, g_vkContext.Device.queueFamilies.graphicsFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdInfo.fence = VkSyncManager::createSingleUseFence();
	cmdInfo.usingSingleUseFence = true;
	cmdInfo.queue = g_vkContext.Device.queueFamilies.graphicsFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(&cmdInfo);


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


	VkCommandManager::endSingleUseCommandBuffer(&cmdInfo, commandBuffer);
}
