/* RenderPipeline.hpp - Manages the rendering pipeline.
*/

#pragma once

// Forward declaration
class BufferManager;

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

// Local
#include <ApplicationContext.hpp>
#include <Shaders/BufferManager.hpp>
#include <LoggingManager.hpp>
#include <MemoryManager.hpp>
#include <Constants.h>


class RenderPipeline {
public:
	RenderPipeline(VulkanContext& context, MemoryManager& memMgr, BufferManager& vertBuf, bool autoCleanup = true);
	~RenderPipeline();

	void init();
	void cleanup();

	/* Writes a command into a command buffer.
	* @param buffer: The buffer to be recorded into.
	* @param imageIndex: The index of the swap-chain image from which commands are recorded.
	*/
	void recordCommandBuffer(VkCommandBuffer& buffer, uint32_t imageIndex);


	/* Creates a framebuffer for each image in the swap-chain. */
	void createFrameBuffers();


	/* Destroys each swap-chain image's framebuffer. This should only be called when the swap-chain is recreated. */
	void destroyFrameBuffers();


	/* Gets a list of image framebuffers.
	* @return A vector of type VkFrameBuffer containing a framebuffer for every swap-chain image.
	*/
	inline std::vector<VkFramebuffer> getImageFrameBuffers() const { return imageFrameBuffers; };

	
	/* Creates a command pool.
	* @param vkContext: The application context.
	* @param memMgr: The application memory manager.
	* @param device: The logical device.
	* @param queueFamilyIndex: The index of the queue family for which the command pool is to be created.
	* @param flags (Default: VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT): The command pool flags.
	*/
	static VkCommandPool createCommandPool(VulkanContext& vkContext, MemoryManager& memMgr, VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);


	/* Allocates a command buffer vector. */
	void allocCommandBuffers(VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers);

private:
	bool cleanOnDestruction = true;
	VulkanContext& vkContext;
	BufferManager& vertexBuffer;

	MemoryManager& memoryManager;
	std::vector<uint32_t> framebufTaskIDs; // Stores framebuffer cleanup task IDs (used exclusively in the swap-chain recreation process)

	std::vector<VkFramebuffer> imageFrameBuffers;

	// Command pools manage the memory that is used to store the buffers
	// Command buffers are allocated from them
	VkCommandPool graphicsCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> graphicsCmdBuffers;

	VkCommandPool transferCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> transferCmdBuffers;

	// Synchronization
	std::vector<VkSemaphore> imageReadySemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	/* Creates synchronization objects. */
	void createSyncObjects();
};