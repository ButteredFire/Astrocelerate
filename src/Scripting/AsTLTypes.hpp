#pragma once

#include <cmath>
#include <string>
#include <limits>
#include <cstdint>
#include <climits>
#include <variant>
#include <typeindex>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>


// AstroAssembly (Astrocelerate Assembly, "As"-Two Language, AsTL) Data Types, as per the Specification
namespace AsTL {
		// Raw bytes (8-bit unsigned integer)
	using BYTE = uint8_t;

		// Indices and IDs (16-bit unsigned integer)
	using IDX = uint16_t;

		// I16 (16-bit signed integer)
	using I16 = int16_t;

		// I32 (32-bit signed integer)
	using I32 = int32_t;

		// F64 (64-bit float)
	using F64 = double;

		// BOOL (1-byte boolean)
	using BOOL = bool;

		// VEC3 (24-byte 3-component float vector)
	struct VEC3 {
		union {
			F64 data[3];

			// Aliases for `data`
			// NOTE: Anonymous structs inside unions are not ISO C++ compliant, but are supported by major compilers
			struct { F64 x, y, z; };
		};

		static constexpr F64 eps = std::numeric_limits<double>::epsilon();

		VEC3() : data{ 0.0, 0.0, 0.0 } {}

		VEC3(F64 v) : data{ v, v, v } {}

		template<typename... Args>
		requires (sizeof...(Args) == 3 && (std::is_arithmetic_v<Args> && ...))
		VEC3(Args... args) : data{ static_cast<F64>(args)... } {}

		template<typename... Args>
		requires (sizeof...(Args) == 3 && (std::is_arithmetic_v<Args> && ...))
		void set(Args... args) {
			F64 tmp[] = { static_cast<F64>(args)... };
			x = tmp[0];
			y = tmp[1];
			z = tmp[2];
		}


		bool operator==(const VEC3 &other) const {
			return x == other.x && y == other.y && z == other.z;
		}

		bool approxEquals(const VEC3& other, F64 tolerance = eps) const {
			return	std::abs(x - other.x) < tolerance &&
					std::abs(y - other.y) < tolerance &&
					std::abs(z - other.z) < tolerance;
		}

		// Component-wise addition
		VEC3 operator+(const VEC3& other) const {
			return { x + other.x, y + other.y, z + other.z };
		}

		// Component-wise subtraction
		VEC3 operator-(const VEC3& other) const {
			return { x - other.x, y - other.y, z - other.z };
		}

		// Component-wise multiplication (VEC3 * VEC3)
		VEC3 operator*(const VEC3 &other) const {
			return { x * other.x, y * other.y, z * other.z };
		}

		// Component-wise division (VEC3 / VEC3)
		VEC3 operator/(const VEC3 &other) const {
			return { x / other.x, y / other.y, z / other.z };
		}

		// Scalar addition with Right-side I32 (VEC3 + I32)
		VEC3 operator+(I32 orgScalar) const {
			F64 scalar = static_cast<F64>(orgScalar);
			return { x + scalar, y + scalar, z + scalar };
		}

		// Scalar addition with Right-side F64 (VEC3 + F64)
		VEC3 operator+(F64 scalar) const {
			return { x + scalar, y + scalar, z + scalar };
		}

		// Scalar subtraction with Right-side I32 (VEC3 - I32)
		VEC3 operator-(I32 orgScalar) const {
			F64 scalar = static_cast<F64>(orgScalar);
			return { x - scalar, y - scalar, z - scalar };
		}

		// Scalar subtraction with Right-side F64 (VEC3 - F64)
		VEC3 operator-(F64 scalar) const {
			return { x - scalar, y - scalar, z - scalar };
		}

		// Scalar multiplication with Right-side I32 (VEC3 * I32)
		VEC3 operator*(I32 orgScalar) const {
			F64 scalar = static_cast<F64>(orgScalar);
			return { x * scalar, y * scalar, z * scalar };
		}

		// Scalar multiplication with Right-side F64 (VEC3 * F64)
		VEC3 operator*(F64 scalar) const {
			return { x * scalar, y * scalar, z * scalar };
		}

		// Scalar division with Right-side I32 (VEC3 / I32)
		VEC3 operator/(I32 orgScalar) const {
			F64 scalar = static_cast<F64>(orgScalar);
			return { x / scalar, y / scalar, z / scalar };
		}

