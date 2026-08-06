#pragma once

/* Used for:
	std::visit(OverloadedVisit {...}, variant);
*/
template<class... Ts>
struct OverloadedVisit : Ts... { using Ts::operator()...; };


template <typename T, typename Variant> struct is_alternative : std::false_type {};
template <typename T, typename... Args> struct is_alternative<T, std::variant<Args...>> : std::disjunction<std::is_same<T, Args>...> {};

template <typename T, typename Variant>
inline constexpr bool is_alternative_v = is_alternative<T, Variant>::value;

// Checks whether a type is contained within a variant's type list (compile-time)
template <typename T, typename Variant>
concept AlternativeOf = is_alternative_v<T, Variant>;
