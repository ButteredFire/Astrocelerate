#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Mercury : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "MERCURY"
			};
		}
		double getGravParam() const override { return 2.2032e+13; }
		double getEquatRadius() const override { return 2.4397e+6; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 1.24005856e-7); }
		double getJ2() const override { return 6.0e-5; }
		double getFlattening() const override { return 0.0; }
		double getMass() const override { return 3.3011e+23; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Mercury/Mercury.gltf");
		}
	};
}
