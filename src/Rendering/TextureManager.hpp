/* TextureManager.hpp - Manages textures and related operations (e.g., creation, modification).
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

// STB
#include <stb/stb_image.h>

// C++ STLs
#include <iostream>

// Other
#include <Constants.h>
#include <LoggingManager.hpp>
#include <ApplicationContext.hpp>
#include <GarbageCollector.hpp>
#include <Shaders/BufferManager.hpp>


class TextureManager {
public:
	TextureManager() {};
	~TextureManager() {};


	/* Creates a texture image.
		@param vkContext: The application context.
		@param imgPath: The path to the image.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.

        @return A pair containing the image object (pair::first), and its memory allocation (pair::second).
	*/
	static std::pair<VkImage, VmaAllocation> createTextureImage(VulkanContext& vkContext, const char* imgPath, int channels = STBI_rgb_alpha);
    

    /* Creates an image object.
        @param vkContext: The application context.
        @param image: The image to be created.
        @param imgAllocation: The memory allocation for the image.
        @param width: The width of the image.
        @param height: The height of the image.
        @param depth: The depth of the image.
        @param imgFormat: The format of the image.
        @param imgTiling: The tiling mode of the image.
        @param imgUsageFlags: The usage flags for the image.
        @param imgAllocCreateInfo: The allocation create info for the image.
    */
    static void createImage(VulkanContext& vkContext, VkImage& image, VmaAllocation& imgAllocation, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VmaAllocationCreateInfo& imgAllocCreateInfo);


    /* Handles image layout transition.
		@param image: The image to be used in the image memory barrier.
        @param imgFormat: The format of the image.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
    */
    static void transitionImageLayout(VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout);
};