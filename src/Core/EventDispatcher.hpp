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
		Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
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
	*/
	template<typename EventType>
	inline void publish(const EventType& event) {
		std::type_index eventTypeIndex = std::type_index(typeid(EventType));

		if (m_subscribers.find(eventTypeIndex) == m_subscribers.end()) {
			Log::print(Log::T_WARNING, __FUNCTION__, "Failed to find event type: Event type does not exist!");
			return;
		}

		std::vector<HandlerCallback> callbacks = m_subscribers[eventTypeIndex];

		if (callbacks.size() == 0) {
			Log::print(Log::T_WARNING, __FUNCTION__, "There are no subscribers to this event!");
			return;
		}

		for (auto& callback : callbacks) {
			callback(&event);
		}

		Log::print(Log::T_SUCCESS, __FUNCTION__, "	Invoked " + std::to_string(callbacks.size()) + " callback(s) for event type " + enquote(typeid(EventType).name()) + ".");
	}

	
private:
	using HandlerCallback = std::function<void(const void*)>;
	std::unordered_map<std::type_index, std::vector<HandlerCallback>> m_subscribers;
};
