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


// A structure defining a vertex.
struct Vertex {
	glm::vec2 position;
	glm::vec3 color;

    /* Gets the vertex input binding description. */
    static VkVertexInputBindingDescription getBindingDescription() {
        // A vertex binding describes at which rate to load data from memory throughout the vertices.
        // It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
        VkVertexInputBindingDescription bindingDescription{};

            // Our data is currently packed together in 1 array, so we're only going to have one binding (whose index is 0).
        bindingDescription.binding = 0;

            // Specifies the number of bytes from one entry to the next (i.e., byte stride between consecutive elements in a buffer)
        bindingDescription.stride = sizeof(Vertex);

            // Specifies how to move to the next entry
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next entry after each vertex (for per-vertex data); for instanced rendering, use INPUT_RATE_INSTANCE

        return bindingDescription;
    }


    /* Gets the vertex attribute description. */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription() {
        // Attribute descriptions specify the type of the attributes passed to the vertex shader, which binding to load them from (and at which offset)
        // Each vertex attribute (e.g., position, color) must have its own attribute description.
        std::array<VkVertexInputAttributeDescription, 2> attribDescriptions{};

            // Attribute: Position
        attribDescriptions[0].binding = 0;
        attribDescriptions[0].location = 0;
        attribDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // R32G32 because `position` is a vec2. If it were a vec3, its format would be R32G32B32_SFLOAT or similar
        attribDescriptions[0].offset = offsetof(Vertex, position);
    
            // Attribute: Color
        attribDescriptions[1].binding = 0;
        attribDescriptions[1].location = 1;
        attribDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribDescriptions[1].offset = offsetof(Vertex, color);


        return attribDescriptions;
    }
};


const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};