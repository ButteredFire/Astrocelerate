/* EventTypes.hpp - Defines event types for the event dispatcher.
*/

#pragma once


/* Event types for the event dispatcher. */
namespace Event {
	enum Identifier {
		EVENT_ID_SWAPCHAIN_RECREATION,
		EVENT_ID_INIT_FRAMEBUFFERS,
		EVENT_ID_UPDATE_RIGID_BODIES
	};


	/* Used when the swapchain is recreated. */
	struct SwapchainRecreation {
		const Identifier eventType = Identifier::EVENT_ID_SWAPCHAIN_RECREATION;
	};


	/* Used when data required to initialize framebuffers is available (usually after graphics pipeline intiialization). */
	struct InitFrameBuffers {
		const Identifier eventType = Identifier::EVENT_ID_INIT_FRAMEBUFFERS;
	};


	/* Used when rigid bodies need to be updated. */
	struct UpdateRigidBodies {
		const Identifier eventType = Identifier::EVENT_ID_UPDATE_RIGID_BODIES;
	};
}