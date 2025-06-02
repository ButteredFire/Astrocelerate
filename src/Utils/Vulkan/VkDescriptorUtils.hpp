/* VkDescriptorUtils.hpp - Manages descriptors, descriptor sets, descriptor set layouts, and descriptor pools.
*/

#pragma once

#include <glfw_vulkan.hpp>
#include <vector>

#include <Core/GarbageCollector.hpp>
#include <Core/ServiceLocator.hpp>

#include <CoreStructs/Contexts.hpp>


class VkDescriptorUtils {
public:
	static void CreateDescriptorPool(VulkanContext& vkContext, VkDescriptorPool& descriptorPool, std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags createFlags);
};