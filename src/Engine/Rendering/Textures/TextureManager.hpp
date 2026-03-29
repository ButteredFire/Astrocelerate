/* TextureManager.hpp - Manages textures and related operations (e.g., creation, modification).
*/

#pragma once

#include <iostream>

#include <vk_mem_alloc.h>
#include <stb/stb_image.h>


#include <Core/Data/Constants.h>
#include <Core/Utils/SystemUtils.hpp>
#include <Core/Utils/FilePathUtils.hpp>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Threading/ThreadManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/Utils/VkImageUtils.hpp>
#include <Platform/Vulkan/Utils/VkFormatUtils.hpp>
#include <Platform/Vulkan/Utils/VkBufferUtils.hpp>
#include <Platform/Vulkan/Utils/VkCommandUtils.hpp>

#include <Engine/Registry/Event/EventDispatcher.hpp>
#include <Engine/Rendering/Data/Geometry.hpp>



class TextureManager {
public:
	TextureManager(const Ctx::VkRenderDevice *renderDeviceCtx);
	~TextureManager() = default;


    /* Sets the global texture array.
		@param texArrayDescriptorSet: The descriptor set for the global texture array.
		@param renderPass: The render pass for which the texture array will be used (used to determine the appropriate image layout for the textures in the array).
    */
    inline void setTextureArray(VkDescriptorSet texArrayDescriptorSet, VkRenderPass renderPass) {
		m_texArrayDescriptorSet = texArrayDescriptorSet;
		renderPass = renderPass;
    }


    /* Creates a normal texture (a texture that is not part of the global texture array, and thus is not used in shaders).
    	@param texSource: The source path of the texture.
        @param texImgFormat (Default: Surface format): The texture's image format.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
    
        @return The created texture's properties.
    */
    Geometry::Texture createTexture(const std::string &texSource, VkFormat texImgFormat = VK_FORMAT_UNDEFINED, int channels = STBI_rgb_alpha);


    /* Reserves a texture for the global texture array (bindless texture descriptor set).
        @note: This function only allocates a blank slot for this texture. Actual texture creation happens when flushReservedTextures is called.
        @note: Use reserveTexture only in worker threads. To create a texture in the main thread, use createTexture instead.

        @param texSource: The source path of the texture.
        @param texImgFormat (Default: Surface format): The texture's image format.
        @param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
    
        @return The reserved texture's index into the global texture array.
    */
    uint32_t reserveTexture(const std::string &texSource, VkFormat texImgFormat = VK_FORMAT_UNDEFINED, int channels = STBI_rgb_alpha);


    /* Flushes all reserved textures by creating them and updating the global texture array descriptor set accordingly.
        @note: This should be called after all reserved textures are created (e.g., when the worker thread calling reserveTexture joins).
    */
    void flushReservedTextures();


    /* Handles image layout transition.
		@param renderDevice: The current render device context.
        @param image: The image to be used in the image memory barrier.
        @param imgFormat: The format of the image.
        @param oldLayout: The old image layout.
        @param newLayout: The new image layout.
        @param useSecondaryCmdBuf (Default: False): Whether to record the image transition into a secondary command buffer (True), or into a primary command buffer (False). If True, the secondary command buffer will NOT be auto-submitted, and MUST be done manually (i.e., via vkQueueSubmit).
        @param pSecondaryCmdBuf: The secondary command buffer to be recorded to.
        @param pInheritanceInfo: The secondary command buffer's inheritance info.
    */
    static void SwitchImageLayout(const Ctx::VkRenderDevice *renderDevice, VkImage image, VkFormat imgFormat, VkImageLayout oldLayout, VkImageLayout newLayout, bool useSecondaryCmdBuf = false, VkCommandBuffer *pSecondaryCmdBuf = nullptr, VkCommandBufferInheritanceInfo *pInheritanceInfo = nullptr);


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

    const Ctx::VkRenderDevice *m_renderDeviceCtx;
    

    // Texture data
    VkDescriptorSet m_texArrayDescriptorSet;
    VkRenderPass m_renderPass;

        // Maps path to its index in the descriptor infos vector.
    std::unordered_map<std::string, uint32_t> m_texturePathToIndexMap;

        // Contains all image views and samplers for the global array. If an element does not contain a VkDescriptorImageInfo, it is a placeholder for a deferred texture.
    std::vector<std::optional<VkDescriptorImageInfo>> m_textureDescriptorInfos;

        // Keeps track of unique samplers for reuse when new textures are loaded (keyed by sampler create info hash).
    std::unordered_map<size_t, VkSampler> m_uniqueSamplers;

    struct _TextureInfo {
        int width;
        int height;
        VkImage image;
        VkImageLayout imageLayout;
    };

    struct _IndexedTextureProps {
        uint32_t index;
        std::string texSource;
        VkFormat texImgFormat;
        int channels;
    };
    std::vector<_IndexedTextureProps> m_deferredTextureProps;
    

    void bindEvents();

    
    /* Creates a texture image.
        @param imgFormat: The texture's image format.
        @param texSource: The source path of the texture.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.

        @return The texture information.
    */
    _TextureInfo createTextureImage(VkFormat imgFormat, const char* texSource, int channels = STBI_rgb_alpha);


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