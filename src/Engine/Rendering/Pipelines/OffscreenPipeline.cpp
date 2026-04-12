#include "OffscreenPipeline.hpp"


OffscreenPipeline::OffscreenPipeline(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx) :
	m_renderDeviceCtx(renderDeviceCtx),
	m_windowCtx(windowCtx) {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_cleanupManager = ServiceLocator::GetService<CleanupManager>(__FUNCTION__);

	m_swapchainResourceGroup = m_cleanupManager->createCleanupGroup();

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void OffscreenPipeline::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<OffscreenPipeline>();


	//m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
	//	[this](const UpdateEvent::SessionStatus &event) {
	//		using enum UpdateEvent::SessionStatus::Status;
	//
	//		switch (event.sessionStatus) {
	//		case RESET:
	//			m_sessionReady = false;
	//
	//			for (auto &cleanupID : m_offscreenResourceIDs)
	//				m_cleanupManager->executeCleanupTask(cleanupID);
	//			m_offscreenResourceIDs.clear();
	//
	//			m_cleanupManager->executeCleanupTask(m_sessionResourceGroup);
	//
	//			break;
	//		}
	//	}
	//);


	m_eventDispatcher->subscribe<RecreationEvent::Swapchain>(selfIndex,
		[this](const RecreationEvent::Swapchain& event) {
			if (!m_sessionReady)
				return;
	
			recreateOffscreenResources(event.newExtent.width, event.newExtent.height);
		}
	);
}


const Ctx::OffscreenPipeline *OffscreenPipeline::init() {
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

	//initDepthBufferingResources();	// Depth buffering image and view

	initTessellationState();		// Tessellation state



	// Load shaders
	initShaderStage();


	// Create descriptors
	setUpDescriptors();


	// Create the pipeline layout
	createPipelineLayout();


	// Create the render pass
	createRenderPass();


	// Create the graphics pipelines
	createGraphicsPipeline();
	createOrbitGraphicsPipeline();


	// Initialize offscreen resources
	recreateOffscreenResources(m_windowCtx->extent.width, m_windowCtx->extent.height);


	m_sessionReady = true;

	return &m_pipelineData;
}


void OffscreenPipeline::createGraphicsPipeline() {
	PipelineBuilder builder;
	builder.dynamicStateCreateInfo = &m_dynamicStateCreateInfo;
	builder.inputAssemblyCreateInfo = &m_inputAssemblyCreateInfo;
	builder.viewportStateCreateInfo = &m_viewportStateCreateInfo;
	builder.rasterizerCreateInfo = &m_rasterizerCreateInfo;
	builder.multisampleStateCreateInfo = &m_multisampleStateCreateInfo;
	builder.depthStencilStateCreateInfo = &m_depthStencilStateCreateInfo;
	builder.colorBlendStateCreateInfo = &m_colorBlendCreateInfo;
	builder.tessellationStateCreateInfo = &m_tessStateCreateInfo;
	builder.vertexInputStateCreateInfo = &m_vertInputState;

	builder.shaderStages = m_shaderStages;

	builder.renderPass = m_pipelineData.renderPass;
	builder.pipelineLayout = m_pipelineData.pipelineLayout;

	m_pipelineData.pipeline = builder.buildGraphicsPipeline(m_renderDeviceCtx->logicalDevice);
}


void OffscreenPipeline::createOrbitGraphicsPipeline() {
	PipelineBuilder builder;
	builder.dynamicStateCreateInfo = &m_dynamicStateCreateInfo;
	builder.viewportStateCreateInfo = &m_viewportStateCreateInfo;
	builder.multisampleStateCreateInfo = &m_multisampleStateCreateInfo;
	builder.colorBlendStateCreateInfo = &m_colorBlendCreateInfo;
	builder.tessellationStateCreateInfo = &m_tessStateCreateInfo;

	builder.inputAssemblyCreateInfo = &m_orbitIACreateInfo;
	builder.depthStencilStateCreateInfo = &m_orbitDepthStencilStateCreateInfo;
	builder.rasterizerCreateInfo = &m_orbitRasterizerCreateInfo;
	builder.vertexInputStateCreateInfo = &m_orbitVertInputState;
	builder.shaderStages = m_orbitShaderStages;

	builder.renderPass = m_pipelineData.renderPass;
	builder.pipelineLayout = m_pipelineData.pipelineLayout;

	m_pipelineData.orbitPipeline = builder.buildGraphicsPipeline(m_renderDeviceCtx->logicalDevice);
}


