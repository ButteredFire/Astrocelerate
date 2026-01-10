/* Physics.hpp - Common data pertaining to maths.
*/

#pragma once

#include <array>
#include <vector>
#include <climits>

#include <Platform/External/GLM.hpp>

#include <Core/Data/Constants.h>

#include <Core/Utils/SystemUtils.hpp>


#define PI 3.14159265358979323846


namespace Math {
	// Intervals
	enum IntervalType {
		T_INTERVAL_OPEN,                // (a, b)
		T_INTERVAL_CLOSED,              // [a, b]
		T_INTERVAL_HALF_OPEN_LEFT,      // (a, b]
		T_INTERVAL_HALF_OPEN_RIGHT      // [a, b)
	};

	template<SystemUtils::Number Number>
	struct Interval {
		IntervalType intervalType;
		Number left;
		Number right;

		
        class Iterator {
        public:
            Iterator(Number current, Number step, Number end, IntervalType intervalType) :
                m_current(current), m_step(step), m_end(end), m_type(intervalType) {}


            // Only use epsilon for floating-point numbers
            static constexpr Number epsilon = (std::is_floating_point_v<Number>) ? Number(1e-6) : Number(0);


            // Checks when the iteration should stop (i.e., the current value is out of bounds)
            bool operator!=(const Iterator &other) const {
                switch (m_type) {
                case T_INTERVAL_OPEN:
                    return (m_step > 0) ? (m_current + epsilon < m_end) : (m_current - epsilon > m_end);

                case T_INTERVAL_CLOSED:
                    return (m_step > 0) ? (m_current - epsilon <= m_end) : (m_current + epsilon >= m_end);

                case T_INTERVAL_HALF_OPEN_LEFT:
                    return (m_step > 0) ? (m_current - epsilon <= m_end) : (m_current + epsilon >= m_end);

                case T_INTERVAL_HALF_OPEN_RIGHT:
                    return (m_step > 0) ? (m_current < m_end) : (m_current - epsilon > m_end);

                default:
                    return false;
                }
            }

            Number operator*() const { return m_current; }

            Iterator &operator++() {
                m_current += m_step;
                return *this;
            }

        private:
            Number m_current;
            Number m_step;
            Number m_end;
            IntervalType m_type;
        };


        struct Range {
            Iterator beginIter, endIter;
            Iterator begin() const { return beginIter; }
            Iterator end() const { return endIter; }
        };


        Range ComputeRange(Number step = Number(1)) const {
            Number beginCurrent;
            Number endCurrent;

            switch (intervalType) {
            case T_INTERVAL_OPEN:
                beginCurrent = left + step;
                endCurrent = right;
                break;

            case T_INTERVAL_CLOSED:
                beginCurrent = left;
                endCurrent = right + step;
                break;

            case T_INTERVAL_HALF_OPEN_LEFT:
                beginCurrent = left + step;
                endCurrent = right + step;
                break;

            case T_INTERVAL_HALF_OPEN_RIGHT:
            default:
                beginCurrent = left;
                endCurrent = right;
                break;
            }


            return Range{
                .beginIter =    Iterator(beginCurrent, step, right, intervalType),
                .endIter =      Iterator(endCurrent, step, right, intervalType)
            };
        }


        Iterator begin() const  { return ComputeRange().begin(); }
        Iterator end() const    { return ComputeRange().end(); }
	};



    template<SystemUtils::Number Number, uint32_t size>
    class Polynomial {
    public:
        /* Initializes a polynomial.
            @param coefficients: Coefficients of the polynomial, in increasing degree (constant, linear, quadratic, ...).
        */
        inline Polynomial(const std::array<Number, static_cast<size_t>(size)> &coefficients) : m_cfs(coefficients) {};
        inline Polynomial(std::initializer_list<Number> coefficients) {
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


        inline Number operator[](const uint32_t idx) const {
            LOG_ASSERT(idx < size, "Cannot access nonexistent coefficient of degree " + idx + ".");
            return m_cfs[size];
        }

    private:
        std::array<Number, static_cast<size_t>(size)> m_cfs;   // Coefficients vector (in increasing degree)
    };
}
