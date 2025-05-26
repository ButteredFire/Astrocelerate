/* EventDispatcher.hpp - Handles event buses.
*/

#pragma once


#include <unordered_map>
#include <functional>
#include <typeindex>
#include <vector>

#include <Core/LoggingManager.hpp>
#include <Core/EventTypes.hpp>


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

		if (m_subscribers.find(eventTypeIndex) == m_subscribers.end()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to find event of type " + enquote(typeid(EventType).name()) + ": Event does not exist!");
		}

		std::vector<HandlerCallback> callbacks = m_subscribers[eventTypeIndex];

		if (callbacks.size() == 0) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "There are no subscribers to the event of type " + enquote(typeid(EventType).name()) + "!");
		}

		for (auto& callback : callbacks) {
			callback(&event);
		}

		if (!suppressLogs)
			Log::Print(Log::T_SUCCESS, __FUNCTION__, "	Invoked " + std::to_string(callbacks.size()) + " callback(s) for event type " + enquote(typeid(EventType).name()) + ".");
	}

	
private:
	using HandlerCallback = std::function<void(const void*)>;
	std::unordered_map<std::type_index, std::vector<HandlerCallback>> m_subscribers;
};
