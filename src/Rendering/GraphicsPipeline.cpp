/* GraphicsPipeline.cpp - Vulkan graphics pipeline management implementation.
*/

#include "GraphicsPipeline.hpp"

GraphicsPipeline::GraphicsPipeline(VulkanContext& context):
	vkContext(context) {

	memoryManager = ServiceLocator::getService<MemoryManager>();
	bufferManager = ServiceLocator::getService<BufferManager>();

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

GraphicsPipeline::~GraphicsPipeline() {}

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


	// Create uniform buffer descriptors
	createDescriptorSetLayout();

	std::vector<VkDescriptorPoolSize> uniformBufDescPoolSize = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(SimulationConsts::MAX_FRAMES_IN_FLIGHT)}
	};
	createDescriptorPool(uniformBufDescPoolSize, uniformBufferDescriptorPool);
	createDescriptorSets();


	// Create the pipeline layout
	createPipelineLayout();


	// Create the render pass
	createRenderPass();


	// Create the graphics pipeline
	createGraphicsPipeline();
}


void GraphicsPipeline::cleanup() {
	Log::print(Log::T_INFO, __FUNCTION__, "Cleaning up...");

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
		throw Log::RuntimeException(__FUNCTION__, "Failed to create graphics pipeline!");
	}

	vkContext.GraphicsPipeline.pipeline = graphicsPipeline;


	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.mainObjectName = VARIABLE_NAME(graphicsPipeline);
	task.vkObjects = { vkContext.logicalDevice, graphicsPipeline };
	task.cleanupFunc = [&]() { vkDestroyPipeline(vkContext.logicalDevice, graphicsPipeline, nullptr); };

	memoryManager->createCleanupTask(task);
}


void GraphicsPipeline::createPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &uniformBufferDescriptorSetLayout;

	// Push constants are a way of passing dynamic values to shaders
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(vkContext.logicalDevice, &createInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create graphics pipeline layout!");
	}

	vkContext.GraphicsPipeline.layout = pipelineLayout;
	

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.mainObjectName = VARIABLE_NAME(pipelineLayout);
	task.vkObjects = { vkContext.logicalDevice, pipelineLayout };
	task.cleanupFunc = [&]() { vkDestroyPipelineLayout(vkContext.logicalDevice, pipelineLayout, nullptr); };

	memoryManager->createCleanupTask(task);
}


void GraphicsPipeline::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = ShaderConsts::VERT_BIND_UNIFORM_UBO;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	// It is possible for the UBO shader variable to have multiple uniform buffer objects, so we need to specify the number of UBOs in it
	layoutBinding.descriptorCount = 1;

	// Specifies which shader stages will the UBO(s) be referenced and used (through `VkShaderStageFlagBits` values; see the specification for more information)
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Specifies descriptors handling image-sampling
	layoutBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = 1;
	layoutCreateInfo.pBindings = &layoutBinding;

	VkResult result = vkCreateDescriptorSetLayout(vkContext.logicalDevice, &layoutCreateInfo, nullptr, &uniformBufferDescriptorSetLayout);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create descriptor set layout!");
	}

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.mainObjectName = VARIABLE_NAME(uniformBufferDescriptorSetLayout);
	task.vkObjects = { vkContext.logicalDevice, uniformBufferDescriptorSetLayout };
	task.cleanupFunc = [this]() { vkDestroyDescriptorSetLayout(vkContext.logicalDevice, uniformBufferDescriptorSetLayout, nullptr); };

	memoryManager->createCleanupTask(task);
}


void GraphicsPipeline::createDescriptorPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPool& descriptorPool, VkDescriptorPoolCreateFlags createFlags) {
	VkDescriptorPoolCreateInfo descPoolCreateInfo{};
	descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descPoolCreateInfo.pPoolSizes = poolSizes.data();
	descPoolCreateInfo.flags = createFlags;

	// Specifies the maximum number of descriptor sets that can be allocated
	descPoolCreateInfo.maxSets = 0;
	for (const auto& poolSize : poolSizes) {
		descPoolCreateInfo.maxSets += poolSize.descriptorCount;
	}


	VkResult result = vkCreateDescriptorPool(vkContext.logicalDevice, &descPoolCreateInfo, nullptr, &descriptorPool);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create descriptor pool!");
	}

	
	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.mainObjectName = VARIABLE_NAME(descriptorPool);
	task.vkObjects = { vkContext.logicalDevice, descriptorPool };
	task.cleanupFunc = [this, descriptorPool]() { vkDestroyDescriptorPool(vkContext.logicalDevice, descriptorPool, nullptr); };

	memoryManager->createCleanupTask(task);
}


