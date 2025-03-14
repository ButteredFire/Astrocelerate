/* VkPipelineManager.cpp - Vulkan graphics pipeline management implementation.
*/

#include "VkPipelineManager.hpp"

GraphicsPipeline::GraphicsPipeline(VulkanContext& context):
	vkContext(context) {

}

GraphicsPipeline::~GraphicsPipeline() {
	vkDestroyShaderModule(vkContext.logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(vkContext.logicalDevice, fragShaderModule, nullptr);

	vkDestroyPipelineLayout(vkContext.logicalDevice, pipelineLayout, nullptr);
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

	initTessellationState();		// Tessellation state (note: tessellation is disabled for now. To enable it, specify the input assembly state's topology as PATCH_LIST, and add the tessellation create info struct to createGraphicsPipeline())


	// 2. Load shaders
	initShaderStage();


	// Create the pipeline layout
	initPipelineLayout();


	// Create the graphics pipeline
	//createGraphicsPipeline();
}



void GraphicsPipeline::createGraphicsPipeline() {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.basePipelineHandle;
	pipelineCreateInfo.basePipelineIndex;
	pipelineCreateInfo.flags;
	pipelineCreateInfo.layout = pipelineLayout;

	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = VK_NULL_HANDLE;
	pipelineCreateInfo.pColorBlendState = VK_NULL_HANDLE;
	pipelineCreateInfo.pTessellationState = VK_NULL_HANDLE;
	pipelineCreateInfo.pVertexInputState = &vertInputState;

	pipelineCreateInfo.pStages = shaderStages.data();

	pipelineCreateInfo.renderPass;
	pipelineCreateInfo.stageCount;
	pipelineCreateInfo.subpass;
}



void GraphicsPipeline::initShaderStage() {
	// Loads shader bytecode onto buffers
		// Vertex shader
	vertShaderBytecode = readFile("compiled_shaders/VertexShader.spv");
	std::cout << "Loaded vertex shader! SPIR-V bytecode file size is " << vertShaderBytecode.size() << " (bytes).\n";
	vertShaderModule = createShaderModule(vertShaderBytecode);

		// Fragment shader
	fragShaderBytecode = readFile("compiled_shaders/FragmentShader.spv");
	std::cout << "Loaded fragment shader! SPIR-V bytecode file size is " << fragShaderBytecode.size() << " (bytes).\n";
	fragShaderModule = createShaderModule(fragShaderBytecode);

	// Creates shader stages
		// Vertex shader
	VkPipelineShaderStageCreateInfo vertStageInfo{};
	vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Used to identify the create info's shader as the Vertex shader
	vertStageInfo.module = vertShaderModule;
	vertStageInfo.pName = "main"; // pName specifies the function to invoke, known as the entrypoint. This means that it is possible to combine multiple fragment shaders into a single shader module and use different entry points to differentiate between their behaviors. In this case we’ll stick to the standard `main`, however.

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
	vertInputState.vertexBindingDescriptionCount = 0;
	vertInputState.pVertexBindingDescriptions = nullptr;
	// Describes attribute descriptions, i.e., type of the attributes passed to the vertex shader, which binding to load them from (and at which offset)
	vertInputState.vertexAttributeDescriptionCount = 0;
	vertInputState.pVertexAttributeDescriptions = nullptr;
}


void GraphicsPipeline::initDynamicStates() {
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
}


void GraphicsPipeline::initInputAssemblyState() {
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Use PATCH_LIST instead of TRIANGLE_LIST for tessellation
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_TRUE;
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
	colorBlendAttachment.blendEnable = VK_FALSE;

	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
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


void GraphicsPipeline::initPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 0;
	createInfo.pSetLayouts = nullptr;

	// Push constants are a way of passing dynamic values to shaders
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(vkContext.logicalDevice, &createInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline layout!");
	}
}


VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& bytecode) {
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = bytecode.size();
	moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
	
	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(vkContext.logicalDevice, &moduleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}
