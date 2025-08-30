#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Jupiter : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "JUPITER"
			};
		}
		double getGravParam() const override { return 1.26686534e+17; }
		double getEquatRadius() const override { return 7.1492e+7; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 1.758525e-4); }
		double getJ2() const override { return 1.4736e-2; }
		double getFlattening() const override { return 0.06487; }
		double getMass() const override { return 1.8982e+27; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Jupiter/Jupiter.gltf");
		}
	};
}