void GraphicsPipeline::createDescriptorSets() {
	// Creates one descriptor set for every frame in flight (all with the same layout)
	std::vector<VkDescriptorSetLayout> descSetLayouts(SimulationConsts::MAX_FRAMES_IN_FLIGHT, uniformBufferDescriptorSetLayout);

	VkDescriptorSetAllocateInfo descSetAllocInfo{};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.descriptorPool = uniformBufferDescriptorPool;

	descSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(descSetLayouts.size());
	descSetAllocInfo.pSetLayouts = descSetLayouts.data();

	// Creates descriptor sets
	uniformBufferDescriptorSets.resize(descSetLayouts.size());
	
	VkResult result = vkAllocateDescriptorSets(vkContext.logicalDevice, &descSetAllocInfo, uniformBufferDescriptorSets.data());
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create descriptor sets!");
	}


	// Configures the descriptors within the newly allocated descriptor sets
	for (size_t i = 0; i < descSetLayouts.size(); i++) {
		// Descriptors handling buffers (including uniform buffers) are configured with the following struct:
		VkDescriptorBufferInfo descBufInfo{};

		std::vector<VkBuffer> uniformBuffers = bufferManager->getUniformBuffers();
		descBufInfo.buffer = uniformBuffers[i];

		descBufInfo.offset = 0;
		descBufInfo.range = sizeof(UniformBufferObject); // Note: We can also use VK_WHOLE_SIZE if we want to overwrite the whole buffer (like what we're doing)

		// Updates the configuration for each descriptor
		VkWriteDescriptorSet descWrite{};
		descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite.dstSet = uniformBufferDescriptorSets[i];
		descWrite.dstBinding = ShaderConsts::VERT_BIND_UNIFORM_UBO;

		// Since descriptors can be arrays, we must specify the first descriptor's index to update in the array.
		// We are not using an array now, so we can leave it at 0.
		descWrite.dstArrayElement = 0;


		descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrite.descriptorCount = 1; // Specifies how many array elements to update (refer to `VkWriteDescriptorSet::dstArrayElement`)


		/* The descriptor write configuration also needs a reference to its Info struct, and this part depends on the type of descriptor:
		
			- `VkWriteDescriptorSet::pBufferInfo`: Used for descriptors that refer to buffer data
			- `VkWriteDescriptorSet::pImageInfo`: Used for descriptors that refer to image data
			- `VkWriteDescriptorSet::pTexelBufferInfo`: Used for descriptors that refer to buffer views

			We can only choose 1 out of 3.
		*/
		descWrite.pBufferInfo = &descBufInfo;
		//descWrite.pImageInfo = nullptr;
		//descWrite.pTexelBufferView = nullptr;


		// Applies the updates
		vkUpdateDescriptorSets(vkContext.logicalDevice, 1, &descWrite, 0, nullptr);
	}

	vkContext.GraphicsPipeline.uniformBufferDescriptorSets = uniformBufferDescriptorSets;
}


