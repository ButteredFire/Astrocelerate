/* RenderComponents.hpp - Defines components pertaining to rendering operations.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// Dear ImGui
#include <imgui/imgui.h>

#include <Core/Data/Geometry.hpp>


namespace RenderComponent {
    struct MeshRenderable {
        Geometry::MeshOffset meshOffset;                    // Mesh offset into the global vertex and index buffers.
        size_t uboIndex;                                    // Index into the mesh's uniform buffer object.
    };


    struct GUIRenderable {
        ImDrawData* guiDrawData = nullptr;                  // The ImGui draw data.
    };
}