#include "GeometryLoader.hpp"


GeometryLoader::GeometryLoader() {

	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void GeometryLoader::loadGeometryFromFile(const std::string& path) {
	AssimpParser parser;
	Geometry::MeshData meshData = parser.parse(path);

	m_meshes.push_back(meshData);
}


std::vector<Geometry::MeshOffset> GeometryLoader::bakeGeometry() {
	std::vector<Geometry::MeshOffset> offsets;
	offsets.reserve(m_meshes.size());


	// First pass: Gets total vertex and index size for memory pre-allocation
	size_t cumulativeVertexSize = 0;
	size_t cumulativeIndexSize = 0;
	for (const auto& mesh : m_meshes) {
		cumulativeVertexSize += mesh.vertices.size();
		cumulativeIndexSize += mesh.indices.size();
	}

	std::vector<Geometry::Vertex> vertexData;
	vertexData.reserve(cumulativeVertexSize);

	std::vector<uint32_t> indexData;
	indexData.reserve(cumulativeIndexSize);


	// Second pass: Process mesh data
	for (const auto& mesh : m_meshes) {
		size_t prevVertSize = vertexData.size();
		size_t prevIndexSize = indexData.size();

		vertexData.insert(vertexData.end(), mesh.vertices.begin(), mesh.vertices.end());
		indexData.insert(indexData.end(), mesh.indices.begin(), mesh.indices.end());
		
		Geometry::MeshOffset offset{};
		offset.vertexOffset = prevVertSize;
		offset.indexOffset = prevIndexSize;
		offset.indexCount = static_cast<uint32_t>(mesh.indices.size());

		offsets.push_back(offset);
	}

	Event::InitGlobalBuffers event{};
	event.vertexData = vertexData;
	event.indexData = indexData;

	m_eventDispatcher->publish(event);

	Log::print(Log::T_SUCCESS, __FUNCTION__, "Baked " + std::to_string(m_meshes.size()) + " objects.");

	return offsets;
}
