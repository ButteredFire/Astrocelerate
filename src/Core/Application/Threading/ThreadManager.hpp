/* ThreadManager.hpp - Manages threads and multithreading.
*/

#pragma once

#include <mutex>
#include <thread>
#include <sstream>
#include <utility>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>


#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Threading/WorkerThread.hpp>


class ThreadManager {
public:
	friend WorkerThread;

	/* Gets the ID of the main thread.
		@return The ID of the main thread.
	*/
	inline static std::thread::id GetMainThreadID() { return m_mainThreadID; }
	inline static std::string GetMainThreadIDAsString() {
		return ThreadIDToString(m_mainThreadID);
	}


	/* Puts THIS thread to sleep if the main thread is currently unresponsive.
		NOTE: This method is reserved for worker threads.
		@param workerThread: The worker thread calling this function.
	*/
	inline static void SleepIfMainThreadHalted(WorkerThread *workerThread) {
		LOG_ASSERT(std::this_thread::get_id() != m_mainThreadID, "Programmer Error: Cannot call ThreadManager::SleepIfMainThreadHalted in the main thread!");

		std::unique_lock<std::mutex> lock(g_appCtx.MainThread.haltMutex);

		g_appCtx.MainThread.haltCV.wait(lock, [workerThread]() {
			return !g_appCtx.MainThread.isHalted.load() || workerThread->stopRequested();
		});
	}


	/* Gets the name of a thread created via ThreadManager::CreateThread.
		NOTE: If threadID is the main thread ID, an empty string will be returned.

		@param threadID: The thread's ID.

		@return The thread name.
	*/
	inline static std::string GetThreadNameFromID(std::thread::id threadID) {
		std::lock_guard<std::mutex> lock(m_threadMutex);

		if (threadID == m_mainThreadID)
			return "";

		auto it = m_workerThreadMap.find(threadID);
		if (it == m_workerThreadMap.end())
			return "WORKER";

		return it->second->getName();
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
		@param threadName: The thread's name (used for logging purposes).

		@return The new thread.
	*/
	inline static std::shared_ptr<WorkerThread> CreateThread(const std::string &threadName) {
		std::lock_guard<std::mutex> lock(m_threadMutex);

		std::shared_ptr<WorkerThread> workerThread = std::make_shared<WorkerThread>();
		workerThread->setName(threadName);
		

		// If there is already an existing thread with the same name, it is assumed that this new thread represents the latest version of the existing thread. Therefore, we must erase the existing thread from the map.
		auto it = m_uniqueWorkerNames.find(threadName);
		if (it == m_uniqueWorkerNames.end()) {
			m_uniqueWorkerNames.insert(threadName);
		}
		else {
			std::thread::id duplicateWorker;
			for (const auto &[id, worker] : m_workerThreadMap)
				if (worker->getName() == threadName) {
					duplicateWorker = id;
					break;
				}

			m_workerThreadMap.erase(duplicateWorker);
		}

		m_workerThreadMap[workerThread->getID()] = workerThread;


		return workerThread;
	}


	/* Gets the number of currently active threads (including the main thread). */
	inline static uint32_t GetThreadCount() {
		return m_workerThreadMap.size() + 1; // +1 to account for main thread
	}

	inline static auto& GetThreadMap() { return m_workerThreadMap; }

private:
	inline static std::thread::id m_mainThreadID;
	inline static std::mutex m_threadMutex;
	inline static std::mutex m_threadIDToStrMutex; // Dedicated mutex for ThreadManager::ThreadIDToString, as other methods may acquire m_threadMutex and call that method (which cannot acquire m_threadMutex again)


	inline static std::unordered_map<std::thread::id, std::shared_ptr<WorkerThread>> m_workerThreadMap;
	inline static std::unordered_set<std::string> m_uniqueWorkerNames;


	/* Updates the thread map's key ID value of a worker thread. */
	inline static void UpdateThreadID(std::thread::id oldID, std::thread::id newID) {
		std::thread::id emptyID;
		if (oldID == emptyID)
			return;

		m_workerThreadMap[newID] = m_workerThreadMap.at(oldID);
		m_workerThreadMap.erase(oldID);
	}
};