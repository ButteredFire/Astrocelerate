/* Physics.hpp - Common data pertaining to maths.
*/

#pragma once

#include <climits>

#include <External/GLM.hpp>

#include <Core/Data/Constants.h>

#include <Utils/SystemUtils.hpp>


namespace Math {
	// Intervals
	enum IntervalType {
		T_INTERVAL_OPEN,                // (a, b)
		T_INTERVAL_CLOSED,              // [a, b]
		T_INTERVAL_HALF_OPEN_LEFT,      // (a, b]
		T_INTERVAL_HALF_OPEN_RIGHT      // [a, b)
	};

	template<SystemUtils::Number DataType>
	struct Interval {
		IntervalType intervalType;
		DataType left;
		DataType right;

		
        struct Iterator {
            DataType current;
            DataType step;
            DataType end;
            IntervalType type;

            // Only use epsilon for floating-point numbers
            static constexpr DataType epsilon = (std::is_floating_point_v<DataType>)? DataType(1e-6) : DataType(0);


            // Checks when the iteration should stop (i.e., the current value is out of bounds)
            bool operator!=(const Iterator &other) const {
                switch (type) {
                case T_INTERVAL_OPEN:
                    return (step > 0)? (current + epsilon < end) : (current - epsilon > end);

                case T_INTERVAL_CLOSED:
                    return (step > 0)? (current - epsilon <= end) : (current + epsilon >= end);

                case T_INTERVAL_HALF_OPEN_LEFT:
                    return (step > 0)? (current - epsilon <= end) : (current + epsilon >= end);

                case T_INTERVAL_HALF_OPEN_RIGHT:
                    return (step > 0)? (current + epsilon < end) : (current - epsilon > end);

                default:
                    return false;
                }
            }

            DataType operator*() const { return current; }

            Iterator &operator++() {
                current += step;
                return *this;
            }
        };


        struct Range {
            Iterator beginIter, endIter;
            Iterator begin() const { return beginIter; }
            Iterator end() const { return endIter; }
        };


        Range operator()(DataType step = DataType(1)) const {
            DataType start = left;
            DataType finish = right;

            Iterator beginIter{}, endIter{};
            beginIter.step = endIter.step = step;
            beginIter.end = endIter.end = finish;
            beginIter.type = endIter.type = intervalType;


            switch (intervalType) {
            case T_INTERVAL_OPEN:
                beginIter.current = left + step;
                endIter.current = right;
                break;

			case T_INTERVAL_CLOSED:
				beginIter.current = left;
				endIter.current = right + step;
				break;

			case T_INTERVAL_HALF_OPEN_LEFT:
				beginIter.current = left + step;
				endIter.current = right + step;
				break;

			case T_INTERVAL_HALF_OPEN_RIGHT:
				beginIter.current = left;
				endIter.current = right;
				break;
            }


            return Range{
                .beginIter = beginIter,
                .endIter = endIter
            };
        }
	};
}
