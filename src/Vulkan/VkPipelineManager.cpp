/* VkPipelineManager.cpp - Vulkan graphics pipeline management implementation.
*/

#include "VkPipelineManager.hpp"

GraphicsPipeline::GraphicsPipeline(VulkanContext& context):
	vkContext(context) {

}

GraphicsPipeline::~GraphicsPipeline() {
	vkDestroyShaderModule(vkContext.logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(vkContext.logicalDevice, fragShaderModule, nullptr);
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

	//initViewportState();			// Viewport state

	//initRasterizationState();		// Rasterization state

	//initMultisamplingState();		// Multisampling state

	//initDepthStencilState();		// Depth stencil state

	//initColorBlendingState();		// Blending state


	// 2. Load shaders
	initShaderStage();


	// Create the pipeline layout
	//initPipelineLayout();


	// Create the graphics pipeline
	//createGraphicsPipeline();
}



void GraphicsPipeline::createGraphicsPipeline() {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR;
	pipelineCreateInfo.basePipelineHandle;
	pipelineCreateInfo.basePipelineIndex;
	pipelineCreateInfo.flags;
	pipelineCreateInfo.layout;
	pipelineCreateInfo.pColorBlendState;
	pipelineCreateInfo.pDepthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pMultisampleState;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.pRasterizationState;
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pTessellationState;
	pipelineCreateInfo.pVertexInputState = &vertInputState;
	pipelineCreateInfo.pViewportState;
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
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_TRUE;
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