void OffscreenPipeline::createPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	createInfo.pSetLayouts = m_descriptorSetLayouts.data();

	// Push constants are a way of passing dynamic values to shaders
		// Orbit geometry: trajectory color
	m_orbitPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_orbitPushRange.offset = 0;
	m_orbitPushRange.size = sizeof(glm::vec4);

		// Write
	VkPushConstantRange pushConstants[] = {
		m_orbitPushRange
	};
	createInfo.pushConstantRangeCount = SIZE_OF(pushConstants);
	createInfo.pPushConstantRanges = pushConstants;

	VkResult result = vkCreatePipelineLayout(m_renderDeviceCtx->logicalDevice, &createInfo, nullptr, &m_pipelineData.pipelineLayout);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_pipelineData.pipelineLayout) };
	task.vkHandles = { m_pipelineData.pipelineLayout };
	task.cleanupFunc = [&]() { vkDestroyPipelineLayout(m_renderDeviceCtx->logicalDevice, m_pipelineData.pipelineLayout, nullptr); };

	m_cleanupManager->createCleanupTask(task, m_sessionResourceGroup);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create graphics pipeline layout for the offscreen pipeline!");
	}
}


void OffscreenPipeline::createRenderPass() {
	// Main attachments
		// Offscreen color attachment
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;				// Use 1 sample since multisampling is not enabled yet

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	// The render area will be cleared to a uniform value on every render pass instantiation. Since the render pass is run for every frame in our case, we effectively "refresh" the render area.
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Vulkan is free to discard any previous contents (which is fine because we are clearing it anyway)
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// This will be sampled in the present pipeline


	VkAttachmentReference mainColorAttachmentRef{};
	mainColorAttachmentRef.attachment = 0;
	mainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	// Depth attachment
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VkFormatUtils::GetBestDepthImageFormat(m_renderDeviceCtx->physicalDevice);
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
		// Offscreen subpass
	VkSubpassDescription offscreenSubpass{};
	offscreenSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	offscreenSubpass.colorAttachmentCount = 1;
	offscreenSubpass.pColorAttachments = &mainColorAttachmentRef;
	offscreenSubpass.pDepthStencilAttachment = &depthAttachmentRef;


	// Dependencies
		// EXTERNAL -> Offscreen (0)
	VkSubpassDependency offscreenDependency{};
	offscreenDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	offscreenDependency.dstSubpass = 0;
			// Since colorAttachment.initialLayout = UNDEFINED, we don't need to synchronize any `src` operations (and their memory accesses)
	offscreenDependency.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	offscreenDependency.srcAccessMask = 0;
	offscreenDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	offscreenDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			// Pre-emptively set dependency flag to this for future parallelism
	offscreenDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


	// Creates render pass
	VkAttachmentDescription attachments[] = {
		colorAttachment,
		depthAttachment
	};

	VkSubpassDescription subpasses[] = {
		offscreenSubpass
	};

	VkSubpassDependency dependencies[] = {
		offscreenDependency
	};

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(sizeof(attachments) / sizeof(attachments[0]));
	renderPassCreateInfo.pAttachments = attachments;

	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(sizeof(subpasses) / sizeof(subpasses[0]));
	renderPassCreateInfo.pSubpasses = subpasses;

	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(sizeof(dependencies) / sizeof(dependencies[0]));
	renderPassCreateInfo.pDependencies = dependencies;

	VkResult result = vkCreateRenderPass(m_renderDeviceCtx->logicalDevice, &renderPassCreateInfo, nullptr, &m_pipelineData.renderPass);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_pipelineData.renderPass) };
	task.vkHandles = { m_pipelineData.renderPass };
	task.cleanupFunc = [this]() { vkDestroyRenderPass(m_renderDeviceCtx->logicalDevice, m_pipelineData.renderPass, nullptr); };

	m_cleanupManager->createCleanupTask(task, m_sessionResourceGroup);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create render pass for offscreen pipeline!");
	}
}