void GraphicsPipeline::createRenderPass() {
	// Main rendering
	VkAttachmentDescription mainColorAttachment{};
	mainColorAttachment.format = vkContext.surfaceFormat.format;
	mainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;				// Use 1 sample since multisampling is not enabled yet
	mainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	// The render area will be cleared to a uniform value on every render pass instantiation. Since the render pass is run for every frame in our case, we effectively "refresh" the render area.
	mainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	mainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	mainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	mainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Vulkan is free to discard any previous contents (which is fine because we are clearing it anyway)
	mainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	VkAttachmentReference mainColorAttachmentRef{};
	mainColorAttachmentRef.attachment = 0;
	mainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	
	VkSubpassDescription mainSubpass{};
	mainSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	mainSubpass.colorAttachmentCount = 1;
	mainSubpass.pColorAttachments = &mainColorAttachmentRef;

	
	VkSubpassDependency mainDependency{};
	mainDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	mainDependency.dstSubpass = 0;
	mainDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	mainDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	mainDependency.srcAccessMask = 0;
	mainDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;



	// Dear ImGui
		// NOTE: Dear Imgui uses the same color attachment as the main one, since Vulkan only allows for 1 color attachment per render pass.
		// If Dear Imgui has its own render pass, then its color attachment's load operation must be LOAD_OP_LOAD because it needs to load the existing image from the main render pass.
		// However, here, Dear Imgui is a subpass, so it automatically inherits the color atachment contents from the previous subpass (which is the main one). Therefore, we don't need to specify its load operation. 
	VkSubpassDescription imguiSubpass{};
	imguiSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	imguiSubpass.colorAttachmentCount = 1;
	imguiSubpass.pColorAttachments = &mainColorAttachmentRef;


	VkSubpassDependency mainToImguiDependency{};
	mainToImguiDependency.srcSubpass = mainDependency.dstSubpass;
	mainToImguiDependency.dstSubpass = 1;
	mainToImguiDependency.srcStageMask = mainDependency.dstStageMask;
	mainToImguiDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	mainToImguiDependency.srcAccessMask = mainDependency.dstAccessMask;
	mainToImguiDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	// Creates render pass
	VkAttachmentDescription attachments[] = {
		mainColorAttachment
	};

	VkSubpassDescription subpasses[] = {
		mainSubpass,
		imguiSubpass
	};

	VkSubpassDependency dependencies[] = {
		mainDependency,
		mainToImguiDependency
	};

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	renderPassCreateInfo.attachmentCount = (sizeof(attachments) / sizeof(attachments[0]));
	renderPassCreateInfo.pAttachments = attachments;

	renderPassCreateInfo.subpassCount = (sizeof(subpasses) / sizeof(subpasses[0]));
	renderPassCreateInfo.pSubpasses = subpasses;

	renderPassCreateInfo.dependencyCount = (sizeof(dependencies) / sizeof(dependencies[0]));
	renderPassCreateInfo.pDependencies = dependencies;

	VkResult result = vkCreateRenderPass(vkContext.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create render pass!");
	}

	vkContext.GraphicsPipeline.renderPass = renderPass;

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.mainObjectName = VARIABLE_NAME(renderPass);
	task.vkObjects = { vkContext.logicalDevice, renderPass };
	task.cleanupFunc = [&]() { vkDestroyRenderPass(vkContext.logicalDevice, renderPass, nullptr); };

	memoryManager->createCleanupTask(task);
}


void GraphicsPipeline::initShaderStage() {
	// Loads shader bytecode onto buffers
		// Vertex shader
	vertShaderBytecode = readFile(ShaderConsts::VERTEX);
	Log::print(Log::T_SUCCESS, __FUNCTION__, ("Loaded vertex shader! SPIR-V bytecode file size is " + std::to_string(vertShaderBytecode.size()) + " (bytes)."));
	vertShaderModule = createShaderModule(vertShaderBytecode);

		// Fragment shader
	fragShaderBytecode = readFile(ShaderConsts::FRAGMENT);
	Log::print(Log::T_SUCCESS, __FUNCTION__, ("Loaded fragment shader! SPIR-V bytecode file size is " + std::to_string(fragShaderBytecode.size()) + " (bytes)."));
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
	vertBindingDescription = BufferManager::getBindingDescription();
	vertAttribDescriptions = BufferManager::getAttributeDescriptions();

	vertInputState.vertexBindingDescriptionCount = 1;
	vertInputState.pVertexBindingDescriptions = &vertBindingDescription;

	vertInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertAttribDescriptions.size());
	vertInputState.pVertexAttributeDescriptions = vertAttribDescriptions.data();


	CleanupTask vertCleanupTask{}, fragCleanupTask{};
	vertCleanupTask.caller = fragCleanupTask.caller = __FUNCTION__;

	vertCleanupTask.mainObjectName = VARIABLE_NAME(vertShaderModule);
	fragCleanupTask.mainObjectName = VARIABLE_NAME(fragShaderModule);

	vertCleanupTask.vkObjects = { vkContext.logicalDevice, vertShaderModule };
	fragCleanupTask.vkObjects = { vkContext.logicalDevice, fragShaderModule };

	vertCleanupTask.cleanupFunc = [&]() { vkDestroyShaderModule(vkContext.logicalDevice, vertShaderModule, nullptr); };
	fragCleanupTask.cleanupFunc = [&]() { vkDestroyShaderModule(vkContext.logicalDevice, fragShaderModule, nullptr); };

	memoryManager->createCleanupTask(vertCleanupTask);
	memoryManager->createCleanupTask(fragCleanupTask);
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

	// Specifies the vertex order for faces to be considered front-facing (can be clockwise/counter-clockwise)
	// Since we flipped the Y-coordinate of the clip coordinates in `BufferManager::updateUniformBuffer` to prevent images from being rendered upside-down, we must also specify that the vertex order should be counter-clockwise. If we keep it as clockwise, in our Y-flip case, backface culling will appear and prevent any geometry from being drawn.
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
		throw Log::RuntimeException(__FUNCTION__, "Failed to create shader module!");
	}

	return shaderModule;
}
