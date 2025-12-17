#include "VkImageManager.hpp"


CleanupID VkImageManager::CreateImage(VkImage& image, VmaAllocation& imgAllocation, VmaAllocationCreateInfo& imgAllocationCreateInfo, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VkImageType imgType) {
	LOG_ASSERT(((imgType & VK_IMAGE_TYPE_2D == 1) && depth == 1),
		"Unable to create image: Depth must be 1 if the image type is 2D!");


	std::shared_ptr<VkCoreResourcesManager> coreResources = ServiceLocator::GetService<VkCoreResourcesManager>(__FUNCTION__);
	std::shared_ptr<ResourceManager> resourceManager = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);

	const VmaAllocator &vmaAllocator = coreResources->getVmaAllocator();


	VkImageCreateInfo imgCreateInfo{};
	imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	// Specifies the type of image to be created (including the kind of coordinate system the image's texels are going to be addressed)
	/* It is possible to create 1D, 2D, and 3D images.
		+ A 1D image (width) is an array of texels (texture elements/pixels). It is typically used for linear data storage (e.g., lookup tables, gradients).
		+ A 2D image (width * height) is a rectangular grid of texels. It is typically used for textures in 2D and 3D rendering (e.g., diffuse maps, normal maps).
		+ A 3D image (width * height * depth) is a volumetric grid of texels. It is typically used for volumetric data (e.g., 3D textures, volume rendering, scientific visualization).
	*/
	imgCreateInfo.imageType = imgType;

	imgCreateInfo.extent.width = width;
	imgCreateInfo.extent.height = height;
	imgCreateInfo.extent.depth = depth;

	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;

	imgCreateInfo.format = imgFormat;
	imgCreateInfo.tiling = imgTiling;

	/* NOTE: VK_IMAGE_...
		+ ...LAYOUT_UNDEFINED: The image will not be usable by the GPU and the very first transition will discard the pixels.
		+ ...LAYOUT_PREINITIALZED: Same as LAYOUT_UNDEFINED, but the very first transition will preserve the pixels.
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


	VkResult imgCreateResult = vmaCreateImage(vmaAllocator, &imgCreateInfo, &imgAllocationCreateInfo, &image, &imgAllocation, nullptr);

	if (imgCreateResult != VK_SUCCESS) {
		if (imgCreateResult == VK_ERROR_OUT_OF_HOST_MEMORY)
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create image: Your machine has run out of host memory!\nThis could be caused by loading heavy simulations.\nPlease update your " + coreResources->getDeviceName() + " driver and re-run Astrocelerate.");

		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create image!\nVulkan error code: " + TO_STR(imgCreateResult));
	}

	CleanupTask imgTask{};
	imgTask.caller = __FUNCTION__;
	imgTask.objectNames = { VARIABLE_NAME(imgAllocation) };
	imgTask.vkHandles = { imgAllocation };
	imgTask.cleanupFunc = [=]() {
		vmaDestroyImage(vmaAllocator, image, imgAllocation);
	};

	uint32_t taskID = resourceManager->createCleanupTask(imgTask);

	return taskID;
}


CleanupID VkImageManager::CreateImageView(VkImageView& imageView, VkImage& image, VkFormat imgFormat, VkImageAspectFlags imgAspectFlags, VkImageViewType viewType, uint32_t levelCount, uint32_t layerCount) {
	std::shared_ptr<VkCoreResourcesManager> coreResources = ServiceLocator::GetService<VkCoreResourcesManager>(__FUNCTION__);
	std::shared_ptr<ResourceManager> resourceManager = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);

	const VkDevice &logicalDevice = coreResources->getLogicalDevice();


	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;

	// Specifies how the data is interpreted
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = imgFormat; // Specifies image format

	/* Defines how the color channels of the image should be interpreted (swizzling).
	* In essence, swizzling remaps the color channels of the image to how they should be interpreted by the image view.
	*
	* Since we have already chosen an image format in getBestSurfaceFormat(...) as "VK_FORMAT_R8G8B8A8_SRGB" (which stores data in RGBA order), we leave the swizzle mappings as "identity" (i.e., unchanged). This means: Red maps to Red, Green maps to Green, Blue maps to Blue, and Alpha maps to Alpha.
	*
	* In cases where the image format differs from what the shaders or rendering pipeline expect, swizzling can be used to remap the channels.
	* For instance, if we want to change our image format to "B8G8R8A8" (BGRA order instead of RGBA), we must swap the Red and Blue channels:
	*
	* componentMapping.r = VK_COMPONENT_SWIZZLE_B; // Red -> Blue (R -> B)
	* componentMapping.g = VK_COMPONENT_SWIZZLE_IDENTITY; // Green value is the same (RG -> BG)
	* componentMapping.b = VK_COMPONENT_SWIZZLE_R; // Blue -> Red (RGB -> BGR)
	* componentMapping.a = VK_COMPONENT_SWIZZLE_IDENTITY; // Green value is the same (RGBA -> BGRA)
	*/
	VkComponentMapping colorMappingStruct{};
	colorMappingStruct.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	colorMappingStruct.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	colorMappingStruct.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	colorMappingStruct.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	viewCreateInfo.components = colorMappingStruct;

	// Specifies the image's purpose and which part of it should be accessed
	viewCreateInfo.subresourceRange.aspectMask = imgAspectFlags; // Images will be used as color targets
	viewCreateInfo.subresourceRange.baseMipLevel = 0; // Images will have no mipmapping levels
	viewCreateInfo.subresourceRange.levelCount = levelCount;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0; // Images will have no multiple layers
	viewCreateInfo.subresourceRange.layerCount = layerCount;

	VkResult result = vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &imageView);
	LOG_ASSERT(result == VK_SUCCESS, "Failed to create image view!");

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(imageView) };
	task.vkHandles = { imageView };
	task.cleanupFunc = [=]() {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	};

	uint32_t taskID = resourceManager->createCleanupTask(task);

	return taskID;
}


CleanupID VkImageManager::CreateFramebuffer(VkFramebuffer& framebuffer, VkRenderPass& renderPass, std::vector<VkImageView> attachments, uint32_t width, uint32_t height) {
	std::shared_ptr<VkCoreResourcesManager> coreResources = ServiceLocator::GetService<VkCoreResourcesManager>(__FUNCTION__);
	std::shared_ptr<ResourceManager> resourceManager = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);

	const VkDevice &logicalDevice = coreResources->getLogicalDevice();


	VkFramebufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	bufferCreateInfo.renderPass = renderPass;
	bufferCreateInfo.attachmentCount = attachments.size();
	bufferCreateInfo.pAttachments = attachments.data();
	bufferCreateInfo.width = width;
	bufferCreateInfo.height = height;
	bufferCreateInfo.layers = 1;

	VkResult result = vkCreateFramebuffer(coreResources->getLogicalDevice(), &bufferCreateInfo, nullptr, &framebuffer);
	LOG_ASSERT(result == VK_SUCCESS, "Failed to create framebuffer!");
	
	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(framebuffer) };
	task.vkHandles = { framebuffer };
	task.cleanupFunc = [=]() {
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	};

	uint32_t taskID = resourceManager->createCleanupTask(task);
	
	return taskID;
}
