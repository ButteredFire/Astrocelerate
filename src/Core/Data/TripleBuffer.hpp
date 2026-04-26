#pragma once

#include <atomic>
#include <type_traits>


/* Triple-buffer implementation for Producer-Consumer-pattern systems. */
template<typename T>
class TripleBuffer {
public:
    TripleBuffer() : m_writeIndex(0), m_readIndex(1), m_dirtyIndex(2) {
        // Check if a type can be constructed without any arguments
        static_assert(std::is_default_constructible_v<T>, "TripleBuffer requires T to be default-constructible.");
    }


    /* Publishes the latest buffer for consumption.
       @note This only atomically swaps the write buffer with the dirty buffer. To access and modify the write buffer before publishing, call writeRef.
	*/
    void publish() {
        // "Release" stores ensure all previous writes to m_buffers[m_writeIndex] are visible to other threads before the index swap happens
        m_writeIndex = m_dirtyIndex.exchange(m_writeIndex, std::memory_order_release);
    }


    /* Consumes the latest buffer.
        @note This only atomically swaps the write buffer with the read buffer. To read from the read buffer, call readRef.
		@return False if no new buffer has been published since the last consume (in which case readRef will return the most recent old buffer), otherwise True.
    */
    bool consume() {
        // "Acquire" ensures that once we see the new index, we also see the data that was written to that buffer.
        int newest = m_dirtyIndex.exchange(m_readIndex, std::memory_order_acquire);
        if (newest == m_readIndex)
            // Nothing new was published
            return false;

        m_readIndex = newest;
        return true;
    }

    T &writeRef() { return m_buffers[m_writeIndex]; }
    const T &readRef() { return m_buffers[m_readIndex]; }

private:
    T m_buffers[3];
    int m_writeIndex;               // Buffer A: The buffer that is written/published to
    int m_readIndex;                // Buffer B: The buffer that is read from/consumed
    std::atomic<int> m_dirtyIndex;  // Buffer C: The intermediate buffer used for atomic buffer swaps/exchanges (A <-> C, then C <-> B, then consume, then C <-> A, then the old buffer - now at A - is overwritten)
};