void OffscreenPipeline::setUpDescriptors() {
	// Setup
		// Layout bindings
			// Global uniform buffer
	VkDescriptorSetLayoutBinding globalUBOLayoutBinding{};
	globalUBOLayoutBinding.binding = ShaderConst::VERT_BIND_GLOBAL_UBO;
	globalUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalUBOLayoutBinding.descriptorCount = 1;
	globalUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Specifies which shader stages will the UBO(s) be referenced and used (through `VkShaderStageFlagBits` values; see the specification for more information)
	globalUBOLayoutBinding.pImmutableSamplers = nullptr;			// Specifies descriptors handling image-sampling


			// Per-object uniform buffer
	VkDescriptorSetLayoutBinding objectUBOLayoutBinding{};
	objectUBOLayoutBinding.binding = ShaderConst::VERT_BIND_OBJECT_UBO;
	objectUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;  // Allows the same descriptor to reference different offsets within a uniform buffer at draw time. That is, there will be a single big buffer with all object UBOs for each frame, and making this descriptor dynamic lets you bind this buffer once, and access it via offsets.
	objectUBOLayoutBinding.descriptorCount = 1;
	objectUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	objectUBOLayoutBinding.pImmutableSamplers = nullptr;


			// PBR textures
				// Material parameters UBO
	VkDescriptorSetLayoutBinding pbrLayoutBinding{};
	pbrLayoutBinding.binding = ShaderConst::FRAG_BIND_MATERIAL_PARAMETERS;
	pbrLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pbrLayoutBinding.descriptorCount = 1;
	pbrLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pbrLayoutBinding.pImmutableSamplers = nullptr;

			// Texture array
	VkDescriptorSetLayoutBinding texArrayLayoutBinding{};
	texArrayLayoutBinding.binding = ShaderConst::FRAG_BIND_TEXTURE_MAP;
	texArrayLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texArrayLayoutBinding.descriptorCount = SimulationConst::MAX_GLOBAL_TEXTURES;
	texArrayLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texArrayLayoutBinding.pImmutableSamplers = nullptr;

				// Descriptor binding flags for the texture array
	VkDescriptorBindingFlags texArrayBindingFlags =
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |			// Allows descriptors to initially be null (as they'll be dynamically updated)
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |		// Allows updating descriptors after binding pipeline
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;// Allows actual descriptor count to be less than MAX_GLOBAL_TEXTURES

	VkDescriptorSetLayoutBindingFlagsCreateInfo texArrayBindingFlagsCreateInfo{};
	texArrayBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	texArrayBindingFlagsCreateInfo.bindingCount = 1;
	texArrayBindingFlagsCreateInfo.pBindingFlags = &texArrayBindingFlags;



		// Data organization
			// Layout bindings
	VkDescriptorSetLayoutBinding perFrameLayoutBindings[] = {
		globalUBOLayoutBinding,
		objectUBOLayoutBinding
	};

			// Descriptor pool allocation
	VkDescriptorPool perFrameDescriptorPool;
	VkDescriptorPoolSize perFramePoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(SimulationConst::MAX_FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, static_cast<uint32_t>(SimulationConst::MAX_FRAMES_IN_FLIGHT) }
	};


	VkDescriptorPool pbrDescriptorPool;
	VkDescriptorPoolSize pbrPoolSize{
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1
	};

	VkDescriptorPool texArrayDescriptorPool;
	VkDescriptorPoolSize texArrayPoolSize{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = SimulationConst::MAX_GLOBAL_TEXTURES
	};



	// Common descriptor properties creation
	// TODO: Modify CreateDescriptorPool to account for the actual number of maximum descriptor sets (not a fixed large value like 500)

		// Descriptor pools
	VkDescriptorUtils::CreateDescriptorPool(m_renderDeviceCtx->logicalDevice, perFrameDescriptorPool, SIZE_OF(perFramePoolSizes), perFramePoolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 500);
	VkDescriptorUtils::CreateDescriptorPool(m_renderDeviceCtx->logicalDevice, pbrDescriptorPool, 1, &pbrPoolSize, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 10);
	VkDescriptorUtils::CreateDescriptorPool(m_renderDeviceCtx->logicalDevice, texArrayDescriptorPool, 1, &texArrayPoolSize, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, SimulationConst::MAX_GLOBAL_TEXTURES);
	
		// Descriptor set layouts
	VkDescriptorSetLayout setLayout0 = createDescriptorSetLayout(SIZE_OF(perFrameLayoutBindings), perFrameLayoutBindings, 0, nullptr);
	VkDescriptorSetLayout setLayout1 = createDescriptorSetLayout(1, &pbrLayoutBinding, 0, nullptr);
	VkDescriptorSetLayout setLayout2 = createDescriptorSetLayout(1, &texArrayLayoutBinding, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, &texArrayBindingFlagsCreateInfo);

		// NOTE: Indices are important in this vector! Make sure the descriptor set layouts are at the correct indices.
	m_descriptorSetLayouts = {
		setLayout0,		// Set 0: Per-frame
		setLayout1,		// Set 1: Material parameters UBO
		setLayout2		// Set 2: Textures array
	};


	// Specific descriptor set creation
		// Per-frame descriptor sets
	std::vector<VkDescriptorSetLayout> descSetLayouts(SimulationConst::MAX_FRAMES_IN_FLIGHT, setLayout0);
	m_pipelineData.perFrameDescriptorSets.resize(descSetLayouts.size());

	createDescriptorSets(perFrameDescriptorPool, descSetLayouts.size(), m_pipelineData.perFrameDescriptorSets.data(), descSetLayouts.data(), nullptr);
	
		// Singular descriptor sets
			// Material parameters UBO
	createDescriptorSets(pbrDescriptorPool, 1, &m_pipelineData.materialDescriptorSet, &setLayout1, nullptr);

			// Textures array
	const uint32_t initialDescriptorCount = 20;
	VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescSetAllocInfo{};
	variableDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	variableDescSetAllocInfo.descriptorSetCount = 1;
	variableDescSetAllocInfo.pDescriptorCounts = &initialDescriptorCount;

	createDescriptorSets(texArrayDescriptorPool, 1, &m_pipelineData.texArrayDescriptorSet, &setLayout2, &variableDescSetAllocInfo);
}


