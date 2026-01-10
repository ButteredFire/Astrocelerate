/* TextureManager.hpp - Manages textures and related operations (e.g., creation, modification).
*/

#pragma once

#include <iostream>

#include <vk_mem_alloc.h>
#include <stb/stb_image.h>


#include <Core/Data/Constants.h>
#include <Core/Utils/SystemUtils.hpp>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Threading/ThreadManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>

#include <Platform/Vulkan/VkBufferManager.hpp>
#include <Platform/Vulkan/Utils/VkFormatUtils.hpp>

#include <Engine/Rendering/Data/Geometry.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>


// Texture information. Intended for internal use within TextureManager only!
struct TextureInfo {
    int width;
    int height;
    VkImage image;
    VkImageLayout imageLayout;
};


class TextureManager {
public:
	TextureManager(VkCoreResourcesManager *coreResources);
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


    /* Handles image layout transition.
        @param image: The image to be used in the image memory barrier.
        @param imgFormat: The format of the image.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
        @param useSecondaryCmdBuf (Default: False): Whether to record the image transition into a secondary command buffer (True), or into a primary command buffer (False). If True, the secondary command buffer will NOT be auto-submitted, and MUST be done manually (i.e., via vkQueueSubmit).
        @param pSecondaryCmdBuf: The secondary command buffer to be recorded to.
        @param pInheritanceInfo: The secondary command buffer's inheritance info.
    */
    static void SwitchImageLayout(VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout, bool useSecondaryCmdBuf = false, VkCommandBuffer *pSecondaryCmdBuf = nullptr, VkCommandBufferInheritanceInfo *pInheritanceInfo = nullptr);


    /* Defines the pipeline source and destination stages as image layout transition rules.
        @param srcAccessMask: The source access mask to be defined.
        @param dstAccessMask: The destination access mask to be defined.
        @param srcStage: The pipeline source stage to be defined.
        @param dstStage: The pipeline destination stage to be defined.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
    */
    static void DefineImageLayoutTransitionStages(VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
    std::shared_ptr<CleanupManager> m_cleanupManager;
    std::shared_ptr<EventDispatcher> m_eventDispatcher;

    VkCoreResourcesManager *m_coreResources;
    

    // Texture data
    VkRenderPass m_offscreenPipelineRenderPass;
    VkDescriptorSet m_texArrayDescriptorSet;

    uint32_t m_placeholderTextureIndex = 0;

        // Maps path to its index in the descriptor infos vector.
    std::unordered_map<std::string, uint32_t> m_texturePathToIndexMap;

        // Contains all image views and samplers for the global array. If an element does not contain a VkDescriptorImageInfo, it is a placeholder for a deferred texture.
    std::vector<std::optional<VkDescriptorImageInfo>> m_textureDescriptorInfos;

        // Keeps track of unique samplers for reuse when new textures are loaded (keyed by sampler create info hash).
    std::unordered_map<size_t, VkSampler> m_uniqueSamplers;


    // Session data
        // If the scene is not ready (i.e., its resources are not initialized yet), indexed textures should not be updated.
    bool m_sceneReady = false;

    struct _IndexedTextureProps {
        std::string texSource;
        VkFormat texImgFormat;
        int channels;
    };
    std::vector<_IndexedTextureProps> m_deferredTextureProps;
    

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