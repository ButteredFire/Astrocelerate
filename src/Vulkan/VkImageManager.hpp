
#pragma once

#include <vector>

#include <External/GLFWVulkan.hpp>

#include <Core/Application/GarbageCollector.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Core/Data/Contexts/VulkanContext.hpp>


class VkImageManager {
public:
	/* Creates an image.
		@param image: The image to be created.
		@param imgAllocation: The memory allocation for the image.
		@param imgAllocationCreateInfo: The allocation create info for the image.
		@param width, height, depth: The width, height, and depth of the image.
		@param imgFormat: The format of the image.
		@param imgTiling: The tiling mode of the image.
		@param imgUsageFlags: The usage flags for the image.
		@param imgType: The image type.

		@return The image allocation's cleanup task ID.
		
		@note This function assumes the garbage collector service has already been registered.
	*/
	static uint32_t CreateImage(VkImage& image, VmaAllocation& imgAllocation, VmaAllocationCreateInfo& imgAllocationCreateInfo, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VkImageType imgType);


	/* Creates an image view.
		@param imageView: The image view to be created.
		@param image: The image to be used.
		@param imgFormat: The format of the image.
		@param imgAspectFlags: The image aspect flags specifying which aspect(s) of the image are to be included in the image view.
		@param viewType: The type of the image view (e.g., 2D, 3D, cube).
		@param levelCount: The number of mipmap levels in the image.
		@param layerCount: The number of layers in the image view (for 3D images, this is typically 1).

		@return The image view's cleanup task ID.

		@note This function assumes the garbage collector service has already been registered.
	*/
	static uint32_t CreateImageView(VkImageView& imageView, VkImage& image, VkFormat imgFormat, VkImageAspectFlags imgAspectFlags, VkImageViewType viewType, uint32_t levelCount, uint32_t layerCount);


	/* Creates a framebuffer.
		@param framebuffer: The framebuffer to be created.
		@param renderPass: The render pass to be used.
		@param attachments: The image views to be used as attachments in the framebuffer.
		@param width, height: The width and height of the framebuffer.

		@return The framebuffer's cleanup task ID.

		@note This function assumes the garbage collector service has already been registered.
	*/
	static uint32_t CreateFramebuffer(VkFramebuffer& framebuffer, VkRenderPass& renderPass, std::vector<VkImageView> attachments, uint32_t width, uint32_t height);
};