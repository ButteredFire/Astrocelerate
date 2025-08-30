/* YAMLConversions.hpp - Common data pertaining to YAML encoding/decoding and data serialization/deserialization.
*/

#pragma once

#include <optional>
#include <yaml-cpp/yaml.h>
#include <external/GLM.hpp>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/YAMLKeys.hpp>

#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/TelemetryComponents.hpp>
#include <Engine/Components/SpacecraftComponents.hpp>

#include <Utils/SpaceUtils.hpp>


// ----- std::optional -----
// YAML-CPP does not know how to handle this yet, so we have to provide explicit conversions.
namespace YAML {
    template<typename T>
    struct convert<std::optional<T>> {
        static Node encode(const std::optional<T> &rhs) {
            Node node;
            if (rhs.has_value()) {
                node = rhs.value(); // Assign the underlying value
            }
            else {
                node = YAML::Null; // Represent as 'null' in YAML if empty
            }
            return node;
        }

        static bool decode(const Node &node, std::optional<T> &rhs) {
            if (node.IsNull()) {
                rhs = std::nullopt;
                return true;
            }

            try {
                // Try converting the value to <T>. If there are no conversions for <T>, it's a bad conversion.
                rhs = node.as<T>();
                return true;
            }
            catch (const YAML::BadConversion &e) {
                Log::Print(Log::T_WARNING, __FUNCTION__, "Bad conversion for std::optional: " + std::string(e.what()));
                rhs = std::nullopt;
                return false;
            }
        }
    };
}


// ----- glm::dvec3 -----
namespace YAML {
    template<>
    struct convert<glm::dvec3> {
        static Node encode(const glm::dvec3 &rhs) {
            Node node;

            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow); // Optional: makes it a compact array [x, y, z]

            return node;
        }

        static bool decode(const Node &node, glm::dvec3 &rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<double>();
            rhs.y = node[1].as<double>();
            rhs.z = node[2].as<double>();

            return true;
        }
    };
}


// ----- glm::vec3 -----
namespace YAML {
    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3 &rhs) {
            Node node;

            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);

            return node;
        }

        static bool decode(const Node &node, glm::vec3 &rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<double>();
            rhs.y = node[1].as<double>();
            rhs.z = node[2].as<double>();

            return true;
        }
    };
}


// ----- glm::dquat -----
namespace YAML {
    template<>
    struct convert<glm::dquat> {
        static Node encode(const glm::dquat &rhs) {
            Node node;

            node.push_back(rhs.w);
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);

            return node;
        }

        static bool decode(const Node &node, glm::dquat &rhs) {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }

            rhs.w = node[0].as<double>();
            rhs.x = node[1].as<double>();
            rhs.y = node[2].as<double>();
            rhs.z = node[3].as<double>();
            return true;
        }
    };
}


// ----- CoreComponent::Identifiers -----
namespace YAML {
    template<>
    struct convert<CoreComponent::Identifiers> {
        static Node encode(const CoreComponent::Identifiers &rhs) {
            Node node;

            using enum CoreComponent::Identifiers::EntityType;
            switch (rhs.entityType) {
            case STAR:
                node[YAMLData::Core_Identifiers_EntityType] = "STAR";
                break;
            case PLANET:
                node[YAMLData::Core_Identifiers_EntityType] = "PLANET";
                break;
            case MOON:
                node[YAMLData::Core_Identifiers_EntityType] = "MOON";
                break;
            case SPACECRAFT:
                node[YAMLData::Core_Identifiers_EntityType] = "SPACECRAFT";
                break;
            case ASTEROID:
                node[YAMLData::Core_Identifiers_EntityType] = "ASTEROID";
                break;

            case UNKNOWN:
            default:
                node[YAMLData::Core_Identifiers_EntityType] = "UNKNOWN";
                break;
            }

            if (node[YAMLData::Core_Identifiers_SpiceID])
                node[YAMLData::Core_Identifiers_SpiceID] = rhs.spiceID;

            return node;
        }

        static bool decode(const Node &node, CoreComponent::Identifiers &rhs) {
            if (!node.IsMap()) return false;

            std::string entityType = node[YAMLData::Core_Identifiers_EntityType].as<std::string>();

			if (entityType == "STAR") {
				rhs.entityType = CoreComponent::Identifiers::EntityType::STAR;
			}
			else if (entityType == "PLANET") {
				rhs.entityType = CoreComponent::Identifiers::EntityType::PLANET;
			}
			else if (entityType == "MOON") {
				rhs.entityType = CoreComponent::Identifiers::EntityType::MOON;
			}
			else if (entityType == "SPACECRAFT") {
				rhs.entityType = CoreComponent::Identifiers::EntityType::SPACECRAFT;
			}
			else if (entityType == "ASTEROID") {
				rhs.entityType = CoreComponent::Identifiers::EntityType::ASTEROID;
			}
			else {
                if (entityType != "UNKNOWN")
                    Log::Print(Log::T_WARNING, __FUNCTION__, "Unknown entity type: " + entityType);

				rhs.entityType = CoreComponent::Identifiers::EntityType::UNKNOWN;
			}

            if (node[YAMLData::Core_Identifiers_SpiceID])
                rhs.spiceID = node[YAMLData::Core_Identifiers_SpiceID].as<std::optional<std::string>>();

            return true;
        }
    };
}


