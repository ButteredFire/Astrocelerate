/* SystemUtils.hpp - Utilities pertaining to system-level operations.
*/

#pragma once

#include <xhash>
#include <concepts>
#include <vector>

#include <glfw_vulkan.hpp>

#include <Core/LoggingManager.hpp>


namespace SystemUtils {
    // Gets the size of a C array.
#define SIZE_OF(arr) (sizeof((arr)) / sizeof((arr[0])))


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


	/* Aligns a given size to the nearest multiple of the specified alignment.
	   @param size: The size to be aligned.
	   @param alignment: The alignment value.

	   @return: The aligned size.
    */
    inline size_t Align(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }
}