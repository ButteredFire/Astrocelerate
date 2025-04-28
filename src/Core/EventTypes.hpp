/* EventTypes.hpp - Defines event types for the event bus.
*/

#pragma once


namespace EventTypes {
	enum Identifier {
		EVENT_ID_SWAPCHAIN_RECREATION
	};


	/* Swapchain recreation event. */
	struct SwapchainRecreationEvent {
		const Identifier eventType = Identifier::EVENT_ID_SWAPCHAIN_RECREATION;
	};
}