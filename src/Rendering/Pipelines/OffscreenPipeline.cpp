#include "OffscreenPipeline.hpp"


OffscreenPipeline::OffscreenPipeline() {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_bufferManager = ServiceLocator::GetService<VkBufferManager>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void OffscreenPipeline::bindEvents() {
	m_eventDispatcher->subscribe<Event::RequestInitSceneResources>(
		[this](const Event::RequestInitSceneResources &event) {
			this->init();
		}
	);


	m_eventDispatcher->subscribe<Event::UpdateSessionStatus>(
		[this](const Event::UpdateSessionStatus &event) {
			using namespace Event;

			switch (event.sessionStatus) {
			case UpdateSessionStatus::Status::NOT_READY:
				m_sessionReady = false;
				break;

			case UpdateSessionStatus::Status::PREPARE_FOR_INIT:
				m_sessionReady = true;

				for (auto &cleanupID : m_offscreenCleanupIDs)
					m_garbageCollector->executeCleanupTask(cleanupID);
				m_offscreenCleanupIDs.clear();

				for (auto &cleanupID : m_sessionCleanupIDs)
					m_garbageCollector->executeCleanupTask(cleanupID);
				m_sessionCleanupIDs.clear();

				break;

			case UpdateSessionStatus::Status::INITIALIZED:
				m_sessionReady = true;
			}
		}
	);


	m_eventDispatcher->subscribe<Event::SwapchainIsRecreated>(
		[this](const Event::SwapchainIsRecreated& event) {
			if (!m_sessionReady)
				return;

			this->recreateOffscreenResources(g_vkContext.SwapChain.extent.width, g_vkContext.SwapChain.extent.height, event.imageIndex);
		}
	);
}


void OffscreenPipeline::init() {
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


	// Initialize offscreen resources
	initOffscreenColorResources(g_vkContext.SwapChain.extent.width, g_vkContext.SwapChain.extent.height);
	initOffscreenSampler();
	initOffscreenFramebuffer(g_vkContext.SwapChain.extent.width, g_vkContext.SwapChain.extent.height);

	m_eventDispatcher->publish(Event::OffscreenPipelineInitialized{});
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

	builder.renderPass = m_renderPass;
	builder.pipelineLayout = m_pipelineLayout;

	m_graphicsPipeline = builder.buildGraphicsPipeline(g_vkContext.Device.logicalDevice);

	g_vkContext.OffscreenPipeline.pipeline = m_graphicsPipeline;
}


void OffscreenPipeline::createPipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	createInfo.pSetLayouts = m_descriptorSetLayouts.data();

	// Push constants are a way of passing dynamic values to shaders
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(g_vkContext.Device.logicalDevice, &createInfo, nullptr, &m_pipelineLayout);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_pipelineLayout) };
	task.vkHandles = { g_vkContext.Device.logicalDevice, m_pipelineLayout };
	task.cleanupFunc = [&]() { vkDestroyPipelineLayout(g_vkContext.Device.logicalDevice, m_pipelineLayout, nullptr); };

	m_sessionCleanupIDs.push_back(
		m_garbageCollector->createCleanupTask(task)
	);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create graphics pipeline layout for the offscreen pipeline!");
	}

	g_vkContext.OffscreenPipeline.layout = m_pipelineLayout;
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
	depthAttachment.format = VkFormatUtils::GetBestDepthImageFormat(g_vkContext.Device.physicalDevice);
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
	task.vkHandles = { g_vkContext.Device.logicalDevice, m_renderPass };
	task.cleanupFunc = [this]() { vkDestroyRenderPass(g_vkContext.Device.logicalDevice, m_renderPass, nullptr); };

	m_sessionCleanupIDs.push_back(
		m_garbageCollector->createCleanupTask(task)
	);


	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create render pass for offscreen pipeline!");
	}

	g_vkContext.OffscreenPipeline.renderPass = m_renderPass;
	g_vkContext.OffscreenPipeline.subpassCount = m_renderPassCreateInfo.subpassCount;
}


