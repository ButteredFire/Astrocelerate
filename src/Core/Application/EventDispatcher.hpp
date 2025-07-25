/* EventDispatcher.hpp - Handles event buses.
*/

#pragma once


#include <unordered_map>
#include <functional>
#include <typeindex>
#include <vector>
#include <mutex>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Data/EventTypes.hpp>


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
		std::lock_guard<std::mutex> lock(m_mutex); // Protect m_subscribers

		std::type_index eventTypeIndex = std::type_index(typeid(EventType));
		auto& subscribers = m_subscribers[eventTypeIndex];
		
		HandlerCallback callback = [handler](const void* event) {
			handler(*static_cast<const EventType*>(event));
		};
		
		subscribers.push_back(callback);
	}


	/* Publishes an event.
		@tparam EventType: The event type.
		@param event: The event to be published.
		@param suppressLogs: Whether to suppress any output logs (True), or not (False).
	*/
	template<typename EventType>
	inline void publish(const EventType& event, bool suppressLogs = false) {
		std::type_index eventTypeIndex = std::type_index(typeid(EventType));


		// Acquire lock for accessing m_subscribers to find and copy callbacks
		std::vector<HandlerCallback> callbacks;
		{
			std::lock_guard<std::mutex> lock(m_mutex); // Protect m_subscribers

			auto it = m_subscribers.find(eventTypeIndex);
			if (it == m_subscribers.end()) {
				if (!suppressLogs) {
					Log::Print(Log::T_WARNING, __FUNCTION__, "There are no subscribers to event " + enquote(typeid(EventType).name()) + "!");
				}

				return;
			}

			callbacks = it->second; // Make a copy of the handlers to release the lock quickly
		} // Mutex is automatically released here


		if (!suppressLogs && !callbacks.empty())
			Log::Print(Log::T_INFO, __FUNCTION__, "Invoking " + std::to_string(callbacks.size()) + " " + ((callbacks.size() == 1) ? "callback" : "callbacks") + " for event type " + enquote(typeid(EventType).name()) + "...");

		for (auto& callback : callbacks) {
			callback(&event);
		}
	}

	
private:
	using HandlerCallback = std::function<void(const void*)>;
	std::unordered_map<std::type_index, std::vector<HandlerCallback>> m_subscribers;
	std::mutex m_mutex;
};
