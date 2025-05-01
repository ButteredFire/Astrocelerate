/* GraphicsPipeline.cpp - Vulkan graphics pipeline management implementation.
*/

#include "GraphicsPipeline.hpp"

GraphicsPipeline::GraphicsPipeline(VulkanContext& context):
	m_vkContext(context) {

	m_eventDispatcher = ServiceLocator::getService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);
	m_bufferManager = ServiceLocator::getService<BufferManager>(__FUNCTION__);


	m_eventDispatcher->subscribe<Event::SwapchainRecreation>(
		[this](const Event::SwapchainRecreation& event) {
			this->initDepthBufferingResources();
		}
	);


	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

GraphicsPipeline::~GraphicsPipeline() {}

void GraphicsPipeline::init() {
	// Set up fixed-function states
		// Dynamic states
	m_dynamicStates = {
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

	initDepthBufferingResources();	// Depth buffering image and view

	initTessellationState();		// Tessellation state


	// Load shaders
	initShaderStage();


	// Create descriptors
	setUpDescriptors();


	// Create the pipeline layout
	createPipelineLayout();


	// Create the render pass
	createRenderPass();


	// Create the graphics pipeline
	createGraphicsPipeline();


	// Post-initialization: Data is ready to be used for framebuffer creation
	m_eventDispatcher->publish(Event::InitFrameBuffers{});
}


void GraphicsPipeline::createGraphicsPipeline() {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; // Specify the pipeline as the graphics pipeline

	// Shader stage
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	pipelineCreateInfo.pStages = m_shaderStages.data();

	// Fixed-function states
	pipelineCreateInfo.pDynamicState = &m_dynamicStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &m_viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &m_rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &m_multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &m_depthStencilStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &m_colorBlendCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pVertexInputState = &m_vertInputState;

	// Render pass
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.subpass = 0; // Index of the subpass
		/* NOTE:
		* It is also possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible with m_renderPass. The requirements for compatibility are described here: https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility
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

	pipelineCreateInfo.layout = m_pipelineLayout;

	VkResult result = vkCreateGraphicsPipelines(m_vkContext.Device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create graphics pipeline!");
	}

	m_vkContext.GraphicsPipeline.pipeline = m_graphicsPipeline;


	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_graphicsPipeline) };
	task.vkObjects = { m_vkContext.Device.logicalDevice, m_graphicsPipeline };
	task.cleanupFunc = [this]() { vkDestroyPipeline(m_vkContext.Device.logicalDevice, m_graphicsPipeline, nullptr); };

	m_garbageCollector->createCleanupTask(task);
}


void GraphicsPipeline::createPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &m_descriptorSetLayout;

	// Push constants are a way of passing dynamic values to shaders
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(m_vkContext.Device.logicalDevice, &createInfo, nullptr, &m_pipelineLayout);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create graphics pipeline layout!");
	}

	m_vkContext.GraphicsPipeline.layout = m_pipelineLayout;
	

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_pipelineLayout) };
	task.vkObjects = { m_vkContext.Device.logicalDevice, m_pipelineLayout };
	task.cleanupFunc = [&]() { vkDestroyPipelineLayout(m_vkContext.Device.logicalDevice, m_pipelineLayout, nullptr); };

	m_garbageCollector->createCleanupTask(task);
}


void GraphicsPipeline::setUpDescriptors() {
	// Setup
		// Layout bindings
			// Uniform buffer
	VkDescriptorSetLayoutBinding uniformBufferLayoutBinding{};
	uniformBufferLayoutBinding.binding = ShaderConsts::VERT_BIND_UNIFORM_UBO;
	uniformBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferLayoutBinding.descriptorCount = 1;
	uniformBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Specifies which shader stages will the UBO(s) be referenced and used (through `VkShaderStageFlagBits` values; see the specification for more information)
	uniformBufferLayoutBinding.pImmutableSamplers = nullptr;			// Specifies descriptors handling image-sampling


			// Texture sampler
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = ShaderConsts::FRAG_BIND_UNIFORM_TEXURE_SAMPLER;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;		// Image sampling happens in the fragment shader, although it can also be used in the vertex shader for specific reasons (e.g., dynamically deforming a grid of vertices via a heightmap)
	samplerLayoutBinding.pImmutableSamplers = nullptr;



	// Data organization
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
		uniformBufferLayoutBinding,
		samplerLayoutBinding
	};

	for (const auto& binding : layoutBindings) { m_descriptorCount += binding.descriptorCount; }

	std::vector<VkDescriptorPoolSize> poolSize = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(SimulationConsts::MAX_FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(SimulationConsts::MAX_FRAMES_IN_FLIGHT) }
	};



	// Descriptor creation
	createDescriptorSetLayout(layoutBindings);
	createDescriptorPool(m_descriptorCount, poolSize, m_descriptorPool);
	createDescriptorSets();
}


void GraphicsPipeline::createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> layoutBindings) {
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings = layoutBindings.data();


	VkResult result = vkCreateDescriptorSetLayout(m_vkContext.Device.logicalDevice, &layoutCreateInfo, nullptr, &m_descriptorSetLayout);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create descriptor set layout!");
	}


	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_descriptorSetLayout) };
	task.vkObjects = { m_vkContext.Device.logicalDevice, m_descriptorSetLayout };
	task.cleanupFunc = [this]() { vkDestroyDescriptorSetLayout(m_vkContext.Device.logicalDevice, m_descriptorSetLayout, nullptr); };

	m_garbageCollector->createCleanupTask(task);
}


void GraphicsPipeline::createDescriptorPool(uint32_t maxDescriptorSetCount, std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPool& descriptorPool, VkDescriptorPoolCreateFlags createFlags) {
	VkDescriptorPoolCreateInfo descPoolCreateInfo{};
	descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descPoolCreateInfo.pPoolSizes = poolSizes.data();
	descPoolCreateInfo.flags = createFlags;

	// Specifies the maximum number of descriptor sets that can be allocated
	descPoolCreateInfo.maxSets = maxDescriptorSetCount;


	VkResult result = vkCreateDescriptorPool(m_vkContext.Device.logicalDevice, &descPoolCreateInfo, nullptr, &descriptorPool);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create descriptor pool!");
	}

	
	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_descriptorPool) };
	task.vkObjects = { m_vkContext.Device.logicalDevice, descriptorPool };
	task.cleanupFunc = [this, descriptorPool]() { vkDestroyDescriptorPool(m_vkContext.Device.logicalDevice, descriptorPool, nullptr); };

	m_garbageCollector->createCleanupTask(task);
}


void GraphicsPipeline::createDescriptorSets() {
	// Creates one descriptor set for every frame in flight (all with the same layout)
	std::vector<VkDescriptorSetLayout> descSetLayouts(SimulationConsts::MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo descSetAllocInfo{};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.descriptorPool = m_descriptorPool;

	descSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(descSetLayouts.size());
	descSetAllocInfo.pSetLayouts = descSetLayouts.data();


	// Allocates descriptor sets
	m_descriptorSets.resize(descSetLayouts.size());
	
	VkResult result = vkAllocateDescriptorSets(m_vkContext.Device.logicalDevice, &descSetAllocInfo, m_descriptorSets.data());
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create descriptor sets!");
	}


	// Configures the descriptors within the newly allocated descriptor sets
	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.reserve(m_descriptorCount);

	for (size_t i = 0; i < descSetLayouts.size(); i++) {
		descriptorWrites.clear();

		// Uniform buffer
		VkDescriptorBufferInfo descBufInfo{};
		descBufInfo.buffer = m_bufferManager->getUniformBuffers()[i];
		descBufInfo.offset = 0;
		descBufInfo.range = sizeof(UniformBufferObject); // Note: We can also use VK_WHOLE_SIZE if we want to overwrite the whole buffer (like what we're doing)


		// Texture sampler
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = m_vkContext.Texture.imageLayout;
		imageInfo.imageView = m_vkContext.Texture.imageView;
		imageInfo.sampler = m_vkContext.Texture.sampler;


		// Updates the configuration for each descriptor
			// Uniform buffer descriptor write
		VkWriteDescriptorSet uniformBufferDescWrite{};
		uniformBufferDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferDescWrite.dstSet = m_descriptorSets[i];
		uniformBufferDescWrite.dstBinding = ShaderConsts::VERT_BIND_UNIFORM_UBO;

				// Since descriptors can be arrays, we must specify the first descriptor's index to update in the array.
				// We are not using an array now, so we can leave it at 0.
		uniformBufferDescWrite.dstArrayElement = 0;

		uniformBufferDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferDescWrite.descriptorCount = 1; // Specifies how many array elements to update (refer to `VkWriteDescriptorSet::dstArrayElement`)

				/* The descriptor write configuration also needs a reference to its Info struct, and this part depends on the type of descriptor:
		
					- `VkWriteDescriptorSet::pBufferInfo`: Used for descriptors that refer to buffer data
					- `VkWriteDescriptorSet::pImageInfo`: Used for descriptors that refer to image data
					- `VkWriteDescriptorSet::pTexelBufferInfo`: Used for descriptors that refer to buffer views

					We can only choose 1 out of 3.
				*/
		uniformBufferDescWrite.pBufferInfo = &descBufInfo;
		//uniformBufferDescWrite.pImageInfo = nullptr;
		//uniformBufferDescWrite.pTexelBufferView = nullptr;

		descriptorWrites.push_back(uniformBufferDescWrite);


		// Texture sampler descriptor write
		VkWriteDescriptorSet samplerDescWrite{};
		samplerDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerDescWrite.dstSet = m_descriptorSets[i];
		samplerDescWrite.dstBinding = ShaderConsts::FRAG_BIND_UNIFORM_TEXURE_SAMPLER;
		samplerDescWrite.dstArrayElement = 0;
		samplerDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerDescWrite.descriptorCount = 1;
		samplerDescWrite.pImageInfo = &imageInfo;

		descriptorWrites.push_back(samplerDescWrite);


		// Applies the updates
		vkUpdateDescriptorSets(m_vkContext.Device.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}


	m_vkContext.GraphicsPipeline.m_descriptorSets = m_descriptorSets;
}


void GraphicsPipeline::createRenderPass() {
	// Main attachments
		// Color attachment
	VkAttachmentDescription mainColorAttachment{};
	mainColorAttachment.format = m_vkContext.SwapChain.surfaceFormat.format;
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

	
		// Depth attachment
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = getBestDepthImageFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	
	// Subpasses
		// Main subpass
	VkSubpassDescription mainSubpass{};
	mainSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	mainSubpass.colorAttachmentCount = 1;
	mainSubpass.pColorAttachments = &mainColorAttachmentRef;
	mainSubpass.pDepthStencilAttachment = &depthAttachmentRef;


		// ImGui subpass
			/* NOTE: Dear Imgui uses the same color attachment as the main one, since Vulkan only allows for 1 color attachment per render pass.
				If Dear Imgui has its own render pass, then its color attachment's load operation must be LOAD_OP_LOAD because it needs to load the existing image from the main render pass.
				However, here, Dear Imgui is a subpass, so it automatically inherits the color atachment contents from the previous subpass (which is the main one). Therefore, we don't need to specify its load operation.
			*/
	VkSubpassDescription imguiSubpass{};
	imguiSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	imguiSubpass.colorAttachmentCount = 1;
	imguiSubpass.pColorAttachments = &mainColorAttachmentRef;


	// Dependencies
		// EXTERNAL -> Main
	VkSubpassDependency mainDependency{};
	mainDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	mainDependency.dstSubpass = 0;
	mainDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	mainDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	mainDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	mainDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


		// Main -> ImGui
	VkSubpassDependency mainToImguiDependency{};
	mainToImguiDependency.srcSubpass = mainDependency.dstSubpass;
	mainToImguiDependency.dstSubpass = 1;
	mainToImguiDependency.srcStageMask = mainDependency.dstStageMask;
	mainToImguiDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	mainToImguiDependency.srcAccessMask = mainDependency.dstAccessMask;
	mainToImguiDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	// Creates render pass
	VkAttachmentDescription attachments[] = {
		mainColorAttachment,
		depthAttachment
	};

	VkSubpassDescription subpasses[] = {
		mainSubpass,
		imguiSubpass
	};

	VkSubpassDependency dependencies[] = {
		mainDependency,
		mainToImguiDependency
	};

	VkRenderPassCreateInfo m_renderPassCreateInfo{};
	m_renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	m_renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(sizeof(attachments) / sizeof(attachments[0]));
	m_renderPassCreateInfo.pAttachments = attachments;

	m_renderPassCreateInfo.subpassCount = static_cast<uint32_t>(sizeof(subpasses) / sizeof(subpasses[0]));
	m_renderPassCreateInfo.pSubpasses = subpasses;

	m_renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(sizeof(dependencies) / sizeof(dependencies[0]));
	m_renderPassCreateInfo.pDependencies = dependencies;

	VkResult result = vkCreateRenderPass(m_vkContext.Device.logicalDevice, &m_renderPassCreateInfo, nullptr, &m_renderPass);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create render pass!");
	}

	m_vkContext.GraphicsPipeline.renderPass = m_renderPass;
	m_vkContext.GraphicsPipeline.subpassCount = m_renderPassCreateInfo.subpassCount;

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_renderPass) };
	task.vkObjects = { m_vkContext.Device.logicalDevice, m_renderPass };
	task.cleanupFunc = [this]() { vkDestroyRenderPass(m_vkContext.Device.logicalDevice, m_renderPass, nullptr); };

	m_garbageCollector->createCleanupTask(task);
}