void OffscreenPipeline::setUpDescriptors() {
	// Setup
		// Layout bindings
			// Global uniform buffer
	VkDescriptorSetLayoutBinding globalUBOLayoutBinding{};
	globalUBOLayoutBinding.binding = ShaderConsts::VERT_BIND_GLOBAL_UBO;
	globalUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalUBOLayoutBinding.descriptorCount = 1;
	globalUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Specifies which shader stages will the UBO(s) be referenced and used (through `VkShaderStageFlagBits` values; see the specification for more information)
	globalUBOLayoutBinding.pImmutableSamplers = nullptr;			// Specifies descriptors handling image-sampling


	// Per-object uniform buffer
	VkDescriptorSetLayoutBinding objectUBOLayoutBinding{};
	objectUBOLayoutBinding.binding = ShaderConsts::VERT_BIND_OBJECT_UBO;
	objectUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;  // Allows the same descriptor to reference different offsets within a uniform buffer at draw time. That is, there will be a single big buffer with all object UBOs for each frame, and making this descriptor dynamic lets you bind this buffer once, and access it via offsets.
	objectUBOLayoutBinding.descriptorCount = 1;
	objectUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	objectUBOLayoutBinding.pImmutableSamplers = nullptr;


	// PBR textures
		// Material parameters UBO
	VkDescriptorSetLayoutBinding pbrLayoutBinding{};
	pbrLayoutBinding.binding = ShaderConsts::FRAG_BIND_MATERIAL_PARAMETERS;
	pbrLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pbrLayoutBinding.descriptorCount = 1;
	pbrLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pbrLayoutBinding.pImmutableSamplers = nullptr;

		// Texture array
	VkDescriptorSetLayoutBinding texArrayLayoutBinding{};
	texArrayLayoutBinding.binding = ShaderConsts::FRAG_BIND_TEXTURE_MAP;
	texArrayLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texArrayLayoutBinding.descriptorCount = SimulationConsts::MAX_GLOBAL_TEXTURES;
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
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(SimulationConsts::MAX_FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, static_cast<uint32_t>(SimulationConsts::MAX_FRAMES_IN_FLIGHT) }
	};


	VkDescriptorPool pbrDescriptorPool;
	VkDescriptorPoolSize pbrPoolSize{
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1
	};

	VkDescriptorPool texArrayDescriptorPool;
	VkDescriptorPoolSize texArrayPoolSize{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = SimulationConsts::MAX_GLOBAL_TEXTURES
	};



	// Common descriptor properties creation
	// TODO: Modify CreateDescriptorPool to account for the actual number of maximum descriptor sets (not a fixed large value like 500)

		// Descriptor pools
	VkDescriptorUtils::CreateDescriptorPool(perFrameDescriptorPool, SIZE_OF(perFramePoolSizes), perFramePoolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 500);
	VkDescriptorUtils::CreateDescriptorPool(pbrDescriptorPool, 1, &pbrPoolSize, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 10);
	VkDescriptorUtils::CreateDescriptorPool(texArrayDescriptorPool, 1, &texArrayPoolSize, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, SimulationConsts::MAX_GLOBAL_TEXTURES);
	
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
	createPerFrameDescriptorSets(perFrameDescriptorPool, setLayout0);
	
		// Singular descriptor sets
			// Material parameters UBO
	VkDescriptorSet pbrDescriptorSet;
	createSingularDescriptorSet(pbrDescriptorSet, pbrDescriptorPool, setLayout1, nullptr);
	g_vkContext.Textures.pbrDescriptorSet = pbrDescriptorSet;

			// Textures array
	const uint32_t initialDescriptorCount = 20;
	VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescSetAllocInfo{};
	variableDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	variableDescSetAllocInfo.descriptorSetCount = 1;
	variableDescSetAllocInfo.pDescriptorCounts = &initialDescriptorCount;

	VkDescriptorSet texArrayDescriptorSet;
	createSingularDescriptorSet(texArrayDescriptorSet, texArrayDescriptorPool, setLayout2, &variableDescSetAllocInfo);
	g_vkContext.Textures.texArrayDescriptorSet = texArrayDescriptorSet;
	g_vkContext.Textures.actualTextureCount = initialDescriptorCount;
}


VkDescriptorSetLayout OffscreenPipeline::createDescriptorSetLayout(uint32_t bindingCount, VkDescriptorSetLayoutBinding *layoutBindings, VkDescriptorSetLayoutCreateFlags layoutFlags, const void *pNext) {
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.flags = layoutFlags;
	layoutCreateInfo.bindingCount = bindingCount;
	layoutCreateInfo.pBindings = layoutBindings;
	layoutCreateInfo.pNext = pNext;

	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(g_vkContext.Device.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
	
	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(descriptorSetLayout) };
	task.vkHandles = { g_vkContext.Device.logicalDevice, descriptorSetLayout };
	task.cleanupFunc = [this, descriptorSetLayout]() {
		vkDestroyDescriptorSetLayout(g_vkContext.Device.logicalDevice, descriptorSetLayout, nullptr);
	};
	
	m_sessionCleanupIDs.push_back(
		m_garbageCollector->createCleanupTask(task)
	);

	LOG_ASSERT(result == VK_SUCCESS, "Failed to create descriptor set layout!");

	return descriptorSetLayout;
}


