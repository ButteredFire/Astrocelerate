/* VkCommandManager.hpp - Manages command pools and command buffers.
*/

#pragma once

// GLFW & Vulkan
#include <glfw_vulkan.hpp>

// C++ STLs
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

// Local
#include <Core/ECS.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>
#include <Core/GarbageCollector.hpp>
#include <CoreStructs/Contexts/VulkanContext.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Vulkan/VkSyncManager.hpp>

#include <Systems/RenderSystem.hpp>


// A structure that stores the configuration for single-use command buffers
struct SingleUseCommandBufferInfo {
	VkCommandPool commandPool;		// The command pool from which the command buffer is allocated.
	VkQueue queue;					// The queue to which the command buffer's recorded data is submitted (and for which the command pool is allocated).

	VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;							// The buffer level.
	VkCommandBufferUsageFlags bufferUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;   // The buffer usage flags.
	VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr;									// The pointer to the buffer's inheritance info

	VkFence fence = VK_NULL_HANDLE;
	std::vector<VkSemaphore> waitSemaphores = {};
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	std::vector<VkSemaphore> signalSemaphores = {};

	bool usingSingleUseFence = false;		// Is the fence being used (if it is) single-use?
	bool autoSubmit = true;					// Automatically submit after ending buffer recording?
	bool freeAfterSubmit = true;			// Automatically free the buffer after submitting?
};


// A structure that stores command pool creation information.
struct CommandPoolCreateInfo {
	VkDevice logicalDevice;					// The logical device from which to create the command pool.
	uint32_t queueFamilyIndex;				// The index of the queue family for which the command pool is to be created.
	VkCommandPoolCreateFlags flags;			// The command pool creation flags.
};

	// Custom equality operator and hash function for CommandPoolCreateInfo to be used as a key in hashmaps
inline bool operator==(const CommandPoolCreateInfo& lhs, const CommandPoolCreateInfo& rhs) {
	return (lhs.logicalDevice == rhs.logicalDevice) && (lhs.queueFamilyIndex == rhs.queueFamilyIndex) && (lhs.flags == rhs.flags);
}
namespace std {
	template<>
	struct hash<CommandPoolCreateInfo> {
		inline std::size_t operator()(const CommandPoolCreateInfo& info) const noexcept {
			// Combines the hash values of the members
			return std::hash<VkDevice>()(info.logicalDevice) ^
				std::hash<uint32_t>()(info.queueFamilyIndex) ^
				std::hash<VkCommandPoolCreateFlags>()(info.flags);
		}
	};
}


class VkCommandManager {
public:
	VkCommandManager();
	~VkCommandManager();

	void init();

	/* Writes commands into the command buffer to be used for rendering.
		@param buffer: The buffer to be recorded into.
		@param imageIndex: The index of the swap-chain image from which commands are recorded.
		@param currentFrame: The index of the frame currently being rendered.
	*/
	void recordRenderingCommandBuffer(VkCommandBuffer& buffer, uint32_t imageIndex, uint32_t currentFrame);


	/* Begins recording a single-use/anonymous command buffer for single-time commands.
		@param commandBufInfo: The command buffer configuration.

		@return The command buffer in question.
	*/
	static VkCommandBuffer beginSingleUseCommandBuffer(SingleUseCommandBufferInfo* commandBufInfo);


	/* Stops recording a single-use/anonymous command buffer and submit its data to the GPU.
		@param commandBufInfo: The command buffer configuration.
		@param cmdBuffer: The command buffer.
	*/
	static void endSingleUseCommandBuffer(SingleUseCommandBufferInfo* commandBufInfo, VkCommandBuffer& cmdBuffer);


	/* Creates a command pool.
		@param device: The logical device.
		@param queueFamilyIndex: The index of the queue family for which the command pool is to be created.
		@param flags (Default: VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT): The command pool flags.

		@return EITHER A new command pool (if the command pool has unique creation parameters), OR an existing command pool (if all of its creation parameters are the same as the ones passed in).
	*/
	static VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);


	/* Allocates a command buffer vector. */
	void allocCommandBuffers(VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers);

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<GarbageCollector> m_garbageCollector;

	// Command pools manage the memory that is used to store the buffers
	// Command buffers are allocated from them
	VkCommandPool m_graphicsCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_graphicsCmdBuffers;

	VkCommandPool m_transferCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_transferCmdBuffers;

	// Primarily used in command pool creation to implicitly return an existing command pool if the creation parameters matches its own.
	inline static std::unordered_map<CommandPoolCreateInfo, VkCommandPool> cmdPoolMappings;
};