/* VkDescriptorUtils.hpp - Manages descriptors, descriptor sets, descriptor set layouts, and descriptor pools.
*/

#pragma once

#include <vector>


#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/External/GLFWVulkan.hpp>


class VkDescriptorUtils {
public:
	static void CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool &descriptorPool, uint32_t poolSizeCount, VkDescriptorPoolSize *poolSizes, VkDescriptorPoolCreateFlags createFlags, uint32_t maxSets = 500);
};
