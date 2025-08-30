/* Time.hpp - Manages time.
*/

#pragma once

#include <chrono>

#include <mutex>


using high_resolution_clock = std::chrono::high_resolution_clock;
class Time {
public:
	inline static void UpdateDeltaTime() {
		std::lock_guard<std::mutex> lock(m_dtMutex);

		auto currentTime = high_resolution_clock::now();
		m_deltaTime = std::chrono::duration<double>(currentTime - m_previousTime).count();

		m_previousTime = currentTime;
	}

	inline static double GetDeltaTime() { return m_deltaTime; }

	/* Gets the current time. */
	inline static std::chrono::time_point<high_resolution_clock> GetTime() { return high_resolution_clock::now(); }

	inline static float GetTimeScale() { return m_timeScale; }
	inline static void SetTimeScale(float newTimeScale) {
		std::lock_guard<std::mutex> lock(m_timeScaleMutex);
		m_timeScale = newTimeScale;
	}

private:
	inline static std::mutex m_dtMutex;
	inline static std::mutex m_timeScaleMutex;

	inline static double m_deltaTime = 0;
	inline static float m_timeScale = 0.0f;
	inline static std::chrono::time_point<high_resolution_clock> m_previousTime = high_resolution_clock::now();
};