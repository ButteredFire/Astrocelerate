/* BufferManager.hpp - Manages the vertex buffer.
*/
#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

// C++ STLs
#include <iostream>
#include <string>
#include <array>

// Other
#include <Vulkan/VkDeviceManager.hpp>
#include <ApplicationContext.hpp>
#include <LoggingManager.hpp>
#include <MemoryManager.hpp>
#include <ServiceLocator.hpp>


// A structure specifying the properties of a vertex
struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
};


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


class BufferManager {
public:
    BufferManager(VulkanContext& context, bool autoCleanup = true);
    ~BufferManager();

    void init();
    void cleanup();


    /* Creates a buffer.
    * @param vkContext: The application context.
    * @param &buffer: The buffer to be created.
    * @param bufferSize: The size of the buffer (in bytes).
    * @param usageFlags: Flags specifying how the buffer will be used.
    * @param bufferAllocation: The memory block allocated for the buffer.
    * @param bufferAllocationCreateInfo: The buffer allocation create info struct.
    * 
    * @return The cleanup task ID for the newly created buffer.
    */
    static uint32_t createBuffer(VulkanContext& vkContext, VkBuffer& buffer, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VmaAllocation& bufferAllocation, VmaAllocationCreateInfo bufferAllocationCreateInfo);


    /* Copies the contents from a source buffer to a destination buffer.
    * @param srcBuffer: The source buffer that stores the contents to be transferred.
    * @param dstBuffer: The destination buffer to receive the contents from the source buffer.
    * @param deviceSize: The size of either the source or destination buffer (in bytes).
    */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize deviceSize);
    

    /* Updates the uniform buffer. 
	* @param currentImage: The index of the current image/frame.
    */
    void updateUniformBuffer(uint32_t currentImage);


    /* Gets the vertex buffer. */
    inline const VkBuffer& getVertexBuffer() const { return vertexBuffer; }

    /* Gets the vertex data. */
    inline const std::vector<Vertex> getVertexData() const { return vertices; }


    /* Gets the index buffer. */
    inline const VkBuffer& getIndexBuffer() const { return indexBuffer; }

    /* Gets the vertex index data. */
    inline const std::vector<uint32_t> getVertexIndexData() const { return vertIndices; }


    /* Gets the uniform buffers */
    inline const std::vector<VkBuffer>& getUniformBuffers() const { return uniformBuffers; };

    /* Gets the uniform buffer allocations */
    inline const std::vector<VmaAllocation>& getUniformBuffersAllocations() const { return uniformBuffersAllocations; };


    /* Gets the vertex input binding description. */
    static VkVertexInputBindingDescription getBindingDescription();

    /* Gets the vertex attribute descriptions. */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();

private:
    bool cleanOnDestruction = true;
    VulkanContext& vkContext;
    std::shared_ptr<MemoryManager> memoryManager;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation indexBufferAllocation = VK_NULL_HANDLE;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBuffersAllocations;
    std::vector<void*> uniformBuffersMappedData;


    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},   // 0
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},    // 1
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},     // 2
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},    // 3
        {{0.0f, 0.0f, 1.0f}, {0.25f, 0.5f, 0.75f}}
    };

    const std::vector<uint32_t> vertIndices = {
        0, 1, 2,
        2, 3, 0,

        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
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


    /* Creates uniform buffers. */
    void createUniformBuffers();


    /* Finds the memory type suitable for buffer and application requirements.
    *
    * GPUs offer different types of memory to allocate from, each differs in allowed operations and performance characteristics.
	* @param typeFilter: A bitfield specifying the memory types that are suitable.
	* @param properties: The properties of the memory.
	* @return The index of the sutiable memory type.
    */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};