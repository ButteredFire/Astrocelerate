/* ThreadManager.hpp - Manages threads and multithreading.
*/

#pragma once

#include <thread>
#include <sstream>

#include <Core/Application/LoggingManager.hpp>


class ThreadManager {
public:
	/* Gets the ID of the main thread.
		@return The ID of the main thread.
	*/
	inline static std::thread::id GetMainThreadID() { return m_mainThreadID; };
	inline static std::string GetMainThreadIDAsString() {
		return ThreadIDToString(m_mainThreadID);
	};


	/* Converts a thread ID to a string.
		@param threadID: The thread ID in question.
		
		@return The thread ID as a string.
	*/
	inline static std::string ThreadIDToString(std::thread::id threadID) {
		std::stringstream ss;
		ss << threadID;
		return ss.str();
	};


	/* Sets the ID of the main thread.
		@param newThreadID: The ID of the new thread to be considered main.
	*/
	inline static void SetMainThreadID(const std::thread::id newThreadID) {
		m_mainThreadID = newThreadID; 

		std::stringstream stream;
		stream << m_mainThreadID;

		Log::Print(Log::T_INFO, __FUNCTION__, "Main thread has been set to Thread " + stream.str());
	};

private:
	inline static std::thread::id m_mainThreadID;
};