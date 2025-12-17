#include "VkBufferManager.hpp"

VkBufferManager::VkBufferManager(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr) :
	m_vmaAllocator(coreResources->getVmaAllocator()),
	m_queueFamilies(coreResources->getQueueFamilyIndices()),
	m_physicalDevice(coreResources->getPhysicalDevice()),
	m_logicalDevice(coreResources->getLogicalDevice()),
	m_deviceProperties(coreResources->getDeviceProperties()),

	m_swapchainExtent(swapchainMgr->getSwapChainExtent()) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_resourceManager = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);

	m_camera = ServiceLocator::GetService<Camera>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkBufferManager::~VkBufferManager() {}


void VkBufferManager::bindEvents() {
	EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<VkBufferManager>();

	m_eventDispatcher->subscribe<InitEvent::Geometry>(selfIndex,
		[this](const InitEvent::Geometry& event) {
			this->createGlobalVertexBuffer(event.vertexData);
			this->createGlobalIndexBuffer(event.indexData);

			this->m_geomData = event.pGeomData;
			this->m_totalObjects = event.pGeomData->meshCount;
		}
	);


	m_eventDispatcher->subscribe<InitEvent::OffscreenPipeline>(selfIndex,
		[this](const InitEvent::OffscreenPipeline &event) {
			m_matParamsDescriptorSet = event.pbrDescriptorSet;
			m_perFrameDescriptorSets = event.perFrameDescriptorSets;

			this->createPerFrameUniformBuffers();
			this->initPerFrameUniformBuffers();

			this->createMatParamsUniformBuffer();
			this->initMatParamsUniformBuffer();
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this, selfIndex](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_RESET:
				m_sceneInitReady = false;

				// Unmap memory
				for (auto &cleanupID : m_bufferMemCleanupIDs)
					m_resourceManager->executeCleanupTask(cleanupID);
				m_bufferMemCleanupIDs.clear();

				break;

			case RESET:
				// Destroy buffers
				for (auto &cleanupID : m_bufferCleanupIDs)
					m_resourceManager->executeCleanupTask(cleanupID);
				m_bufferCleanupIDs.clear();

				break;

			case PREPARE_FOR_INIT:
				m_sceneInitReady = true;
				this->checkForInit(selfIndex);

				break;
			}
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::PerFrameBuffers>(selfIndex,
		[this](const UpdateEvent::PerFrameBuffers& event) {
			if (!m_sceneInitReady)
				return;

			this->updateGlobalUBO(event.currentFrame, event.renderOrigin);
			this->updateObjectUBOs(event.currentFrame, event.renderOrigin);
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::ViewportSize>(selfIndex,
		[this](const UpdateEvent::ViewportSize &event) {
			m_uiViewportSceneRegion = event.sceneDimensions;
		}
	);
}


void VkBufferManager::checkForInit(const EventDispatcher::SubscriberIndex &selfIndex) {
	// Continually checks when all required event callbacks have been invoked to dispatch the buffer manager initialization event
	auto thread = ThreadManager::CreateThread("WAIT_BUFFER_MGR_DISPATCH");
	thread->set([this, selfIndex]() {
		EventFlags eventFlags = EVENT_FLAG_INIT_GEOMETRY_BIT | EVENT_FLAG_INIT_OFFSCREEN_PIPELINE_BIT;

		m_eventDispatcher->waitForEventCallbacks(selfIndex, eventFlags);

		m_eventDispatcher->dispatch(InitEvent::BufferManager{
			.globalVertexBuffer = m_vertexBuffer,
			.globalIndexBuffer = m_indexBuffer,
			.perFrameDescriptorSets = m_perFrameDescriptorSets
			});
	});

	thread->start();
}


CleanupID VkBufferManager::CreateBuffer(VkBuffer& buffer, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VmaAllocation& bufferAllocation, VmaAllocationCreateInfo bufferAllocationCreateInfo) {

	std::shared_ptr<VkCoreResourcesManager> coreResources = ServiceLocator::GetService<VkCoreResourcesManager>(__FUNCTION__);
	std::shared_ptr<ResourceManager> resourceManager = ServiceLocator::GetService<ResourceManager>(__FUNCTION__);

	const VmaAllocator &vmaAllocator = coreResources->getVmaAllocator();

	// Creates the buffer
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.size = bufferSize;

		// Specifies the purpose of the buffer (It is possible to specify multiple purposes using a bitwise OR)
	bufCreateInfo.usage = usageFlags;

		// Buffers can either be owned by a specific queue family or be shared between multiple queue families.
	QueueFamilyIndices familyIndices = coreResources->getQueueFamilyIndices();
	std::vector<uint32_t> queueFamilyIndices = {
		familyIndices.graphicsFamily.index.value()
	};

	if (familyIndices.familyExists(familyIndices.transferFamily)) {
		bufCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		queueFamilyIndices.push_back(familyIndices.transferFamily.index.value());
	}
	else
		bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	bufCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	bufCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

		// Configures sparse buffer memory (which is irrelevant right now, so we'll leave it at the default value of 0)
	bufCreateInfo.flags = 0;


	VkResult bufCreateResult = vmaCreateBuffer(vmaAllocator, &bufCreateInfo, &bufferAllocationCreateInfo, &buffer, &bufferAllocation, nullptr);
	LOG_ASSERT(bufCreateResult == VK_SUCCESS, "Failed to create buffer!");


	CleanupTask bufTask{};
	bufTask.caller = __FUNCTION__;
	bufTask.objectNames = { VARIABLE_NAME(buffer) };
	bufTask.vkHandles = { buffer, bufferAllocation };
	bufTask.cleanupFunc = [vmaAllocator, buffer, bufferAllocation]() { vmaDestroyBuffer(vmaAllocator, buffer, bufferAllocation); };

	CleanupID bufferTaskID = resourceManager->createCleanupTask(bufTask);

	return bufferTaskID;
}


void VkBufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize) {
	// Uses the transfer queue by default, but if it does not exist, switch to the graphics queue
	QueueFamilyIndices queueFamilies = m_queueFamilies;
	QueueFamilyIndices::QueueFamily selectedFamily = queueFamilies.transferFamily;

	if (!queueFamilies.familyExists(selectedFamily)) {
		Log::Print(Log::T_WARNING, __FUNCTION__, "Transfer queue family is not valid. Switching to graphics queue family...");
		selectedFamily = queueFamilies.graphicsFamily;
	}


	// Begins recording a command buffer to send data to the GPU
	SingleUseCommandBufferInfo cmdBufInfo{};
	cmdBufInfo.commandPool = VkCommandManager::CreateCommandPool(m_logicalDevice, selectedFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdBufInfo.fence = VkSyncManager::CreateSingleUseFence(m_logicalDevice);
	cmdBufInfo.usingSingleUseFence = true;
	cmdBufInfo.queue = selectedFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::BeginSingleUseCommandBuffer(m_logicalDevice, &cmdBufInfo);


	// Copies the data
	VkBufferCopy copyRegion{};
	// Sets the source and destination buffer offsets to 0 (since we want to copy everything, not just a portion)
	copyRegion.srcOffset = copyRegion.dstOffset = 0;
	copyRegion.size = deviceSize;
	// We can actually transfer multiple regions (that is why there is `regionCount` - to specify how many regions to transfer). To copy multiple regions, we can pass in a region array for `pRegions`, and its size for `regionCount`.
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);


	// Stops recording the command buffer and submits recorded data to the GPU
	VkCommandManager::EndSingleUseCommandBuffer(m_logicalDevice, &cmdBufInfo, commandBuffer);
}


void VkBufferManager::writeDataToGPUBuffer(const void *data, VkBuffer &buffer, VkDeviceSize bufferSize) {
	/* How data is written into a device-local-memory allocated buffer:
	*
	* - We want the CPU to access and write data to a buffer in GPU memory (or device-local memory). It is in GPU memory because its memory usage flag is `VMA_MEMORY_USAGE_GPU_ONLY`. However, such buffers are not always directly accessible from the CPU.
	*
	* - Therefore, we will need to use a third buffer: the staging buffer. It acts as a medium through which the CPU can write data into GPU-memory-allocated buffers. The staging buffer is specified to be created in CPU memory (or host-visible memory) via the memory usage flag `VMA_MEMORY_USAGE_CPU_ONLY`.
	*
	* - STEP 1: Allocate a staging buffer in host-visible memory.
	* - STEP 2: Copy/Load the data onto the staging buffer.
	*	+ STEP 2.1: Map the staging buffer's memory block onto the CPU address space so that the CPU can access and write data to it
	*	+ STEP 2.2: Copy the data to the memory block via `memcpy` (which is a CPU operation)
	*	+ STEP 2.3: Unmaps the staging buffer's memory block from the CPU address space to ensure the CPU can no longer access it
	*
	* - STEP 3: Copy the data from the staging buffer to the destination/target buffer. This has already been handled in `copyBuffer`.
	*
	* NOTE: `VMA_MEMORY_USAGE_CPU_ONLY` and `VMA_MEMORY_USAGE_GPU_ONLY` are deprecated.
	* Use `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` and `VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE` respectively.
	*/

	// Creates a staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufAllocation;

	VkBufferUsageFlags stagingBufUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingBufAllocInfo{};
	stagingBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST; // Use VMA_MEMORY_USAGE_AUTO for general usage
	stagingBufAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // Host-visible memory


	/*
	Since the staging buffer's allocation is going to be mapped to CPU memory below (vmaMapMemory), we must specify the expected patern of CPU memory access.

	[VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT]
	This flag indicates that the host will access the memory in a sequential write pattern. This is typically used when the host writes data to the memory in a linear order, such as when uploading a large block of data to a buffer.

	[VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT]
	This flag indicates that the host will access the memory in a random access pattern. This is typically used when the host reads or writes data to the memory in a non-linear order, such as when updating individual elements in a buffer.
	*/
	stagingBufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	uint32_t stagingBufTaskID = CreateBuffer(stagingBuffer, bufferSize, stagingBufUsage, stagingBufAllocation, stagingBufAllocInfo);

	// Copies data to the staging buffer
	void *mappedData;
	vmaMapMemory(m_vmaAllocator, stagingBufAllocation, &mappedData);	// Maps the buffer memory into CPU-accessible memory so that we can write data to it
	memcpy(mappedData, data, static_cast<size_t>(bufferSize));	// Copies the data to the mapped buffer memory
	vmaUnmapMemory(m_vmaAllocator, stagingBufAllocation);		// Unmaps the mapped buffer memory


	// Copies the contents from the staging buffer to the destination buffer
	copyBuffer(stagingBuffer, buffer, bufferSize);


	// The staging buffer has done its job, so we can safely destroy it afterwards
	m_resourceManager->executeCleanupTask(stagingBufTaskID);
}


void VkBufferManager::createGlobalVertexBuffer(const std::vector<Geometry::Vertex>& vertexData) {
	if (!m_sceneInitReady)
		return;

	VkDeviceSize bufferSize = (sizeof(vertexData[0]) * vertexData.size());
	VkBufferUsageFlags vertBufUsage = (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	// NOTE: By default, the VMA will attempt to allocate memory in the preferred type (GPU/CPU), but may fall back to other types should it not be available/suitable (hence "AUTO_PREFER").
	// But we must use GPU memory, so we have to specify the required flags.
	VmaAllocationCreateInfo vertBufAllocInfo{};
	vertBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // PREFERS fast device-local memory
	vertBufAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // FORCES device-local memory

	CleanupID taskID = CreateBuffer(m_vertexBuffer, bufferSize, vertBufUsage, m_vertexBufferAllocation, vertBufAllocInfo);

	m_bufferCleanupIDs.push_back(taskID);

	writeDataToGPUBuffer(vertexData.data(), m_vertexBuffer, bufferSize);
}


void VkBufferManager::createGlobalIndexBuffer(const std::vector<uint32_t>& indexData) {
	if (!m_sceneInitReady)
		return;

	VkDeviceSize bufferSize = (sizeof(indexData[0]) * indexData.size());
	VkBufferUsageFlags indexBufUsage = (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	// NOTE: By default, the VMA will attempt to allocate memory in the preferred type (GPU/CPU), but may fall back to other types should it not be available/suitable (hence "AUTO_PREFER").
	// But we must use GPU memory, so we have to specify the required flags.
	VmaAllocationCreateInfo indexBufAllocInfo{};
	indexBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // PREFERS fast device-local memory
	indexBufAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // FORCES device-local memory

	CleanupID taskID = CreateBuffer(m_indexBuffer, bufferSize, indexBufUsage, m_indexBufferAllocation, indexBufAllocInfo);

	m_bufferCleanupIDs.push_back(taskID);

	writeDataToGPUBuffer(indexData.data(), m_indexBuffer, bufferSize);
}


void VkBufferManager::createPerFrameUniformBuffers() {
	if (!m_sceneInitReady)
		return;

	// NOTE: Since new data is copied to the UBOs every frame, we should not use staging buffers since they add overhead and thus degrade performance.
	// However, we may use staging buffers if we want to utilize UBOs for instancing/compute.

	// Global UBOs
	VkDeviceSize globalBufSize = sizeof(Buffer::GlobalUBO);

	m_globalUBOs.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	m_globalUBOMappedData.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);


	// Object UBOs
		/*
			objectBufSize is the total size of all object UBOs (each corresponding to its object). We could say that objectBufSize is the size of the master object UBO, which stores child object UBOs.
			For every frame, we want to have exactly 1 master object UBO. Therefore, for MAX_FRAMES_IN_FLIGHT frames, we want MAX_FRAMES_IN_FLIGHT master object UBOs.
		*/
	m_alignedObjectUBOSize = SystemUtils::Align(sizeof(Buffer::ObjectUBO), m_deviceProperties.limits.minUniformBufferOffsetAlignment);

	VkDeviceSize objectBufSize = static_cast<VkDeviceSize>(m_alignedObjectUBOSize) * m_totalObjects;

	m_objectUBOs.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	m_objectUBOMappedData.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);


	// Create the UBOs and map them to CPU memory
	VkBufferUsageFlags uniformBufUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VmaAllocationCreateInfo uniformBufAllocInfo{};
	uniformBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	uniformBufAllocInfo.requiredFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	uniformBufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	/* NOTE: Should you transition to staging buffer uploads (maybe for instancing/compute), you'll need this flag:
		uniformBufAllocInfo.flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	*/

	for (size_t i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		// Global UBO
		CleanupID bufCleanupID = CreateBuffer(m_globalUBOs[i].buffer, globalBufSize, uniformBufUsageFlags, m_globalUBOs[i].allocation, uniformBufAllocInfo);

		/*
		The buffer allocation stays mapped to the pointer for the application's whole lifetime.
		This technique is called "persistent mapping". We use it here because, as aforementioned, the UBOs are updated with new data every single frame, and mapping them alone costs a little performance, much less every frame.
		*/
		vmaMapMemory(m_vmaAllocator, m_globalUBOs[i].allocation, &m_globalUBOMappedData[i]);

		m_bufferCleanupIDs.push_back(bufCleanupID);


		// Object UBO
		bufCleanupID = CreateBuffer(m_objectUBOs[i].buffer, objectBufSize, uniformBufUsageFlags, m_objectUBOs[i].allocation, uniformBufAllocInfo);
		vmaMapMemory(m_vmaAllocator, m_objectUBOs[i].allocation, &m_objectUBOMappedData[i]);

		m_bufferCleanupIDs.push_back(bufCleanupID);


		// Cleanup task
		VmaAllocation globalUBOAlloc = m_globalUBOs[i].allocation;
		VmaAllocation objectUBOAlloc = m_objectUBOs[i].allocation;

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { "Global and per-object UBOs" };
		task.vkHandles = { globalUBOAlloc, objectUBOAlloc };
		task.cleanupFunc = [this, i, globalUBOAlloc, objectUBOAlloc]() {
			vmaUnmapMemory(m_vmaAllocator, globalUBOAlloc);
			vmaUnmapMemory(m_vmaAllocator, objectUBOAlloc);
			};

		m_bufferMemCleanupIDs.push_back(
			m_resourceManager->createCleanupTask(task)
		);
	}
}


void VkBufferManager::createMatParamsUniformBuffer() {
	if (!m_sceneInitReady)
		return;

	LOG_ASSERT(m_geomData, "Cannot create material parameters uniform buffer: Geometry data is invalid!");

	VkDeviceSize minUBOAlignment = m_deviceProperties.limits.minUniformBufferOffsetAlignment;

	// Material size & alignment
	m_matStrideSize = SystemUtils::Align(sizeof(Geometry::Material), minUBOAlignment);

	// Buffer configuration
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(m_matStrideSize) * m_geomData->meshMaterials.size();

	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VmaAllocationCreateInfo matParamsUBAllocInfo{};
	matParamsUBAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	matParamsUBAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	matParamsUBAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	CleanupID cleanupTaskID = CreateBuffer(m_matParamsBuffer, bufferSize, bufferUsage, m_matParamsBufferAllocation, matParamsUBAllocInfo);

	m_bufferCleanupIDs.push_back(cleanupTaskID);

	vmaMapMemory(m_vmaAllocator, m_matParamsBufferAllocation, &m_matParamsBufferMappedData);

	CleanupTask task{};
	task.caller = __FUNCTION__;
	task.objectNames = { VARIABLE_NAME(m_matParamsBufferAllocation) };
	task.vkHandles = { m_matParamsBufferAllocation };
	task.cleanupFunc = [this]() {
		vmaUnmapMemory(m_vmaAllocator, m_matParamsBufferAllocation);
	};
	m_bufferMemCleanupIDs.push_back(
		m_resourceManager->createCleanupTask(task)
	);


	// Populate buffer with all materials
	for (size_t i = 0; i < m_geomData->meshMaterials.size(); i++) {
		void *dataOffset = SystemUtils::GetAlignedBufferOffset(m_matStrideSize, m_matParamsBufferMappedData, i);
		memcpy(dataOffset, &m_geomData->meshMaterials[i], sizeof(Geometry::Material));
	}
}


void VkBufferManager::initPerFrameUniformBuffers() {
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	for (size_t i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		descriptorWrites.clear();

		// Global uniform buffer
		VkDescriptorBufferInfo globalUBOInfo{};
		globalUBOInfo.buffer = m_globalUBOs[i].buffer;
		globalUBOInfo.offset = 0;
		globalUBOInfo.range = sizeof(Buffer::GlobalUBO); // Note: We can also use VK_WHOLE_SIZE if we want to overwrite the whole buffer (like what we're doing)


		// Per-object uniform buffer
		size_t alignedObjectUBOSize = SystemUtils::Align(sizeof(Buffer::ObjectUBO), m_deviceProperties.limits.minUniformBufferOffsetAlignment);

		VkDescriptorBufferInfo objectUBOInfo{};
		objectUBOInfo.buffer = m_objectUBOs[i].buffer;
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
		vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}


void VkBufferManager::initMatParamsUniformBuffer() {
	VkDescriptorBufferInfo pbrMaterialUBOInfo{};
	pbrMaterialUBOInfo.buffer = m_matParamsBuffer;
	pbrMaterialUBOInfo.offset = 0;
	pbrMaterialUBOInfo.range = m_matStrideSize;		// NOTE: The range should only be the size of a single material, not the size of the entire buffer.

	VkWriteDescriptorSet pbrMaterialUBOdescWrite{};
	pbrMaterialUBOdescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	pbrMaterialUBOdescWrite.dstSet = m_matParamsDescriptorSet;
	pbrMaterialUBOdescWrite.dstBinding = ShaderConsts::FRAG_BIND_MATERIAL_PARAMETERS;
	pbrMaterialUBOdescWrite.dstArrayElement = 0;
	pbrMaterialUBOdescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pbrMaterialUBOdescWrite.descriptorCount = 1;
	pbrMaterialUBOdescWrite.pBufferInfo = &pbrMaterialUBOInfo;

	vkUpdateDescriptorSets(m_logicalDevice, 1, &pbrMaterialUBOdescWrite, 0, nullptr);
}


void VkBufferManager::updateGlobalUBO(uint32_t currentImage, const glm::dvec3 &renderOrigin) {
	// If the scene dimensions are not available yet, do not update the global UBO
	if (m_uiViewportSceneRegion.x == 0.0f && m_uiViewportSceneRegion.y == 0.0f)
		return;

	Buffer::GlobalUBO ubo{};


	// View
	auto pointLightView = m_registry->getView<RenderComponent::PointLight>();

	ubo.view = m_camera->getRenderSpaceViewMatrix();
	ubo.cameraPosition = SpaceUtils::ToRenderSpace_Position(m_camera->getRelativeTransform().position); // NOTE: The camera position needs to be in render space to match other entities (see updateObjectUBOs).

	if (pointLightView.size() > 0) {
		const auto &[entityWithPointLight, pointLight] = pointLightView[0];

		ubo.lightPosition = glm::dvec3(pointLight.position) - SpaceUtils::ToRenderSpace_Position(renderOrigin);
		ubo.lightColor = pointLight.color;
		ubo.lightRadiantFlux = pointLight.radiantFlux;
	}


	// Perspective
	const float fieldOfView = glm::radians(m_camera->zoom);
	float aspectRatio = static_cast<float>(m_uiViewportSceneRegion.x) / static_cast<float>(m_uiViewportSceneRegion.y);
	float nearClipPlane = 0.01f;
	float farClipPlane = 1e8f;

	//ubo.projection = glm::perspective(fieldOfView, aspectRatio, nearClipPlane, farClipPlane);

	ubo.projection = glm::perspectiveRH_ZO(fieldOfView, aspectRatio, farClipPlane, nearClipPlane);
	/*
		GLM was originally designed for OpenGL, and because of that, the Y-coordinate of the clip coordinates is flipped.
		If this behavior is left as is, then images will be flipped upside down.
		One way to change this behavior is to flip the sign on the Y-axis scaling factor in the projection matrix.
	*/
	ubo.projection[1][1] *= -1; // Flip y-axis


	memcpy(m_globalUBOMappedData[currentImage], &ubo, sizeof(ubo));
}


void VkBufferManager::updateObjectUBOs(uint32_t currentImage, const glm::dvec3& renderOrigin) {
	auto view = m_registry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody, RenderComponent::MeshRenderable, TelemetryComponent::RenderTransform>();

	for (auto&& [entity, transform, rigidBody, meshRenderable, renderT] : view) {
		Buffer::ObjectUBO ubo{};

		const glm::mat4 identityMat = glm::mat4(1.0f);

		glm::dvec3 renderPosition;
		renderPosition = SpaceUtils::ToRenderSpace_Position(transform.position - renderOrigin);

			// Update the entity's global position data after scaling to account for parent's visual scale
		//refFrame._computedGlobalPosition = renderPosition;
		//m_registry->updateComponent(entity, refFrame);


		// Scale in render space
		double renderScale = SpaceUtils::GetRenderableScale(SpaceUtils::ToRenderSpace_Scale(transform.scale)) * meshRenderable.visualScale;

		/* NOTE:
			Model matrices are constructed according to the Scale -> Rotate -> Translate (S-R-T) order.
			Specifically, a model matrix, M, is constructed like so:
					M = T * R * S * v_local (where v_local is the position of the vertex in local space).

			The reason why the construction looks backwards is because matrix multiplication is right-to-left for column vectors (the default in GLM). So, when we construct M as M = T * R * S * v_local, the operations are applied as:
					M = T * (R * (S * v_local)), hence S-R-T order.
		*/
		glm::mat4 modelMatrix =
			glm::translate(identityMat, glm::vec3(renderPosition))
			* glm::mat4(glm::toMat4(transform.rotation))
			* glm::scale(identityMat, glm::vec3(static_cast<float>(renderScale)));


		ubo.model = modelMatrix;
		ubo.normalMatrix = glm::transpose(glm::inverse(modelMatrix));

		// Write to telemetry dashboard
		renderT.position = renderPosition;
		renderT.rotation = transform.rotation;
		renderT.visualScale = renderScale;
		m_registry->updateComponent(entity, renderT);


		// Write mesh (and submesh) data to memory
		for (uint32_t meshIndex : meshRenderable.meshRange) {
			void *uboDst = SystemUtils::GetAlignedBufferOffset(m_alignedObjectUBOSize, m_objectUBOMappedData[currentImage], meshIndex);
			memcpy(uboDst, &ubo, sizeof(ubo));
		}
	}
}


uint32_t VkBufferManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	// Queries info about available memory types on the GPU
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);
	
	/* The VkPhysicalDeviceMemoryProperties struct has two arrays:
	* - memoryHeaps: An array of structures, each describing a memory heap (i.e., distinct memory resources, e.g., VRAM, RAM) from which memory can be allocated. This is useful if we want to know what heap a memory type comes from.
	* - memoryTypes: An array of structures, each describing a memory type that can be used with a given memory heap in memoryHeaps.
	*/
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		// Checks if the memory type is suitable for the buffer (i.e., if its bit in the typeFilter bitfield is 1)
		const bool MEMTYPE_SUITABLE = (typeFilter & (1 << i));

		// Checks if the memory type supports all features specified in the `properties` bitfield
		const bool FEATURE_SUPPORTED = ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties);

		if (MEMTYPE_SUITABLE && FEATURE_SUPPORTED) {
			return i;
		}
	}

	throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to find suitable memory type!");
}
