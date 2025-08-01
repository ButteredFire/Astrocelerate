/* VkDescriptorUtils.hpp - Manages descriptors, descriptor sets, descriptor set layouts, and descriptor pools.
*/

#pragma once

#include <External/GLFWVulkan.hpp>
#include <vector>

#include <Core/Application/GarbageCollector.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Core/Data/Contexts/VulkanContext.hpp>


class VkDescriptorUtils {
public:
	static void CreateDescriptorPool(VkDescriptorPool &descriptorPool, uint32_t poolSizeCount, VkDescriptorPoolSize *poolSizes, VkDescriptorPoolCreateFlags createFlags, uint32_t maxSets = 500);
};