/* YAMLConversions.hpp - Common data pertaining to YAML encoding/decoding and data serialization/deserialization.
*/

#pragma once

#include <optional>
#include <yaml-cpp/yaml.h>
#include <external/GLM.hpp>

#include <Core/Application/LoggingManager.hpp>

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
                Log::Print(Log::T_WARNING, __FUNCTION__, "Bad conversion for std::optional: " + e.what());
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


// ----- CoreComponent::Transform -----
namespace YAML {
    template<>
    struct convert<CoreComponent::Transform> {
        static Node encode(const CoreComponent::Transform &rhs) {
            Node node;

            node["position"] = rhs.position;
            node["rotation"] = SpaceUtils::QuatToEulerAngles(rhs.rotation);

            return node;
        }

        static bool decode(const Node &node, CoreComponent::Transform &rhs) {
            if (!node.IsMap()) return false;

            rhs.position = node["position"].as<glm::dvec3>();
            rhs.rotation = SpaceUtils::EulerAnglesToQuat(node["rotation"].as<glm::dvec3>());

            return true;
        }
    };
}


// ----- PhysicsComponent::ReferenceFrame -----
namespace YAML {
    template<>
    struct convert<PhysicsComponent::ReferenceFrame> {
        static Node encode(const PhysicsComponent::ReferenceFrame &rhs) {
            Node node;

            node["parentID"] = rhs._parentID_str;

            node["localTransform"]["position"] = rhs.localTransform.position;
            node["localTransform"]["rotation"] = SpaceUtils::QuatToEulerAngles(rhs.localTransform.rotation);

            node["scale"] = rhs.scale;
            node["visualScale"] = rhs.visualScale;
            // if (rhs.visualScaleAffectsChildren) node["visualScaleAffectsChildren"] = rhs.visualScaleAffectsChildren;
            
            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::ReferenceFrame &rhs) {
            if (!node.IsMap()) return false;

            rhs._parentID_str = node["parentID"].as<std::string>();
            
            rhs.localTransform.position = node["localTransform"]["position"].as<glm::dvec3>();
            rhs.localTransform.rotation = SpaceUtils::EulerAnglesToQuat(node["localTransform"]["rotation"].as<glm::dvec3>());
            
            rhs.scale = node["scale"].as<double>();
            rhs.visualScale = node["visualScale"].as<double>();
            // if (node["visualScaleAffectsChildren"]) rhs.visualScaleAffectsChildren = node["visualScaleAffectsChildren"].as<bool>();
            
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

            node["velocity"] = rhs.velocity;
            node["acceleration"] = rhs.acceleration;
            node["mass"] = rhs.mass;

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::RigidBody &rhs) {
            if (!node.IsMap()) return false;

            rhs.velocity = node["velocity"].as<glm::dvec3>();
            rhs.acceleration = node["acceleration"].as<glm::dvec3>();
            rhs.mass = node["mass"].as<double>();

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

            node["equatRadius"] = rhs.equatRadius;
            node["flattening"] = rhs.flattening;
            node["gravParam"] = rhs.gravParam;
            node["rotVelocity"] = rhs.rotVelocity;
            node["j2"] = rhs.j2;

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::ShapeParameters &rhs) {
            if (!node.IsMap()) return false;

            rhs.equatRadius = node["equatRadius"].as<double>();
            rhs.flattening = node["flattening"].as<double>();
            rhs.gravParam = node["gravParam"].as<double>();
            rhs.rotVelocity = node["rotVelocity"].as<glm::dvec3>();
            rhs.j2 = node["j2"].as<double>();

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

            node["centralMass"] = rhs._centralMass_str;

            return node;
        }

        static bool decode(const Node &node, PhysicsComponent::OrbitingBody &rhs) {
            if (!node.IsMap()) return false;

            rhs._centralMass_str = node["centralMass"].as<std::string>();

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

            node["meshPath"] = rhs.meshPath;

            return node;
        }

        static bool decode(const Node &node, RenderComponent::MeshRenderable &rhs) {
            if (!node.IsMap()) return false;

            rhs.meshPath = node["meshPath"].as<std::string>();

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

            node["dragCoefficient"]             = rhs.dragCoefficient;
            node["referenceArea"]               = rhs.referenceArea;
            node["reflectivityCoefficient"]     = rhs.reflectivityCoefficient;

            return node;
        }

        static bool decode(const Node &node, SpacecraftComponent::Spacecraft &rhs) {
            if (!node.IsMap()) return false;

            rhs.dragCoefficient                 = node["dragCoefficient"].as<double>();
            rhs.referenceArea                   = node["referenceArea"].as<double>();
            rhs.reflectivityCoefficient         = node["reflectivityCoefficient"].as<double>();

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

            node["thrustMagnitude"] = rhs.thrustMagnitude;
            node["specificImpulse"] = rhs.specificImpulse;
            node["currentFuelMass"] = rhs.currentFuelMass;
            node["maxFuelMass"]     = rhs.maxFuelMass;

            return node;
        }

        static bool decode(const Node &node, SpacecraftComponent::Thruster &rhs) {
            if (!node.IsMap()) return false;

            rhs.thrustMagnitude     = node["thrustMagnitude"].as<double>();
            rhs.specificImpulse     = node["specificImpulse"].as<double>();
            rhs.currentFuelMass     = node["currentFuelMass"].as<double>();
            rhs.maxFuelMass         = node["maxFuelMass"].as<double>();

            return true;
        }
    };
}
