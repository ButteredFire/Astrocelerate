/* RenderPipeline.hpp - Manages the rendering pipeline.
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

// Local
#include <Vulkan/VulkanContexts.hpp>
#include <Vulkan/VkDeviceManager.hpp>
#include <LoggingManager.hpp>
#include <Constants.h>


class RenderPipeline {
public:
	RenderPipeline(VulkanContext& context);
	~RenderPipeline();

	void init();
	void cleanup();

	/* Writes a command into a command buffer. */
	void recordCommandBuffer(VkCommandBuffer& buffer, uint32_t imageIndex);


	/* Creates a framebuffer for each image in the swap-chain. */
	void createFrameBuffers();


	/* Gets a list of image framebuffers.
	* @return A vector of type VkFrameBuffer containing a framebuffer for every swap-chain image.
	*/
	inline std::vector<VkFramebuffer> getImageFrameBuffers() const { return imageFrameBuffers; };

private:
	VulkanContext& vkContext;
	std::vector<VkFramebuffer> imageFrameBuffers;

	// Command pools manage the memory that is used to store the buffers
	// Command buffers are allocated from them
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;

	// Synchronization
	std::vector<VkSemaphore> imageReadySemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	
	/* Creates the command pools. */
	void createCommandPools();

	
	/* Creates the command buffers. */
	void createCommandBuffers();


	/* Creates synchronization objects. */
	void createSyncObjects();
};