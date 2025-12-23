/* EventDispatcher.hpp - Handles event buses.
*/

#pragma once


#include <bit>
#include <mutex>
#include <queue>
#include <bitset>
#include <vector>
#include <sstream>
#include <typeindex>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>

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
	using EventHandler = std::function<void(const EventType &)>;

	using EventIndex = std::type_index;
	using SubscriberIndex = std::type_index;


	/* Registers for the ability to subscribe to events.
		@tparam Subscriber: The subscriber type.

		@return A subscriber index.
	*/
	template<typename Subscriber>
	inline SubscriberIndex registerSubscriber() {
		std::lock_guard<std::mutex> lock(m_subscribersMutex);

		SubscriberIndex subscriberTypeIndex = std::type_index(typeid(Subscriber));

		if (m_subscribers.count(subscriberTypeIndex)) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Received request to register subscriber " + enquote(subscriberTypeIndex.name()) + ", but it already exists! The existing subscriber index will be returned.");

			return subscriberTypeIndex;
		}

		m_subscribers.insert(subscriberTypeIndex);

		m_eventsSubscribedTo[subscriberTypeIndex] = EventMask{};

		return subscriberTypeIndex;
	}


	/* Gets the index of a subscriber.
		@tparam Subscriber: The subscriber type.

		@return The corresponding subscriber index.
	*/
	template<typename Subscriber>
	inline SubscriberIndex getSubscriberIndex() {
		std::lock_guard<std::mutex> lock(m_subscribersMutex);

		SubscriberIndex subscriberTypeIndex = std::type_index(typeid(Subscriber));
		auto it = m_subscribers.find(subscriberTypeIndex);
		LOG_ASSERT(it != m_subscribers.end(), "Cannot get index of subscriber " + enquote(subscriberTypeIndex.name()) + ": Subscriber is not registered!");

		return it->second;
	}


	/* Subscribes to an event type.
		@tparam EventType: The event type.
		@param handler: The event handler function.
	*/
	template<typename EventType>
	inline void subscribe(SubscriberIndex subscriberTypeIndex, EventHandler<EventType> handler) {
		std::lock_guard<std::mutex> lock(m_eventsMutex); // Protect m_events

		EventIndex eventTypeIndex = EventIndex(typeid(EventType));
		{
			LOG_ASSERT(m_subscribers.count(subscriberTypeIndex), "Subscription by subscriber " + enquote(subscriberTypeIndex.name()) + " to event " + enquote(eventTypeIndex.name()) + " has been denied: Subscriber is not registered!");
		}
		auto &events = m_events[eventTypeIndex];

		// Type-erase event callback function
		HandlerCallback callback = [handler](const void *event) {
			handler(*static_cast<const EventType *>(event));
		};

		// Register subscriber to event
		events.push_back(Callback{
			.callback = callback,
			.callbackOrigin = subscriberTypeIndex
		});
	}


	/* Resets the registry that keeps track of all event callbacks that have been invoked. */
	inline void resetEventCallbackRegistry() {
		size_t counter = 0;
		for (auto &[_, eventMask] : m_eventsSubscribedTo)
			eventMask.reset(), counter++;

		Log::Print(Log::T_INFO, __FUNCTION__, "Cleared event callback registry: " + TO_STR(counter) + " " + PLURAL(counter, "event", "events") + " " + PLURAL(counter, "has", "have") + " been reset.");
	}


	/* Checks if a subscriber has subscribed to a variable number of event types.
		@param subscriberTypeIndex: The subscriber's type index.
		@param eventFlags: The event flags to check.

		@return True if the subscriber has subscribed to the event(s), False otherwise.
	*/
	inline bool eventCallbacksInvoked(SubscriberIndex subscriberTypeIndex, EventFlags eventFlags) {
		auto it = m_eventsSubscribedTo.find(subscriberTypeIndex);

		LOG_ASSERT(it != m_eventsSubscribedTo.end(), "Cannot find event callbacks for subscriber " + enquote(subscriberTypeIndex.name()) + ": Subscriber is not registered!");

		// Construct temporary bitset with the event flags and compare it to the subscriber's current bitset
		EventMask eventMask(static_cast<unsigned long long>(eventFlags));
		EventMask result = eventMask & it->second;
		return result == eventMask;
	}


	/* Freezes the current thread to wait until the specified event callbacks have been invoked.
		@param subscriberTypeIndex: The subscriber's type index.
		@param eventFlags: The event flags to check.
	*/
	inline void waitForEventCallbacks(SubscriberIndex subscriberTypeIndex, EventFlags eventFlags) {
		std::unique_lock<std::mutex> lock(m_eventCallbacksWaitMutex);
		m_eventCallbacksWaitCondVar.wait(lock, [this, subscriberTypeIndex, eventFlags]() { return eventCallbacksInvoked(subscriberTypeIndex, eventFlags); });
	}


	/* Dispatches an event.
		@tparam EventType: The event type.
		@param event: The event to be dispatched.
		@param suppressLogs (Default: False): Whether to suppress any output logs (True), or not (False).
		@param noWorkerEventQueue (Default: False): Whether to defer the event to the main thread if this function is called from a worker thread (True), or prevent queueing and dispatch event directly (False).
	*/
	template<typename EventType>
	inline void dispatch(const EventType &event, bool suppressLogs = false, bool noWorkerEventQueue = false) {
		EventIndex eventTypeIndex = EventIndex(typeid(EventType));
		std::thread::id mainThreadID = ThreadManager::GetMainThreadID();

		EventFlag eventFlag = event.eventFlag;

		if (std::this_thread::get_id() == mainThreadID || mainThreadID == std::thread::id() || noWorkerEventQueue) {
			// if (the current thread is the main thread || no main thread is set || override event queueing if in worker thread),
			// dispatch the event directly.
			internalDispatch(eventTypeIndex, eventFlag, &event, suppressLogs);
			m_eventCallbacksWaitCondVar.notify_one();
		}

		else {
			// If there is a main thread and the current thread is not it, this must be a worker thread.
			// In this case, the event will be queued for processing in the main thread later.
			std::thread::id threadID = std::this_thread::get_id();
			if (!suppressLogs)
				Log::Print(Log::T_VERBOSE, __FUNCTION__, "Queueing event " + enquote(typeid(EventType).name()) + " dispatched in Worker Thread " + ThreadManager::ThreadIDToString(threadID) + " (" + ThreadManager::GetThreadNameFromID(threadID) + ")...");


			std::lock_guard<std::mutex> lock(m_eventQueueMutex);

			LOG_ASSERT(m_eventQueue.size() <= MAX_QUEUED_EVENTS,
				"Queued worker event count exceeded safe thresholds! Last worker event was dispatched from Worker Thread " + ThreadManager::ThreadIDToString(threadID) + " (" + ThreadManager::GetThreadNameFromID(threadID) + ").");

			m_eventQueue.push(QueuedEvent{
				.type = eventTypeIndex,
				.callback = [this, eventTypeIndex, eventFlag, event, suppressLogs](const void * /* Must be void(const void*) (a.k.a. HandlerCallback) instead of void(void) for consistency */) {

					/* Explanation for eventCopy:
						In this case, event is a local variable on a worker thread's stack.
						If we directly pass it in internalDispatch by reference, by the time it gets executed on the main thread, it will have been destroyed/invalidated.
						Therefore, we need to copy it by value. eventCopy does that explicitly, although capture by value in the lambda is enough.
					*/
					EventType eventCopy = event;

					this->internalDispatch(eventTypeIndex, eventFlag, &eventCopy, suppressLogs);
				}
			});

			m_eventQueueCondition.notify_one();
		}
	}


	/* Processes all events dispatched from worker threads. */
	inline void pollQueuedEvents() {
		if (std::this_thread::get_id() != ThreadManager::GetMainThreadID())
			return;

		std::unique_lock<std::mutex> lock(m_eventQueueMutex);

		m_eventQueueCondition.wait_for(lock, std::chrono::milliseconds(10));

		//if (!m_eventQueue.empty())
		//	Log::Print(Log::T_VERBOSE, __FUNCTION__, "Processing " + TO_STR(m_eventQueue.size()) + " queued " + PLURAL(m_eventQueue.size(), "event", "events") + "...");

		while (!m_eventQueue.empty()) {
			m_eventQueue.front().callback(nullptr);
			m_eventQueue.pop();
		}
	}
	
