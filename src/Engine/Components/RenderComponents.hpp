/* RenderComponents.hpp - Defines components pertaining to rendering operations.
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Dear ImGui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <CoreStructs/Geometry.hpp>
#include <Engine/Components/ComponentTypes.hpp>


namespace Component {
    struct Renderable {
        ComponentType::Renderable type;             // The renderable type.

        // Vertex rendering
        std::vector<VkBuffer> vertexBuffers;                // A vector of vertex buffers.
        std::vector<VkDeviceSize> vertexBufferOffsets;      // A vector of vertex buffer offsets corresponding to their vertex buffers.
        VkBuffer indexBuffer = VK_NULL_HANDLE;              // The index buffer.
        std::vector<Geometry::Vertex> vertexData;
        std::vector<uint32_t> vertexIndexData;              // Vertex index data for the index buffer.
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;     // The descriptor set to be used in vertex rendering.
        

        // GUI rendering
        ImDrawData* guiDrawData = nullptr;                  // The ImGui draw data.
    };
}