VkDescriptorSetLayout OffscreenPipeline::createDescriptorSetLayout(uint32_t bindingCount, VkDescriptorSetLayoutBinding *layoutBindings, VkDescriptorSetLayoutCreateFlags layoutFlags, const void *pNext) {
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.flags = layoutFlags;
	layoutCreateInfo.bindingCount = bindingCount;
	layoutCreateInfo.pBindings = layoutBindings;
	layoutCreateInfo.pNext = pNext;

	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(m_renderDeviceCtx->logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
	
	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(descriptorSetLayout) };
	task.vkHandles = { descriptorSetLayout };
	task.cleanupFunc = [this, descriptorSetLayout]() {
		vkDestroyDescriptorSetLayout(m_renderDeviceCtx->logicalDevice, descriptorSetLayout, nullptr);
	};
	
	m_cleanupManager->createCleanupTask(task, m_sessionResourceGroup);

	LOG_ASSERT(result == VK_SUCCESS, "Failed to create descriptor set layout!");

	return descriptorSetLayout;
}


void OffscreenPipeline::createDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, VkDescriptorSet *descriptorSets, VkDescriptorSetLayout *descriptorSetLayouts, const void *pNext) {
	VkDescriptorSetAllocateInfo descSetAllocInfo{};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.descriptorPool = descriptorPool;
	descSetAllocInfo.descriptorSetCount = descriptorSetCount;
	descSetAllocInfo.pSetLayouts = descriptorSetLayouts;
	descSetAllocInfo.pNext = pNext;

	VkResult result = vkAllocateDescriptorSets(m_renderDeviceCtx->logicalDevice, &descSetAllocInfo, descriptorSets);
	LOG_ASSERT(result == VK_SUCCESS, "Failed to create descriptor sets!");

	CleanupTask task{};
	task.caller = __FUNCTION__;
	for (int i = 0; i < descriptorSetCount; i++) {
		task.objectNames.push_back(VARIABLE_NAME(descriptorSets) + "[" + std::to_string(i) + "]");
		task.vkHandles.push_back(descriptorSets[i]);
	}

	task.cleanupFunc = [this, descriptorSetCount, descriptorSets, descriptorPool]() {
		vkFreeDescriptorSets(m_renderDeviceCtx->logicalDevice, descriptorPool, descriptorSetCount, descriptorSets);
	};

	m_cleanupManager->createCleanupTask(task, m_sessionResourceGroup);
}