void GraphicsPipeline::initShaderStage() {
	// Loads shader bytecode onto buffers
		// Vertex shader
	m_vertShaderBytecode = FilePathUtils::readFile(ShaderConsts::VERTEX);
	Log::print(Log::T_SUCCESS, __FUNCTION__, ("Loaded vertex shader! SPIR-V bytecode file size is " + std::to_string(m_vertShaderBytecode.size()) + " (bytes)."));
	m_vertShaderModule = createShaderModule(m_vertShaderBytecode);

		// Fragment shader
	m_fragShaderBytecode = FilePathUtils::readFile(ShaderConsts::FRAGMENT);
	Log::print(Log::T_SUCCESS, __FUNCTION__, ("Loaded fragment shader! SPIR-V bytecode file size is " + std::to_string(m_fragShaderBytecode.size()) + " (bytes)."));
	m_fragShaderModule = createShaderModule(m_fragShaderBytecode);

	// Creates shader stages
		// Vertex shader
	VkPipelineShaderStageCreateInfo vertStageInfo{};
	vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Used to identify the create info's shader as the Vertex shader
	vertStageInfo.module = m_vertShaderModule;
	vertStageInfo.pName = "main"; // pName specifies the function to invoke, known as the entry point. This means that it is possible to combine multiple fragment shaders into a single shader module and use different entry points to differentiate between their behaviors. In this case we’ll stick to the standard `main`, however.

		// Fragment shader
	VkPipelineShaderStageCreateInfo fragStageInfo{};
	fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = m_fragShaderModule;
	fragStageInfo.pName = "main";


	m_shaderStages = {
		vertStageInfo,
		fragStageInfo
	};


	// Specifies the format of the vertex data to be passed to the vertex buffer.
	m_vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// Describes binding, i.e., spacing between the data and whether the data is per-vertex or per-instance
		// Gets vertex input binding and attribute descriptions
	m_vertBindingDescription = Vertex::getVertexInputBindingDescription();
	m_vertAttribDescriptions = Vertex::getVertexAttributeDescriptions();

	m_vertInputState.vertexBindingDescriptionCount = 1;
	m_vertInputState.pVertexBindingDescriptions = &m_vertBindingDescription;

	m_vertInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertAttribDescriptions.size());
	m_vertInputState.pVertexAttributeDescriptions = m_vertAttribDescriptions.data();


	CleanupTask vertCleanupTask{}, fragCleanupTask{};
	vertCleanupTask.caller = fragCleanupTask.caller = __FUNCTION__;

	vertCleanupTask.objectNames = { VARIABLE_NAME(m_vertShaderModule) };
	fragCleanupTask.objectNames = { VARIABLE_NAME(m_fragShaderModule) };

	vertCleanupTask.vkObjects = { m_vkContext.Device.logicalDevice, m_vertShaderModule };
	fragCleanupTask.vkObjects = { m_vkContext.Device.logicalDevice, m_fragShaderModule };

	vertCleanupTask.cleanupFunc = [&]() { vkDestroyShaderModule(m_vkContext.Device.logicalDevice, m_vertShaderModule, nullptr); };
	fragCleanupTask.cleanupFunc = [&]() { vkDestroyShaderModule(m_vkContext.Device.logicalDevice, m_fragShaderModule, nullptr); };

	m_garbageCollector->createCleanupTask(vertCleanupTask);
	m_garbageCollector->createCleanupTask(fragCleanupTask);
}


