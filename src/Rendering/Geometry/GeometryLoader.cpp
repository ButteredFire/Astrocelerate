#include "GeometryLoader.hpp"


GeometryLoader::GeometryLoader() {
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void GeometryLoader::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<GeometryLoader>();

	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_INIT:
				m_meshes.clear();
				m_leftEndpoint = -1;	// Cancels out the +1 addition above
				m_rightEndpoint = -1;	// Accounts for 0-indexed mesh-offset range
				break;
			}
		}
	);
}


Math::Interval<uint32_t> GeometryLoader::loadGeometryFromFile(const std::string& path) {
	std::lock_guard<std::mutex> lock(m_meshLoadMutex);

	// Parse geometry data
	AssimpParser parser;
	Geometry::MeshData meshData = parser.parse(path);
	m_meshes.push_back(meshData);


	// Calculate range
	m_leftEndpoint = m_rightEndpoint + 1;
	m_rightEndpoint += meshData.childMeshOffsets.size();

	return Math::Interval<uint32_t>{
		.intervalType = Math::T_INTERVAL_CLOSED,
		.left = static_cast<uint32_t>(m_leftEndpoint),
		.right = static_cast<uint32_t>(m_rightEndpoint)
	};
}


Geometry::GeometryData* GeometryLoader::bakeGeometry() {
	std::lock_guard<std::mutex> lock(m_meshLoadMutex);


	// First pass: Gets total counts for memory pre-allocation
	size_t cumulativeVertexCount = 0;
	size_t cumulativeIndexCount = 0;
	size_t cumulativeMaterialCount = 0;
	size_t cumulativeChildMeshOffsetCount = 0;
	for (const auto& mesh : m_meshes) {
		cumulativeVertexCount += mesh.vertices.size();
		cumulativeIndexCount += mesh.indices.size();
		cumulativeMaterialCount += mesh.materials.size();
		cumulativeChildMeshOffsetCount += mesh.childMeshOffsets.size();
	}

	std::vector<Geometry::Vertex> globalVertexData;
	globalVertexData.reserve(cumulativeVertexCount);

	std::vector<uint32_t> globalIndexData;
	globalIndexData.reserve(cumulativeIndexCount);

	std::vector<Geometry::Material> globalMaterialData;
	globalMaterialData.reserve(cumulativeMaterialCount);

	std::vector<Geometry::MeshOffset> globalMeshOffsets;
	globalMeshOffsets.reserve(cumulativeChildMeshOffsetCount);


	// Second pass: Process mesh data
	for (const auto& mesh : m_meshes) {
		size_t currentVertexOffset = globalVertexData.size();
		size_t currentIndexOffset = globalIndexData.size();
		size_t currentMaterialOffset = globalMaterialData.size();

		globalVertexData.insert(globalVertexData.end(), mesh.vertices.begin(), mesh.vertices.end());
		globalIndexData.insert(globalIndexData.end(), mesh.indices.begin(), mesh.indices.end());
		globalMaterialData.insert(globalMaterialData.end(), mesh.materials.begin(), mesh.materials.end());

		
		// Process child meshes of THIS mesh
		for (auto childMeshOffset : mesh.childMeshOffsets) {
			// (The copy is intentional)
			childMeshOffset.vertexOffset += static_cast<uint32_t>(currentVertexOffset);
			childMeshOffset.indexOffset += static_cast<uint32_t>(currentIndexOffset);
			childMeshOffset.materialIndex += static_cast<uint32_t>(currentMaterialOffset);
			// NOTE: childMeshOffset.indexCount has already been computed in AssimpParser::processMeshGeometry

			globalMeshOffsets.push_back(childMeshOffset);
		}
	}

	// NOTE: geomData is heap-allocated so that it's accessible throughout the session lifetime
	Geometry::GeometryData *geomData = new Geometry::GeometryData();
	geomData->meshCount = globalMeshOffsets.size();
	geomData->meshOffsets = globalMeshOffsets;
	geomData->meshMaterials = globalMaterialData;

	CleanupTask geomDataTask{};
	geomDataTask.caller = __FUNCTION__;
	geomDataTask.objectNames = { VARIABLE_NAME(geomData) };
	geomDataTask.cleanupFunc = [geomData]() {
		delete geomData;
	};
	m_garbageCollector->createCleanupTask(geomDataTask);

	LOG_ASSERT(geomData, "Cannot bake geometry: Unable to allocate enough memory for geometry data!");


	InitEvent::Geometry event{};
	event.vertexData = globalVertexData;
	event.indexData = globalIndexData;
	event.pGeomData = geomData;

	m_eventDispatcher->dispatch(event);

	Log::Print(Log::T_SUCCESS, __FUNCTION__, "Baked " + std::to_string(m_meshes.size()) + " meshes.");


	return geomData;
}
