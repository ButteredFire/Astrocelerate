/* WorkerThread - Defines a thread wrapper with basic mechanisms.
*/

#pragma once

#include <thread>
#include <atomic>
#include <future>
#include <utility>
#include <optional>

#include <Core/Application/LoggingManager.hpp>
#include <Engine/Threading/ThreadManager.hpp>


class WorkerThread {
public:
	WorkerThread() { set([]() {}); }
	~WorkerThread() {
		if (m_detached.load())
			return;

		requestStop();
		waitForStop();
	}


	/* Defines the worker thread with the given callable function/lambda.
		@tparam Callable: The callable type (e.g., function pointer, lambda, functor).

		@param func: The callable function/lambda to execute in the worker thread.
	*/
	template<typename Callable>
	inline void set(Callable &&func) {  // Callable&& is a forwarding reference, used for efficiency (see perfect forwarding)
		// If the previous thread is still running, wait for it to stop
		requestStop();
		waitForStop();
		m_stopRequested.store(false);

		// Define new thread
		std::promise<void> promise;
		std::future<void> future = promise.get_future();
		m_thread = std::thread(
			[this, startFuture = std::move(future), callback = std::forward<Callable>(func)]() mutable {
				// Freeze this thread until it is allowed to continue execution
					// std::future<void>::get() attempts to get a value of type "void" from the associated std::promise<void>. This effectively freezes the thread until the promise's value is set.
					// (We use "void" because we just need the "side effect" (thread freezing) and not the actual value.)
				startFuture.get();


				// Run callback function
				m_running.store(true);
					callback();
				m_running.store(false);
			}
		);

		m_startPromise.emplace(std::move(promise));
		m_created.store(true);

		if (m_threadID.load() != m_thread.get_id())
			ThreadManager::UpdateThreadID(m_threadID.load(), m_thread.get_id());

		m_threadID.store(m_thread.get_id());
	}
	

	/* Starts the worker thread. */
	inline void start(bool detached = false) {
		LOG_ASSERT(m_created.load(), "Cannot start worker thread: Thread callable has not been set!");

		// Check if thread has already been started
		if (m_running.load() || !m_startPromise.has_value()) // If the promise doesn't have a value, it has already been used in a Future-Promise pair and is now invalid. This means the thread has already started doing work.
			return;

		// Resume worker thread execution (defined in `WorkerThread::set`)
		m_startPromise->set_value(); //NOTE: set_value can only be called once for any promise in a Future-Promise pair, since using it invalidates the promise afterwards (promises are single-use)
			// Clear the optional to signify the promise has been used
		m_startPromise.reset();


		if (detached) {
			m_thread.detach();
			m_detached.store(true);
		}
	}


	/* Waits for the worker thread to stop execution. */
	inline void waitForStop() {
		//LOG_ASSERT(m_created.load(), "Cannot wait for worker thread: Thread callable has not been set!");

		// Check if thread's execution is waiting to be resumed
		if (m_startPromise.has_value()) {
			// If the thread was created (promise exists) but never started (promise has not been invalidated), the promise still holds the key
			m_startPromise->set_value();
			m_startPromise.reset();
		}


		// Check if thread (after execution has been resumed) is still running the callable
		if (m_thread.joinable())
			m_thread.join();
	}

	
	inline void requestStop()			{ checkDetached(); m_stopRequested.store(true); }
	inline bool stopRequested()			{ checkDetached(); return m_stopRequested.load(); }
	inline bool isRunning()				{ checkDetached(); return m_running.load(); }
	inline bool isDetached() const		{ return m_detached.load(); }

	inline void setName(const std::string &threadName) { m_threadName = threadName; }
	inline std::string getName() const { return m_threadName; }

	inline std::thread::id getID() const{
		LOG_ASSERT(m_created.load(), "Cannot get worker thread ID: Thread callable has not been set!");
		return m_thread.get_id();
	}

private:
	std::thread m_thread;
	std::string m_threadName = "Worker";
	std::atomic<std::thread::id> m_threadID;
	std::atomic<bool> m_created{ false };
	std::atomic<bool> m_stopRequested{ false };
	std::atomic<bool> m_running{ false };
	std::atomic<bool> m_detached{ false };

	// Problem: We want to get a thread's ID before it is running. However, when a thread is created, it automatically runs.
	// Solution: Within the created thread, we must make it wait for us to get its ID, process it (e.g., store the ID), and only then allow it to run. To do this, we'll use the Future-Promise mechanism.
	std::optional<std::promise<void>> m_startPromise;


	/* NOTE: For atomic variables, why use `store` and `load` instead of just assignment (=) and reading directly from the atomic respectively?
		1) `store` and `load` allow for finer-grained controls (e.g., change memory ordering),
		2) In this scenario specifically, using these methods means explicitly signalling that the variable with those methods IS an atomic variable, and also signalling thread-safe operations.
	*/


	inline void checkDetached() {
		LOG_ASSERT(!m_detached.load(), "Cannot execute this function after this worker thread has been detached!");
	}
};