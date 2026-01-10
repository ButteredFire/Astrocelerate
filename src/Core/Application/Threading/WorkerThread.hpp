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
		m_thread = std::jthread([this](std::stop_token stopToken) {
			threadLoop(stopToken);
		});
	}
	
	~WorkerThread() = default; // NOTE: std::jthread automatically joins the thread on destruction (instead of std::thread, which calls std::terminate if the thread has not been joined)


	/* Defines the worker thread with the given callable function/lambda.
		@tparam Callable: The callable type (e.g., function pointer, lambda, functor). The callable must accept a std::stop_token.

		@param func: The callable function/lambda to execute in the worker thread.
	*/
	template<typename Callable>
	requires std::invocable<Callable, std::stop_token>
	inline void set(Callable &&func) {  // Callable&& is a forwarding reference, used for efficiency (see perfect forwarding)
		// If the previous thread is still running, wait for it to stop
		requestStop();
		waitForStop();

		std::lock_guard<std::mutex> lock(m_mutex);
		m_work = std::forward<Callable>(func);
		m_workStopSource = std::stop_source();

		m_active.store(false);
		m_workAssigned.store(true);
	}
	

	/* Starts the worker thread.
		@param detached: Should the thread be detached?
	*/
	inline void start(bool detached = false) {
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_workAssigned.load())
				return;

			m_active.store(true);
			m_stopRequested.store(false);
		}
		m_cv.notify_one();

		if (detached) {
			m_thread.detach();
			m_detached.store(true);
		}
	}


	/* Waits for the worker thread to stop execution.
		@param condVars (optional): Condition variables to be notified for the thread to wake up and finish its work
	*/
	template<typename... CVs>
		requires ((std::same_as<std::condition_variable *, CVs> ||
					std::same_as<std::condition_variable_any *, CVs>) && ...)
	inline void waitForStop(CVs... condVars) {
		g_appCtx.MainThread.haltCV.notify_all();	// Wakes up sleeping threads due to main thread halts
		((condVars->notify_all()), ...);				// Wakes up any other external CVs the work might be blocked on

		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return !m_active.load(); });
	}

	
	/* Requests the thread to terminate execution. */
	inline void requestStop() {
		m_workStopSource.request_stop();
		m_stopRequested.store(true);
	}

    inline bool stopRequested()          { return m_stopRequested.load(); }
    inline bool isRunning()              { return m_active.load(); }
    inline bool isDetached() const       { return m_detached.load(); }

	inline void setName(const std::string &threadName) { m_threadName = threadName; }

    inline std::string getName() const	 { return m_threadName; }
    inline std::thread::id getID() const { return m_thread.get_id(); }

private:
	std::jthread m_thread;
	std::mutex m_mutex;
	std::condition_variable_any m_cv; // Use std::condition_variable_any as it is required to work with std::stop_token

	std::function<void(std::stop_token)> m_work;
	std::stop_source m_workStopSource;

	std::string m_threadName = "Worker";

	std::atomic<bool> m_active{ false };
	std::atomic<bool> m_workAssigned{ false };
	std::atomic<bool> m_stopRequested{ false };
	std::atomic<bool> m_detached{ false };
	

	void threadLoop(std::stop_token stopToken) {
		// stopToken.stop_requested() will only be true if the associated thread has been destroyed.
		// Therefore, to implement a pause/resume mechanism, we must use std::stop_source.
		while (!stopToken.stop_requested()) {
			std::function<void(std::stop_token)> work;
			std::stop_token workStopToken;
			{
				std::unique_lock<std::mutex> lock(m_mutex);

				// NOTE: The thread automatically wakes up when the predicate is true OR `stopToken.request_stop` is called.
				// In the latter case, the CV will inherently check `stopToken.stop_requested`, so there's no need to include that condition in the predicate.
				m_cv.wait(lock, stopToken, [this] { return m_active.load() && m_work; });

				if (stopToken.stop_requested())
					return;
				
				work = std::move(m_work);
				workStopToken = m_workStopSource.get_token();
			}

			if (work)
				work(workStopToken);

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_active.store(false);
			}
			m_cv.notify_all();
		}
	}
};