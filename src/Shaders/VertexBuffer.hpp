/* VertexBuffer.hpp - Manages the vertex buffer.
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
#include <ApplicationContext.hpp>
#include <LoggingManager.hpp>


// A structure defining a vertex.
struct Vertex {
	glm::vec2 position;
	glm::vec3 color;
};



class VertexBuffer {
public:
    VertexBuffer(VulkanContext& context);
    ~VertexBuffer();

    void init();
    void cleanup();

    /* Gets the vertex buffer. */
    inline const VkBuffer getBuffer() const { return vertexBuffer; }

    /* Gets the vertex data. */
    inline const std::vector<Vertex> getVertexData() const { return vertices; }


    /* Gets the vertex input binding description. */
    static VkVertexInputBindingDescription getBindingDescription();

    /* Gets the vertex attribute descriptions. */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();

private:
    VulkanContext& vkContext;
    
    VkBufferCreateInfo bufCreateInfo{};
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };


    /* Creates a vertex buffer. */
    void createVertexBuffer();


    /* Allocates memory for the buffer. */
    void allocBufferMemory();


    /* Populates the vertex buffer with vertex data. */
    void loadVertexBuffer();

    /* Finds the memory type suitable for buffer and application requirements.
    *
    * GPUs offer different types of memory to allocate from, each differs in allowed operations and performance characteristics.
	* @param typeFilter: A bitfield specifying the memory types that are suitable.
	* @param properties: The properties of the memory.
	* @return The index of the sutiable memory type.
    */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};