void OffscreenPipeline::initShaderStage() {
	// Loads shader bytecode onto buffers
	static std::function<void(VkShaderModule &, const std::string &, const std::string)> loadShader = [this](VkShaderModule &module, const std::string &shaderPath, const std::string shaderPurpose) {
		const std::vector<char> buf = FilePathUtils::ReadFile(shaderPath);
		Log::Print(Log::T_SUCCESS, __FUNCTION__, ("Loaded " + shaderPurpose + ". SPIR-V bytecode file size is " + std::to_string(buf.size()) + " (bytes)."));
		module = createShaderModule(buf);
	};

		// Opaque geometry
	loadShader(m_vertShaderModule, ShaderConst::OPAQUE_GEOM_VERTEX, "opaque geometry shader (vertex)");
	loadShader(m_fragShaderModule, ShaderConst::OPAQUE_GEOM_FRAGMENT, "opaque geometry shader (fragment)");

		// Orbit geometry
	loadShader(m_orbitVertShaderModule, ShaderConst::ORBIT_GEOM_VERTEX, "orbit geometry shader (vertex)");
	loadShader(m_orbitFragShaderModule, ShaderConst::ORBIT_GEOM_FRAGMENT, "orbit geometry shader (fragment)");


	// Creates shader stages
		// Opaque geometry - Vertex shader
	VkPipelineShaderStageCreateInfo vertStageInfo{};
	{
		vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Used to identify the create info's shader as the Vertex shader
		vertStageInfo.module = m_vertShaderModule;
		vertStageInfo.pName = "main"; // pName specifies the function to invoke, known as the entry point. This means that it is possible to combine multiple fragment shaders into a single shader module and use different entry points to differentiate between their behaviors. In this case we’ll stick to the standard `main`, however.
	}

		// Opaque geometry - Fragment shader
	VkPipelineShaderStageCreateInfo fragStageInfo{};
	{
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = m_fragShaderModule;
		fragStageInfo.pName = "main";
	}
	
		// Orbit geometry - Vertex shader
	VkPipelineShaderStageCreateInfo orbitVertStageInfo{};
	{
		orbitVertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		orbitVertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		orbitVertStageInfo.module = m_orbitVertShaderModule;
		orbitVertStageInfo.pName = "main";
	}

		// Orbit geometry - Fragment shader
	VkPipelineShaderStageCreateInfo orbitFragStageInfo{};
	{
		orbitFragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		orbitFragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		orbitFragStageInfo.module = m_orbitFragShaderModule;
		orbitFragStageInfo.pName = "main";
	}


	m_shaderStages = {
		vertStageInfo,
		fragStageInfo
	};

	m_orbitShaderStages = {
		orbitVertStageInfo,
		orbitFragStageInfo
	};


	// Specifies the format of the vertex data to be passed to the vertex buffer
		// Opaque geometry
	{
		m_vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// Describes binding, i.e., spacing between the data and whether the data is per-vertex or per-instance
			// Gets vertex input binding and attribute descriptions
		m_vertBindingDescription = Geometry::Vertex::getVertexInputBindingDescription();
		m_vertAttribDescriptions = Geometry::Vertex::getVertexAttributeDescriptions();

		m_vertInputState.vertexBindingDescriptionCount = 1;
		m_vertInputState.pVertexBindingDescriptions = &m_vertBindingDescription;

		m_vertInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertAttribDescriptions.size());
		m_vertInputState.pVertexAttributeDescriptions = m_vertAttribDescriptions.data();
	}

		// Orbit geometry
	{
		m_orbitVertBindingDescription.binding = 0;
		m_orbitVertBindingDescription.stride = sizeof(glm::vec3);
		m_orbitVertBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		m_orbitVertAttribDescription.binding = 0;
		m_orbitVertAttribDescription.location = 0;
		m_orbitVertAttribDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
		m_orbitVertAttribDescription.offset = 0;

		m_orbitVertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_orbitVertInputState.vertexBindingDescriptionCount = 1;
		m_orbitVertInputState.pVertexBindingDescriptions = &m_orbitVertBindingDescription;
		m_orbitVertInputState.vertexAttributeDescriptionCount = 1;
		m_orbitVertInputState.pVertexAttributeDescriptions = &m_orbitVertAttribDescription;
	}


	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_vertShaderModule), VARIABLE_NAME(m_fragShaderModule), VARIABLE_NAME(m_orbitVertShaderModule), VARIABLE_NAME(m_orbitFragShaderModule) };
	task.vkHandles = { m_vertShaderModule, m_fragShaderModule, m_orbitVertShaderModule, m_orbitFragShaderModule };
	task.cleanupFunc = [&]() {
		vkDestroyShaderModule(m_renderDeviceCtx->logicalDevice, m_vertShaderModule, nullptr);
		vkDestroyShaderModule(m_renderDeviceCtx->logicalDevice, m_fragShaderModule, nullptr);

		vkDestroyShaderModule(m_renderDeviceCtx->logicalDevice, m_orbitVertShaderModule, nullptr);
		vkDestroyShaderModule(m_renderDeviceCtx->logicalDevice, m_orbitFragShaderModule, nullptr);
	};

	m_cleanupManager->createCleanupTask(task, m_sessionResourceGroup);
}


