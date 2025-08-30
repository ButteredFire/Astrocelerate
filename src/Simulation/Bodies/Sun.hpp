#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Sun : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::STAR,
				.spiceID = "SUN"
			};
		}
		double getGravParam() const override { return 1.32712440018e+20; }
		double getEquatRadius() const override { return 6.96342e+8; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 2.76865624e-6); }
		double getJ2() const override { return 2.2e-7; }
		double getFlattening() const override { return 9.0e-6; }
		double getMass() const override { return 1.98855e+30; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Sun/Sun.gltf");
		}
	};
}
