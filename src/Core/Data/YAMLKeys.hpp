/* YAMLKeys.hpp - Common data pertaining to main entry names in YAML simulation files.
*/

#pragma once

#include <string_view>	// std::string_view is efficient as it avoids string copying.


#define _YAMLStrType const std::string

// File configuration keys
namespace YAMLFileConfig {
    _YAMLStrType ROOT           = "FileConfig";
    _YAMLStrType Version        = "Version";
    _YAMLStrType Description    = "Description";
}


// Simulation configuration keys
namespace YAMLSimConfig {
    _YAMLStrType ROOT                   = "SimulationConfig";
    _YAMLStrType Kernels                = "Kernels";
    _YAMLStrType Kernel_Path            = "Path";
    _YAMLStrType CoordSys               = "CoordinateSystem";
    _YAMLStrType CoordSys_Frame         = "Frame";
    _YAMLStrType CoordSys_Epoch         = "Epoch";
    _YAMLStrType CoordSys_EpochFormat  = "EpochFormat";
}


// Scene keys and values
namespace YAMLScene {
    // Keys
    _YAMLStrType ROOT                                   = "Scene";
    _YAMLStrType Entity                                 = "Entity";
    _YAMLStrType Entity_Components                      = "Components";
    _YAMLStrType Entity_Components_Type                 = "Type";
    _YAMLStrType Entity_Components_Type_Data            = "Data";

    // Values
    _YAMLStrType Core_Identifiers               = "Core::Identifiers";
    _YAMLStrType Core_Transform                 = "Core::Transform";
    _YAMLStrType Physics_RigidBody              = "Physics::RigidBody";
    _YAMLStrType Physics_ShapeParameters        = "Physics::ShapeParameters";
    _YAMLStrType Spacecraft_Spacecraft          = "Spacecraft::Spacecraft";
    _YAMLStrType Spacecraft_Thruster            = "Spacecraft::Thruster";
    _YAMLStrType Render_MeshRenderable          = "Render::MeshRenderable";
}