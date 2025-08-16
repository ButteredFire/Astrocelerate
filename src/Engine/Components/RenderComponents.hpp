/* RenderComponents.hpp - Defines components pertaining to rendering operations.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// Dear ImGui
#include <imgui/imgui.h>

#include <Core/Data/Math.hpp>
#include <Core/Data/Geometry.hpp>


namespace RenderComponent {
    // Common/Global scene data. For any given scene, there should only be 1 SceneData component.
    struct SceneData {
        Geometry::GeometryData *pGeomData;
    };


    struct MeshRenderable {
        std::string meshPath;                   // The source path to the mesh file.
        Math::Interval<uint32_t> meshRange;     // The mesh-offset range of THIS mesh (i.e., the index range of its child meshes in the mesh offsets array).
        double visualScale;                     // The mesh's visual size.
    };
}