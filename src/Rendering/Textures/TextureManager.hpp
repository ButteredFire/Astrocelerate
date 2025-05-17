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
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <CoreStructs/Contexts.hpp>
#include <Core/GarbageCollector.hpp>

#include <Vulkan/VkBufferManager.hpp>

#include <Rendering/Pipelines/GraphicsPipeline.hpp>


class TextureManager {
public:
	TextureManager(VulkanContext& context);
	~TextureManager() = default;


    /* Creates a texture. 
		@param texSource: The source path of the texture.
        @param texImgFormat (Default: Surface format): The texture's image format.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
    */
    void createTexture(const char* texSource, VkFormat texImgFormat = VK_FORMAT_UNDEFINED, int channels = STBI_rgb_alpha);


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
        @param vkContext: The application context.
        @param image: The image to be used in the image memory barrier.
        @param imgFormat: The format of the image.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
    */
    static void switchImageLayout(VulkanContext& vkContext, VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout);


    /* Defines the pipeline source and destination stages as image layout transition rules.
        @param srcAccessMask: The source access mask to be defined.
        @param dstAccessMask: The destination access mask to be defined.
        @param srcStage: The pipeline source stage to be defined.
        @param dstStage: The pipeline destination stage to be defined.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
    */
    static void defineImageLayoutTransitionStages(VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
    VulkanContext& m_vkContext;

    std::shared_ptr<GarbageCollector> m_garbageCollector;

    VkImage m_textureImage = VK_NULL_HANDLE;
    VkFormat m_textureImageFormat = VkFormat();
    VmaAllocation m_textureImageAllocation = VK_NULL_HANDLE;

    VkImageView m_textureImageView = VK_NULL_HANDLE;

    VkSampler m_textureSampler = VK_NULL_HANDLE;


    /* Creates a texture image. 
        @param texSource: The source path of the texture.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
    */
    void createTextureImage(const char* texSource, int channels = STBI_rgb_alpha);


    /* Creates a texture image view. */
    void createTextureImageView();


    /* Creates a texture sampler.
        Samplers apply filtering, transformations, etc. to the raw texture and compute the final texels for the (fragment) shader to read.
        It allows for texture customization (e.g., interpolation, texture repeats, anisotropic filtering) and solves problems like over-/under-sampling.
    */
    void createTextureSampler();


	/* Copies the contents of a buffer to an image.
		@param buffer: The buffer to be copied from.
		@param image: The image to be copied to.
		@param width: The width of the image.
		@param height: The height of the image.
    */
    void copyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);
};