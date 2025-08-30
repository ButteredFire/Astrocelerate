#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Venus : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "VENUS"
			};
		}
		double getGravParam() const override { return 3.24859e+14; }
		double getEquatRadius() const override { return 6.0518e+6; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, -2.99042211e-7); }
		double getJ2() const override { return 4.458e-6; }
		double getFlattening() const override { return 0.0; }
		double getMass() const override { return 4.8675e+24; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Venus/Venus.gltf");
		}
	};
}
