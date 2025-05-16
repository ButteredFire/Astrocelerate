#include "VkBufferManager.hpp"

VkBufferManager::VkBufferManager(VulkanContext& context):
	m_vkContext(context) {

	m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::getService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	bindEvents();

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}

VkBufferManager::~VkBufferManager() {}


void VkBufferManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::InitGlobalBuffers>(
		[this](const Event::InitGlobalBuffers& event) {
			this->createGlobalVertexBuffer(event.vertexData);
			this->createGlobalIndexBuffer(event.indexData);
		}
	);


	m_eventDispatcher->subscribe<Event::UpdateUBOs>(
		[this](const Event::UpdateUBOs& event) {
			this->updateGlobalUBO(event.currentFrame);
			this->updateObjectUBOs(event.currentFrame);
		}
	);
}


void VkBufferManager::init() {
	/*
	//Component::Mesh mesh = ModelLoader::loadModel((std::string(APP_SOURCE_DIR) + std::string("/assets/Models/Spacecraft/SpaceX_Starship/Starship.obj")), ModelLoader::FileType::T_OBJ);

	//std::string modelPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "Spacecraft/SpaceX_Starship/Starship.obj");
	//std::string modelPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "Spacecraft/NASA_Endurance/Endurance_Unified.fbx");
	std::string modelPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/SolarSailSpaceship/ColoredPerVertex/SolarSailSpaceship.obj");
	//std::string modelPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/Cube/Cube.obj");
	//std::string modelPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/Plane/Plane.obj");
	//std::string modelPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models", "TestModels/VikingRoom/viking_room.obj");
	AssimpParser parser;
	Geometry::MeshData rawData = parser.parse(modelPath);

	m_vertices = rawData.vertices;
	m_vertIndices = rawData.indices;


	m_UBOEntity = m_registry->createEntity();

	m_UBORigidBody.position = glm::vec3(0.0f, 0.0f, -1500.0f);
	m_UBORigidBody.velocity = glm::vec3(0.0f, 0.0f, 5.0f);
	m_UBORigidBody.acceleration = glm::vec3(0.0f, 0.0f, 5.0f);
	m_UBORigidBody.mass = 900;

	m_registry->addComponent(m_UBOEntity.id, m_UBORigidBody);


	createGlobalVertexBuffer(m_vertices);
	createGlobalIndexBuffer(m_vertIndices);
	*/

	// Load geometry
	GeometryLoader geometryLoader;

	//std::string planetPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models/TestModels", "Sphere/Sphere.obj");
	std::string planetPath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models/CelestialBodies", "Earth/Earth.glb");
	std::string satellitePath = FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Models/TestModels", "Cube/Cube.obj");

	geometryLoader.loadGeometryFromFile(planetPath);
	geometryLoader.loadGeometryFromFile(satellitePath);
	std::vector<Geometry::MeshOffset> meshOffsets = geometryLoader.bakeGeometry();

	m_totalObjects = 2;

	// Define rigid bodies
	m_planet = m_registry->createEntity();
	m_satellite = m_registry->createEntity();

	// Planet components
	Component::RigidBody planetRB{};
	planetRB.velocity = glm::vec3(0.0f);
	planetRB.acceleration = glm::vec3(0.0f);
	planetRB.mass = (5.972 * 10e24);

	Component::Transform planetTransform{};
	planetTransform.position = glm::vec3(0.0f);
	planetTransform.scale = glm::vec3(1.0f);

	Component::MeshRenderable planetRenderable{};
	planetRenderable.uboIndex = 0;
	planetRenderable.meshOffset = meshOffsets[0];


		// Satellite components
	Component::RigidBody satelliteRB{};
	satelliteRB.velocity = glm::vec3(-20.0f, 0.0f, 0.0f);
	satelliteRB.acceleration = glm::vec3(0.0f);
	satelliteRB.mass = 20;

	Component::Transform satelliteTransform{};
	satelliteTransform.position = glm::vec3(0.0f, 500.0f, 0.0f);

	Component::MeshRenderable satelliteRenderable{};
	satelliteRenderable.uboIndex = 1;
	satelliteRenderable.meshOffset = meshOffsets[1];


	m_registry->addComponent(m_planet.id, planetRB);
	m_registry->addComponent(m_planet.id, planetTransform);
	m_registry->addComponent(m_planet.id, planetRenderable);

	m_registry->addComponent(m_satellite.id, satelliteRB);
	m_registry->addComponent(m_satellite.id, satelliteTransform);
	m_registry->addComponent(m_satellite.id, satelliteRenderable);


	m_alignedObjectUBOSize = SystemUtils::align(sizeof(Buffer::ObjectUBO), m_vkContext.Device.deviceProperties.limits.minUniformBufferOffsetAlignment);
	createUniformBuffers();
}


