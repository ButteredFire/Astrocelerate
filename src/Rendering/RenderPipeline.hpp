/* RenderPipeline.hpp - Manages the rendering pipeline.
*/

#pragma once

// Forward declaration
class VertexBuffer;

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
#include <Shaders/VertexBuffer.hpp>
#include <LoggingManager.hpp>
#include <Constants.h>


class RenderPipeline {
public:
	RenderPipeline(VulkanContext& context, VertexBuffer& vertBuf, bool autoCleanup = true);
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

	
	/* Creates a command pool. */
	VkCommandPool createCommandPool(uint32_t queueFamilyIndex);


	/* Allocates a command buffer vector. */
	void allocCommandBuffers(VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers);

private:
	bool cleanOnDestruction = true;
	VulkanContext& vkContext;
	VertexBuffer& vertexBuffer;

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