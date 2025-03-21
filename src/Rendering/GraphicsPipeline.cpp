/* GraphicsPipeline.cpp - Vulkan graphics pipeline management implementation.
*/

#include "GraphicsPipeline.hpp"

GraphicsPipeline::GraphicsPipeline(VulkanContext& context):
	vkContext(context) {

}

GraphicsPipeline::~GraphicsPipeline() {
	cleanup();
}

void GraphicsPipeline::init() {
	// Set up fixed-function states
		// Dynamic states
	dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	initDynamicStates();

	initInputAssemblyState();		// Input assembly state

	initViewportState();			// Viewport state

	initRasterizationState();		// Rasterization state

	initMultisamplingState();		// Multisampling state

	initDepthStencilState();		// Depth stencil state

	initColorBlendingState();		// Blending state

	initTessellationState();		// Tessellation state


	// Load shaders
	initShaderStage();

	// Create the pipeline layout
	createPipelineLayout();

	// Create the render pass
	createRenderPass();

	// Create the graphics pipeline
	createGraphicsPipeline();
}


void GraphicsPipeline::cleanup() {
	if (vkIsValid(vertShaderModule))
		vkDestroyShaderModule(vkContext.logicalDevice, vertShaderModule, nullptr);
	
	if (vkIsValid(fragShaderModule))
		vkDestroyShaderModule(vkContext.logicalDevice, fragShaderModule, nullptr);

	if (vkIsValid(renderPass))
		vkDestroyRenderPass(vkContext.logicalDevice, renderPass, nullptr);
	
	if (vkIsValid(pipelineLayout))
		vkDestroyPipelineLayout(vkContext.logicalDevice, pipelineLayout, nullptr);
	
	if (vkIsValid(graphicsPipeline))
		vkDestroyPipeline(vkContext.logicalDevice, graphicsPipeline, nullptr);
}


void GraphicsPipeline::createGraphicsPipeline() {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; // Specify the pipeline as the graphics pipeline

	// Shader stage
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	// Fixed-function states
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pVertexInputState = &vertInputState;

	// Render pass
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0; // Index of the subpass
	/* NOTE:
	* It is also possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible with renderPass. The requirements for compatibility are described here: https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility
	*/

	// Pipeline properties
	/* NOTE:
	* Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. 
	* The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality in common with an existing pipeline and switching between pipelines from the same parent can also be done quicker. 
	* 
	* You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
	* These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo.
	*/
		// Right now, there is only a single pipeline, so we'll specify the handle and index as null and -1 (an invalid index)
	//pipelineCreateInfo.flags;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	pipelineCreateInfo.layout = pipelineLayout;

	VkResult result = vkCreateGraphicsPipelines(vkContext.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS) {
		cleanup();
		throw Log::runtimeException(__FUNCTION__, "Failed to create graphics pipeline!");
	}

	vkContext.GraphicsPipeline.pipeline = graphicsPipeline;
}


void GraphicsPipeline::createPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 0;
	createInfo.pSetLayouts = nullptr;

	// Push constants are a way of passing dynamic values to shaders
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(vkContext.logicalDevice, &createInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		cleanup();
		throw Log::runtimeException(__FUNCTION__, "Failed to create graphics pipeline layout!");
	}

	vkContext.GraphicsPipeline.layout = pipelineLayout;
}


void GraphicsPipeline::createRenderPass() {
	// Specifies the framebuffer attachment that will be used while rendering
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = vkContext.surfaceFormat.format;		// Swap-chain image format
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;				// Use 1 sample since multisampling is not enabled yet

		// loadOp and storeOp specify how the data in the attachment will be dealt with before and after rendering.
		// loadOp and storeOp apply to color and depth data; stencilLoadOp and stencilStoreOp apply to stencil data.
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// initialLayout specifies the layout the image will have before the render pass begins;
		// finalLayout specifies the layout the image will have when the render pass ends
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		// Ignore the image's previous layout (which works because we'll clear the contents of the image when the render pass ends anyway)
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// Used for presenting a presentable image (i.e., an image processed by the swap-chain) for display

	/* A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes, for example a sequence of post-processing effects that are applied one after another. If you group these rendering operations into one render pass, then Vulkan is able to reorder the operations and conserve memory bandwidth for possibly better performance. 
	* However, we’ll stick to a single subpass for now.
	*/
	// Specifies the reference to the attachment
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;	// The attachment descriptions array currently consists of a single description, so the reference to it is 0 (the index referenced from the fragment shader with `layout(location = 0) ...`)
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Specifies that the attachment will function as a color buffer

	
	// Specifies properties of a subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Specifies the subpass as being a graphics subpass (for the graphics pipeline)
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;



	// Subpass dependencies define memory and execution dependencies between subpasses. They are used to synchronize access to attachments in the render passes.
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Specifies the implicit subpass before the render pass
	dependency.dstSubpass = 0; // Specifies the first subpass

	// Specifies the operations to wait for (and the stages in which they occur)
		// We must wait for the swap-chain to finish reading the image before it can be accessed. To this end, we can wait on the color attachment output stage.
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
		
		// The operations that should wait on this are in the color attachment stage and involve the writing of the color attachment. These settings will prevent the transition from happening until it’s actually necessary (and allowed): when we want to start writing colors to it.
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	// Creates render pass
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(vkContext.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		throw Log::runtimeException(__FUNCTION__, "Failed to create render pass!");
	}

	vkContext.GraphicsPipeline.renderPass = renderPass;
}