void OffscreenPipeline::initDynamicStates() {
	m_dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	m_dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
	m_dynamicStateCreateInfo.pDynamicStates = m_dynamicStates.data();
}


void OffscreenPipeline::initInputAssemblyState() {
	// Opaque geometry
	m_inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Use PATCH_LIST instead of TRIANGLE_LIST for tessellation
	m_inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	
	// Orbit trajectory geometry
	m_orbitIACreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_orbitIACreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	m_orbitIACreateInfo.primitiveRestartEnable = VK_FALSE;
}


void OffscreenPipeline::initViewportState() {
	m_viewport.x = m_viewport.y = 0.0f;
	m_viewport.width = static_cast<float>(m_windowCtx->extent.width);
	m_viewport.height = static_cast<float>(m_windowCtx->extent.height);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	// Since we want to draw the entire framebuffer, we'll specify a scissor rectangle that covers it entirely (i.e., that has the same extent as the swap chain's)
	// If we want to (re)draw only a partial part of the framebuffer from (a, b) to (x, y), we'll specify the offset as {a, b} and extent as {x, y}
	m_scissorRectangle.offset = { 0, 0 };
	m_scissorRectangle.extent = m_windowCtx->extent;

	// NOTE: We don't need to specify pViewports and pScissors since the m_viewport was set as a dynamic state. Therefore, we only need to specify the m_viewport and scissor counts at pipeline creation time. The actual objects can be set up later at drawing time.
	m_viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	//m_viewportStateCreateInfo.pViewports = &m_viewport;
	m_viewportStateCreateInfo.viewportCount = 1;
	//m_viewportStateCreateInfo.pScissors = &m_scissorRectangle;
	m_viewportStateCreateInfo.scissorCount = 1;
}


