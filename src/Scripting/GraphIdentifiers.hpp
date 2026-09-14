#pragma once

#include <string>
#include <variant>
#include <unordered_map>


#define NODECLASS(X)		\
	X(Control)				\
	X(Console)				\
	X(Math)					\
	X(Misc)					\
	X(Simulation)

#define NODECATEGORY(X)		\
	X(Constant)				\
	X(Arithmetic)			\
	X(Logic)				\
	X(Time)					\
	X(Orbit)				\
	X(Body)					\
	X(Shape)				\
	X(Spacecraft)

#define NODEFUNCTION(X)		\
	X(Branch)				\
	X(DoOnce)				\
	X(Sequence)				\
	X(ForLoop)				\
	X(WhileLoop)			\
	X(Print)				\
	X(StringConcat)			\
	X(Pi)					\
	X(Add)					\
	X(Subtract)				\
	X(Multiply)				\
	X(Divide)				\
	X(Modulo)				\
	X(Negate)				\
	X(Vec3Normalize)		\
	X(Vec3Magnitude)		\
	X(Vec3Dot)				\
	X(Vec3Cross)			\
	X(GreaterThan)			\
	X(GreaterThanEqualTo)	\
	X(LessThan)				\
	X(LessThanEqualTo)		\
	X(EqualTo)				\
	X(NotEqualTo)			\
	X(And)					\
	X(Or)					\
	X(Not)					\
	X(Sine)					\
	X(Arcsine)				\
	X(Cosine)				\
	X(Arccosine)			\
	X(Tangent)				\
	X(Arctangent)			\
	X(Arctangent2)			\
	X(Cotangent)			\
	X(Arccotangent)			\
	X(Absolute)				\
	X(Minimum)				\
	X(Maximum)				


namespace Graph {
	// Node class (widest scope)
	enum class ClassScope {
		#define X(Enum) Enum,
			NODECLASS(X)
		#undef X
	};

	// Node category (2nd-widest scope)
	enum class CatScope {
		#define X(Enum) Enum,
			NODECATEGORY(X)
		#undef X
	};

	// Node function (final scope)
	enum class FuncScope {
		#define X(Enum) Enum,
			NODEFUNCTION(X)
		#undef X
	};


	using ScopeType = std::variant<ClassScope, CatScope, FuncScope>;


	/* Converts a scope enum to its string representation. */
	inline std::string ScopeToString(ScopeType scope) {
		using enum ClassScope;
		using enum CatScope;
		using enum FuncScope;

		static std::unordered_map<ScopeType, std::string> convert = {
			#define X(Enum) { Enum, #Enum },
					NODECLASS(X)
					NODECATEGORY(X)
					NODEFUNCTION(X)
			#undef X
		};

		return convert.at(scope);
	}


	/* Converts a stream of graph scopes to a qualified identifier. */
	template<typename... Identifiers>
	// ScopeType is a variant of scoped enums;
	// to check if a given type is present in the variant, we use std::is_constructible_v<std::variant<Ts...>, T>;
	// this checks if the variant can be constructed from a value of a given type `T`
		requires(std::is_constructible_v<ScopeType, Identifiers> && ...)
	inline static std::string MakeQualifiedID(Identifiers... ids) {
		return (... + (std::string("::") + ScopeToString(ids)));
	}
}
