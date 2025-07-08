/* VkBufferManager.hpp - Manages the vertex buffer.
*/
#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// GLM
#include <External/GLM.hpp>

#include <chrono>

// C++ STLs
#include <iostream>
#include <string>
#include <array>
#include <limits>

// Other
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkCommandManager.hpp>
#include <Vulkan/VkSyncManager.hpp>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Application/GarbageCollector.hpp>
#include <Core/Data/Buffer.hpp>
#include <Core/Data/Geometry.hpp>
#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Engine/ECS.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Engine/Components/ModelComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/TelemetryComponents.hpp>

#include <Utils/SystemUtils.hpp>
#include <Utils/FilePathUtils.hpp>
#include <Utils/SpaceUtils.hpp>

#include <Scene/Camera.hpp>


class VkBufferManager {
public:
    VkBufferManager();
    ~VkBufferManager();

    void init();


    /* Creates a buffer.
        @param &buffer: The buffer to be created.
        @param bufferSize: The size of the buffer (in bytes).
        @param usageFlags: Flags specifying how the buffer will be used.
        @param bufferAllocation: The memory block allocated for the buffer.
        @param bufferAllocationCreateInfo: The buffer allocation create info struct.
        
        @return The cleanup task ID for the newly created buffer.
    */
    static uint32_t createBuffer(VkBuffer& buffer, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VmaAllocation& bufferAllocation, VmaAllocationCreateInfo bufferAllocationCreateInfo);


    /* Creates the global vertex buffer. */
    void createGlobalVertexBuffer(const std::vector<Geometry::Vertex>& vertexData);


    /* Creates the global index buffer. */
    void createGlobalIndexBuffer(const std::vector<uint32_t>& indexData);


    /* Creates the PBR material parameters uniform buffer. */
    void createMatParamsUniformBuffer();


    /* Copies the contents from a source buffer to a destination buffer.
        @param srcBuffer: The source buffer that stores the contents to be transferred.
        @param dstBuffer: The destination buffer to receive the contents from the source buffer.
        @param deviceSize: The size of either the source or destination buffer (in bytes).
    */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize);


    /* Updates the global uniform buffer.
        @param currentImage: The index of the current frame.
    */
    void updateGlobalUBO(uint32_t currentImage);


    /* Updates per-object uniform buffers.
        @param currentImage: The index of the current frame.
        @param renderOrigin: The render anchor frame for relative position computations.
    */
    void updateObjectUBOs(uint32_t currentImage, const glm::dvec3& renderOrigin);


    /* Gets the vertex buffer. */
    inline const VkBuffer& getVertexBuffer() const { return m_vertexBuffer; }

    /* Gets the vertex data. */
    inline const std::vector<Geometry::Vertex> getVertexData() const { return m_vertices; }


    /* Gets the index buffer. */
    inline const VkBuffer& getIndexBuffer() const { return m_indexBuffer; }

    /* Gets the vertex index data. */
    inline const std::vector<uint32_t> getVertexIndexData() const { return m_vertIndices; }


    /* Gets the PBR material parameters uniform buffer. */
    inline const VkBuffer &getMatParamsUniformBuffer() const { return m_matParamsBuffer; }

    /* Gets the global uniform buffers */
    inline const std::vector<Buffer::BufferAndAlloc>& getGlobalUBOs() const { return m_globalUBOs; }


    /* Gets the object uniform buffers */
    inline const std::vector<Buffer::BufferAndAlloc>& getObjectUBOs() const { return m_objectUBOs; }

private:
    std::shared_ptr<Registry> m_registry;
    std::shared_ptr<EventDispatcher> m_eventDispatcher;
    std::shared_ptr<GarbageCollector> m_garbageCollector;

    std::shared_ptr<Camera> m_camera;

    size_t m_totalObjects = 0;
    Geometry::GeometryData *m_geomData = nullptr;
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexBufferAllocation = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_indexBufferAllocation = VK_NULL_HANDLE;

    VkBuffer m_matParamsBuffer = VK_NULL_HANDLE;
    VmaAllocation m_matParamsBufferAllocation = VK_NULL_HANDLE;
    void *m_matParamsBufferMappedData = nullptr;
    size_t m_matStrideSize = 0;

    std::vector<Buffer::BufferAndAlloc> m_globalUBOs;
    std::vector<void*> m_globalUBOMappedData;

    // Per-object UBOs
    std::vector<Buffer::BufferAndAlloc> m_objectUBOs;
    std::vector<void*> m_objectUBOMappedData;
    size_t m_alignedObjectUBOSize = 0;


    std::vector<Geometry::Vertex> m_vertices = {};
    std::vector<uint32_t> m_vertIndices = {};


    Entity m_renderSpace{};


    void bindEvents();


    /* Writes data to a buffer that is allocated in GPU (device-local) memory.
        @param data: The data to write to the buffer.
        @param buffer: The buffer to which the data is to be written.
        @param bufferSize: The size of the buffer (in bytes).
    */
    void writeDataToGPUBuffer(const void* data, VkBuffer& buffer, VkDeviceSize bufferSize);


    /* Creates uniform buffers. */
    void createUniformBuffers();


    /* Finds the memory type suitable for buffer and application requirements.
        
        GPUs offer different types of memory to allocate from, each differs in allowed operations and performance characteristics.
	    @param typeFilter: A bitfield specifying the memory types that are suitable.
	    @param properties: The properties of the memory.

	    @return The index of the sutiable memory type.
    */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