// ----- CoreComponent::Transform -----
namespace YAML {
    template<>
    struct convert<CoreComponent::Transform> {
        static Node encode(const CoreComponent::Transform &rhs) {
            Node node;

            node[YAMLData::Core_Transform_Position] = rhs.position;
            node[YAMLData::Core_Transform_Rotation] = SpaceUtils::QuatToEulerAngles(rhs.rotation);
            node[YAMLData::Core_Transform_Scale] = rhs.scale;

            return node;
        }

        static bool decode(const Node &node, CoreComponent::Transform &rhs) {
            if (!node.IsMap()) return false;

            rhs.position = node[YAMLData::Core_Transform_Position].as<glm::dvec3>();
            rhs.rotation = SpaceUtils::EulerAnglesToQuat(node[YAMLData::Core_Transform_Rotation].as<glm::dvec3>());
            rhs.scale = node[YAMLData::Core_Transform_Scale].as<double>();

            return true;
        }
    };
}


// ----- PhysicsComponent::RigidBody -----
namespace YAML {
    template<>
    struct convert<PhysicsComponent::RigidBody> {
        static Node encode(const PhysicsComponent::RigidBody &rhs) {
            Node node;

            node[YAMLData::Physics_RigidBody_Velocity] = rhs.velocity;
            node[YAMLData::Physics_RigidBody_Acceleration] = rhs.acceleration;
            node[YAMLData::Physics_RigidBody_Mass] = rhs.mass;

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::RigidBody &rhs) {
            if (!node.IsMap()) return false;

            rhs.velocity = node[YAMLData::Physics_RigidBody_Velocity].as<glm::dvec3>();
            rhs.acceleration = node[YAMLData::Physics_RigidBody_Acceleration].as<glm::dvec3>();
            rhs.mass = node[YAMLData::Physics_RigidBody_Mass].as<double>();

            return true;
        }
    };
}


// ----- PhysicsComponent::Propagator -----
namespace YAML {
    template<>
    struct convert<PhysicsComponent::Propagator> {
        static Node encode(const PhysicsComponent::Propagator &rhs) {
            Node node;

            using enum PhysicsComponent::Propagator::Type;
            switch (rhs.propagatorType) {
            case SGP4:
                node[YAMLData::Physics_Propagator_PropagatorType] = "SGP4";
                break;
            }

            node[YAMLData::Physics_Propagator_TLEPath] = rhs.tlePath;

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::Propagator &rhs) {
            if (!node.IsMap()) return false;

            std::string propagatorType = node[YAMLData::Physics_Propagator_PropagatorType].as<std::string>();
            if (propagatorType == "SGP4")
                rhs.propagatorType = PhysicsComponent::Propagator::Type::SGP4;
            else
                throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot deserialize data for component " + YAMLScene::Physics_Propagator + ": Cannot recognize propagator type " + enquote(propagatorType) + "!");


            rhs.tlePath = node[YAMLData::Physics_Propagator_TLEPath].as<std::string>();

            return true;
        }
    };
}


// ----- PhysicsComponent::ShapeParameters -----
namespace YAML {
    template<>
    struct convert<PhysicsComponent::ShapeParameters> {
        static Node encode(const PhysicsComponent::ShapeParameters &rhs) {
            Node node;

            node[YAMLData::Physics_ShapeParameters_EquatRadius] = rhs.equatRadius;
            node[YAMLData::Physics_ShapeParameters_Flattening] = rhs.flattening;
            node[YAMLData::Physics_ShapeParameters_GravParam] = rhs.gravParam;
            node[YAMLData::Physics_ShapeParameters_RotVelocity] = rhs.rotVelocity;
            node[YAMLData::Physics_ShapeParameters_J2] = rhs.j2;

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::ShapeParameters &rhs) {
            if (!node.IsMap()) return false;

            rhs.equatRadius = node[YAMLData::Physics_ShapeParameters_EquatRadius].as<double>();
            rhs.flattening = node[YAMLData::Physics_ShapeParameters_Flattening].as<double>();
            rhs.gravParam = node[YAMLData::Physics_ShapeParameters_GravParam].as<double>();
            rhs.rotVelocity = node[YAMLData::Physics_ShapeParameters_RotVelocity].as<glm::dvec3>();
            rhs.j2 = node[YAMLData::Physics_ShapeParameters_J2].as<double>();

            return true;
        }
    };
}


