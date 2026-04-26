/* NU2ANOM imeplementation
	C++ adaptation of David Vallado's "ν to Anomaly" algorithm for Astrocelerate.

	Sources:
	- "ν to Anomaly" Pseudocode: Algorithm 5, Fundamentals of Astrodynamics and Applications
	- newtonnu MATLAB Implementation: https://github.com/jgte/matlab-sgp4/blob/master/newtonnu.m
*/

#pragma once

#include <array>
#include <cmath>

#include <Platform/External/GLM.hpp>

#include <Core/Data/Math.hpp>


namespace Kepler {
	/* Solves Kepler's equation when the true anomaly is known.
		@param e: Eccentricity
		@param nu: True anomaly (ν)

		@return The array {e0, m}, representing {eccentric anomaly, mean anomaly}.
	*/
	inline std::array<double, 2> nu2anom(double e, double nu) {
		double e0 = NAN;
		double m = NAN;
		

		// Circular orbit (e = 0)
		if (glm::abs(e) < R_EPSILON) {
			e0 = nu;
			m = nu;
		}

		// Elliptical orbit (0 < e < 1)
		else if (e < 1.0 - R_EPSILON) {
			double sin_e = (glm::sin(nu) * glm::sqrt(1.0 - e * e)) / (1.0 + e * glm::cos(nu));
			double cos_e = (e + glm::cos(nu)) / (1.0 + e * glm::cos(nu));

			e0 = glm::atan(sin_e, cos_e); // GLM's atan(y, x) overload is equivalent to std::atan2
			m = e0 - e * glm::sin(e0);

			// Normalize `e0` and `m` to stay within [0, 2pi) rad for elliptical orbits
			e0 = std::fmod(e0, TWOPI);
			m = std::fmod(m, TWOPI);
			if (m < 0.0)
				m += TWOPI;
		}

		// Parabolic orbit (e = 1)
		else if (1.0 - R_EPSILON < e && e < 1.0 + R_EPSILON) {
			e0 = glm::tan(nu * 0.5);
			m = e0 + (e0 * e0 * e0) / 3.0;
		}

		// Hyperbolic orbit (e > 1)
		else if (e > 1.0 + R_EPSILON) {
			double sinh_e = (glm::sin(nu) * glm::sqrt(e * e - 1.0)) / (1.0 + e * glm::cos(nu));
			//double cosh_e = (e + glm::cos(nu)) / (1.0 + e * glm::cos(nu));
			
			e0 = glm::asinh(sinh_e);
			m = e * glm::sinh(e0) - e0;
		}
		

		return std::array<double, 2>{ e0, m };
	}
}