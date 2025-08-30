/* SpaceUtils.hpp - Utilities pertaining to world and simulation space.  
*/  

#pragma once  

#include <algorithm>
#include <External/GLM.hpp>  

#include <Core/Data/Constants.h>

#include <Utils/SystemUtils.hpp>


namespace SpaceUtils {  
	constexpr double SCALE_FACTOR = 1000.0;  // This controls how much the scene is compressed
	constexpr double TRANSITION_DISTANCE = SimulationConsts::SIMULATION_SCALE * 10.0; // The distance where linear scaling switches to logarithmic scaling
	constexpr double OBJ_SCALE_VISUAL_BOOST = 10.0; // Exaggeration of object scale

	/* Converts to simulation space. */
	inline auto ToSimulationSpace(const SystemUtils::MultipliableByDouble auto& value) {
		return value * SimulationConsts::SIMULATION_SCALE;
	}


	/* Applies logarithmic scaling to simulation position to convert it to render space. */
	inline glm::dvec3 ToRenderSpace_Position(const glm::dvec3& vec) {
		/*
		double distance = glm::length(vec);
		if (distance == 0.0)
			// Prevents division-by-zero when normalizing later
			return glm::dvec3(0.0);

		if (distance <= TRANSITION_DISTANCE)
			// If distance from origin is too small, use linear scale
			return vec * (SpaceUtils::SCALE_FACTOR / SimulationConsts::SIMULATION_SCALE);

		glm::dvec3 direction = vec / distance;

		// Calculate the log base for continuity.
		// This factor ensures that at TRANSITION_DISTANCE, the log scale matches the linear scale.
		// Value at transition using linear scale: TRANSITION_DISTANCE * (SCALE_FACTOR / SIM_SCALE) = 10.0 * SCALE_FACTOR
		// So, we need: logScaleFactor * SCALE_FACTOR * glm::log(1.0 + TRANSITION_DISTANCE / SIM_SCALE) = 10.0 * SCALE_FACTOR
		// logScaleFactor * glm::log(1.0 + 10.0) = 10.0
		// logScaleFactor * glm::log(11.0) = 10.0
		// logScaleFactor = 10.0 / glm::log(11.0)
		const double LOG_SCALE_CORRECTION_FACTOR = 10.0 / glm::log(1.0 + TRANSITION_DISTANCE / SimulationConsts::SIMULATION_SCALE);

		double scaledDistance = LOG_SCALE_CORRECTION_FACTOR * SpaceUtils::SCALE_FACTOR * glm::log(1.0 + distance / SimulationConsts::SIMULATION_SCALE);
		
		return direction * scaledDistance;
		*/
		return vec / SimulationConsts::SIMULATION_SCALE;
	}


	/* Converts scale to render space. */
	inline double ToRenderSpace_Scale(const double simulationScalar) {
		return simulationScalar / SimulationConsts::SIMULATION_SCALE;
		//return simulationScalar * OBJ_SCALE_VISUAL_BOOST / SimulationConsts::SIMULATION_SCALE;
	}


	/* Gets the maximum smallest scale that could be rendered in render space. */  
	inline double GetRenderableScale(double scale) {  
		return std::fmax(scale, 0.01);  
	}


	/* Converts Euler angles to Quaternions.
		@param eulerAngles: The vector containing Euler angles.
		@param inRadians (default: False): Are the angles in radians? If True, they will be treated as radians. Otherwise, they will be treated as degrees.

		@return The corresponding quaternion.
	*/
	inline glm::dquat EulerAnglesToQuat(const glm::dvec3 &eulerAngles, bool inRadians = false) {
		return glm::dquat(
			((inRadians)? eulerAngles : glm::radians(eulerAngles))
		);
	}


	/* Converts Quaternions to Euler angles.
		@param quat: The quaternion to be converted.
		@param convertToRadians (default: False): Should the resulting vector be in radians? If True, it will be in radians. Otherwise, it will be in degrees.

		@return The corresponding vector with Euler coordinates.
	*/
	inline glm::dvec3 QuatToEulerAngles(const glm::dquat &quat, bool convertToRadians = false) {
		glm::dvec3 eulerRads = glm::eulerAngles(quat);
		if (convertToRadians)
			return eulerRads;

		return glm::degrees(eulerRads);
	}
}