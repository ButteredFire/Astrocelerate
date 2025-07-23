#include "PresentPipeline.hpp"


PresentPipeline::PresentPipeline() {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_bufferManager = ServiceLocator::GetService<VkBufferManager>(__FUNCTION__);


	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}



void PresentPipeline::init() {

	initViewportState();			// Viewport state

	initRasterizationState();		// Rasterization state

	initColorBlendingState();		// Blending state

	createRenderPass();

	// Post-initialization: Data is ready to be used for framebuffer creation
	m_eventDispatcher->publish(Event::PresentPipelineInitialized{});
}


void PresentPipeline::createGraphicsPipeline() {
	/*
	PipelineBuilder builder;
	//builder.dynamicStateCreateInfo = m_dynamicStateCreateInfo;
	//builder.inputAssemblyCreateInfo = m_inputAssemblyCreateInfo;
	builder.viewportStateCreateInfo = &m_viewportStateCreateInfo;
	builder.rasterizerCreateInfo = &m_rasterizerCreateInfo;
	//builder.multisampleStateCreateInfo = m_multisampleStateCreateInfo;
	//builder.depthStencilStateCreateInfo = m_depthStencilStateCreateInfo;
	builder.colorBlendStateCreateInfo = &m_colorBlendCreateInfo;
	//builder.tessellationStateCreateInfo = m_tessStateCreateInfo;
	//builder.vertexInputStateCreateInfo = m_vertInputState;

	//builder.shaderStages = m_shaderStages;

	builder.renderPass = m_renderPass;
	builder.pipelineLayout = m_pipelineLayout;

	m_graphicsPipeline = builder.buildGraphicsPipeline(g_vkContext.Device.logicalDevice);

	g_vkContext.PresentPipeline.pipeline = m_graphicsPipeline;
	*/
}


void PresentPipeline::createPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 0;
	createInfo.pSetLayouts = nullptr;

	// Push constants are a way of passing dynamic values to shaders
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(g_vkContext.Device.logicalDevice, &createInfo, nullptr, &m_pipelineLayout);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_pipelineLayout) };
	task.vkObjects = { g_vkContext.Device.logicalDevice, m_pipelineLayout };
	task.cleanupFunc = [&]() { vkDestroyPipelineLayout(g_vkContext.Device.logicalDevice, m_pipelineLayout, nullptr); };

	m_garbageCollector->createCleanupTask(task);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create graphics pipeline layout for the presentation pipeline!");
	}

	g_vkContext.PresentPipeline.layout = m_pipelineLayout;
}


void PresentPipeline::createRenderPass() {
	// Main attachments
		// Color attachment
	VkAttachmentDescription mainColorAttachment{};
	mainColorAttachment.format = g_vkContext.SwapChain.surfaceFormat.format;
	mainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;				// Use 1 sample since multisampling is not enabled yet

	// NOTE: loadOp = CLEAR is fine if we don't care about the "background" of the application (because the GUI is probably going to completely cover the screen anyway)
	// HOWEVER, if we want to draw/fill the background first then set loadOp = LOAD, and initialLayout = COLOR_ATTACHMENT_OPTIMAL.
	mainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	mainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	mainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	mainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	mainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	mainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	VkAttachmentReference mainColorAttachmentRef{};
	mainColorAttachmentRef.attachment = 0;
	mainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	// Subpasses
		// Main/ImGui subpass
	VkSubpassDescription mainSubpass{};
	mainSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	mainSubpass.colorAttachmentCount = 1;
	mainSubpass.pColorAttachments = &mainColorAttachmentRef;
	//mainSubpass.pDepthStencilAttachment = &depthAttachmentRef;


	/* NOTE: Dear Imgui uses the same color attachment as the main one, since Vulkan only allows for 1 color attachment per render pass.
		If Dear Imgui has its own render pass, then its color attachment's load operation must be LOAD_OP_LOAD because it needs to load the existing image from the main render pass.
		However, here, Dear Imgui is a subpass, so it automatically inherits the color atachment contents from the previous subpass (which is the main one). Therefore, we don't need to specify its load operation.
	*/


	// Dependencies
		// External -> Main/ImGui (0)
	VkSubpassDependency mainDependency{};
	mainDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	mainDependency.dstSubpass = 0;
	mainDependency.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	mainDependency.srcAccessMask = 0;
	mainDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	mainDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	mainDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; // NOTE: Alternatively, disable for global dependency


	// Creates render pass
	VkAttachmentDescription attachments[] = {
		mainColorAttachment
		//depthAttachment
	};

	VkSubpassDescription subpasses[] = {
		mainSubpass
	};

	VkSubpassDependency dependencies[] = {
		mainDependency
	};

	VkRenderPassCreateInfo m_renderPassCreateInfo{};
	m_renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	m_renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(sizeof(attachments) / sizeof(attachments[0]));
	m_renderPassCreateInfo.pAttachments = attachments;

	m_renderPassCreateInfo.subpassCount = static_cast<uint32_t>(sizeof(subpasses) / sizeof(subpasses[0]));
	m_renderPassCreateInfo.pSubpasses = subpasses;

	m_renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(sizeof(dependencies) / sizeof(dependencies[0]));
	m_renderPassCreateInfo.pDependencies = dependencies;

	VkResult result = vkCreateRenderPass(g_vkContext.Device.logicalDevice, &m_renderPassCreateInfo, nullptr, &m_renderPass);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_renderPass) };
	task.vkObjects = { g_vkContext.Device.logicalDevice, m_renderPass };
	task.cleanupFunc = [this]() { vkDestroyRenderPass(g_vkContext.Device.logicalDevice, m_renderPass, nullptr); };

	m_garbageCollector->createCleanupTask(task);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create render pass!");
	}

	g_vkContext.PresentPipeline.renderPass = m_renderPass;
	g_vkContext.PresentPipeline.subpassCount = m_renderPassCreateInfo.subpassCount;
}


