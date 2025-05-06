#include "GeometryLoader.hpp"


GeometryLoader::GeometryLoader() {

	m_bufferManager = ServiceLocator::getService<BufferManager>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void GeometryLoader::loadGeometryFromFile(const std::string& path) {
	AssimpParser parser;
	Geometry::MeshData meshData = parser.parse(path);

	PrecomputedMeshData precomp{};
	precomp.meshData = meshData;

	if (m_meshes.empty()) {
		precomp.cumulativeVertexSize = meshData.vertices.size();
		precomp.cumulativeIndexSize = meshData.indices.size();
	}
	else {
		precomp.cumulativeVertexSize = meshData.vertices.size() + m_meshes.back().cumulativeVertexSize;
		precomp.cumulativeIndexSize = meshData.indices.size() + m_meshes.back().cumulativeIndexSize;
	}

	m_meshes.push_back(precomp);
}


std::vector<Geometry::MeshOffset> GeometryLoader::bakeGeometry() {
	std::vector<Geometry::MeshOffset> offsets;
	offsets.reserve(m_meshes.size());

	std::vector<Geometry::Vertex> vertexData;
	vertexData.reserve(m_meshes.back().cumulativeVertexSize);

	std::vector<uint32_t> indexData;
	indexData.reserve(m_meshes.back().cumulativeIndexSize);

	for (const auto& precompMesh : m_meshes) {
		Geometry::MeshData mesh = precompMesh.meshData;

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

	m_bufferManager->createGlobalVertexBuffer(vertexData);
	m_bufferManager->createGlobalIndexBuffer(indexData);

	Log::print(Log::T_SUCCESS, __FUNCTION__, "Baked " + std::to_string(m_meshes.size()) + " objects.");

	return offsets;
}