void GraphicsPipeline::initShaderStage() {
	// Loads shader bytecode onto buffers
		// Vertex shader
	vertShaderBytecode = readFile("compiled_shaders/VertexShader.spv");
	Log::print(Log::INFO, __FUNCTION__, ("Loaded vertex shader! SPIR-V bytecode file size is " + std::to_string(vertShaderBytecode.size()) + " (bytes)."));
	vertShaderModule = createShaderModule(vertShaderBytecode);

		// Fragment shader
	fragShaderBytecode = readFile("compiled_shaders/FragmentShader.spv");
	Log::print(Log::INFO, __FUNCTION__, ("Loaded fragment shader! SPIR-V bytecode file size is " + std::to_string(fragShaderBytecode.size()) + " (bytes)."));
	fragShaderModule = createShaderModule(fragShaderBytecode);

	// Creates shader stages
		// Vertex shader
	VkPipelineShaderStageCreateInfo vertStageInfo{};
	vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Used to identify the create info's shader as the Vertex shader
	vertStageInfo.module = vertShaderModule;
	vertStageInfo.pName = "main"; // pName specifies the function to invoke, known as the entry point. This means that it is possible to combine multiple fragment shaders into a single shader module and use different entry points to differentiate between their behaviors. In this case we’ll stick to the standard `main`, however.

		// Fragment shader
	VkPipelineShaderStageCreateInfo fragStageInfo{};
	fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragShaderModule;
	fragStageInfo.pName = "main";


	shaderStages = {
		vertStageInfo,
		fragStageInfo
	};


	// Specifies the format of the vertex data to be passed to the vertex buffer.
	vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// Describes binding, i.e., spacing between the data and whether the data is per-vertex or per-instance
		// Gets vertex input binding and attribute descriptions
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescription();

	vertInputState.vertexBindingDescriptionCount = 1;
	vertInputState.pVertexBindingDescriptions = &bindingDescription;

	vertInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
}


void GraphicsPipeline::initDynamicStates() {
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
}


void GraphicsPipeline::initInputAssemblyState() {
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Use PATCH_LIST instead of TRIANGLE_LIST for tessellation
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
}


void GraphicsPipeline::initViewportState() {
	viewport.x = viewport.y = 0.0f;
	viewport.width = static_cast<float>(vkContext.swapChainExtent.width);
	viewport.height = static_cast<float>(vkContext.swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Since we want to draw the entire framebuffer, we'll specify a scissor rectangle that covers it entirely (i.e., that has the same extent as the swap chain's)
	// If we want to (re)draw only a partial part of the framebuffer from (a, b) to (x, y), we'll specify the offset as {a, b} and extent as {x, y}
	scissorRectangle.offset = { 0, 0 };
	scissorRectangle.extent = vkContext.swapChainExtent;

	// NOTE: We don't need to specify pViewports and pScissors since the viewport was set as a dynamic state. Therefore, we only need to specify the viewport and scissor counts at pipeline creation time. The actual objects can be set up later at drawing time.
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	//viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.viewportCount = 1;
	//viewportStateCreateInfo.pScissors = &scissorRectangle;
	viewportStateCreateInfo.scissorCount = 1;
}


void GraphicsPipeline::initRasterizationState() {
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	// If depth clamp is enabled, then fragments that are beyond the near and far planes are clamped to them rather than discarded.
	// This is useful in some cases like shadow maps, but using this requires enabling a GPU feature.
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;

	// If rasterizerDiscardEnable is set to TRUE, then geometry will never be passed through the rasterizer stage. This effectively disables any output to the framebuffer.
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;

	// NOTE: Using any mode other than FILL requires enabling a GPU feature.
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // Use VK_POLYGON_MODE_LINE for wireframe rendering

	rasterizerCreateInfo.lineWidth = 1.0f;

	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // Determines the type of culling to use
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // Specifies the vertex order for faces to be considered front-facing (can be clockwise/counter-clockwise)

	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;
}


void GraphicsPipeline::initMultisamplingState() {
	// TODO: Finish multisampling state and include it in createGraphicsPipeline()
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
}


void GraphicsPipeline::initDepthStencilState() {
	// TODO: Finish depth stencil state structure and include it in createGraphicsPipeline()
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
}


void GraphicsPipeline::initColorBlendingState() {
	// ColorBlendAttachmentState contains the configuration per attached framebuffer
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;

	// Alpha blending implementation (requires blendEnable to be TRUE)
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// ColorBlendStateCreateInfo references the array of structures for all of the framebuffers and allows us to set blend constants that we can use as blend factors.
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;
}


void GraphicsPipeline::initTessellationState() {
	tessStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessStateCreateInfo.patchControlPoints = 3; // Number of control points per patch (e.g., 3 for triangles)
}


VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& bytecode) {
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = bytecode.size();
	moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
	
	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(vkContext.logicalDevice, &moduleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw Log::runtimeException(__FUNCTION__, "Failed to create shader module!");
	}

	return shaderModule;
}