void OffscreenPipeline::initRasterizationState() {
	// Opaque geometry
	{
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


	// Orbit geometry
	{
		m_orbitRasterizerCreateInfo = m_rasterizerCreateInfo;
		m_orbitRasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
	}
}


void OffscreenPipeline::initMultisamplingState() {
	m_multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	m_multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampleStateCreateInfo.minSampleShading = 1.0f;
	m_multisampleStateCreateInfo.pSampleMask = nullptr;
	m_multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	m_multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
}


void OffscreenPipeline::initDepthStencilState() {
	// Opaque geometry
	{
		m_depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		// Specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
		m_depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;

		// Specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
		m_depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;


		// Specifies the depth comparison operator that is performed to determine whether to keep or discard a fragment.
		// VK_COMPARE_OP_LESS means "lower depth = closer". In other words, the depth value of new fragments should be LESS since they are closer to the camera, and thus they will overwrite the existing fragments.
		// However, since we are using inverted Z-depth mapping (i.e., reverse-Z), we must use VK_COMPARE_OP_GREATER.
		m_depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER;

		// Configures depth bound testing (optional). It allows you to only keep fragments that fall within the specified depth range.
		// We won't be using this for now.
		m_depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		m_depthStencilStateCreateInfo.minDepthBounds = 0.0f;
		m_depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

		// Stencil buffer operations
		m_depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		m_depthStencilStateCreateInfo.front = {};
		m_depthStencilStateCreateInfo.back = {};
	}


	// Orbit trajectory geometry: read depth but don't write
	{
		m_orbitDepthStencilStateCreateInfo = m_depthStencilStateCreateInfo;
		m_orbitDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
	}
}


void OffscreenPipeline::initColorBlendingState() {
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


void OffscreenPipeline::initTessellationState() {
	m_tessStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	m_tessStateCreateInfo.patchControlPoints = 3; // Number of control points per patch (e.g., 3 for triangles)
}


void OffscreenPipeline::recreateOffscreenResources(uint32_t width, uint32_t height) {
	m_cleanupManager->executeCleanupTask(m_swapchainResourceGroup);

	initDepthBufferingResources(width, height);
	initOffscreenColorResources(width, height);
	initOffscreenSampler();
	initOffscreenFramebuffer(width, height);

	m_eventDispatcher->dispatch(RecreationEvent::OffscreenResources{});
}


void OffscreenPipeline::initDepthBufferingResources(uint32_t width, uint32_t height) {
	// Specifies depth image data
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageAspectFlags imgAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

	uint32_t imgDepth = 1;
	VkFormat depthFormat = VkFormatUtils::GetBestDepthImageFormat(m_renderDeviceCtx->physicalDevice);

	bool hasStencilComponent = VkFormatUtils::FormatHasStencilComponent(depthFormat);


	// Creates a depth image
	VmaAllocationCreateInfo imgAllocInfo{};
	imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkImageUtils::CreateImage(m_renderDeviceCtx, m_depthImage, m_depthImgAllocation, imgAllocInfo, width, height, imgDepth, depthFormat, imgTiling, imgUsage, VK_IMAGE_TYPE_2D);


	// Creates a depth image view
	VkImageUtils::CreateImageView(m_renderDeviceCtx->logicalDevice, m_depthImgView, m_depthImage, depthFormat, imgAspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1, 1);


	// Explicitly transitions the layout of the depth image to a depth attachment.
	// This is not necessary, since it will be done in the render pass anyway. This is rather being explicit for the sake of being explicit. 
	VkCommandBuffer secondaryCmdBuf;

	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = m_pipelineData.renderPass;
	inheritanceInfo.subpass = 0;	// Offscreen subpass

	TextureManager::SwitchImageLayout(m_renderDeviceCtx, m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, true, &secondaryCmdBuf, &inheritanceInfo);
}


void OffscreenPipeline::initOffscreenColorResources(uint32_t width, uint32_t height) {
	m_pipelineData.images.resize(m_OFFSCREEN_RESOURCE_COUNT);
	m_pipelineData.imageViews.resize(m_OFFSCREEN_RESOURCE_COUNT);
	m_pipelineData.imageSamplers.resize(m_OFFSCREEN_RESOURCE_COUNT);
	m_pipelineData.frameBuffers.resize(m_OFFSCREEN_RESOURCE_COUNT);


	// Image
	uint32_t depth = 1;

	VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // NOTE: Use SAMPLED_BIT as we will be sampling the simulation render as a texture later
	VkImageType imgType = VK_IMAGE_TYPE_2D;


	// Image allocation
	VmaAllocationCreateInfo imgAllocInfo{};
	imgAllocInfo.flags = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;


	// Image view
	VkImageAspectFlags imgAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	uint32_t levelCount = 1;
	uint32_t layerCount = 1;


	for (size_t i = 0; i < m_OFFSCREEN_RESOURCE_COUNT; i++) {
		uint32_t imageResourceID = VkImageUtils::CreateImage(m_renderDeviceCtx, m_pipelineData.images[i], m_colorImgAlloc, imgAllocInfo, width, height, depth, imgFormat, imgTiling, imgUsageFlags, imgType);

		uint32_t imageViewResourceID = VkImageUtils::CreateImageView(m_renderDeviceCtx->logicalDevice, m_pipelineData.imageViews[i], m_pipelineData.images[i], imgFormat, imgAspectFlags, viewType, levelCount, layerCount);

		m_cleanupManager->addTaskDependency(imageResourceID, m_swapchainResourceGroup);
		m_cleanupManager->addTaskDependency(imageViewResourceID, m_swapchainResourceGroup);
	}
}


void OffscreenPipeline::initOffscreenSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = m_renderDeviceCtx->chosenDevice.properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Color for clamp_to_border
	samplerInfo.unnormalizedCoordinates = VK_FALSE; // UVs are [0, 1]
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	for (size_t i = 0; i < m_OFFSCREEN_RESOURCE_COUNT; i++) {

		VkResult result = vkCreateSampler(m_renderDeviceCtx->logicalDevice, &samplerInfo, nullptr, &m_pipelineData.imageSamplers[i]);
		
		VkSampler& sampler = m_pipelineData.imageSamplers[i];
		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(m_colorImgSampler) };
		task.vkHandles = { sampler };

		task.cleanupFunc = [this, sampler]() {
			vkDestroySampler(m_renderDeviceCtx->logicalDevice, sampler, nullptr);
		};

		uint32_t samplerResourceID = m_cleanupManager->createCleanupTask(task);
		m_cleanupManager->addTaskDependency(samplerResourceID, m_swapchainResourceGroup);


		LOG_ASSERT(result == VK_SUCCESS, "Failed to create offscreen sampler!");
	}
}


void OffscreenPipeline::initOffscreenFramebuffer(uint32_t width, uint32_t height) {
	for (size_t i = 0; i < m_OFFSCREEN_RESOURCE_COUNT; i++) {
		std::vector<VkImageView> attachments = {
			m_pipelineData.imageViews[i],
			m_depthImgView
		};

		uint32_t framebufferResourceID = VkImageUtils::CreateFramebuffer(m_renderDeviceCtx->logicalDevice, m_pipelineData.frameBuffers[i], m_pipelineData.renderPass, attachments, width, height);

		m_cleanupManager->addTaskDependency(framebufferResourceID, m_swapchainResourceGroup);
	}
}


VkShaderModule OffscreenPipeline::createShaderModule(const std::vector<char>& bytecode) {
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = bytecode.size();
	moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_renderDeviceCtx->logicalDevice, &moduleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create shader module!");
	}

	return shaderModule;
}