// ----- PhysicsComponent::OrbitingBody -----
namespace YAML {
    template<>
    struct convert<PhysicsComponent::OrbitingBody> {
        static Node encode(const PhysicsComponent::OrbitingBody &rhs) {
            Node node;

            node["CentralMass"] = rhs._centralMass_str; // This key was not in your YAMLData list

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::OrbitingBody &rhs) {
            if (!node.IsMap()) return false;

            rhs._centralMass_str = node["CentralMass"].as<std::string>(); // This key was not in your YAMLData list

            return true;
        }
    };
}


// ----- RenderComponent::MeshRenderable -----
namespace YAML {
    template<>
    struct convert<RenderComponent::MeshRenderable> {
        static Node encode(const RenderComponent::MeshRenderable &rhs) {
            Node node;

            node[YAMLData::Render_MeshRenderable_MeshPath] = rhs.meshPath;
            node[YAMLData::Render_MeshRenderable_VisualScale] = rhs.visualScale;

            return node;
        }

        static bool decode(const Node &node, RenderComponent::MeshRenderable &rhs) {
            if (!node.IsMap()) return false;

            rhs.meshPath = node[YAMLData::Render_MeshRenderable_MeshPath].as<std::string>();
            rhs.visualScale = node[YAMLData::Render_MeshRenderable_VisualScale].as<double>();

            return true;
        }
    };
}


// ----- TelemetryComponent::RenderTransform -----
namespace YAML {
    template<>
    struct convert<TelemetryComponent::RenderTransform> {
        static Node encode(const TelemetryComponent::RenderTransform &rhs) {
            Node node;
            return node;
        }

        static bool decode(const Node &node, TelemetryComponent::RenderTransform &rhs) {
            return true;
        }
    };
}



// ----- SpacecraftComponent::Spacecraft -----
namespace YAML {
    template<>
    struct convert<SpacecraftComponent::Spacecraft> {
        static Node encode(const SpacecraftComponent::Spacecraft &rhs) {
            Node node;

            node[YAMLData::Spacecraft_Spacecraft_DragCoefficient] = rhs.dragCoefficient;
            node[YAMLData::Spacecraft_Spacecraft_ReferenceArea] = rhs.referenceArea;
            node[YAMLData::Spacecraft_Spacecraft_ReflectivityCoefficient] = rhs.reflectivityCoefficient;

            return node;
        }

        static bool decode(const Node &node, SpacecraftComponent::Spacecraft &rhs) {
            if (!node.IsMap()) return false;

            rhs.dragCoefficient = node[YAMLData::Spacecraft_Spacecraft_DragCoefficient].as<double>();
            rhs.referenceArea = node[YAMLData::Spacecraft_Spacecraft_ReferenceArea].as<double>();
            rhs.reflectivityCoefficient = node[YAMLData::Spacecraft_Spacecraft_ReflectivityCoefficient].as<double>();

            return true;
        }
    };
}



// ----- SpacecraftComponent::Thruster -----
namespace YAML {
    template<>
    struct convert<SpacecraftComponent::Thruster> {
        static Node encode(const SpacecraftComponent::Thruster &rhs) {
            Node node;

            node[YAMLData::Spacecraft_Thruster_ThrustMagnitude] = rhs.thrustMagnitude;
            node[YAMLData::Spacecraft_Thruster_SpecificImpulse] = rhs.specificImpulse;
            node[YAMLData::Spacecraft_Thruster_CurrentFuelMass] = rhs.currentFuelMass;
            node[YAMLData::Spacecraft_Thruster_MaxFuelMass] = rhs.maxFuelMass;

            return node;
        }

        static bool decode(const Node &node, SpacecraftComponent::Thruster &rhs) {
            if (!node.IsMap()) return false;

            rhs.thrustMagnitude = node[YAMLData::Spacecraft_Thruster_ThrustMagnitude].as<double>();
            rhs.specificImpulse = node[YAMLData::Spacecraft_Thruster_SpecificImpulse].as<double>();
            rhs.currentFuelMass = node[YAMLData::Spacecraft_Thruster_CurrentFuelMass].as<double>();
            rhs.maxFuelMass = node[YAMLData::Spacecraft_Thruster_MaxFuelMass].as<double>();

            return true;
        }
    };
}