uint32_t VkBufferManager::createBuffer(VulkanContext& vkContext, VkBuffer& buffer, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VmaAllocation& bufferAllocation, VmaAllocationCreateInfo bufferAllocationCreateInfo) {

	std::shared_ptr<GarbageCollector> m_garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);

	// Creates the buffer
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.size = bufferSize;

		// Specifies the purpose of the buffer (It is possible to specify multiple purposes using a bitwise OR)
	bufCreateInfo.usage = usageFlags;

		// Buffers can either be owned by a specific queue family or be shared between multiple queue families.
	bufCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

		// If the sharing mode is CONCURRENT, we must specify queue families
	QueueFamilyIndices familyIndices = vkContext.Device.queueFamilies;
	uint32_t queueFamilyIndices[] = {
		familyIndices.graphicsFamily.index.value(),
		familyIndices.transferFamily.index.value()
	};
	bufCreateInfo.queueFamilyIndexCount = 2;
	bufCreateInfo.pQueueFamilyIndices = queueFamilyIndices;

		// Configures sparse buffer memory (which is irrelevant right now, so we'll leave it at the default value of 0)
	bufCreateInfo.flags = 0;


	VkResult bufCreateResult = vmaCreateBuffer(vkContext.vmaAllocator, &bufCreateInfo, &bufferAllocationCreateInfo, &buffer, &bufferAllocation, nullptr);
	if (bufCreateResult != VK_SUCCESS) {
		throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create buffer!");
	}


	CleanupTask bufTask{};
	bufTask.caller = __FUNCTION__;
	bufTask.objectNames = { VARIABLE_NAME(m_buffer) };
	bufTask.vkObjects = { vkContext.vmaAllocator, buffer, bufferAllocation };
	bufTask.cleanupFunc = [vkContext, buffer, bufferAllocation]() { vmaDestroyBuffer(vkContext.vmaAllocator, buffer, bufferAllocation); };

	uint32_t bufferTaskID = m_garbageCollector->createCleanupTask(bufTask);


	return bufferTaskID;
}


