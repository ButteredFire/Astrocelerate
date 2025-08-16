/* CoordinateSystem - Implementation of the interface for a coordinate system.
*/

#pragma once

#include <array>
#include <vector>

#include <External/GLM.hpp>
#include <External/SPICE.hpp>

#include <Core/Data/CoordSys.hpp>


class CoordinateSystem {
public:
	virtual ~CoordinateSystem() = default;


	/* Initializes the inertial frame with the necessary SPICE kernels.
		@param kernelPaths: A vector of paths to SPICE kernel files (e.g., "naif0012.tls", "de440s.bsp").
		@param epoch: The epoch for which the frame is initialized.
		@param epochFormat: The format of the epoch (e.g., "YYYY-MM-DD HH:MM:SS TZ").
	*/
	virtual void init(const std::vector<std::string> &kernelPaths, CoordSys::Epoch epoch, const std::string &epochFormat) = 0;


	/* Gets the state vector (position and velocity) of a body relative to this frame's origin.
		@param targetName: The name of the target body (e.g., "Earth", "Mars").
		@param ephTime: The ephemeris time at which to get the state vector.
		
		@return The state vector [x, y, z, vx, vy, vz], in meters.
	*/
	virtual std::array<double, 6> getBodyState(const std::string &targetName, double ephTime) = 0;


	/* Gets the rotation matrix of this inertial frame at a given ephemeris time.
		The rotation matrix is used to transform vectors from this inertial frame to another frame.

		For example, if you have a vector in the inertial frame and you want to transform it to a body-fixed frame,
		this rotation matrix can be used to perform that transformation.

		@param targetFrame: The name of the target frame to which the rotation matrix is relative (e.g., "J2000", "IAU_EARTH").
		@param ephTime: The ephemeris time at which to get the rotation matrix.
		
		@return A 3x3 rotation matrix.
	*/
	virtual glm::mat3 getRotationMatrix(const std::string &targetFrame, double ephTime) = 0;


	/* Gets the epoch in Ephemeris Time. */
	virtual double getEpochET() const = 0;
};