void GraphicsPipeline::initDynamicStates() {
	m_dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	m_dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
	m_dynamicStateCreateInfo.pDynamicStates = m_dynamicStates.data();
}


void GraphicsPipeline::initInputAssemblyState() {
	m_inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Use PATCH_LIST instead of TRIANGLE_LIST for tessellation
	m_inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
}


void GraphicsPipeline::initViewportState() {
	m_viewport.x = m_viewport.y = 0.0f;
	m_viewport.width = static_cast<float>(m_vkContext.SwapChain.extent.width);
	m_viewport.height = static_cast<float>(m_vkContext.SwapChain.extent.height);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	// Since we want to draw the entire framebuffer, we'll specify a scissor rectangle that covers it entirely (i.e., that has the same extent as the swap chain's)
	// If we want to (re)draw only a partial part of the framebuffer from (a, b) to (x, y), we'll specify the offset as {a, b} and extent as {x, y}
	m_scissorRectangle.offset = { 0, 0 };
	m_scissorRectangle.extent = m_vkContext.SwapChain.extent;

	// NOTE: We don't need to specify pViewports and pScissors since the m_viewport was set as a dynamic state. Therefore, we only need to specify the m_viewport and scissor counts at pipeline creation time. The actual objects can be set up later at drawing time.
	m_viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	//m_viewportStateCreateInfo.pViewports = &m_viewport;
	m_viewportStateCreateInfo.viewportCount = 1;
	//m_viewportStateCreateInfo.pScissors = &m_scissorRectangle;
	m_viewportStateCreateInfo.scissorCount = 1;
}


