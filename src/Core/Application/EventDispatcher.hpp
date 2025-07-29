/* EventDispatcher.hpp - Handles event buses.
*/

#pragma once


#include <mutex>
#include <queue>
#include <vector>
#include <sstream>
#include <typeindex>
#include <functional>
#include <unordered_map>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/EventTypes.hpp>

#include <Engine/Threading/ThreadManager.hpp>


class EventDispatcher {
public:
	EventDispatcher() {
		Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	}
	~EventDispatcher() = default;

	template<typename EventType>
	using EventHandler = std::function<void(const EventType&)>;
	

	/* Subscribes to an event type.
		@tparam EventType: The event type.
		@param handler: The event handler function.
	*/
	template<typename EventType>
	inline void subscribe(EventHandler<EventType> handler) {
		std::lock_guard<std::mutex> lock(m_subscribersMutex); // Protect m_subscribers

		std::type_index eventTypeIndex = std::type_index(typeid(EventType));
		auto& subscribers = m_subscribers[eventTypeIndex];
		
		HandlerCallback callback = [handler](const void* event) {
			handler(*static_cast<const EventType*>(event));
		};
		
		subscribers.push_back(callback);
	}


	/* Dispatches an event.
		@tparam EventType: The event type.
		@param event: The event to be dispatched.
		@param suppressLogs: Whether to suppress any output logs (True), or not (False).
	*/
	template<typename EventType>
	inline void dispatch(const EventType& event, bool suppressLogs = false) {
		std::type_index eventTypeIndex = std::type_index(typeid(EventType));
		std::thread::id mainThreadID = ThreadManager::GetMainThreadID();

		if (std::this_thread::get_id() == mainThreadID || mainThreadID == std::thread::id()) {
			// if (the current thread is the main thread || no main thread is set), dispatch the event directly.
			internalDispatch(eventTypeIndex, &event, suppressLogs);
		}
		else {
			// If there is a main thread and the current thread is not it, this must be a worker thread.
			// In this case, the event will be queued for processing in the main thread later.
			Log::Print(Log::T_INFO, __FUNCTION__, "Queueing event " + enquote(typeid(EventType).name()) + " dispatched in Worker Thread " + ThreadManager::ThreadIDToString(std::this_thread::get_id()) + "...");
			
			std::lock_guard<std::mutex> lock(m_eventQueueMutex);
			m_eventQueue.push(QueuedEvent{
				.type = eventTypeIndex,
				.callback = [this, eventTypeIndex, event, suppressLogs](const void* /* Must be void(const void*) (a.k.a. HandlerCallback) instead of void(void) for consistency */) {

					/* Explanation for eventCopy:
						In this case, event is a local variable on a worker thread's stack.
						If we directly pass it in internalDispatch by reference, by the time it gets executed on the main thread, it will have been destroyed.
						Therefore, we need to copy it by value. eventCopy does that explicitly, although capture by value in the lambda is enough.
					*/
					EventType eventCopy = event;

					this->internalDispatch(eventTypeIndex, &eventCopy, suppressLogs);
				}
			});
		}
	}


	/* Processes all events dispatched from worker threads. */
	inline void processQueuedEvents() {
		if (std::this_thread::get_id() != ThreadManager::GetMainThreadID())
			return;

		std::vector<QueuedEvent> pendingEvents;
		{
			std::lock_guard<std::mutex> lock(m_eventQueueMutex);

			while (!m_eventQueue.empty()) {
				pendingEvents.push_back(m_eventQueue.front());
				m_eventQueue.pop();
			}
		}

		if (!pendingEvents.empty())
			Log::Print(Log::T_INFO, __FUNCTION__, "Processing " + TO_STR(pendingEvents.size()) + " queued event" + PLURAL(pendingEvents.size(), "s") + "...");

		for (auto &event : pendingEvents)
			event.callback(nullptr);
	}
	
private:
	using HandlerCallback = std::function<void(const void*)>;
	std::unordered_map<std::type_index, std::vector<HandlerCallback>> m_subscribers;
	std::mutex m_subscribersMutex;

	// Queued events in worker threads
	struct QueuedEvent {
		std::type_index type;
		HandlerCallback callback;
	};

	std::queue<QueuedEvent> m_eventQueue;
	std::mutex m_eventQueueMutex;


	/* Internal event dispatching logic.
		This gets callled conditionally in the public dispatch() function depending on which thread is the main thread.
	*/
	inline void internalDispatch(std::type_index eventTypeIndex, const void *event, bool suppressLogs = false) {
		// Acquire lock for accessing m_subscribers to find and copy callbacks
		std::vector<HandlerCallback> callbacks;
		{
			std::lock_guard<std::mutex> lock(m_subscribersMutex); // Protect m_subscribers

			auto it = m_subscribers.find(eventTypeIndex);
			if (it == m_subscribers.end()) {
				if (!suppressLogs) {
					Log::Print(Log::T_WARNING, __FUNCTION__, "There are no subscribers to event " + enquote(eventTypeIndex.name()) + "!");
				}
				return;
			}

			callbacks = it->second; // Make a copy of the handlers to release the lock quickly
		} // Mutex is automatically released on scope exit with RAII wrappers like std::lock_guard or std::unique_lock


		if (!suppressLogs && !callbacks.empty())
			Log::Print(Log::T_INFO, __FUNCTION__, "Invoking " + TO_STR(callbacks.size()) + " " + ((callbacks.size() == 1) ? "callback" : "callbacks") + " for event type " + enquote(eventTypeIndex.name()) + "...");

		for (auto &callback : callbacks)
			callback(event);
	}
};
