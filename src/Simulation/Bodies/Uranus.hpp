#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Uranus : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "URANUS"
			};
		}
		double getGravParam() const override { return 5.793939e+15; }
		double getEquatRadius() const override { return 2.5559e+7; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 1.0123e-4); }
		double getJ2() const override { return 3.34343e-3; }
		double getFlattening() const override { return 0.0229; }
		double getMass() const override { return 8.681e+25; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Uranus/Uranus.gltf");
		}
	};
}