void VkBufferManager::createGlobalVertexBuffer(const std::vector<Geometry::Vertex>& vertexData) {
	VkDeviceSize bufferSize = (sizeof(vertexData[0]) * vertexData.size());
	VkBufferUsageFlags vertBufUsage = (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	// NOTE: By default, the VMA will attempt to allocate memory in the preferred type (GPU/CPU), but may fall back to other types should it not be available/suitable (hence "AUTO_PREFER").
	// But we must use GPU memory, so we have to specify the required flags.
	VmaAllocationCreateInfo vertBufAllocInfo{};
	vertBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // PREFERS fast device-local memory
	vertBufAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // FORCES device-local memory

	createBuffer(m_vkContext, m_vertexBuffer, bufferSize, vertBufUsage, m_vertexBufferAllocation, vertBufAllocInfo);

	writeDataToGPUBuffer(vertexData.data(), m_vertexBuffer, bufferSize);
}


void VkBufferManager::createGlobalIndexBuffer(const std::vector<uint32_t>& indexData) {
	VkDeviceSize bufferSize = (sizeof(indexData[0]) * indexData.size());
	VkBufferUsageFlags indexBufUsage = (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	// NOTE: By default, the VMA will attempt to allocate memory in the preferred type (GPU/CPU), but may fall back to other types should it not be available/suitable (hence "AUTO_PREFER").
	// But we must use GPU memory, so we have to specify the required flags.
	VmaAllocationCreateInfo indexBufAllocInfo{};
	indexBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // PREFERS fast device-local memory
	indexBufAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // FORCES device-local memory

	createBuffer(m_vkContext, m_indexBuffer, bufferSize, indexBufUsage, m_indexBufferAllocation, indexBufAllocInfo);

	writeDataToGPUBuffer(indexData.data(), m_indexBuffer, bufferSize);
}


void VkBufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize) {
	// Uses the transfer queue by default, but if it does not exist, switch to the graphics queue
	QueueFamilyIndices::QueueFamily queueFamily = m_vkContext.Device.queueFamilies.transferFamily;
	if (queueFamily.deviceQueue == VK_NULL_HANDLE || !queueFamily.index.has_value()) {
		Log::print(Log::T_WARNING, __FUNCTION__, "Transfer queue family is not valid. Switching to graphics queue family...");
		queueFamily = m_vkContext.Device.queueFamilies.graphicsFamily;
	}


	// Begins recording a command buffer to send data to the GPU
	SingleUseCommandBufferInfo cmdBufInfo{};
	cmdBufInfo.commandPool = VkCommandManager::createCommandPool(m_vkContext, m_vkContext.Device.logicalDevice, queueFamily.index.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	cmdBufInfo.fence = VkSyncManager::createSingleUseFence(m_vkContext);
	cmdBufInfo.usingSingleUseFence = true;
	cmdBufInfo.queue = queueFamily.deviceQueue;

	VkCommandBuffer commandBuffer = VkCommandManager::beginSingleUseCommandBuffer(m_vkContext, &cmdBufInfo);


	// Copies the data
	VkBufferCopy copyRegion{};
		// Sets the source and destination buffer offsets to 0 (since we want to copy everything, not just a portion)
	copyRegion.srcOffset = copyRegion.dstOffset = 0;
	copyRegion.size = deviceSize;
		// We can actually transfer multiple regions (that is why there is `regionCount` - to specify how many regions to transfer). To copy multiple regions, we can pass in a region array for `pRegions`, and its size for `regionCount`.
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);


	// Stops recording the command buffer and submits recorded data to the GPU
	VkCommandManager::endSingleUseCommandBuffer(m_vkContext, &cmdBufInfo, commandBuffer);
}


void VkBufferManager::updateGlobalUBO(uint32_t currentImage) {
	Buffer::GlobalUBO ubo{};


	// View
		// glm::lookAt(eyePosition, centerPosition, upAxis);
	glm::vec3 eyePosition = glm::vec3(1.5f, 2.0f, 1.0f) * 500.0f;
	glm::vec3 centerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 upAxis = glm::vec3(0.0f, 0.0f, 1.0f);

	ubo.view = glm::lookAt(eyePosition, centerPosition, upAxis);


	// Perspective
		// glm::perspective(fieldOfView, aspectRatio, nearClipPlane, farClipPlane);
	constexpr float fieldOfView = glm::radians(60.0f);
	float aspectRatio = static_cast<float>(m_vkContext.SwapChain.extent.width) / static_cast<float>(m_vkContext.SwapChain.extent.height);
	float nearClipPlane = 0.01f;
	float farClipPlane = 1e10f;

	ubo.projection = glm::perspective(fieldOfView, aspectRatio, nearClipPlane, farClipPlane);


	/*
		GLM was originally designed for OpenGL, and because of that, the Y-coordinate of the clip coordinates is flipped.
		If this behavior is left as is, then images will be flipped upside down.
		One way to change this behavior is to flip the sign on the Y-axis scaling factor in the projection matrix.
	*/
	ubo.projection[1][1] *= -1; // Flip y-axis


	memcpy(m_globalUBOMappedData[currentImage], &ubo, sizeof(ubo));
}


void VkBufferManager::updateObjectUBOs(uint32_t currentImage) {
	m_eventDispatcher->publish(Event::UpdateRigidBodies{}, true);

	auto view = m_registry->getView<Component::RigidBody, Component::MeshRenderable, Component::Transform>();

	for (const auto& [entity, rigidBody, meshRenderable, transform] : view) {
		// Timekeeping ensures that the geometry rotates at a constant rate regardless of frame rate
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


		// Actual UBO population
		Buffer::ObjectUBO ubo{};

		// Model matrix (Scale -> Rotate -> Translate)
			// glm::scale(transformation, scale);
			// glm::rotate(transformation, rotationAngle, rotationAxis);
			// glm::translate(transformation, translation);
		const glm::mat4 identityMat = glm::mat4(1.0f);
		float rotationAngle = (time * glm::radians(0.5f));
		glm::vec3 rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);

		glm::mat4 modelMatrix = glm::mat4(1.0f);

		modelMatrix *= glm::scale(identityMat, transform.scale);
		modelMatrix *= glm::rotate(identityMat, rotationAngle, rotationAxis);
		modelMatrix *= glm::translate(identityMat, transform.position);

		ubo.model = modelMatrix;

		// Writing to memory
		void* uboDst = getObjectUBO(currentImage, meshRenderable.uboIndex);
		memcpy(uboDst, &ubo, sizeof(ubo));
	}
}


