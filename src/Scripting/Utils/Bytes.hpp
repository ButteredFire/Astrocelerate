#pragma once

#include <bit>
#include <ranges>
#include <concepts>
#include <algorithm>


namespace CompilerUtils {

    /* Reverses the bytes for a given integer value.
        Replacement implementation for C++23's std::byteswap: https://en.cppreference.com/cpp/numeric/byteswap
    */
    template<std::integral T>
    constexpr T Byteswap(T value) noexcept {
        static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
        auto valRep = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        std::ranges::reverse(valRep);
        return std::bit_cast<T>(valRep);
    }
}
