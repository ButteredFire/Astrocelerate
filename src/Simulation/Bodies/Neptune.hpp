#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Neptune : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "NEPTUNE"
			};
		}
		double getGravParam() const override { return 6.8351e+15; }
		double getEquatRadius() const override { return 2.4764e+7; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 1.0833e-4); }
		double getJ2() const override { return 3.4111e-3; }
		double getFlattening() const override { return 0.01708; }
		double getMass() const override { return 1.0243e+26; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Neptune/Neptune.gltf");
		}
	};
}
