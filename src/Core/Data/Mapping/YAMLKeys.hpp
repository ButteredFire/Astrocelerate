/* YAMLKeys.hpp - Common data pertaining to main entry names in YAML simulation files.
*/

#pragma once

#include <string_view>	// std::string_view is efficient as it avoids string copying.
#include <string>
#include <optional>
#include <unordered_map>


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
    _YAMLStrType CoordSys_EpochFormat   = "EpochFormat";
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
    _YAMLStrType Physics_OrbitalElements        = "Physics::OrbitalElements";
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

    _YAMLStrType Physics_OrbitalElements_SemiMajorAxis      = "SemiMajorAxis";
    _YAMLStrType Physics_OrbitalElements_Eccentricity       = "Eccentricity";
	_YAMLStrType Physics_OrbitalElements_Inclination        = "Inclination";
	_YAMLStrType Physics_OrbitalElements_RAAN               = "RAAN";
	_YAMLStrType Physics_OrbitalElements_ArgPeriapsis       = "ArgPeriapsis";
	_YAMLStrType Physics_OrbitalElements_TrueAnomaly        = "TrueAnomaly";
	_YAMLStrType Physics_OrbitalElements_ParentBody         = "ParentBody";

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


// Descriptions for YAML keys
namespace YAMLKeyDescription {
    struct KeyDescription {
        enum class Type {
            SCALAR_STRING,
            SCALAR_NUMBER,
            SCALAR_BOOL,

            SEQUENCE,

            // In YAML, "arrays" (e.g., `Type: [X, Y, Z]`) are inline versions of sequences (e.g., `Type: \n    -X \n    -Y \n    -Z`)
            SEQUENCE_ARRAY_3,   // 3-component vector: [X, Y, Z]
            SEQUENCE_ARRAY_4,   // 4-component vector: [X, Y, Z, T]

            MAPPING
        };

        std::string description{};            // Description
        std::optional<std::string> unit{};    // Expected Simulation Unit (optional)
        std::optional<Type> type{};           // Expected Type (optional)
    };


