/* SymplecticEuler.hpp - Implementation of the Symplectic Euler numerical integrator.
*/

#pragma once

#include <Platform/External/GLM.hpp>


template<typename State, typename Derivative>
class SymplecticEuler {
public:
	SymplecticEuler() = default;
	~SymplecticEuler() = default;

	/* Integrates the state and velocity.
		@param state: The current state of the system.
		@param velocity: The current velocity of the system.
		@param acceleration: The current acceleration of the system.
		@param dt: The time step for integration.
	*/
	static void Integrate(State& state, Derivative& velocity, const Derivative& acceleration, double dt) {
		state += velocity * dt;
		velocity += acceleration * dt;
	}
};


// Symplectic Euler Integrator
using SymplecticEulerIntegrator = SymplecticEuler<glm::dvec3, glm::dvec3>;