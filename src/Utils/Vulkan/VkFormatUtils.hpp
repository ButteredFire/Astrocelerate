/* VkFormatUtils.hpp - Utilities pertaining to image formats.
*/

#pragma once

#include <vector>

#include <External/GLFWVulkan.hpp>

#include <Core/Application/LoggingManager.hpp>


namespace VkFormatUtils {
    /* Does the (depth) format contain a stencil component? */
    inline bool FormatHasStencilComponent(VkFormat format) {
        return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
            (format == VK_FORMAT_D24_UNORM_S8_UINT);
    }


    /* Finds a supported image format.
        @param formats: The list of formats from which to find the format of best fit.
        @param imgTiling: The image tiling mode, used to determine format support.
        @param formatFeatures: The format's required features, used to determine the best format.

        @return The most suitable supported image format.
    */
    inline VkFormat FindSuppportedImageFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling imgTiling, VkFormatFeatureFlagBits formatFeatures) {
        for (const auto& format : formats) {
            VkFormatProperties formatProperties{};
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

            if (imgTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & formatFeatures) == formatFeatures) {
                return format;
            }

            if (imgTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & formatFeatures) == formatFeatures) {
                return format;
            }
        }


        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to find a suitable image format!");
    }


    /* Gets the most suitable image format for depth images.
        @return The most suitable image format.
    */
    inline VkFormat GetBestDepthImageFormat(VkPhysicalDevice& physicalDevice) {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlagBits formatFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        return FindSuppportedImageFormat(physicalDevice, candidates, imgTiling, formatFeatures);
    }
}