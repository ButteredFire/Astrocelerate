/* SpaceUtils.hpp - Utilities pertaining to world and simulation space.  
*/  

#pragma once  

#include <algorithm>
#include <glm_config.hpp>  

#include <Core/Constants.h>

#include <Utils/SystemUtils.hpp>


namespace SpaceUtils {  
	/* Converts to simulation space/scale. Simulation space is one wherein objects retain real world data. */
	inline auto ToSimulationSpace(const SystemUtils::MultipliableByDouble auto& value) {
		return value * SimulationConsts::SIMULATION_SCALE;
	}  


	/* Converts to render space/scale. Render space is one wherein objects have their data scaled down for visual output. */
	inline auto ToRenderSpace(const SystemUtils::DivisibleByDouble auto& value) {
		return value / SimulationConsts::SIMULATION_SCALE;
	}


	/* Gets the maximum smallest scale that could be rendered in render space. */  
	inline double GetRenderableScale(double scale) {  
		return std::fmax(scale, 0.1);  
	}  
}