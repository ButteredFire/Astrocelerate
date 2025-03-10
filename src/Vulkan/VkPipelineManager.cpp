/* VkPipelineManager.cpp - Vulkan graphics pipeline management implementation.
*/

#include "VkPipelineManager.hpp"

GraphicsPipeline::GraphicsPipeline(VulkanContext& context):
	vkContext(context) {

}

GraphicsPipeline::~GraphicsPipeline() {
}

void GraphicsPipeline::init() {
	loadShaders();
}



void GraphicsPipeline::loadShaders() {
	vertShaderBytecode = readFile("compiled_shaders/VertexShader.spv");
	std::cout << "Loaded vertex shader! SPIR-V bytecode file size is " << vertShaderBytecode.size() << " (bytes).\n";
	createShaderModule(vertShaderBytecode);

	fragShaderBytecode = readFile("compiled_shaders/FragmentShader.spv");
	std::cout << "Loaded fragment shader! SPIR-V bytecode file size is " << fragShaderBytecode.size() << " (bytes).\n";
	createShaderModule(fragShaderBytecode);
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
