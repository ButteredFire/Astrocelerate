#pragma once

#include <concepts>


namespace CompilerUtils {
	// Arithmetic concepts

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


	// Logical concepts

	template<typename T1, typename T2>
	concept CompLessThan = requires(T1 a, T2 b) {
		a < b;
	};

	template<typename T1, typename T2>
	concept CompLessThanEqualTo = requires(T1 a, T2 b) {
		a <= b;
	};

	template<typename T1, typename T2>
	concept CompGreaterThan = requires(T1 a, T2 b) {
		a > b;
	};

	template<typename T1, typename T2>
	concept CompGreaterThanEqualTo = requires(T1 a, T2 b) {
		a >= b;
	};

	template<typename T1, typename T2>
	concept CompEqualTo = requires(T1 a, T2 b) {
		a == b;
	};

	template<typename T1, typename T2>
	concept CompNotEqualTo = requires(T1 a, T2 b) {
		a != b;
	};

	template<typename T1, typename T2>
	concept LogicalAnd = requires(T1 a, T2 b) {
		a&& b;
	};

	template<typename T1, typename T2>
	concept LogicalOr = requires(T1 a, T2 b) {
		a || b;
	};

	template<typename T>
	concept LogicalNot = requires(T a) {
		!a;
	};


	// Other

	template<typename T>
	concept HasSizeMethod = requires(const T & container) {
		{ container.size() } -> std::convertible_to<size_t>;
	};

} // namespace CompilerUtils