void VkBufferManager::writeDataToGPUBuffer(const void* data, VkBuffer& buffer, VkDeviceSize bufferSize) {
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

	uint32_t stagingBufTaskID = createBuffer(m_vkContext, stagingBuffer, bufferSize, stagingBufUsage, stagingBufAllocation, stagingBufAllocInfo);

	// Copies data to the staging buffer
	void* mappedData;
	vmaMapMemory(m_vkContext.vmaAllocator, stagingBufAllocation, &mappedData);	// Maps the buffer memory into CPU-accessible memory so that we can write data to it
	memcpy(mappedData, data, static_cast<size_t>(bufferSize));	// Copies the data to the mapped buffer memory
	vmaUnmapMemory(m_vkContext.vmaAllocator, stagingBufAllocation);		// Unmaps the mapped buffer memory


	// Copies the contents from the staging buffer to the destination buffer
	copyBuffer(stagingBuffer, buffer, bufferSize);


	// The staging buffer has done its job, so we can safely destroy it afterwards
	m_garbageCollector->executeCleanupTask(stagingBufTaskID);
}


void VkBufferManager::createUniformBuffers() {
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
	VkDeviceSize objectBufSize = static_cast<VkDeviceSize>(m_alignedObjectUBOSize) * m_totalObjects;

	m_objectUBOs.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);
	m_objectUBOMappedData.resize(SimulationConsts::MAX_FRAMES_IN_FLIGHT);


	VkBufferUsageFlags uniformBufUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (size_t i = 0; i < SimulationConsts::MAX_FRAMES_IN_FLIGHT; i++) {
		// Creates the UBOs and map them to CPU memory
		VmaAllocationCreateInfo uniformBufAllocInfo{};
		uniformBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		uniformBufAllocInfo.requiredFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uniformBufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		/* NOTE: Should you transition to staging buffer uploads (maybe for instancing/compute), you'll need this flag:
			uniformBufAllocInfo.flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		*/
		
			// Global UBO (per frame)
		createBuffer(m_vkContext, m_globalUBOs[i].buffer, globalBufSize, uniformBufUsageFlags, m_globalUBOs[i].allocation, uniformBufAllocInfo);

			/*
			The buffer allocation stays mapped to the pointer for the application's whole lifetime.
			This technique is called "persistent mapping". We use it here because, as aforementioned, the UBOs are updated with new data every single frame, and mapping them alone costs a little performance, much less every frame.
			*/
		vmaMapMemory(m_vkContext.vmaAllocator, m_globalUBOs[i].allocation, &m_globalUBOMappedData[i]);


			// Object UBO (per object per frame)
		createBuffer(m_vkContext, m_objectUBOs[i].buffer, objectBufSize, uniformBufUsageFlags, m_objectUBOs[i].allocation, uniformBufAllocInfo);
		vmaMapMemory(m_vkContext.vmaAllocator, m_objectUBOs[i].allocation, &m_objectUBOMappedData[i]);
		
		/*
		for (size_t j = 0; j < m_totalObjects; j++) {
			size_t index = i * m_totalObjects + j;
		}
		*/


		// Cleanup task
		VmaAllocation globalUBOAlloc = m_globalUBOs[i].allocation;
		VmaAllocation objectUBOAlloc = m_objectUBOs[i].allocation;

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { "Global and per-object UBOs" };
		task.vkObjects = { m_vkContext.vmaAllocator, globalUBOAlloc, objectUBOAlloc };
		task.cleanupFunc = [this, i, globalUBOAlloc, objectUBOAlloc]() {
			vmaUnmapMemory(m_vkContext.vmaAllocator, globalUBOAlloc);
			vmaUnmapMemory(m_vkContext.vmaAllocator, objectUBOAlloc);
			/*
			for (size_t j = 0; j < m_totalObjects; j++) {
				size_t index = i * m_totalObjects + j;
			}
			*/
		};

		m_garbageCollector->createCleanupTask(task);
	}
}


uint32_t VkBufferManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	// Queries info about available memory types on the GPU
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_vkContext.Device.physicalDevice, &memoryProperties);
	
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