void PresentPipeline::initViewportState() {
	m_viewport.x = m_viewport.y = 0.0f;
	m_viewport.width = static_cast<float>(g_vkContext.SwapChain.extent.width);
	m_viewport.height = static_cast<float>(g_vkContext.SwapChain.extent.height);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	// Since we want to draw the entire framebuffer, we'll specify a scissor rectangle that covers it entirely (i.e., that has the same extent as the swap chain's)
	// If we want to (re)draw only a partial part of the framebuffer from (a, b) to (x, y), we'll specify the offset as {a, b} and extent as {x, y}
	m_scissorRectangle.offset = { 0, 0 };
	m_scissorRectangle.extent = g_vkContext.SwapChain.extent;

	// NOTE: We don't need to specify pViewports and pScissors since the m_viewport was set as a dynamic state. Therefore, we only need to specify the m_viewport and scissor counts at pipeline creation time. The actual objects can be set up later at drawing time.
	m_viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	//m_viewportStateCreateInfo.pViewports = &m_viewport;
	m_viewportStateCreateInfo.viewportCount = 1;
	//m_viewportStateCreateInfo.pScissors = &m_scissorRectangle;
	m_viewportStateCreateInfo.scissorCount = 1;
}


void PresentPipeline::initRasterizationState() {
	m_rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	// If depth clamp is enabled, then fragments that are beyond the near and far planes are clamped to them rather than discarded.
	// This is useful in some cases like shadow maps, but using this requires enabling a GPU feature.
	m_rasterizerCreateInfo.depthClampEnable = VK_FALSE;

	// If rasterizerDiscardEnable is set to TRUE, then geometry will never be passed through the rasterizer stage. This effectively disables any output to the framebuffer.
	m_rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;

	// NOTE: Using any mode other than FILL requires enabling a GPU feature.
	m_rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // Use VK_POLYGON_MODE_LINE for wireframe rendering

	m_rasterizerCreateInfo.lineWidth = 1.0f;

	m_rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // Determines the type of culling to use

	// Specifies the vertex order for faces to be considered front-facing (can be clockwise/counter-clockwise)
	// Since we flipped the Y-coordinate of the clip coordinates in `VkBufferManager::updateUniformBuffer` to prevent images from being rendered upside-down, we must also specify that the vertex order should be counter-clockwise. If we keep it as clockwise, in our Y-flip case, backface culling will appear and prevent any geometry from being drawn.
	m_rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	m_rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	m_rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	m_rasterizerCreateInfo.depthBiasClamp = 0.0f;
	m_rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;
}


void PresentPipeline::initColorBlendingState() {
	// ColorBlendAttachmentState contains the configuration per attached framebuffer
	m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_colorBlendAttachment.blendEnable = VK_TRUE;

	// Alpha blending implementation (requires blendEnable to be TRUE)
	m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

	m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// ColorBlendStateCreateInfo references the array of structures for all of the framebuffers and allows us to set blend constants that we can use as blend factors.
	m_colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	m_colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	m_colorBlendCreateInfo.attachmentCount = 1;
	m_colorBlendCreateInfo.pAttachments = &m_colorBlendAttachment;
	m_colorBlendCreateInfo.blendConstants[0] = 0.0f;
	m_colorBlendCreateInfo.blendConstants[1] = 0.0f;
	m_colorBlendCreateInfo.blendConstants[2] = 0.0f;
	m_colorBlendCreateInfo.blendConstants[3] = 0.0f;
}
