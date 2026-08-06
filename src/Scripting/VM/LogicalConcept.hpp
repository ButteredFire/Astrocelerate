#pragma once

#include <concepts>

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
	a && b;
};

template<typename T1, typename T2>
concept LogicalOr = requires(T1 a, T2 b) {
	a || b;
};

template<typename T>
concept LogicalNot = requires(T a) {
	!a;
};