		// Scalar division with Right-side F64 (VEC3 / F64)
		VEC3 operator/(F64 scalar) const {
			return { x / scalar, y / scalar, z / scalar };
		}

		// Vector dot product
		F64 dot(const VEC3 &other) const {
			return	x * other.x +
					y * other.y +
					z * other.z;
		}

		// Vector cross product
		VEC3 cross(const VEC3 &other) const {
			return {
				y * other.z - other.y * z,
				z * other.x - x * other.z,
				x * other.y - other.x * y
			};
		}

		// Vector magnitude
		F64 mag() const {
			return std::sqrt(x * x + y * y + z * z);
		}

		// Vector normalization
		VEC3 norm() const {
			F64 magnitude = mag();
			if (magnitude == 0.0)
				return VEC3();
			
			return { x / magnitude, y / magnitude, z / magnitude };
		}
	};
	// Scalar multiplication with Left-side I32 (I32 * VEC3)
	inline VEC3 operator*(I32 orgScalar, const VEC3& vec) {
		F64 scalar = static_cast<F64>(orgScalar);
		return { vec.x * scalar, vec.y * scalar, vec.z * scalar };
	}
	// Scalar multiplication with Left-side F64 (F64 * VEC3)
	inline VEC3 operator*(F64 scalar, const VEC3& vec) {
		return { vec.x * scalar, vec.y * scalar, vec.z * scalar };
	}

		// String
	using STR = std::string;


	// All supported stack values
	using StackValue = std::variant<IDX, I16, I32, F64, BOOL, VEC3, STR>;

		// Subsets
	using NumericStackValue = std::variant<I16, I32, F64>;	// Numeric types (not including BOOL)
	using IntStackValue = std::variant<I16, I32>;			// Integral types (not inclulding BOOL)
	using FPStackValue = std::variant<F64>;					// Floating-point types

	// Type indices & Serialized strings for suppported stack values
	inline const std::type_index TID_BYTE		= typeid(BYTE);
	inline const std::string SER_BYTE			= "BYTE";

	inline const std::type_index TID_IDX		= typeid(IDX);
	inline const std::string SER_IDX			= "IDX";

	inline const std::type_index TID_I16		= typeid(I16);
	inline const std::string SER_I16			= "I16";

	inline const std::type_index TID_I32		= typeid(I32);
	inline const std::string SER_I32			= "I32";

	inline const std::type_index TID_F64		= typeid(F64);
	inline const std::string SER_F64			= "F64";

	inline const std::type_index TID_BOOL		= typeid(BOOL);
	inline const std::string SER_BOOL			= "BOOL";

	inline const std::type_index TID_VEC3		= typeid(VEC3);
	inline const std::string SER_VEC3			= "VEC3";

	inline const std::type_index TID_STR		= typeid(STR);
	inline const std::string SER_STR			= "STR";

	// Wildcard types
	inline const std::type_index TID_NUMERIC	= typeid(NumericStackValue);
	inline const std::string SER_NUMERIC		= "NUMERIC";

	inline const std::type_index TID_INTEGRAL	= typeid(IntStackValue);
	inline const std::string SER_INTS			= "INTEGRAL";

	inline const std::type_index TID_FPOINT		= typeid(FPStackValue);
	inline const std::string SER_FLOATS			= "FPOINT";

	inline const std::type_index TID_ANY		= typeid(StackValue);
	inline const std::string SER_ANY			= "ANY";


	// Acceptable conversions
	inline const std::unordered_map<
		std::type_index,
		std::unordered_set<std::type_index>
	> ValidConversionMap = {
		{ TID_BYTE, {} },	// Internal type
		{ TID_IDX, {} },	// Internal type

		{ TID_BOOL, { TID_BOOL, TID_STR } },

		{ TID_I16, { TID_I16, TID_I32, TID_F64, TID_STR } },
		{ TID_I32, { TID_I16, TID_I32, TID_F64, TID_STR } },
		{ TID_F64, { TID_I16, TID_I32, TID_F64, TID_STR } },

		{ TID_VEC3, { TID_VEC3, TID_STR } },
		{ TID_STR, { TID_STR } }
	};


	/* Is the type a wildcard type (True), or a concrete type (False)? */
	inline bool IsWildcard(std::type_index type) {
		return (
			type == TID_NUMERIC ||
			type == TID_INTEGRAL ||
			type == TID_FPOINT ||
			type == TID_ANY
		);
	}


