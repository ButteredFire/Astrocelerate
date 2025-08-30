/* SystemUtils.hpp - Utilities pertaining to system-level operations.
*/

#pragma once

#include <xhash>
#include <concepts>
#include <vector>
#include <random>
#include <chrono>

#include <External/GLFWVulkan.hpp>

#include <Core/Application/LoggingManager.hpp>


namespace SystemUtils {
    // Gets the size of a C array.
#define SIZE_OF(arr) (sizeof((arr)) / sizeof((arr[0])))


    // Concept: Is a number
    template<typename T>
    concept Number = std::regular<T> && requires(T a, T b) {
        { a + b }  -> std::same_as<T>;
        { a - b }  -> std::same_as<T>;
        { a * b }  -> std::same_as<T>;
        { a / b }  -> std::same_as<T>;
        { a == b } -> std::same_as<bool>;
        { a < b }  -> std::same_as<bool>;
    };


    // Concept: Supports division by a double
    template<typename T>
    concept DivisibleByDouble = requires(T a, double b) {
        { a / b } -> std::convertible_to<T>;
    };


    // Concept: Supports multiplication by a double
    template<typename T>
    concept MultipliableByDouble = requires(T a, double b) {
        { a * b } -> std::convertible_to<T>;
    };


    /* Combines multiple hash values into a single hash value. 
		This function uses the std::hash function to generate a hash for the value and combines it with the seed using a bitwise XOR operation and some arithmetic operations.
		
        @tparam T: The type of the value to be combined.
		
        @param seed: The initial hash value.
		@param value: The value to be combined with the hash.
    */
    template <typename T>
    void CombineHash(std::size_t& seed, const T& value) {
        std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }


	/* Aligns a given size to the nearest multiple of the specified (power-of-two) alignment.
	   @param size: The size to be aligned.
	   @param alignment: The alignment value.

	   @return: The aligned size.
    */
    inline size_t Align(size_t size, size_t alignment) {
        const bool isPowOf2 = (alignment > 0) && ((alignment & (alignment - 1)) == 0);
        LOG_ASSERT(isPowOf2, "Cannot align size to the nearest multiple of " + std::to_string(alignment) + ": Alignment is not a power of two!");

        return (size + alignment - 1) & ~(alignment - 1);
    }


    /* Computes the byte offset into a buffer.
        @param alignedBufSize: The buffer size, aligned to the nearest multiple of a power of two.
        @param mappedData: The void pointer to the buffer's data.
        @param stride: The index into the buffer.

        @return A void pointer to the start of a child buffer within the buffer.
    */
    inline void* GetAlignedBufferOffset(const size_t alignedBufSize, void *mappedData, size_t stride) {
        /*
            First, we retrieve the pointer to the buffer. We must cast it to a Byte pointer so that subsequent pointer increments will mean incrementing by bytes.
            Then, we calculate the offset of the target child buffer from the base (start) of the buffer.
            Finally, we move the base pointer to the offset (start of the target child buffer).
        */

        Byte *base = static_cast<Byte*>(mappedData);
        size_t offset = alignedBufSize * stride;

        return static_cast<void*>(base + offset);
    }
}