void OffscreenPipeline::createPerFrameDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout) {
	// Creates one descriptor set for every frame in flight (all with the same layout)
	// NOTE/TODO: Right now, our single pool handles all bindings across all sets. That's okay for small-scale applications, but can bottleneck fast if we scale. To solve this, use separate pools per descriptor type if we are hitting fragmentation or pool exhaustion.

	std::vector<VkDescriptorSetLayout> descSetLayouts(SimulationConsts::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	VkDescriptorSetAllocateInfo descSetAllocInfo{};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.descriptorPool = descriptorPool;

	descSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(descSetLayouts.size());
	descSetAllocInfo.pSetLayouts = descSetLayouts.data();


	// Allocates descriptor sets
	m_perFrameDescriptorSets.resize(descSetLayouts.size());

	VkResult result = vkAllocateDescriptorSets(g_vkContext.Device.logicalDevice, &descSetAllocInfo, m_perFrameDescriptorSets.data());

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_perFrameDescriptorSets) };
	task.vkHandles = { g_vkContext.Device.logicalDevice, descriptorPool };
	task.cleanupFunc = [this, descriptorPool]() {
		vkFreeDescriptorSets(g_vkContext.Device.logicalDevice, descriptorPool, m_perFrameDescriptorSets.size(), m_perFrameDescriptorSets.data());
	};

	m_sessionCleanupIDs.push_back(
		m_garbageCollector->createCleanupTask(task)
	);

	LOG_ASSERT(result == VK_SUCCESS, "Failed to create per-frame descriptor sets!");


	// Configures the descriptors within the newly allocated descriptor sets
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	for (size_t i = 0; i < descSetLayouts.size(); i++) {
		descriptorWrites.clear();

		// Global uniform buffer
		VkDescriptorBufferInfo globalUBOInfo{};
		globalUBOInfo.buffer = m_bufferManager->getGlobalUBOs()[i].buffer;
		globalUBOInfo.offset = 0;
		globalUBOInfo.range = sizeof(Buffer::GlobalUBO); // Note: We can also use VK_WHOLE_SIZE if we want to overwrite the whole buffer (like what we're doing)


		// Per-object uniform buffer
		size_t alignedObjectUBOSize = SystemUtils::Align(sizeof(Buffer::ObjectUBO), g_vkContext.Device.deviceProperties.limits.minUniformBufferOffsetAlignment);

		VkDescriptorBufferInfo objectUBOInfo{};
		objectUBOInfo.buffer = m_bufferManager->getObjectUBOs()[i].buffer;
		objectUBOInfo.offset = 0; // Offset will be dynamic during draw calls
		objectUBOInfo.range = alignedObjectUBOSize;


		// Updates the configuration for each descriptor
			// Global uniform buffer descriptor write
		VkWriteDescriptorSet globalUBODescWrite{};
		globalUBODescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		globalUBODescWrite.dstSet = m_perFrameDescriptorSets[i];
		globalUBODescWrite.dstBinding = ShaderConsts::VERT_BIND_GLOBAL_UBO;

		// Since descriptors can be arrays, we must specify the first descriptor's index to update in the array.
		// We are not using an array now, so we can leave it at 0.
		globalUBODescWrite.dstArrayElement = 0;

		globalUBODescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalUBODescWrite.descriptorCount = 1; // Specifies how many array elements to update (refer to `VkWriteDescriptorSet::dstArrayElement`)

		/* The descriptor write configuration also needs a reference to its Info struct, and this part depends on the type of descriptor:

			- `VkWriteDescriptorSet::pBufferInfo`: Used for descriptors that refer to buffer data
			- `VkWriteDescriptorSet::pImageInfo`: Used for descriptors that refer to image data
			- `VkWriteDescriptorSet::pTexelBufferInfo`: Used for descriptors that refer to buffer views

			We can only choose 1 out of 3.
		*/
		globalUBODescWrite.pBufferInfo = &globalUBOInfo;
		//uniformBufferDescWrite.pImageInfo = nullptr;
		//uniformBufferDescWrite.pTexelBufferView = nullptr;

		descriptorWrites.push_back(globalUBODescWrite);


		// Object uniform buffer descriptor write
		VkWriteDescriptorSet objectUBODescWrite{};
		objectUBODescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		objectUBODescWrite.dstSet = m_perFrameDescriptorSets[i];
		objectUBODescWrite.dstBinding = ShaderConsts::VERT_BIND_OBJECT_UBO;
		objectUBODescWrite.dstArrayElement = 0;
		objectUBODescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		objectUBODescWrite.descriptorCount = 1;
		objectUBODescWrite.pBufferInfo = &objectUBOInfo;

		descriptorWrites.push_back(objectUBODescWrite);


		// Applies the updates
		vkUpdateDescriptorSets(g_vkContext.Device.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}


	g_vkContext.OffscreenPipeline.perFrameDescriptorSets = m_perFrameDescriptorSets;
}