    using enum KeyDescription::Type;
    inline const std::unordered_map<std::string, KeyDescription> Mapping = {
        // File configuration
        { YAMLFileConfig::ROOT,
            {
                "Top-level file configuration node containing metadata about the simulation file (version, description, etc.).",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLFileConfig::Version,
            {
                "Simulation file format version string used for compatibility checks when loading the file.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLFileConfig::Description,
            {
                "Human-readable description or notes about the simulation file.",
                std::nullopt,
                SCALAR_STRING
            }
        },


        // Simulation configuration
        { YAMLSimConfig::ROOT,
            {
                "Top-level simulation configuration node.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLSimConfig::Kernels,
            {
                "Array of SPICE kernel entries required by the simulation (leapseconds, ephemerides, etc.).",
                std::nullopt,
                SEQUENCE
            }
        },
        { YAMLSimConfig::Kernel_Path,
            {
                "Filesystem path to a SPICE kernel file. Relative paths are resolved against the application's root directory.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLSimConfig::CoordSys,
            {
                "Coordinate system configuration node. Defines the simulation frame and epoch used for transforms and state initialization.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLSimConfig::CoordSys_Frame,
            {
                "The primary reference frame for the simulation.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLSimConfig::CoordSys_Epoch,
            {
                "A specific fixed moment in time used as a reference point for time-dependent astronomical quantities, such as state vectors and orbital elements.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLSimConfig::CoordSys_EpochFormat,
            {
                "Optional epoch format string that controls how epoch values are parsed or displayed.",
                std::nullopt,
                SCALAR_STRING
            }
        },


        // Scene keys
        { YAMLScene::ROOT,
            {
                "Top-level scene node containing an array of entity definitions.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Entity,
            {
                "The name of an entity in the scene.\nBuilt-in bodies use the 'Body::' prefix.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLScene::Entity_Components,
            {
                "Array of component nodes attached to the entity.\nEach element has a 'Type' and associated 'Data'.",
                std::nullopt,
                SEQUENCE
            }
        },
        { YAMLScene::Entity_Components_Type,
            {
                "Component 'Type' identifier string used by the loader to determine which component data to apply.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLScene::Entity_Components_Type_Data,
            {
                "Container node holding the field data for the specified component.",
                std::nullopt,
                MAPPING
            }
        },


        // Component type identifiers
        { YAMLScene::Core_Identifiers,
            {
                "Specifies identity metadata for an entity.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Core_Transform,
            {
                "Specifies the entity's spatial state in the chosen coordinate frame.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Physics_RigidBody,
            {
                "Specifies the instantaneous dynamic state of the entity.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Physics_Propagator,
            {
                "Configures an orbital propagator for the entity.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Physics_ShapeParameters,
            {
                "Specifies the body's physical shape and gravity-related parameters.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Physics_OrbitalElements,
            {
                "Specifies Keplerian orbital elements and a parent-body reference.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Spacecraft_Spacecraft,
            {
                "Specifies spacecraft-level properties used for environmental force modeling such as drag and radiation pressure.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Spacecraft_Thruster,
            {
                "Specifies thruster performance and fuel state used for propulsion modeling.",
                std::nullopt,
                MAPPING
            }
        },
        { YAMLScene::Render_MeshRenderable,
            {
                "Specifies a visual mesh asset and per-renderable visual scale for the entity's appearance.",
                std::nullopt,
                MAPPING
            }
        },


        // Built-in body identifiers
        { YAMLScene::Body_Prefix,
            {
                "Prefix used for built-in celestial bodies (e.g., 'Body::Earth').\nEntities beginning with this prefix will be initialized according to a predefined component preset that can be overridden by specifying individual component overrides in the entity's Components array.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLScene::Body_Sun,     { "Built-in preset: Sun.",     std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Mercury, { "Built-in preset: Mercury.", std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Venus,   { "Built-in preset: Venus.",   std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Earth,   { "Built-in preset: Earth.",   std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Moon,    { "Built-in preset: Moon.",    std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Mars,    { "Built-in preset: Mars.",    std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Jupiter, { "Built-in preset: Jupiter.", std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Saturn,  { "Built-in preset: Saturn.",  std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Uranus,  { "Built-in preset: Uranus.",  std::nullopt, SCALAR_STRING } },
        { YAMLScene::Body_Neptune, { "Built-in preset: Neptune.", std::nullopt, SCALAR_STRING } },


        // Individual data field keys
            // Core::Identifiers
        { YAMLData::Core_Identifiers_EntityType,
            {
                "Specifies the entity's semantic type (e.g., PLANET, SPACECRAFT, STAR).",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLData::Core_Identifiers_SpiceID,
            {
                "Optional NAIF/SPICE integer identifier (as string) used for SPICE calls when present.",
                std::nullopt,
                SCALAR_STRING
            }
        },

            // Core::Transform
        { YAMLData::Core_Transform_Position,
            {
                "Position vector for the entity's transform (interpreted in the chosen coordinate frame).",
                "m",
                SEQUENCE_ARRAY_3
            }
        },
        { YAMLData::Core_Transform_Rotation,
            {
                "Rotation (orientation) for the entity's transform. Values are Euler angles in degrees by default.",
                "°",
                SEQUENCE_ARRAY_3
            }
        },
        { YAMLData::Core_Transform_Scale,
            {
                "Scale factor for the entity's visual/physical size.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },

            // Physics::RigidBody
        { YAMLData::Physics_RigidBody_Velocity,
            {
                "Linear velocity vector for the rigid body component.",
                "m/s",
                SEQUENCE_ARRAY_3
            }
        },
        { YAMLData::Physics_RigidBody_Acceleration,
            {
                "Linear acceleration vector for the rigid body component.",
                "m/s²",
                SEQUENCE_ARRAY_3
            }
        },
        { YAMLData::Physics_RigidBody_Mass,
            {
                "Mass for the rigid body component.",
                "kg",
                SCALAR_NUMBER
            }
        },

            // Physics::Propagator
        { YAMLData::Physics_Propagator_PropagatorType,
            {
                "Propagator implementation selector (for example 'SGP4').\nIf set to SGP4, the loader will expect a valid TLE file and assume Earth as parent body.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLData::Physics_Propagator_TLEPath,
            {
                "Relative or absolute path to a Two-Line Element set describing the entity's orbital parameters.",
                std::nullopt,
                SCALAR_STRING
            }
        },

            // Physics::ShapeParameters
        { YAMLData::Physics_ShapeParameters_EquatRadius,
            {
                "Equatorial radius of the body.",
                "m",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_ShapeParameters_Flattening,
            {
                "Flattening (oblateness) parameter of the body.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_ShapeParameters_GravParam,
            {
                "Standard gravitational parameter (GM) of the body used in physics calculations.",
                "m³/s²",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_ShapeParameters_RotVelocity,
            {
                "Rotational/angular velocity of the body (3-element vector representing axis rates).",
                "rad/s",
                SEQUENCE_ARRAY_3
            }
        },
        { YAMLData::Physics_ShapeParameters_J2,
            {
                "Second zonal harmonic (J2) coefficient describing the oblateness perturbation.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },

            // Physics::OrbitalElements
        { YAMLData::Physics_OrbitalElements_SemiMajorAxis,
            {
                "Semi-major axis (a) of the orbit used to initialize orbital geometry.",
                "m",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_OrbitalElements_Eccentricity,
            {
                "Eccentricity (e) of the orbit.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_OrbitalElements_Inclination,
            {
                "Inclination (i) of the orbital plane.",
                "rad",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_OrbitalElements_RAAN,
            {
                "Right ascension of the ascending node (Ω / RAAN).",
                "rad",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_OrbitalElements_ArgPeriapsis,
            {
                "Argument of periapsis (ω) of the orbit.",
                "rad",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_OrbitalElements_TrueAnomaly,
            {
                "True anomaly (ν) describing the body's position along its orbit.",
                "rad",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Physics_OrbitalElements_ParentBody,
            {
                "String reference to another entity that acts as the central (parent) body for these orbital elements.\nMust reference an existing scene entity and cannot be self-referential.",
                std::nullopt,
                SCALAR_STRING
            }
        },

            // Spacecraft::Spacecraft
        { YAMLData::Spacecraft_Spacecraft_DragCoefficient,
            {
                "Drag coefficient used for atmospheric drag modeling on spacecraft.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },
        { YAMLData::Spacecraft_Spacecraft_ReferenceArea,
            {
                "Reference area used with drag coefficient to compute aerodynamic forces.",
                "m²",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Spacecraft_Spacecraft_ReflectivityCoefficient,
            {
                "Reflectivity coefficient used for simple radiation pressure calculations.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },

            // Spacecraft::Thruster
        { YAMLData::Spacecraft_Thruster_ThrustMagnitude,
            {
                "Thrust produced by the thruster.",
                "N",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Spacecraft_Thruster_SpecificImpulse,
            {
                "Specific impulse (Isp) of the thruster; used together with thrust to calculate fuel consumption.",
                "s",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Spacecraft_Thruster_CurrentFuelMass,
            {
                "Current fuel mass available to the thruster.",
                "kg",
                SCALAR_NUMBER
            }
        },
        { YAMLData::Spacecraft_Thruster_MaxFuelMass,
            {
                "Maximum fuel capacity for the thruster/system.",
                "kg",
                SCALAR_NUMBER
            }
        },

            // Render::MeshRenderable
        { YAMLData::Render_MeshRenderable_MeshPath,
            {
                "Filesystem path to the visual mesh asset.",
                std::nullopt,
                SCALAR_STRING
            }
        },
        { YAMLData::Render_MeshRenderable_VisualScale,
            {
                "Visual scale applied to the loaded mesh.",
                std::nullopt,
                SCALAR_NUMBER
            }
        },
    };
}
