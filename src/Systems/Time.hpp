/* Time.hpp - Manages time.
*/

#pragma once

#include <chrono>


using high_resolution_clock = std::chrono::high_resolution_clock;
class Time {
public:
	inline static void UpdateDeltaTime() {
		auto currentTime = high_resolution_clock::now();
		deltaTime = std::chrono::duration<double>(currentTime - previousTime).count();

		previousTime = currentTime;
	}

	inline static double GetDeltaTime() { return deltaTime; }

private:
	inline static double deltaTime = 0;
	inline static std::chrono::time_point<high_resolution_clock> previousTime = high_resolution_clock::now();
};