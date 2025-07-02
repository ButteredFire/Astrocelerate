/* ODEs.hpp - Defines ordinary differential equations.
*/

#pragma once

#include <Core/Data/Physics.hpp>

namespace ODE {
	using namespace Physics;

	struct NewtonianTwoBody {
		double centralMass;		// Mass of the body being orbited.

		State operator()(const State& state, double t) const {
			using namespace PhysicsConsts;

			glm::dvec3 relativePosition = state.position;
			glm::dvec3 currentVelocity = state.velocity;

			double distance = glm::length(relativePosition);

			// A small threshold to prevent numerical issues
			if (distance < 1e-12) {
				return State{
					.position = currentVelocity,
					.velocity = glm::dvec3(0.0, 0.0, 0.0) // Acceleration is zero at the center
				};
			}

			/*
				Mass 1: Orbiting body
				Mass 2: The body Mass 1 is orbiting around
				Let `r_vec = r_vec_1 - r_vec_2` be the relative position vector pointing from Mass 2 to Mass 1.

				Therefore, the force on Mass 1 due to Mass 2 is given by Newton's law of gravitation:
							F_vec_12  = -G * (m_1 * m_2) / |r_vec|^3  * r_vec
				<=>			m_1 * a_1 = -G * (m_1 * m_2) / |r_vec|^3  * r_vec		(Newton's Second Law)
				<=>			a_1       = -G * m_2 / |r_vec|^3  * r_vec
				<=>			a_1       = -G * (m_2 * r_vec) / r^3
			*/
			glm::dvec3 acceleration = -G * (centralMass * relativePosition) / (distance * distance * distance);


			// NOTE: The ODE returns the derivative of the state with respect to time (dState/dt)
			return State{
				.position = currentVelocity,		// dr/dt = v(t)
				.velocity = acceleration			// dv/dt = a(t)
			};
		}
	};
}