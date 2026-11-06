/* WorkerThread - Defines a thread wrapper with basic mechanisms.
*/

#pragma once

#include <thread>
#include <atomic>
#include <future>
#include <thread>
#include <utility>
#include <concepts>
#include <optional>


#include <Core/Data/Contexts/AppContext.hpp>
#include <Core/Application/IO/LoggingManager.hpp>


class WorkerThread {
public:
	WorkerThread() {
		m_thread = std::jthread(
			[this](std::stop_token stopToken) {
				threadLoop(stopToken);
			}
		);
	}
	
	~WorkerThread() {
		stop();
	}
	

	// Wrapper is non-copyable and non-movable to avoid stale `this` captures
	WorkerThread(const WorkerThread&) = delete;
	WorkerThread& operator=(const WorkerThread&) = delete;
	WorkerThread(WorkerThread&&) = delete;
	WorkerThread& operator=(WorkerThread&&) = delete;


	/* Defines the worker thread with the given callable function/lambda.
		@tparam Callable:	The callable type (e.g., function pointer, lambda, functor).
							The callable must accept a std::stop_token.
		@param func:		The callable function/lambda to execute in the worker thread.
	*/
	template<typename Callable>
	requires std::invocable<Callable, std::stop_token>
	void set(Callable &&func) {
		// If the previous thread is still running, wait for it to stop
		requestStop();
		waitForStop();

		std::lock_guard<std::mutex> lock(m_mutex);
		m_work = std::forward<Callable>(func);
		m_workStopSource = std::stop_source();
		m_workAssigned.store(true);
	}
	

	/* Starts the worker thread. */
	void start() {
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_workAssigned.load())
				return;

			m_active.store(true);
			m_stopRequested.store(false);
		}
		m_cv.notify_one();
	}


	/* Waits for the worker thread to stop execution.
		@param condVars (optional): Condition variables to be notified for the thread to wake up and finish its work
	*/
	template<typename... CVs>
		requires ((std::same_as<std::condition_variable *, CVs> ||
					std::same_as<std::condition_variable_any *, CVs>) && ...)
	void waitForStop(CVs... condVars) {
		g_appCtx.MainThread.haltCV.notify_all();	// Wakes up sleeping threads due to main thread halts
		((condVars->notify_all()), ...);			// Wakes up any other external CVs the work might be blocked on

		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return !m_active.load(); });
	}

	
	/* Requests the thread to terminate execution. */
	void requestStop() {
		m_workStopSource.request_stop();
		m_stopRequested.store(true);
	}
	
	
	/* Force-stops and terminates the thread. */
	void stop() {
		m_workStopSource.request_stop();
		m_thread.request_stop();
		m_cv.notify_all();

		if (m_thread.joinable()) {
			if (std::this_thread::get_id() != m_thread.get_id()) {
				m_thread.join();
			}
		}
	}


    bool stopRequested()          { return m_stopRequested.load(); }
    bool isRunning()              { return m_active.load(); }

	void setName(std::string threadName) { m_threadName = std::move(threadName); }

    std::string getName() const		{ return m_threadName; }
    std::thread::id getID() const	{ return m_thread.get_id(); }

private:
	std::mutex m_mutex;
	std::condition_variable_any m_cv; // Use std::condition_variable_any as it is required to work with std::stop_token

	std::function<void(std::stop_token)> m_work;
	std::stop_source m_workStopSource;

	std::string m_threadName = "Worker";

	std::atomic<bool> m_active{ false };
	std::atomic<bool> m_workAssigned{ false };
	std::atomic<bool> m_stopRequested{ false };
	
	// Declare jthread last: in C++, construction and destruction follow a LIFO approach.
	// Member declaration order guarantees m_thread is destroyed (joined) before m_mutex and m_cv are destroyed.
	std::jthread m_thread;

	void threadLoop(std::stop_token stopToken) {
		// stopToken.stop_requested() will only be true if the associated thread has been destroyed.
		// Therefore, to implement a pause/resume mechanism, we must use std::stop_source.
		while (!stopToken.stop_requested()) {
			std::function<void(std::stop_token)> currentWork;
			std::stop_token currentStopToken;

			{
				std::unique_lock<std::mutex> lock(m_mutex);

				m_cv.wait(lock, stopToken, [this, &stopToken] {
					return stopToken.stop_requested() || (m_active.load() && m_work);
					});

				if (stopToken.stop_requested())
					return;

				currentWork = std::move(m_work);
				currentStopToken = m_workStopSource.get_token();
			}

			if (currentWork) {
				currentWork(currentStopToken);
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_active.store(false);
				m_workAssigned.store(false);
			}
			m_cv.notify_all();
		}
	}
};