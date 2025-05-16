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


namespace Component {
    struct MeshRenderable {
        Geometry::MeshOffset meshOffset;                    // Mesh offset into the global vertex and index buffers.
        size_t uboIndex;                                    // Index into the mesh's uniform buffer object,
        size_t textureIndex;                                // Index into a texture atlas or descriptor array. (TODO)
    };


    struct GUIRenderable {
        ImDrawData* guiDrawData = nullptr;                  // The ImGui draw data.
    };
}