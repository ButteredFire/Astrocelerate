/* RenderPipeline.cpp - Render pipeline implementation.
*/

#include "RenderPipeline.hpp"


RenderPipeline::RenderPipeline(VulkanContext& context, VertexBuffer& vertBuf) :
	vkContext(context),
	vertexBuffer(vertBuf) {}

RenderPipeline::~RenderPipeline() {
	cleanup();
}


void RenderPipeline::init() {
	createFrameBuffers();

	QueueFamilyIndices familyIndices = VkDeviceManager::getQueueFamilies(vkContext.physicalDevice, vkContext.vkSurface);
	graphicsCmdPool = createCommandPool(familyIndices.graphicsFamily.index.value());
	transferCmdPool = createCommandPool(familyIndices.transferFamily.index.value());

	allocCommandBuffers(graphicsCmdPool, graphicsCmdBuffers);
	vkContext.RenderPipeline.graphicsCmdBuffers = graphicsCmdBuffers;

	allocCommandBuffers(transferCmdPool, transferCmdBuffers);
	vkContext.RenderPipeline.transferCmdBuffers = transferCmdBuffers;

	vertexBuffer.init();
	createSyncObjects();
}


void RenderPipeline::cleanup() {
	Log::print(Log::INFO, __FUNCTION__, "Cleaning up...");

	for (const auto& buffer : imageFrameBuffers) {
		if (vkIsValid(buffer))
			vkDestroyFramebuffer(vkContext.logicalDevice, buffer, nullptr);
	}

	// Command buffers
	if (!graphicsCmdBuffers.empty())
		vkFreeCommandBuffers(vkContext.logicalDevice, graphicsCmdPool, static_cast<uint32_t>(graphicsCmdBuffers.size()), graphicsCmdBuffers.data());


	if (!transferCmdBuffers.empty())
		vkFreeCommandBuffers(vkContext.logicalDevice, transferCmdPool, static_cast<uint32_t>(transferCmdBuffers.size()), transferCmdBuffers.data());


	// Command pools
	if (vkIsValid(graphicsCmdPool))
		vkDestroyCommandPool(vkContext.logicalDevice, graphicsCmdPool, nullptr);

	if (vkIsValid(transferCmdPool))
		vkDestroyCommandPool(vkContext.logicalDevice, transferCmdPool, nullptr);


	// Synchronization objects
	for (size_t i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkIsValid(imageReadySemaphores[i]))
			vkDestroySemaphore(vkContext.logicalDevice, imageReadySemaphores[i], nullptr);
		
		if (vkIsValid(renderFinishedSemaphores[i]))
			vkDestroySemaphore(vkContext.logicalDevice, renderFinishedSemaphores[i], nullptr);

		if (vkIsValid(inFlightFences[i]))
			vkDestroyFence(vkContext.logicalDevice, inFlightFences[i], nullptr);
	}
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
		throw Log::RuntimeException(__FUNCTION__, "Failed to start recording command buffer!");
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
	clearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f }; // (0, 0, 0, 1) -> Black;
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
	viewport.x = 0.0f;
	viewport.y = 0.0f;
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
	VkBuffer vertexBuffers[] = {
		vertexBuffer.getBuffer()
	};
	VkDeviceSize offsets[] = {
		0
	};
	vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);

	std::vector<Vertex> vertexData = vertexBuffer.getVertexData();
	vkCmdDraw(buffer, static_cast<uint32_t>(vertexData.size()), 1, 0, 0);

	// End the render pass
	vkCmdEndRenderPass(buffer);

	// Stop recording the command buffer
	VkResult endCmdBufferResult = vkEndCommandBuffer(buffer);
	if (endCmdBufferResult != VK_SUCCESS) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Failed to record command buffer!");
	}
}



void RenderPipeline::createFrameBuffers() {
	imageFrameBuffers.resize(vkContext.swapChainImageViews.size());

	for (size_t i = 0; i < imageFrameBuffers.size(); i++) {
		if (vkContext.swapChainImageViews[i] == VK_NULL_HANDLE) {
			cleanup();
			throw Log::RuntimeException(__FUNCTION__, "Cannot read null image view!");
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
			throw Log::RuntimeException(__FUNCTION__, "Failed to create frame buffer!");
		}
	}
}


VkCommandPool RenderPipeline::createCommandPool(uint32_t queueFamilyIndex) {
	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allows command buffers to be re-recorded individually

	// Command buffers are executed by submitting them on a device queue.
	// Each command pool can only allocate command buffers that are submitted on a single type of queue.
	poolCreateInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool commandPool;
	VkResult result = vkCreateCommandPool(vkContext.logicalDevice, &poolCreateInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Failed to create command pool!");
	}

	return commandPool;
}


