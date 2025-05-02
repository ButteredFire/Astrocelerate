/* Time.hpp - Manages time.
*/

#pragma once

#include <chrono>


using high_resolution_clock = std::chrono::high_resolution_clock;
class Time {
public:
	inline static void UpdateDeltaTime() {
		auto currentTime = high_resolution_clock::now();
		deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();

		previousTime = currentTime;
	}

	inline static float GetDeltaTime() { return deltaTime; }

private:
	inline static float deltaTime = 0.0f;
	inline static std::chrono::time_point<high_resolution_clock> previousTime = high_resolution_clock::now();
};