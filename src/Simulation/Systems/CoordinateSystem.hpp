/* CoordinateSystem - Implementation of a SPICE coordinate system.
*/

#pragma once

#include <array>
#include <vector>

#include <External/GLM.hpp>
#include <External/SPICE.hpp>

#include <Core/Data/Bodies.hpp>
#include <Core/Data/CoordSys.hpp>
#include <Core/Application/LoggingManager.hpp>

#include <Utils/SPICEUtils.hpp>
#include <Utils/FilePathUtils.hpp>


class CoordinateSystem {
public:
	CoordinateSystem();
	~CoordinateSystem();


	/* Resets the coordinate system. */
	void reset();


	/* Initializes the coordinate system.
		@param kernelPaths: A vector of paths to SPICE kernel files (e.g., "naif0012.tls", "de440s.bsp").
		@param frame: The frame used for the coordinate system.
		@param epoch: The epoch for which the frame is initialized.
		@param epochFormat: The format of the epoch (e.g., "YYYY-MM-DD HH:MM:SS TZ").
	*/
	void init(const std::vector<std::string> &kernelPaths, CoordSys::Frame frame, CoordSys::Epoch epoch, const std::string &epochFormat);


	/* Gets the state vector (position and velocity) of a body relative to this system's origin.
		@param targetName: The name of the target body (e.g., "Earth", "Mars").
		@param ephTime: The ephemeris time at which to get the state vector.
		
		@return The state vector [x, y, z, vx, vy, vz], in meters.
	*/
	std::array<double, 6> getBodyState(const std::string &targetName, double ephTime);


	/* Gets the rotation matrix of this system at a given ephemeris time.
		The rotation matrix is used to transform vectors from this system to another frame.

		For example, if you have a vector in the system and you want to transform it to a body-fixed frame,
		this rotation matrix can be used to perform that transformation.

		@param targetFrame: The name of the target frame to which the rotation matrix is relative (e.g., "J2000", "IAU_EARTH").
		@param ephTime: The ephemeris time at which to get the rotation matrix.
		
		@return A 3x3 rotation matrix.
	*/
	glm::mat3 getRotationMatrix(const std::string &targetFrame, double ephTime);


	/* Transforms a vector from the TEME coordinate system to this system's frame at a given ephemeris time.
		@param stateVector: The state vector to be transformed.
		@param ephTime: The ephemeris time (ET) at which to perform the transformation.

		@return A 6-component array containing the transformed position and velocity vectors in this system's frame.
	*/
	std::array<double, 6> TEMEToThisFrame(const std::array<double, 6> &stateVector, double ephTime);


	/* Gets the epoch in Ephemeris Time. */
	inline double getEpochET() const { return m_epochET; };

	/* Gets the epoch in Julian Ephemeris Date. */
	inline double getEpochJED() const {
		return unitim_c(m_epochET, "ET", "JED");	// ET -> JED (Ephemeris Time -> Julian Ephemeris Date)
	}

private:
	std::string m_observerName{};
	std::string m_frameName{};

	std::string m_epochFormat{};
	double m_epochET = 0;	// Epoch in Ephemeris Time (ET)
};