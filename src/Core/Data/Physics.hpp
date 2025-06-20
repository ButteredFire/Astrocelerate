/* Physics.hpp - Common data pertaining to physics.
*/

#pragma once

#include <External/GLM.hpp>

#include <Core/Data/Constants.h>


namespace Physics {
	struct State {
		glm::dvec3 position;
		glm::dvec3 velocity;

		State operator+(const State& other) const {
			return State{
				.position = position + other.position,
				.velocity = velocity + other.velocity
			};
		}

		State operator-(const State& other) const {
			return State{
				.position = position - other.position,
				.velocity = velocity - other.velocity
			};
		}

		State operator*(double scalar) const {
			return State{
				.position = position * scalar,
				.velocity = velocity * scalar
			};
		}

		State operator/(double scalar) const {
			return State{
				.position = position / scalar,
				.velocity = velocity / scalar
			};
		}
	};
	/* NOTE:
		The operator* overload member in State only allows for `State * scalar`
		We must have this non-member operator* overload for `scaler * State`.
	*/
	inline State operator*(double scalar, const State& state) {
		return State{
			.position = scalar * state.position,
			.velocity = scalar * state.velocity
		};
	}


	// Physics reference frame types
	enum class FrameType {
		Inertial,
		Barycentric,
		Rotating,
		PlanetFixed
	};
}