/* TextureManager.hpp - Manages textures and related operations (e.g., creation, modification).
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>
#include <vk_mem_alloc.h>

// STB
#include <stb/stb_image.h>

// C++ STLs
#include <iostream>

// Other
#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Application/GarbageCollector.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Geometry.hpp>
#include <Core/Data/Contexts/VulkanContext.hpp>

#include <Vulkan/VkBufferManager.hpp>

#include <Utils/SystemUtils.hpp>
#include <Utils/Vulkan/VkFormatUtils.hpp>


// Texture information. Intended for internal use within TextureManager only!
struct TextureInfo {
    int width;
    int height;
    VkImage image;
    VkImageLayout imageLayout;
};


class TextureManager {
public:
	TextureManager();
	~TextureManager() = default;


    /* Creates an independent texture.
    	@param texSource: The source path of the texture.
        @param texImgFormat (Default: Surface format): The texture's image format.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
    
        @return The created texture's properties.
    */
    Geometry::Texture createIndependentTexture(const std::string &texSource, VkFormat texImgFormat = VK_FORMAT_UNDEFINED, int channels = STBI_rgb_alpha);


    /* Creates a texture that is a part of the global texture array. 
		@param texSource: The source path of the texture.
        @param texImgFormat (Default: Surface format): The texture's image format.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
    
        @return The created texture's index into an internally managed global texture array.
    */
    uint32_t createIndexedTexture(const std::string& texSource, VkFormat texImgFormat = VK_FORMAT_UNDEFINED, int channels = STBI_rgb_alpha);


    /* Creates an image object.
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
    static void createImage(VkImage& image, VmaAllocation& imgAllocation, uint32_t width, uint32_t height, uint32_t depth, VkFormat imgFormat, VkImageTiling imgTiling, VkImageUsageFlags imgUsageFlags, VmaAllocationCreateInfo& imgAllocCreateInfo);


    /* Handles image layout transition.
        @param image: The image to be used in the image memory barrier.
        @param imgFormat: The format of the image.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
    */
    static void switchImageLayout(VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout);


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
    std::shared_ptr<GarbageCollector> m_garbageCollector;
    std::shared_ptr<EventDispatcher> m_eventDispatcher;
    
    uint32_t m_placeholderTextureIndex = 0;

    std::unordered_map<std::string, uint32_t> m_texturePathToIndexMap; // Maps path to its index in the descriptor infos vector.
    std::vector<VkDescriptorImageInfo> m_textureDescriptorInfos;       // Contains all image views and samplers for the global array.

    // Keeps track of unique samplers for reuse when new textures are loaded (keyed by sampler create info hash).
    std::unordered_map<size_t, VkSampler> m_uniqueSamplers;

    // This is set to True when all pipelines are initialized.
    // Before this, new textures are added to a deference list, and the texture array descriptor set will be updated when it is valid.
    // After this, the texture array descriptor set will be immeadiately updated upon the creation of new textures.
    bool m_textureArrayDescSetIsValid = false;
    

    void bindEvents();


    /* Updates the global texture array descriptor set.
        @param texIndex: The index of the texture to be updated.
        @param texImageInfo: The descriptor image info containing the image view and sampler for the texture.
    */
    void updateTextureArrayDescriptorSet(uint32_t texIndex, const VkDescriptorImageInfo &texImageInfo);

    
    /* Creates a texture image.
        @param imgFormat: The texture's image format.
        @param texSource: The source path of the texture.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.

        @return The texture information.
    */
    TextureInfo createTextureImage(VkFormat imgFormat, const char* texSource, int channels = STBI_rgb_alpha);


    /* Creates a texture image view.
        @param image: The image for which the image view is to be created.
        @param imgFormat: The image's format.

        @return The texture image view.
    */
    VkImageView createTextureImageView(VkImage image, VkFormat imgFormat);


    /* Creates a texture sampler.
        Samplers apply filtering, transformations, etc. to the raw texture and compute the final texels for the (fragment) shader to read.
        It allows for texture customization (e.g., interpolation, texture repeats, anisotropic filtering) and solves problems like over-/under-sampling.

        NOTE: If maxAnisotropy is set to FLT_MAX (maximum float value), then the anisotropy limit value for the current logical device will be used.

        @return The created sampler.
    */
    VkSampler createTextureSampler(
        VkFilter magFilter, VkFilter minFilter,
        VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
        VkBorderColor borderColor,
        VkBool32 anisotropyEnable, float maxAnisotropy,
        VkBool32 unnormalizedCoordinates,
        VkBool32 compareEnable, VkCompareOp compareOp,
        VkSamplerMipmapMode mipmapMode, float mipLodBias, float minLod, float maxLod
    );


	/* Copies the contents of a buffer to an image.
		@param buffer: The buffer to be copied from.
		@param image: The image to be copied to.
		@param width: The width of the image.
		@param height: The height of the image.
    */
    void copyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);
};