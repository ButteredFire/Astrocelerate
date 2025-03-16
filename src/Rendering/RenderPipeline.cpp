/* RenderPipeline.cpp - Render pipeline implementation.
*/

#include "RenderPipeline.hpp"


RenderPipeline::RenderPipeline(VulkanContext& context) :
	vkContext(context) {}

RenderPipeline::~RenderPipeline() {
	cleanup();
}


void RenderPipeline::init() {
	createFrameBuffers();
	createCommandPools();
}


void RenderPipeline::cleanup() {
	for (const auto& buffer : imageFrameBuffers) {
		vkDestroyFramebuffer(vkContext.logicalDevice, buffer, nullptr);
	}

	vkDestroyCommandPool(vkContext.logicalDevice, commandPool, nullptr);
}


void RenderPipeline::recordCommandBuffer(VkCommandBuffer& buffer, uint32_t imageIndex) {
	// Specifies details about how the passed-in command buffer will be used before beginning
	VkCommandBufferBeginInfo bufferBeginInfo{};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		/* Available flags: VK_COMMAND_BUFFER_USAGE_...
		* ...ONE_TIME_SUBMIT_BIT: The command buffer will be re-recorded right after executing it once
		* ...RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass
		* ...SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending execution
		*/
	bufferBeginInfo.flags = 0;

		// pInheritanceInfo is only relevant for secondary command buffers
		// It specifies which state the secondary buffer shold inherit from the primary buffer that is calling it
	bufferBeginInfo.pInheritanceInfo = nullptr; 

	// NOTE: vkBeginCommandBuffer will implicitly reset the framebuffer if it has already been recorded before
	VkResult beginCmdBufferResult = vkBeginCommandBuffer(buffer, &bufferBeginInfo);
	if (beginCmdBufferResult != VK_SUCCESS) {
		cleanup();
		throw std::runtime_error("Failed to start recording command buffer!");
	}

	// Starts a render pass to start recording the drawing commands
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkContext.GraphicsPipeline.renderPass;
	renderPassBeginInfo.framebuffer = imageFrameBuffers[imageIndex];

		// Defines the render area (from (0, 0) to (extent.width, extent.height))
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = vkContext.swapChainExtent;

		// Defines the clear values to use (we must specify this since the color attachment's load operation is VK_ATTACHMENT_LOAD_OP_CLEAR)
	VkClearValue clearColor{};
	VkClearColorValue colorValue = {0.0f, 0.0f, 0.0f, 1.0f}; // (0, 0, 0, 1) -> Black
	clearColor.color = colorValue;
	clearColor.depthStencil = VkClearDepthStencilValue(); // Null for now (if depth stencil is implemented, you must also specify the color attachment load and store operations before specifying the clear value here)

	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	/* The final parameter controls how the drawing commands within the render pass will be provided. It can have 2 values: VK_SUBPASS_...
	* ...CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed
	* ..SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
	*/
	vkCmdBeginRenderPass(buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


	// Commands are now ready to record. Functions that record commands begin with the vkCmd prefix.

	// Binds graphics pipeline
		// The 2nd parameter specifies the pipeline type (in this case, it's the graphics pipeline)
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.GraphicsPipeline.pipeline);

	// Specify viewport and scissor states (since they're dynamic states)
		// Viewport
	VkViewport viewport{};
	viewport.x = viewport.y = 0.0f;
	viewport.width = static_cast<float>(vkContext.swapChainExtent.width);
	viewport.height = static_cast<float>(vkContext.swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(buffer, 0, 1, &viewport);

		// Scissor
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = vkContext.swapChainExtent;
	vkCmdSetScissor(buffer, 0, 1, &scissor);

	// Draw
	/* vkCmdDraw parameters (Vulkan specs):
	* commandBuffer: The command buffer to record the draw commands into
	* vertexCount: The number of vertices to draw
	* instanceCount: The number of instances to draw (useful for instanced rendering)
	* firstVertex: The index of the first vertex to draw. It is used as an offset into the vertex buffer, and defines the lowest value of gl_VertexIndex.
	* firstInstance: The instance ID of the first instance to draw. It is used as an offset for instanced rendering, and defines the lowest value of gl_InstanceIndex.
	*/
	vkCmdDraw(buffer, 3, 1, 0, 0);

	// End the render pass
	vkCmdEndRenderPass(buffer);

	// Stop recording the command buffer
	VkResult endCmdBufferResult = vkEndCommandBuffer(buffer);
	if (endCmdBufferResult != VK_SUCCESS) {
		cleanup();
		throw std::runtime_error("Failed to record command buffer!");
	}
}



void RenderPipeline::createFrameBuffers() {
	imageFrameBuffers.resize(vkContext.swapChainImageViews.size());

	for (uint32_t i = 0; i < vkContext.swapChainImageViews.size(); i++) {
		if (vkContext.swapChainImageViews[i] == VK_NULL_HANDLE) {
			cleanup();
			throw std::runtime_error("Cannot read null image view!");
		}

		VkImageView attachments[] = {
			vkContext.swapChainImageViews[i]
		};

		VkFramebufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		bufferCreateInfo.renderPass = vkContext.GraphicsPipeline.renderPass;
		bufferCreateInfo.attachmentCount = (sizeof(attachments) / sizeof(VkImageView));
		bufferCreateInfo.pAttachments = attachments;
		bufferCreateInfo.width = vkContext.swapChainExtent.width;
		bufferCreateInfo.height = vkContext.swapChainExtent.height;
		bufferCreateInfo.layers = 1; // Refer to swapChainCreateInfo.imageArrayLayers in VkSwapchainManager

		VkResult result = vkCreateFramebuffer(vkContext.logicalDevice, &bufferCreateInfo, nullptr, &imageFrameBuffers[i]);
		if (result != VK_SUCCESS) {
			cleanup();
			throw std::runtime_error("Failed to create frame buffer!");
		}
	}
}


void RenderPipeline::createCommandPools() {
	QueueFamilyIndices familyIndices = VkDeviceManager::getQueueFamilies(vkContext.physicalDevice, vkContext.vkSurface);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allows command buffers to be re-recorded individually

	// Command buffers are executed by submitting them on a device queue.
	// Each command pool can only allocate command buffers that are submitted on a single type of queue.
	// We're going to record commands for drawing, which is why we've chosen the graphics queue family.
	poolCreateInfo.queueFamilyIndex = familyIndices.graphicsFamily.index.value();

	VkResult result = vkCreateCommandPool(vkContext.logicalDevice, &poolCreateInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		cleanup();
		throw std::runtime_error("Failed to create command pool!");
	}
}


void RenderPipeline::createCommandBuffers() {
	VkCommandBufferAllocateInfo bufferAllocInfo{};
	bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	bufferAllocInfo.commandPool = commandPool;

	// Specifies whether the command buffer is primary or secondary
	// BUFFER_LEVEL_PRIMARY: The buffer can be submitted to a queue for execution, but cannot be directly called from other command buffers.
	// BUFFER_LEVEL_SECONDARY: The buffer cannot be submitted directly, but can be called from primary command buffers.
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	// Allocate 1 command buffer
	bufferAllocInfo.commandBufferCount = 1;

	VkResult result = vkAllocateCommandBuffers(vkContext.logicalDevice, &bufferAllocInfo, &commandBuffer);
	if (result != VK_SUCCESS) {
		cleanup();
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

