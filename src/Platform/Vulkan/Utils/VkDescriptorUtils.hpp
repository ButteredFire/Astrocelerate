/* VkDescriptorUtils - Manages descriptors, descriptor sets, descriptor set layouts, and descriptor pools.
*/

#pragma once

#include <vector>
#include <memory>


#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/External/GLFWVulkan.hpp>


namespace VkDescriptorUtils {
	inline void CreateDescriptorPool(VkDevice logicalDevice, VkDescriptorPool &descriptorPool, uint32_t poolSizeCount, VkDescriptorPoolSize *poolSizes, VkDescriptorPoolCreateFlags createFlags, uint32_t maxSets = 500) {
		std::shared_ptr<CleanupManager> cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);

		VkDescriptorPoolCreateInfo descPoolCreateInfo{};
		descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCreateInfo.poolSizeCount = poolSizeCount;
		descPoolCreateInfo.pPoolSizes = poolSizes;
		descPoolCreateInfo.flags = createFlags;

		// TODO: Adjust `maxSets` depending on scenario instead of an arbitrary value like 500.
		// Specifies the maximum number of descriptor sets that can be allocated
		//descPoolCreateInfo.maxSets = maxDescriptorSetCount;
		descPoolCreateInfo.maxSets = maxSets;

		VkResult result = vkCreateDescriptorPool(logicalDevice, &descPoolCreateInfo, nullptr, &descriptorPool);

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(descriptorPool) };
		task.vkHandles = { descriptorPool };
		task.cleanupFunc = [logicalDevice, descriptorPool]() {
			vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
			};

		cleanupManager->createCleanupTask(task);


		if (result != VK_SUCCESS) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create descriptor pool!");
		}
	}
}