void GraphicsPipeline::initRasterizationState() {
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
	// Since we flipped the Y-coordinate of the clip coordinates in `BufferManager::updateUniformBuffer` to prevent images from being rendered upside-down, we must also specify that the vertex order should be counter-clockwise. If we keep it as clockwise, in our Y-flip case, backface culling will appear and prevent any geometry from being drawn.
	m_rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	m_rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	m_rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	m_rasterizerCreateInfo.depthBiasClamp = 0.0f;
	m_rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;
}


void GraphicsPipeline::initMultisamplingState() {
	// TODO: Finish multisampling state and include it in createGraphicsPipeline()
	m_multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	m_multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampleStateCreateInfo.minSampleShading = 1.0f;
	m_multisampleStateCreateInfo.pSampleMask = nullptr;
	m_multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	m_multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
}


void GraphicsPipeline::initDepthStencilState() {
	m_depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	// Specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
	m_depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;

	// Specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
	m_depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;


	// Specifies the depth comparison operator that is performed to determine whether to keep or discard a fragment.
	// VK_COMPARE_OP_LESS means "lower depth = closer". In other words, the depth value of new fragments should be LESS since they are closer to the camera, and thus they will overwrite the existing fragments.
	m_depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;


	// Configures depth bound testing (optional). It allows you to only keep fragments that fall within the specified depth range.
	// We won't be using this for now.
	m_depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	m_depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	m_depthStencilStateCreateInfo.maxDepthBounds = 1.0f;



	// Configures stencil buffer operations.
	m_depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilStateCreateInfo.front = {};
	m_depthStencilStateCreateInfo.back = {};
}


