/* SpaceUtils.hpp - Utilities pertaining to world and simulation space.
*/

#pragma once

#include <glm_config.hpp>

#include <Core/Constants.h>


namespace SpaceUtils {
	/* @deprecated */
	inline glm::vec3 ToRenderSpace(const glm::dvec3& simPos, const glm::dvec3& camPos) {
		return static_cast<glm::vec3>((simPos - camPos) * SimulationConsts::SIMULATION_SCALE);
	}


	/* Converts to real space/scale. */
	inline glm::dvec3 ToRealSpace(const glm::dvec3& vec3) {
		return (vec3 / SimulationConsts::SIMULATION_SCALE);
	}


	/* Converts to simulation space/scale. */
	inline glm::vec3 ToSimSpace(const glm::dvec3& vec3) {
		return static_cast<glm::vec3>(vec3 * SimulationConsts::SIMULATION_SCALE);
	}
}