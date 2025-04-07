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
#include <ApplicationContext.hpp>
#include <Shaders/BufferManager.hpp>
#include <LoggingManager.hpp>
#include <MemoryManager.hpp>
#include <ServiceLocator.hpp>
#include <Constants.h>


class BufferManager;


class VkCommandManager {
public:
	VkCommandManager(VulkanContext& context);
	~VkCommandManager();

	void init();

	/* Writes a command into a command buffer.
		@param buffer: The buffer to be recorded into.
		@param imageIndex: The index of the swap-chain image from which commands are recorded.
		@param currentFrame: The index of the frame currently being rendered.
	*/
	void recordCommandBuffer(VkCommandBuffer& buffer, uint32_t imageIndex, uint32_t currentFrame);


	/* Begins recording a single-use/anonymous command buffer for single-time commands.
		@return The command buffer in question.
	*/
	VkCommandBuffer beginSingleUseCommandBuffer();

	/* Stops recording a single-use/anonymous command buffer.
		@param cmdBuffer: The command buffer in question.
	*/
	void endSingleUseCommandBuffer(VkCommandBuffer& cmdBuffer);


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
	std::shared_ptr<MemoryManager> memoryManager;


	// Command pools manage the memory that is used to store the buffers
	// Command buffers are allocated from them
	VkCommandPool graphicsCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> graphicsCmdBuffers;

	VkCommandPool transferCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> transferCmdBuffers;
};