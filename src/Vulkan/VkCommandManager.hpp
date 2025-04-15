/* VkCommandManager.hpp - Manages command pools and command buffers.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// Local
#include <Core/ECSCore.hpp>
#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/GarbageCollector.hpp>
#include <Core/ApplicationContext.hpp>

#include <Engine/Components/RenderComponents.hpp>

#include <Shaders/BufferManager.hpp>

#include <Systems/RenderSystem.hpp>


class BufferManager;


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


class VkCommandManager {
public:
	VkCommandManager(VulkanContext& context);
	~VkCommandManager();

	void init();

	/* Writes commands into the command buffer to be used for rendering.
		@param buffer: The buffer to be recorded into.
		@param renderables: The component array of renderable entities.
		@param imageIndex: The index of the swap-chain image from which commands are recorded.
		@param currentFrame: The index of the frame currently being rendered.
	*/
	void recordRenderingCommandBuffer(VkCommandBuffer& buffer, ComponentArray<RenderableComponent>& renderables, uint32_t imageIndex, uint32_t currentFrame);


	/* Begins recording a single-use/anonymous command buffer for single-time commands.
		@param commandBufInfo: The command buffer configuration.

		@return The command buffer in question.
	*/
	static VkCommandBuffer beginSingleUseCommandBuffer(VulkanContext& vkContext, SingleUseCommandBufferInfo* commandBufInfo);


	/* Stops recording a single-use/anonymous command buffer and submit its data to the GPU.
		@param commandBufInfo: The command buffer configuration.
		@param cmdBuffer: The command buffer.
	*/
	static void endSingleUseCommandBuffer(VulkanContext& vkContext, SingleUseCommandBufferInfo* commandBufInfo, VkCommandBuffer& cmdBuffer);


	/* Creates a command pool.
		@param vkContext: The application context.
		@param device: The logical device.
		@param queueFamilyIndex: The index of the queue family for which the command pool is to be created.
		@param flags (Default: VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT): The command pool flags.
	*/
	static VkCommandPool createCommandPool(VulkanContext& vkContext, VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);


	/* Allocates a command buffer vector. */
	void allocCommandBuffers(VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers);

private:
	VulkanContext& vkContext;

	std::shared_ptr<BufferManager> bufferManager;
	std::shared_ptr<GarbageCollector> garbageCollector;


	// Command pools manage the memory that is used to store the buffers
	// Command buffers are allocated from them
	VkCommandPool graphicsCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> graphicsCmdBuffers;

	VkCommandPool transferCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> transferCmdBuffers;
};