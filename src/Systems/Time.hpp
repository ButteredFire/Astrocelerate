/* Time.hpp - Manages time.
*/

#pragma once

#include <chrono>


using high_resolution_clock = std::chrono::high_resolution_clock;
class Time {
public:
	inline static void UpdateDeltaTime() {
		auto currentTime = high_resolution_clock::now();
		m_deltaTime = std::chrono::duration<double>(currentTime - m_previousTime).count();

		m_previousTime = currentTime;
	}

	inline static double GetDeltaTime() { return m_deltaTime; }

	inline static float GetTimeScale() { return m_timeScale; }
	inline static void SetTimeScale(float newTimeScale) { m_timeScale = newTimeScale; }

private:
	inline static double m_deltaTime = 0;
	inline static float m_timeScale = 0.0f;
	inline static std::chrono::time_point<high_resolution_clock> m_previousTime = high_resolution_clock::now();
};