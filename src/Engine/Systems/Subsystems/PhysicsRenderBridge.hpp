#pragma once

#include <mutex>
#include <queue>

#include <Core/Data/TripleBuffer.hpp>

#include <Engine/Rendering/Data/Buffer.hpp>

#include <Platform/External/GLM.hpp>


/* Data handoff mechanism between PhysicsSystem and RenderSystem. */
class PhysicsRenderBridge {
public:
	/* Swaps the completed write buffer into the read slot. */
	inline void publish(Buffer::PhysRendFramePacket &&frame) {
		m_frameBuffers.writeRef() = std::move(frame);
		m_frameBuffers.publish();

		m_staleBuffer.store(false, std::memory_order_release);
	}


	/* Returns a copy of the latest snapshot.
	   @param outFrame: The frame to be populated with the latest snapshot.
	   
	   @return False if no new frame has been published since last consume (in which case outFrame is populated with the most recent old snapshot), otherwise True.
	*/
	inline bool consume(Buffer::PhysRendFramePacket &outFrame) {
		if (m_staleBuffer.load(std::memory_order_acquire)) {
			outFrame = m_frameBuffers.readRef();
			return false;
		}

		m_frameBuffers.consume();
		outFrame = m_frameBuffers.readRef();

		m_staleBuffer.store(true, std::memory_order_release);
		return true;
	}
	
private:
	TripleBuffer<Buffer::PhysRendFramePacket> m_frameBuffers;
	std::atomic<bool> m_staleBuffer;
};