void OffscreenPipeline::createSingularDescriptorSet(VkDescriptorSet &descriptorSet, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, const void *pNext) {
	VkDescriptorSetAllocateInfo descSetAllocInfo{};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.descriptorPool = descriptorPool;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	descSetAllocInfo.pNext = pNext;

	VkResult result = vkAllocateDescriptorSets(g_vkContext.Device.logicalDevice, &descSetAllocInfo, &descriptorSet);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(descriptorSet) };
	task.vkHandles = { g_vkContext.Device.logicalDevice, descriptorPool };
	task.cleanupFunc = [this, descriptorSet, descriptorPool]() {
		vkFreeDescriptorSets(g_vkContext.Device.logicalDevice, descriptorPool, 1, &descriptorSet);
	};

	m_sessionCleanupIDs.push_back(
		m_garbageCollector->createCleanupTask(task)
	);

	LOG_ASSERT(result == VK_SUCCESS, "Failed to create singular descriptor set!");
}


void OffscreenPipeline::initShaderStage() {
	// Loads shader bytecode onto buffers
		// Vertex shader
	m_vertShaderBytecode = FilePathUtils::ReadFile(ShaderConsts::VERTEX);
	Log::Print(Log::T_SUCCESS, __FUNCTION__, ("Loaded vertex shader! SPIR-V bytecode file size is " + std::to_string(m_vertShaderBytecode.size()) + " (bytes)."));
	m_vertShaderModule = createShaderModule(m_vertShaderBytecode);

	// Fragment shader
	m_fragShaderBytecode = FilePathUtils::ReadFile(ShaderConsts::FRAGMENT);
	Log::Print(Log::T_SUCCESS, __FUNCTION__, ("Loaded fragment shader! SPIR-V bytecode file size is " + std::to_string(m_fragShaderBytecode.size()) + " (bytes)."));
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
	m_vertBindingDescription = Geometry::Vertex::getVertexInputBindingDescription();
	m_vertAttribDescriptions = Geometry::Vertex::getVertexAttributeDescriptions();

	m_vertInputState.vertexBindingDescriptionCount = 1;
	m_vertInputState.pVertexBindingDescriptions = &m_vertBindingDescription;

	m_vertInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertAttribDescriptions.size());
	m_vertInputState.pVertexAttributeDescriptions = m_vertAttribDescriptions.data();


	CleanupTask cleanupTask{};
	cleanupTask.caller = __FUNCTION__;
	cleanupTask.objectNames = { VARIABLE_NAME(m_vertShaderModule), VARIABLE_NAME(m_fragShaderModule) };
	cleanupTask.vkHandles = { g_vkContext.Device.logicalDevice, m_vertShaderModule, m_fragShaderModule };
	cleanupTask.cleanupFunc = [&]() {
		vkDestroyShaderModule(g_vkContext.Device.logicalDevice, m_vertShaderModule, nullptr);
		vkDestroyShaderModule(g_vkContext.Device.logicalDevice, m_fragShaderModule, nullptr);
		};

	m_sessionCleanupIDs.push_back(
		m_garbageCollector->createCleanupTask(cleanupTask)
	);
}


void OffscreenPipeline::initDynamicStates() {
	m_dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	m_dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
	m_dynamicStateCreateInfo.pDynamicStates = m_dynamicStates.data();
}


void OffscreenPipeline::initInputAssemblyState() {
	m_inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Use PATCH_LIST instead of TRIANGLE_LIST for tessellation
	m_inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
}


