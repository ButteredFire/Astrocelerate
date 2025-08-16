/* ECIFrame - Implementation of the Earth-Centered Inertial Frame.
*/

#pragma once

#include "CoordinateSystem.hpp"

#include <Core/Application/LoggingManager.hpp>

#include <Utils/SPICEUtils.hpp>


class ECIFrame : public CoordinateSystem {
public:
	ECIFrame();
	~ECIFrame();

	void init(const std::vector<std::string> &kernelPaths, CoordSys::Epoch epoch, const std::string &epochFormat) override;

	std::array<double, 6> getBodyState(const std::string &targetName, double ephTime) override;

	glm::mat3 getRotationMatrix(const std::string &targetFrame, double ephTime) override;

	inline double getEpochET() const override { return m_epochET; }

private:
	const std::string m_frameName = "J2000";	// The SPICE frame name "J2000" implicitly refers to J2000 ECI.

	std::string m_epochName{};
	std::string m_epochFormat{};
	double m_epochET = 0;	// Epoch in Ephemeris Time (ET)
};