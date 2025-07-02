/* Physics.hpp - Common data pertaining to maths.
*/

#pragma once

#include <External/GLM.hpp>

#include <Core/Data/Constants.h>


namespace Math {
	// Intervals
	enum IntervalType {
		T_INTERVAL_OPEN,
		T_INTERVAL_CLOSED,
		T_INTERVAL_HALF_OPEN_LEFT,
		T_INTERVAL_HALF_OPEN_RIGHT
	};

	template<typename DataType>
	struct Interval {
		IntervalType intervalType;
		DataType left;
		DataType right;
	};
}