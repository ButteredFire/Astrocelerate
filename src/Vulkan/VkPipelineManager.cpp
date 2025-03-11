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
	// Specify dynamic states
	dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	// Shader stage
	initShaderStage();
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

	VkPipelineShaderStageCreateInfo fragStageInfo{};
	fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragShaderModule;
	fragStageInfo.pName = "main";

	shaderStages = {
		vertStageInfo,
		fragStageInfo
	};
}


void GraphicsPipeline::initDynamicStates() {
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
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
