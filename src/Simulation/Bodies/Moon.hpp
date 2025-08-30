#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Moon : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::MOON,
				.spiceID = "MOON"
			};
		}
		double getGravParam() const override { return 4.9048695e+12; }
		double getEquatRadius() const override { return 1.7374e+6; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 2.6616995e-6); }
		double getJ2() const override { return 2.027e-4; }
		double getFlattening() const override { return 0.0011; }
		double getMass() const override { return 7.342e+22; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Moon/Moon.gltf");
		}
	};
}
