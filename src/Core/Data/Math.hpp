/* Physics.hpp - Common data pertaining to maths.
*/

#pragma once

#include <array>
#include <vector>
#include <climits>

#include <External/GLM.hpp>

#include <Core/Data/Constants.h>

#include <Utils/SystemUtils.hpp>


#define PI 3.14159265358979323846


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


    template<SystemUtils::Number DataType, uint32_t size>
    class Polynomial {
    public:
        /* Initializes a polynomial.
            @param coefficients: Coefficients of the polynomial, in increasing degree (constant, linear, quadratic, ...).
        */
        inline Polynomial(const std::array<DataType, static_cast<size_t>(size)> &coefficients) : m_cfs(coefficients) {};
        inline Polynomial(std::initializer_list<DataType> coefficients) {
            LOG_ASSERT(size == coefficients.size(), "Cannot initialize polynomial: Polynomial is declared with " + std::to_string(size) + " terms, but initializer list contains " + std::to_string(coefficients.size()) + " coefficients.");

            size_t i = 0;
            for (const auto &coef : coefficients) {
                m_cfs[i++] = coef;
            }
        }


        /* Evaluates this polynomial.
            @return The resulting value.
        */
        template<typename T>
        inline T evaluate(T input) const {
            T val = 0;
            for (int32_t i = (int32_t)size - 1; i >= 0; i--)
                val = val * input + m_cfs[i];

            return val;
        }


        inline DataType operator[](const uint32_t idx) const {
            LOG_ASSERT(idx < size, "Cannot access nonexistent coefficient of degree " + idx + ".");
            return m_cfs[size];
        }

    private:
        std::array<DataType, static_cast<size_t>(size)> m_cfs;   // Coefficients vector (in increasing degree)
    };
}
