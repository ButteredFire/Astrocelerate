/* SystemUtils.hpp - Utilities pertaining to system-level operations.
*/

#pragma once

#include <xhash>
#include <concepts>
#include <vector>

#include <glfw_vulkan.hpp>

#include <Core/LoggingManager.hpp>


namespace SystemUtils {
    // Concept: Supports division by a double
    template<typename T>
    concept DivisibleByDouble = requires(T a, double b) {
        { a / b } -> std::convertible_to<T>;
    };


    // Concept: Supports multiplication by a double
    template<typename T>
    concept MultipliableByDouble = requires(T a, double b) {
        { a * b } -> std::convertible_to<T>;
    };


    /* Combines multiple hash values into a single hash value. 
		This function uses the std::hash function to generate a hash for the value and combines it with the seed using a bitwise XOR operation and some arithmetic operations.
		
        @tparam T: The type of the value to be combined.
		
        @param seed: The initial hash value.
		@param value: The value to be combined with the hash.
    */
    template <typename T>
    void CombineHash(std::size_t& seed, const T& value) {
        std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }


	/* Aligns a given size to the nearest multiple of the specified alignment.
	   @param size: The size to be aligned.
	   @param alignment: The alignment value.

	   @return: The aligned size.
    */
    inline size_t Align(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }


    /* Gets the most suitable image format for depth images.
        @return The most suitable image format.
    */
    VkFormat GetBestDepthImageFormat(VkPhysicalDevice& physicalDevice) {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlagBits formatFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        return SystemUtils::FindSuppportedImageFormat(physicalDevice, candidates, imgTiling, formatFeatures);
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
}