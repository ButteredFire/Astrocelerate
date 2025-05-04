/* SystemUtils.hpp - Utilities pertaining to system-level operations.
*/

#pragma once

#include <xhash>

namespace SystemUtils {
    /* Combines multiple hash values into a single hash value. */
    template <typename T>
    void combineHash(std::size_t& seed, const T& value) {
        std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}