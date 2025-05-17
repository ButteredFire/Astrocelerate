/* RK4.hpp - Implementation of the fourth-order Runge-Kutta (RK4) numerical integrator.
*/

#pragma once

#include <functional>

#include <glm_config.hpp>


// TODO
// Fourth-order Runge-Kutta Integrator
template<typename State, typename DerivativeFunction>
class RK4Integrator {
public:
	RK4Integrator() = default;
	~RK4Integrator() = default;

	/* Integrates the state using the RK4 method.
		@param state: The current state of the system.
		@param t: The current time.
		@param dt: The time step for integration.
		@param derivativeFunction: A function that computes the derivative of the state.
	*/
	static void Integrate(State& state, double t, double dt, DerivativeFunction derivativeFunction) {
		State k1 = derivativeFunction(state, t);
		State k2 = derivativeFunction(state + 0.5 * dt * k1, t + 0.5 * dt);
		State k3 = derivativeFunction(state + 0.5 * dt * k2, t + 0.5 * dt);
		State k4 = derivativeFunction(state + dt * k3, t + dt);
		state = state + (k1 + 2.0 * k2 + 2.0 * k3 + k4) / 6.0 * dt;
	}
}
