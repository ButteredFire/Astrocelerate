#pragma once

#include <concepts>

template <typename T1, typename T2>
concept Addable = requires(T1 a, T2 b) {
	a + b;
};

template <typename T1, typename T2>
concept Subtractable = requires(T1 a, T2 b) {
	a - b;
};

template <typename T1, typename T2>
concept Multipliable = requires(T1 a, T2 b) {
	a * b;
};

template <typename T1, typename T2>
concept Divisible = requires(T1 a, T2 b) {
	a / b;
};

template <typename T1, typename T2>
concept CanDoModulo = std::integral<T1> && std::integral<T2> && requires(T1 a, T2 b) {
	a % b;
};

template <typename T>
concept Negatable = requires(T a) {
	{ -a };
};