void RenderPipeline::allocCommandBuffers(VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers) {
	commandBuffers.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo bufferAllocInfo{};
	bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	bufferAllocInfo.commandPool = commandPool;

	// Specifies whether the command buffer is primary or secondary
	// BUFFER_LEVEL_PRIMARY: The buffer can be submitted to a queue for execution, but cannot be directly called from other command buffers.
	// BUFFER_LEVEL_SECONDARY: The buffer cannot be submitted directly, but can be called from primary command buffers.
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	bufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	VkResult result = vkAllocateCommandBuffers(vkContext.logicalDevice, &bufferAllocInfo, commandBuffers.data());
	if (result != VK_SUCCESS) {
		cleanup();
		throw Log::RuntimeException(__FUNCTION__, "Failed to allocate command buffers!");
	}
}


void RenderPipeline::createSyncObjects() {
	/* A note on synchronization:
	* - Since the GPU executes commands in parallel, and since each step in the frame-rendering process depends on the previous step (and the completion thereof), we must explicitly define an order of operations to prevent these steps from being executed concurrently (which results in unintended/undefined behavior). To that effect, we may use various synchronization primitives:
	*
	* - Semaphores: Used to synchronize queue operations within the GPU.
	* - Fences: Used to synchronize the CPU with the GPU.
	*
	* Detailed explanation:
	*
	* 1) SEMAPHORES
		* The semaphore is used to add order between queue operations (which is the work we submit to a queue (e.g., graphics/presentation queue), either in a command buffer or from within a function).
		* Semaphores are both used to order work either within the same queue or between different queues.
		*
		* There are two types of semaphores: binary, and timeline. We will be using binary semaphores.
			- The binary semaphore has a binary state: signaled OR unsignaled. On initialization, it is unsignaled. The binary semaphore can be used to order operations like this: To order 2 operations `opA` and `opB`, we configure `opA` so that it signals the binary semaphore on completion, and configure `opB` so that it only begins when the binary semaphore is signaled (i.e., `opB` will "wait" for that semaphore). After `opB` begins executing, the binary semaphore is reset to unsignaled to allow for future reuse.
		*
	* 2) FENCES
		* The fence, like the semaphore, is used to synchronize execution, but it is for the CPU (a.k.a., the "host").
		* We use the fence in cases where the host needs to know when the GPU has finished something.
		*
		* The fence, like the binary semaphore, is either in a signaled OR unsignaled state. When we want to execute something (i.e., a command buffer), we attach a fence to it and configure the fence to be signaled on its completion. Then, we must make the host wait for the fence to be signaled (i.e., halt/block any execution on the CPU until the fence is signaled) to guarantee that the work is completed before the host continues.
		*
		* In general, it is preferable to not block the host unless necessary. We want to feed the GPU and the host with useful work to do. Waiting on fences to signal is not useful work. Thus we prefer semaphores, or other synchronization primitives not yet covered, to synchronize our work. However, certain operations require the host to wait (e.g., rendering/drawing a frame, to ensure that the CPU waits until the GPU has finished processing the previous frame before starting to process the next one).
		*
		* Fences must be reset manually to put them back into the unsignaled state. This is because fences are used to control the execution of the host, and so the host gets to decide when to reset the fence. Contrast this to semaphores which are used to order work on the GPU without the host being involved.
	*/
	imageReadySemaphores.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	// Specifies the fence to be created already in the signal state.
	// We need to do this, because if the fence is created unsignaled (default), when we call drawFrame() in the Renderer for the first time, `vkWaitForFences` will wait for the fence to be signaled, only to wait indefinitely because the fence can only be signaled after a frame has finished rendering, and we are calling drawFrames() for the first time so there is no frame initially.
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		VkResult imageSemaphoreCreateResult = vkCreateSemaphore(vkContext.logicalDevice, &semaphoreCreateInfo, nullptr, &imageReadySemaphores[i]);
		VkResult renderSemaphoreCreateResult = vkCreateSemaphore(vkContext.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
		VkResult inFlightFenceCreateResult = vkCreateFence(vkContext.logicalDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]);

		if (imageSemaphoreCreateResult != VK_SUCCESS || renderSemaphoreCreateResult != VK_SUCCESS) {
			cleanup();
			throw Log::RuntimeException(__FUNCTION__, "Failed to create semaphores for a frame!");
		}

		if (inFlightFenceCreateResult != VK_SUCCESS) {
			cleanup();
			throw Log::RuntimeException(__FUNCTION__, "Failed to create in-flight fence for a frame!");
		}
	}

	vkContext.RenderPipeline.imageReadySemaphores = imageReadySemaphores;
	vkContext.RenderPipeline.renderFinishedSemaphores = renderFinishedSemaphores;
	vkContext.RenderPipeline.inFlightFences = inFlightFences;
}

