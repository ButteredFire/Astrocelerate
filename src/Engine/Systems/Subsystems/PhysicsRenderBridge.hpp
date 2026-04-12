#pragma once

#include <mutex>

#include <Engine/Rendering/Data/Buffer.hpp>

#include <Platform/External/GLM.hpp>


/* Data handoff mechanism between PhysicsSystem and RenderSystem. */
class PhysicsRenderBridge {
public:
	/* Swaps the completed write buffer into the read slot. */
	inline void publish(Buffer::PhysRendFramePacket &&frame) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_readBuffer = std::move(frame);
		m_frameReady = true;
	}


	/* Returns a copy of the latest snapshot.
	   @param outFrame: The frame to be populated with the latest snapshot.
	   
	   Returns false if no new frame has been published since last consume, in which case outFrame is populated with the most recent old snapshot.
	*/
	inline bool consume(Buffer::PhysRendFramePacket &outFrame) {
		std::lock_guard<std::mutex> lock(m_mutex);
		outFrame = m_readBuffer;

		if (!m_frameReady)
			return false;

		m_frameReady = false;
		return true;
	}
	
private:
	std::mutex m_mutex;
	Buffer::PhysRendFramePacket m_readBuffer;
	bool m_frameReady = false;
};
