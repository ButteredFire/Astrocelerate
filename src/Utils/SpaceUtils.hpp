/* SpaceUtils.hpp - Utilities pertaining to world and simulation space.  
*/  

#pragma once  

#include <algorithm>
#include <glm_config.hpp>  

#include <Core/Constants.h>  


namespace SpaceUtils {  
	/* Converts to simulation space/scale. Simulation space is one wherein objects retain real world data. */  
	inline glm::dvec3 ToSimulationSpace(const glm::dvec3& vec3) {  
		return (vec3 / SimulationConsts::SIMULATION_SCALE);  
	}  

	/* Converts to render space/scale. Render space is one wherein objects have their data scaled down for visual output. */  
	inline glm::vec3 ToRenderSpace(const glm::dvec3& vec3) {  
		return static_cast<glm::vec3>(vec3 * SimulationConsts::SIMULATION_SCALE);  
	}  

	/* Gets the maximum smallest scale that could be rendered in render space. */  
	inline double GetRenderableScale(double scale) {  
		return std::fmax(scale, 0.005);  
	}  
}