	/* Converts a serialized string to the corresponding stack value type. */
	inline std::type_index StringToStackValue(const std::string &type) {
		if (type == SER_BYTE)			return TID_BYTE;
		if (type == SER_IDX)			return TID_IDX;

		else if (type == SER_I16)		return TID_I16;
		else if (type == SER_I32)		return TID_I32;
		else if (type == SER_F64)		return TID_F64;
		else if (type == SER_BOOL)		return TID_BOOL;
		else if (type == SER_VEC3)		return TID_VEC3;

		else if (type == SER_STR)		return TID_STR;

		else if (type == SER_NUMERIC)	return TID_NUMERIC;
		else if (type == SER_INTS)		return TID_INTEGRAL;
		else if (type == SER_FLOATS)	return TID_FPOINT;
		else if (type == SER_ANY)		return TID_ANY;

		else							return typeid(void);
	}


	/* Converts a stack value type to the corresponding serialized string. */
	inline std::string StackValueToString(std::type_index type) {
		if (type == TID_BYTE)			return SER_BYTE;
		if (type == TID_IDX)			return SER_IDX;

		else if (type == TID_I16)		return SER_I16;
		else if (type == TID_I32)		return SER_I32;
		else if (type == TID_F64)		return SER_F64;
		else if (type == TID_BOOL)		return SER_BOOL;
		else if (type == TID_VEC3)		return SER_VEC3;
		else if (type == TID_STR)		return SER_STR;

		else if (type == TID_NUMERIC)	return SER_NUMERIC;
		else if (type == TID_INTEGRAL)	return SER_INTS;
		else if (type == TID_FPOINT)	return SER_FLOATS;
		else if (type == TID_ANY)		return SER_ANY;

		else return "???";
	}

	inline std::string StackValueToString(const StackValue &stackVal) {
		std::string rv = "???";
		std::visit([&](const auto &val) {
			rv = StackValueToString(typeid(decltype(val)));
		}, stackVal);

		return rv;
	}


	/* Converts a stack value type to the corresponding representative string. */
	inline std::string StackValueToRepString(std::type_index type) {
		if (type == TID_BYTE)			return "Type Code";
		if (type == TID_IDX)			return "Address";

		else if (type == TID_I16)		return "16-bit Integer";
		else if (type == TID_I32)		return "Integer";
		else if (type == TID_F64)		return "Float";
		else if (type == TID_BOOL)		return "Boolean";
		else if (type == TID_VEC3)		return "Vector3";

		else if (type == TID_STR)		return "String";

		else if (type == TID_NUMERIC)	return "Numeric Type";
		else if (type == TID_INTEGRAL)	return "Integral Type";
		else if (type == TID_FPOINT)	return "Floating-Point Type";
		else if (type == TID_ANY)		return "Any Type";
		
		else return "Unknown Type";
	}

	inline std::string StackValueToRepString(const StackValue &stackVal) {
		std::string rv{};
		std::visit([&](const auto &val) {
			rv = StackValueToRepString(typeid(decltype(val)));
		}, stackVal);

		return rv;
	}


	/* Gets the default value of a stack value type. */
	inline StackValue GetDefaultStackValue(std::type_index type) {
		if (type == TID_BYTE)			return BYTE();
		if (type == TID_IDX)			return IDX();

		else if (type == TID_I16)		return I16();
		else if (type == TID_I32)		return I32();
		else if (type == TID_F64)		return F64();
		else if (type == TID_BOOL)		return BOOL();
		else if (type == TID_VEC3)		return VEC3();

		else if (type == TID_STR)		return "";

		else if (type == TID_INTEGRAL)	return I16();
		else if (type == TID_NUMERIC)	return F64();
		else if (type == TID_FPOINT)	return F64();

		std::string excMsg = "No default value exists for type index " + std::string(type.name());
		throw std::runtime_error(excMsg.c_str());
	}


	// Forward-declared opaque execution context for Node callbacks; concrete implementation must exist elsewhere
	struct OpaqueExecCtx;
}


namespace std {
	template<>
	struct hash<AsTL::VEC3> {
		size_t operator()(const AsTL::VEC3 &vec) const noexcept {
			std::size_t h1 = std::hash<AsTL::F64>{}(vec.x);
			std::size_t h2 = std::hash<AsTL::F64>{}(vec.y);
			std::size_t h3 = std::hash<AsTL::F64>{}(vec.z);

			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}
