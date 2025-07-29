/* YAMLKeys.hpp - Common data pertaining to main entry names in YAML simulation files.
*/

#pragma once

#include <string_view>	// std::string_view is efficient as it avoids string copying.


namespace YAMLKey {
    // Entries (keys)
    static constexpr std::string_view Physics_ReferenceFrame        = "PhysicsComponent::ReferenceFrame";
    static constexpr std::string_view Physics_RigidBody             = "PhysicsComponent::RigidBody";
    static constexpr std::string_view Physics_ShapeParameters       = "PhysicsComponent::ShapeParameters";
    static constexpr std::string_view Physics_OrbitingBody          = "PhysicsComponent::OrbitingBody";
    static constexpr std::string_view Spacecraft_Spacecraft         = "SpacecraftComponent::Spacecraft";
    static constexpr std::string_view Spacecraft_Thruster           = "SpacecraftComponent::Thruster";
    static constexpr std::string_view Render_MeshRenderable         = "RenderComponent::MeshRenderable";
    static constexpr std::string_view Telemetry_RenderTransform     = "TelemetryComponent::RenderTransform";

    // Special values
    static constexpr std::string_view Ref       = "ref.";
}