private:
	// Maximum number of queued events. In case the main thread goes unresponsive and thus is unable to process queued events, we must prevent the event queue from growing to enormous sizes and consuming enormous memory.
	// Workers are assumed to also halt execution when the main thread is unresponsive. Thus, exceeding this max-events threshold could indicate a bad memory leak.
	const size_t MAX_QUEUED_EVENTS = 1000;

	// Subscriber and event data
	using HandlerCallback = std::function<void(const void*)>;		// Type-erased callback version of EventHandler
	using EventMask = std::bitset<EVENT_FLAG_COUNT>;

	struct Callback {
		HandlerCallback callback;
		SubscriberIndex callbackOrigin;
	};
	
	std::unordered_set<SubscriberIndex> m_subscribers;
	std::unordered_map<SubscriberIndex, EventMask> m_eventsSubscribedTo;
	std::mutex m_subscribersMutex;

	std::unordered_map<EventIndex, std::vector<Callback>> m_events;
	std::mutex m_eventsMutex;


	// Queued events in worker threads
	struct QueuedEvent {
		EventIndex type;
		HandlerCallback callback;
	};

	std::queue<QueuedEvent> m_eventQueue;
	std::mutex m_eventQueueMutex;
	std::condition_variable m_eventQueueCondition;	// Condition variable to signal when new (deferred) events are available.

	std::mutex m_eventCallbacksWaitMutex;
	std::condition_variable m_eventCallbacksWaitCondVar;


	/* Internal event dispatching logic.
		This gets callled conditionally in the public dispatch() function depending on which thread is the main thread.
	*/
	inline void internalDispatch(EventIndex eventTypeIndex, EventFlag eventFlag, const void *event, bool suppressLogs = false) {
		// Acquire lock for accessing m_events to find and copy callbacks
		std::vector<Callback> callbacks;
		{
			std::lock_guard<std::mutex> lock(m_eventsMutex); // Protect m_events

			auto it = m_events.find(eventTypeIndex);
			if (it == m_events.end()) {
				if (!suppressLogs)
					Log::Print(Log::T_WARNING, __FUNCTION__, "There are no subscribers to event " + enquote(eventTypeIndex.name()) + "!");

				return;
			}

			callbacks = it->second; // Make a copy of the handlers to release the lock quickly
		} // NOTE: Mutex is automatically released on scope exit with RAII wrappers like std::lock_guard or std::unique_lock


		if (!suppressLogs && !callbacks.empty())
			Log::Print(Log::T_VERBOSE, __FUNCTION__, "Invoking " + TO_STR(callbacks.size()) + " " + ((callbacks.size() == 1) ? "callback" : "callbacks") + " for event type " + enquote(eventTypeIndex.name()) + "...");


		for (auto &callback : callbacks) {
			if (!suppressLogs)
				Log::Print(Log::T_VERBOSE, __FUNCTION__, "[Subscriber " + enquote(callback.callbackOrigin.name()) + ", event " + enquote(eventTypeIndex.name()) + "] Firing callback...");

			callback.callback(event);
			m_eventsSubscribedTo[callback.callbackOrigin].set(ctz(eventFlag));
		}
	}


	/* Count trailing zeroes (used to convert `1 << x` to `x`) */
	inline unsigned int ctz(EventFlag eventFlag) {
		return std::countr_zero(static_cast<unsigned int>(eventFlag));
	}
};
