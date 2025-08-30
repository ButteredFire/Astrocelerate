#pragma once

#include "ICelestialBody.hpp"


namespace _Bodies {
	class Saturn : public ICelestialBody {
	public:
		CoreComponent::Identifiers getIdentifiers() const override {
			return CoreComponent::Identifiers{
				.entityType = CoreComponent::Identifiers::EntityType::PLANET,
				.spiceID = "SATURN"
			};
		}
		double getGravParam() const override { return 3.7931207e+16; }
		double getEquatRadius() const override { return 6.0268e+7; }
		glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 1.6378e-4); }
		double getJ2() const override { return 1.6298e-2; }
		double getFlattening() const override { return 0.09796; }
		double getMass() const override { return 5.6834e+26; }

		std::string getMeshPath() const override {
			return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Saturn/Saturn.gltf");
		}
	};
}
