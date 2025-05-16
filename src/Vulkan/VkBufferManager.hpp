/* VkBufferManager.hpp - Manages the vertex buffer.
*/
#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#include <glm_config.hpp>

#include <chrono>

// C++ STLs
#include <iostream>
#include <string>
#include <array>

// Other
#include <Vulkan/VkDeviceManager.hpp>
#include <Vulkan/VkCommandManager.hpp>
#include <Vulkan/VkSyncManager.hpp>

#include <Core/ECS.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>
#include <Core/EventDispatcher.hpp>
#include <Core/GarbageCollector.hpp>

#include <CoreStructs/Buffer.hpp>
#include <CoreStructs/Geometry.hpp>
#include <CoreStructs/ApplicationContext.hpp>

#include <Engine/Components/ModelComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/WorldSpaceComponents.hpp>

#include <Utils/SystemUtils.hpp>
#include <Utils/FilePathUtils.hpp>
#include <Utils/SpaceUtils.hpp>

#include <Systems/Time.hpp>

#include <Rendering/Geometry/ModelParser.hpp>
#include <Rendering/Geometry/GeometryLoader.hpp>


/* VERY IMPORTANT EXPLANATION BEHIND alignas(...) PER STRUCT MEMBER: "Alignment requirements"

For example, if you have a struct like this:

    struct ABC {
        glm::vec2 foo;
        glm::mat4 bar;
        glm::mat4 foobar;
    }

... and you declare the struct in the vertex shader like this:

    layout(binding = 0) uniform ABC {
        vec2 foo;
        mat4 bar;
        mat4 foobar;
    } abc;

... the entire rendering pipeline fails, and nothing will be rendered on the screen. That is because we have not taken into account "alignment requirements." More information here: https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html#_alignment_requirements

[SOLUTION]
Always be explicit about alignment:

    struct ABC {
        alignas(8) glm::vec2 foo;
        alignas(16) glm::mat4 bar;
        alignas(16) glm::mat4 foobar;
    }

*/
// A structure specifying the properties of a uniform buffer object (UBO)
struct UniformBufferObject {
    alignas(16) glm::mat4 model;            // Object transformation matrix
    alignas(16) glm::mat4 view;             // Camera transformation matrix
    alignas(16) glm::mat4 projection;       // Depth and perspective transformation matrix
};


class VkBufferManager {
public:
    VkBufferManager(VulkanContext& context);
    ~VkBufferManager();

    void init();


    /* Creates a buffer.
        @param vkContext: The application context.
        @param &buffer: The buffer to be created.
        @param bufferSize: The size of the buffer (in bytes).
        @param usageFlags: Flags specifying how the buffer will be used.
        @param bufferAllocation: The memory block allocated for the buffer.
        @param bufferAllocationCreateInfo: The buffer allocation create info struct.
        
        @return The cleanup task ID for the newly created buffer.
    */
    static uint32_t createBuffer(VulkanContext& vkContext, VkBuffer& buffer, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VmaAllocation& bufferAllocation, VmaAllocationCreateInfo bufferAllocationCreateInfo);


    /* Creates the global vertex buffer. */
    void createGlobalVertexBuffer(const std::vector<Geometry::Vertex>& vertexData);


    /* Creates the global index buffer. */
    void createGlobalIndexBuffer(const std::vector<uint32_t>& indexData);


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
    */
    void updateObjectUBOs(uint32_t currentImage);


    /* Gets the vertex buffer. */
    inline const VkBuffer& getVertexBuffer() const { return m_vertexBuffer; }

    /* Gets the vertex data. */
    inline const std::vector<Geometry::Vertex> getVertexData() const { return m_vertices; }


    /* Gets the index buffer. */
    inline const VkBuffer& getIndexBuffer() const { return m_indexBuffer; }

    /* Gets the vertex index data. */
    inline const std::vector<uint32_t> getVertexIndexData() const { return m_vertIndices; }


    /* Gets the global uniform buffers */
    inline const std::vector<Buffer::BufferAndAlloc>& getGlobalUBOs() const { return m_globalUBOs; }


    /* Gets the object uniform buffers */
    inline const std::vector<Buffer::BufferAndAlloc>& getObjectUBOs() const { return m_objectUBOs; }

private:
    VulkanContext& m_vkContext;

    std::shared_ptr<Registry> m_registry;
    std::shared_ptr<EventDispatcher> m_eventDispatcher;
    std::shared_ptr<GarbageCollector> m_garbageCollector;

    
    Entity m_UBOEntity{};
    Component::RigidBody m_UBORigidBody{};

    Entity m_planet{};
    Entity m_satellite{};

    size_t m_totalObjects;
    
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexBufferAllocation = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_indexBufferAllocation = VK_NULL_HANDLE;


    std::vector<Buffer::BufferAndAlloc> m_globalUBOs;
    std::vector<void*> m_globalUBOMappedData;

    // Per-object UBOs
    std::vector<Buffer::BufferAndAlloc> m_objectUBOs;
    std::vector<void*> m_objectUBOMappedData;
    size_t m_alignedObjectUBOSize = 0;


    std::vector<Geometry::Vertex> m_vertices = {};
    std::vector<uint32_t> m_vertIndices = {};


    void bindEvents();



    /* Computes the byte offset into the buffer where the object UBO lies.
        @param currentImage: The index of the current frame.
        @param uboIndex: The mesh's index into its UBO.
    */
    inline void* getObjectUBO(uint32_t currentImage, size_t uboIndex) {
        static size_t alignedUBOSize = SystemUtils::align(sizeof(Buffer::ObjectUBO), m_vkContext.Device.deviceProperties.limits.minUniformBufferOffsetAlignment);

        /*
            First, we retrieve the pointer to the master object UBO of the current frame. We must cast it to a Byte pointer so that subsequent pointer increments will mean incrementing by bytes.
            Then, we calculate the offset of the target child object UBO from the base (start) of the master object UBO.
            Finally, we move the base pointer to the offset (start of the target UBO).
        */

        Byte* base = static_cast<Byte*>(m_objectUBOMappedData[currentImage]);
        Byte offset = static_cast<Byte>(alignedUBOSize * uboIndex);

        return static_cast<void*>(base + offset);
    }


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