void OffscreenPipeline::initViewportState() {
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


void OffscreenPipeline::initRasterizationState() {
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



	// Configures stencil buffer operations.
	m_depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilStateCreateInfo.front = {};
	m_depthStencilStateCreateInfo.back = {};
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


void OffscreenPipeline::initDepthBufferingResources() {
	// Specifies depth image data
	VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imgUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageAspectFlags imgAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

	uint32_t imgWidth = g_vkContext.SwapChain.extent.width;
	uint32_t imgHeight = g_vkContext.SwapChain.extent.height;
	uint32_t imgDepth = 1;

	VkFormat depthFormat = VkFormatUtils::GetBestDepthImageFormat(g_vkContext.Device.physicalDevice);

	bool hasStencilComponent = VkFormatUtils::FormatHasStencilComponent(depthFormat);


	// Creates a depth image
	VmaAllocationCreateInfo imgAllocInfo{};
	imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imgAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	TextureManager::createImage(m_depthImage, m_depthImgAllocation, imgWidth, imgHeight, imgDepth, depthFormat, imgTiling, imgUsage, imgAllocInfo);


	// Creates a depth image view
	VkImageManager::CreateImageView(m_depthImgView, m_depthImage, depthFormat, imgAspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
	g_vkContext.OffscreenPipeline.depthImageView = m_depthImgView;


	// Explicitly transitions the layout of the depth image to a depth attachment.
	// This is not necessary, since it will be done in the render pass anyway. This is rather being explicit for the sake of being explicit. 
	TextureManager::switchImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


void OffscreenPipeline::recreateOffscreenResources(uint32_t width, uint32_t height, uint32_t currentFrame) {
	for (auto& cleanupID : m_offscreenCleanupIDs)
		m_garbageCollector->executeCleanupTask(cleanupID);

	m_offscreenCleanupIDs.clear();

	initOffscreenColorResources(width, height);
	initOffscreenSampler();
	initOffscreenFramebuffer(width, height);

	m_eventDispatcher->publish(Event::OffscreenResourcesAreRecreated{});
}


void OffscreenPipeline::initOffscreenColorResources(uint32_t width, uint32_t height) {
	m_colorImages.resize(m_OFFSCREEN_RESOURCE_COUNT);
	m_colorImgViews.resize(m_OFFSCREEN_RESOURCE_COUNT);
	m_colorImgSamplers.resize(m_OFFSCREEN_RESOURCE_COUNT);
	m_colorImgFramebuffers.resize(m_OFFSCREEN_RESOURCE_COUNT);


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
		uint32_t imageCleanupID = VkImageManager::CreateImage(m_colorImages[i], m_colorImgAlloc, imgAllocInfo, width, height, depth, imgFormat, imgTiling, imgUsageFlags, imgType);

		uint32_t imageViewCleanupID = VkImageManager::CreateImageView(m_colorImgViews[i], m_colorImages[i], imgFormat, imgAspectFlags, viewType, levelCount, layerCount);

		m_offscreenCleanupIDs.push_back(imageCleanupID);
		m_offscreenCleanupIDs.push_back(imageViewCleanupID);
	}

	g_vkContext.OffscreenResources.images = m_colorImages;
	g_vkContext.OffscreenResources.imageViews = m_colorImgViews;
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
	samplerInfo.maxAnisotropy = g_vkContext.Device.deviceProperties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Color for clamp_to_border
	samplerInfo.unnormalizedCoordinates = VK_FALSE; // UVs are [0, 1]
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	for (size_t i = 0; i < m_OFFSCREEN_RESOURCE_COUNT; i++) {

		VkResult result = vkCreateSampler(g_vkContext.Device.logicalDevice, &samplerInfo, nullptr, &m_colorImgSamplers[i]);
		
		VkSampler& sampler = m_colorImgSamplers[i];
		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(m_colorImgSampler) };
		task.vkHandles = { g_vkContext.Device.logicalDevice, sampler };

		task.cleanupFunc = [this, sampler]() {
			vkDestroySampler(g_vkContext.Device.logicalDevice, sampler, nullptr);
		};

		uint32_t samplerCleanupID = m_garbageCollector->createCleanupTask(task);
		m_offscreenCleanupIDs.push_back(samplerCleanupID);


		LOG_ASSERT(result == VK_SUCCESS, "Failed to create offscreen sampler!");
	}

	g_vkContext.OffscreenResources.samplers = m_colorImgSamplers;
}


void OffscreenPipeline::initOffscreenFramebuffer(uint32_t width, uint32_t height) {
	for (size_t i = 0; i < m_colorImages.size(); i++) {
		std::vector<VkImageView> attachments = {
			m_colorImgViews[i],
			m_depthImgView
		};

		uint32_t framebufferCleanupID = VkImageManager::CreateFramebuffer(m_colorImgFramebuffers[i], m_renderPass, attachments, width, height);

		m_offscreenCleanupIDs.push_back(framebufferCleanupID);
	}

	g_vkContext.OffscreenResources.framebuffers = m_colorImgFramebuffers;
}


VkShaderModule OffscreenPipeline::createShaderModule(const std::vector<char>& bytecode) {
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = bytecode.size();
	moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(g_vkContext.Device.logicalDevice, &moduleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create shader module!");
	}

	return shaderModule;
}