void GraphicsPipeline::initColorBlendingState() {
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


void GraphicsPipeline::initDepthBufferingResources() {
	// Specifies depth image data
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageAspectFlags imgAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

	uint32_t imgWidth = m_vkContext.SwapChain.extent.width;
	uint32_t imgHeight = m_vkContext.SwapChain.extent.height;
	uint32_t imgDepth = 1;
	
	VkFormat depthFormat = getBestDepthImageFormat();

	bool hasStencilComponent = formatHasStencilComponent(depthFormat);


	// Creates a depth image
	VmaAllocationCreateInfo imgAllocInfo{};
	imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	TextureManager::createImage(m_vkContext, m_depthImage, m_depthImageAllocation, imgWidth, imgHeight, imgDepth, depthFormat, imgTiling, imgUsage, imgAllocInfo);


	// Creates a depth image view
	VkSwapchainManager::createImageView(m_vkContext, m_depthImage, m_depthImageView, depthFormat, imgAspectFlags);
	m_vkContext.GraphicsPipeline.depthImageView = m_depthImageView;


	// Explicitly transitions the layout of the depth image to a depth attachment.
	// This is not necessary, since it will be done in the render pass anyway. This is rather being explicit for the sake of being explicit. 
	TextureManager::switchImageLayout(m_vkContext, m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


VkFormat GraphicsPipeline::getBestDepthImageFormat() {
	std::vector<VkFormat> candidates = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkFormatFeatureFlagBits formatFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	return findSuppportedFormat(candidates, imgTiling, formatFeatures);
}


VkFormat GraphicsPipeline::findSuppportedFormat(const std::vector<VkFormat>& formats, VkImageTiling imgTiling, VkFormatFeatureFlagBits formatFeatures) {
	for (const auto& format : formats) {
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(m_vkContext.Device.physicalDevice, format, &formatProperties);

		if (imgTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & formatFeatures) == formatFeatures) {
			return format;
		}

		if (imgTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & formatFeatures) == formatFeatures) {
			return format;
		}
	}


	throw Log::RuntimeException(__FUNCTION__, "Failed to find a suitable image format!");
}


void GraphicsPipeline::initTessellationState() {
	m_tessStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	m_tessStateCreateInfo.patchControlPoints = 3; // Number of control points per patch (e.g., 3 for triangles)
}


VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& bytecode) {
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = bytecode.size();
	moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
	
	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_vkContext.Device.logicalDevice, &moduleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create shader module!");
	}

	return shaderModule;
}
