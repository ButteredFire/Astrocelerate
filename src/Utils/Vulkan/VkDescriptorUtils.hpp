/* VkDescriptorUtils.hpp - Manages descriptors, descriptor sets, descriptor set layouts, and descriptor pools.
*/

#pragma once

#include <External/GLFWVulkan.hpp>
#include <vector>

#include <Core/Application/ResourceManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>




class VkDescriptorUtils {
public:
	static void CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool &descriptorPool, uint32_t poolSizeCount, VkDescriptorPoolSize *poolSizes, VkDescriptorPoolCreateFlags createFlags, uint32_t maxSets = 500);
};