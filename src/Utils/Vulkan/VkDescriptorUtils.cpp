#include "VkDescriptorUtils.hpp"


void VkDescriptorUtils::CreateDescriptorPool(VkDescriptorPool& descriptorPool, std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags createFlags) {
	std::shared_ptr<GarbageCollector> garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

	VkDescriptorPoolCreateInfo descPoolCreateInfo{};
	descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descPoolCreateInfo.pPoolSizes = poolSizes.data();
	descPoolCreateInfo.flags = createFlags;

	// TODO: See why the hell `maxSets` is the number of frames in flight
	// Specifies the maximum number of descriptor sets that can be allocated
	//descPoolCreateInfo.maxSets = maxDescriptorSetCount;
	descPoolCreateInfo.maxSets = 500;

	VkResult result = vkCreateDescriptorPool(g_vkContext.Device.logicalDevice, &descPoolCreateInfo, nullptr, &descriptorPool);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(descriptorPool) };
	task.vkObjects = { g_vkContext.Device.logicalDevice, descriptorPool };
	task.cleanupFunc = [device = g_vkContext.Device.logicalDevice, descriptorPool]() {
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	};

	garbageCollector->createCleanupTask(task);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create descriptor pool!");
	}
}