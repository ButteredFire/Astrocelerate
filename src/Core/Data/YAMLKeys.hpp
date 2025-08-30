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
    _YAMLStrType Physics_Propagator             = "Physics::Propagator";
    _YAMLStrType Physics_ShapeParameters        = "Physics::ShapeParameters";
    _YAMLStrType Spacecraft_Spacecraft          = "Spacecraft::Spacecraft";
    _YAMLStrType Spacecraft_Thruster            = "Spacecraft::Thruster";
    _YAMLStrType Render_MeshRenderable          = "Render::MeshRenderable";

    // Special values
        // Entity values: Default entities
    _YAMLStrType Body_Prefix        = "Body::";
    _YAMLStrType Body_Sun           = Body_Prefix + "Sun";
    _YAMLStrType Body_Mercury       = Body_Prefix + "Mercury";
    _YAMLStrType Body_Venus         = Body_Prefix + "Venus";
    _YAMLStrType Body_Earth         = Body_Prefix + "Earth";
    _YAMLStrType Body_Moon          = Body_Prefix + "Moon";
    _YAMLStrType Body_Mars          = Body_Prefix + "Mars";
    _YAMLStrType Body_Jupiter       = Body_Prefix + "Jupiter";
    _YAMLStrType Body_Saturn        = Body_Prefix + "Saturn";
    _YAMLStrType Body_Uranus        = Body_Prefix + "Uranus";
    _YAMLStrType Body_Neptune       = Body_Prefix + "Neptune";
}


namespace YAMLData {
    _YAMLStrType Core_Identifiers_EntityType                = "EntityType";
	_YAMLStrType Core_Identifiers_SpiceID                   = "SpiceID";

	_YAMLStrType Core_Transform_Position                    = "Position";
	_YAMLStrType Core_Transform_Rotation                    = "Rotation";
	_YAMLStrType Core_Transform_Scale                       = "Scale";

	_YAMLStrType Physics_RigidBody_Velocity                 = "Velocity";
	_YAMLStrType Physics_RigidBody_Acceleration             = "Acceleration";
	_YAMLStrType Physics_RigidBody_Mass                     = "Mass";

	_YAMLStrType Physics_Propagator_PropagatorType          = "Type";
	_YAMLStrType Physics_Propagator_TLEPath                 = "TLEPath";

	_YAMLStrType Physics_ShapeParameters_EquatRadius        = "EquatRadius";
	_YAMLStrType Physics_ShapeParameters_Flattening         = "Flattening";
	_YAMLStrType Physics_ShapeParameters_GravParam          = "GravParam";
	_YAMLStrType Physics_ShapeParameters_RotVelocity        = "RotVelocity";
	_YAMLStrType Physics_ShapeParameters_J2                 = "J2";

    _YAMLStrType Spacecraft_Spacecraft_DragCoefficient      = "DragCoefficient";
    _YAMLStrType Spacecraft_Spacecraft_ReferenceArea        = "ReferenceArea";
	_YAMLStrType Spacecraft_Spacecraft_ReflectivityCoefficient = "ReflectivityCoefficient";

    _YAMLStrType Spacecraft_Thruster_ThrustMagnitude        = "ThrustMagnitude";
    _YAMLStrType Spacecraft_Thruster_SpecificImpulse        = "SpecificImpulse";
    _YAMLStrType Spacecraft_Thruster_CurrentFuelMass        = "CurrentFuelMass";
    _YAMLStrType Spacecraft_Thruster_MaxFuelMass            = "MaxFuelMass";
    
    _YAMLStrType Render_MeshRenderable_MeshPath             = "MeshPath";
    _YAMLStrType Render_MeshRenderable_VisualScale          = "VisualScale";
}
