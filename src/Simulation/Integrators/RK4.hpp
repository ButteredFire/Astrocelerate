/* RK4.hpp - Implementation of the fourth-order Runge-Kutta (RK4) numerical integrator.
*/

#pragma once

#include <functional>

#include <External/GLM.hpp>


// Fourth-order Runge-Kutta Integrator
template<typename State, typename ODESystem>
class RK4Integrator {
public:
	RK4Integrator() = default;
	~RK4Integrator() = default;

	/* Integrates the state using the RK4 method.
		@param state: The current state of the system.
		@param t: The current time.
		@param dt: The time step for integration.
		@param f: The ODE system function that computes the derivatives.
	*/
	static void Integrate(State& state, double t, double dt, ODESystem f) {
		State k1 = f(state, t);
		State k2 = f(state + 0.5 * dt * k1, t + 0.5 * dt);
		State k3 = f(state + 0.5 * dt * k2, t + 0.5 * dt);
		State k4 = f(state + dt * k3, t + dt);
		state = state + dt * (k1 + 2.0 * k2 + 2.0 * k3 + k4) / 6.0;
	}
};
