#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Mars : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "MARS"
			};
		}
		double getGravParam() const override { return 4.282837e+13; }
		double getEquatRadius() const override { return 3.3962e+6; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 7.0882195e-5); }
		double getJ2() const override { return 1.96e-4; }
		double getFlattening() const override { return 0.00589; }
		double getMass() const override { return 6.4171e+23; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Mars/Mars.gltf");
		}
	};
}
