/* BufferManager.hpp - Manages the vertex buffer.
*/
#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>

// C++ STLs
#include <iostream>
#include <string>
#include <array>

// Other
#include <Vulkan/VkDeviceManager.hpp>
#include <ApplicationContext.hpp>
#include <LoggingManager.hpp>
#include <MemoryManager.hpp>


// A structure defining a vertex.
struct Vertex {
	glm::vec2 position;
	glm::vec3 color;
};



class BufferManager {
public:
    BufferManager(VulkanContext& context, MemoryManager& memMgr, bool autoCleanup = true);
    ~BufferManager();

    void init();
    void cleanup();


    /* Creates a buffer.
    * @param &buffer: The buffer to be created.
    * @param deviceSize: The size of the buffer (in bytes).
    * @param usageFlags: Flags specifying how the buffer will be used.
    * @param memoryUsage: Flags specifying how the buffer's allocated memory block is used.
    * @param bufferAllocation: The memory block allocated for the buffer.
    * 
    * @return The cleanup task ID for the newly created buffer.
    */
    uint32_t createBuffer(VkBuffer& buffer, VkDeviceSize deviceSize, VkBufferUsageFlags usageFlags, VmaAllocation& allocation, VmaMemoryUsage memoryUsage);


    /* Copies the contents from a source buffer to a destination buffer.
    * @param srcBuffer: The source buffer that stores the contents to be transferred.
    * @param dstBuffer: The destination buffer to receive the contents from the source buffer.
    * @param deviceSize: The size of either the source or destination buffer (in bytes).
    */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize);


    /* Gets the vertex buffer. */
    inline const VkBuffer& getVertexBuffer() const { return vertexBuffer; }

    /* Gets the vertex data. */
    inline const std::vector<Vertex> getVertexData() const { return vertices; }


    /* Gets the index buffer. */
    inline const VkBuffer& getIndexBuffer() const { return indexBuffer; }

    /* Gets the vertex index data. */
    inline const std::vector<uint32_t> getVertexIndexData() const { return vertIndices; }


    /* Gets the vertex input binding description. */
    static VkVertexInputBindingDescription getBindingDescription();

    /* Gets the vertex attribute descriptions. */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();

private:
    bool cleanOnDestruction = true;
    VulkanContext& vkContext;
    MemoryManager& memoryManager;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation indexBufferAllocation = VK_NULL_HANDLE;

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},   // 0
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},    // 1
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},     // 2
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}     // 3
    };

    const std::vector<uint32_t> vertIndices = {
        0, 1, 2, 2, 3, 0
    };


    /* Writes data to a buffer that is allocated in GPU (device-local) memory.
    * @param data: The data to write to the buffer.
    * @param buffer: The buffer to which the data is to be written.
    * @param bufferSize: The size of the buffer (in bytes).
    */
    void writeDataToGPUBuffer(const void* data, VkBuffer& buffer, VkDeviceSize bufferSize);


    /* Creates the vertex buffer. */
    void createVertexBuffer();


    /* Creates the index buffer. */
    void createIndexBuffer();


    /* Finds the memory type suitable for buffer and application requirements.
    *
    * GPUs offer different types of memory to allocate from, each differs in allowed operations and performance characteristics.
	* @param typeFilter: A bitfield specifying the memory types that are suitable.
	* @param properties: The properties of the memory.
	* @return The index of the sutiable memory type.
    */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};