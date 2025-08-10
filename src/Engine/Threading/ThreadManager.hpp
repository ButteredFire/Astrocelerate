/* ThreadManager.hpp - Manages threads and multithreading.
*/

#pragma once

#include <mutex>
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


	/* Gets the name of a thread created via ThreadManager::CreateThread.
		NOTE: If threadID is the main thread ID, an empty string will be returned.

		@param threadID: The thread's ID.

		@return The thread name.
	*/
	inline static std::string GetThreadNameFromID(std::thread::id threadID) {
		std::lock_guard<std::mutex> lock(m_threadMutex);

		if (threadID == m_mainThreadID)
			return "";

		auto it = m_threadIDToName.find(threadID);
		if (it == m_threadIDToName.end()) {
			Log::Print(Log::T_ERROR, __FUNCTION__, "Cannot find name for thread ID " + ThreadIDToString(threadID) + ". Using default thread name.");
			return "Worker";
		}

		return it->second;
	}


	/* Converts a thread ID to a string.
		@param threadID: The thread ID in question.
		
		@return The thread ID as a string.
	*/
	inline static std::string ThreadIDToString(std::thread::id threadID) {
		std::lock_guard<std::mutex> lock(m_threadIDToStrMutex);

		std::stringstream ss;
		ss << threadID;
		return ss.str();
	};


	/* Sets the ID of the main thread.
		@param newThreadID: The ID of the new thread to be considered main.
	*/
	inline static void SetMainThreadID(const std::thread::id newThreadID) {
		std::lock_guard<std::mutex> lock(m_threadMutex);

		m_mainThreadID = newThreadID;
		Log::Print(Log::T_INFO, __FUNCTION__, "Main thread has been set to Thread " + ThreadIDToString(m_mainThreadID));
	};


	/* Creates a new thread.
		@param threadName (Default: "Worker"): The thread's name (used for logging purposes). It is recommended to name it its responsibility in shortened, upper snake case style (e.g., "SCENE_INIT" for a thread working on scene loading).
		@param work: The work for the thread to handle, as a void function with no parameters.

		@return The new thread.
	*/
	inline static std::thread CreateThread(const std::string &threadName = "Worker", const std::function<void(void)> &work = []() {}) {
		std::lock_guard<std::mutex> lock(m_threadMutex);

		std::thread newThread(work);
		m_threadIDToName[newThread.get_id()] = threadName;
		return newThread;
	};

private:
	inline static std::thread::id m_mainThreadID;
	inline static std::mutex m_threadMutex;
	inline static std::mutex m_threadIDToStrMutex; // Dedicated mutex for ThreadManager::ThreadIDToString, as other methods may acquire m_threadMutex and call that method (which cannot acquire m_threadMutex again)
	inline static std::unordered_map<std::thread::id, std::string> m_threadIDToName;
};