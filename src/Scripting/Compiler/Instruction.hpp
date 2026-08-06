#pragma once

#include <bitset>
#include <cstdint>
#include <optional>
#include <concepts>

#include <Scripting/AsTLTypes.hpp>


#if defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
	#define FORCE_INLINE __attribute__((always_inline))
#else
	#define FORCE_INLINE inline
#endif


namespace Compiler {
	// An AstroAssembly instruction has a fixed size of 4 bytes:
	// [ Opcode ][ Bitmask/Padding ][ Operand(s) ]
	//     1              1               2
	//    Byte           Byte           Bytes

	using RawInstructionT = uint32_t;
	using RawOpcodeT = uint8_t;			// Instruction Opcode size (8 bits)
	using RawBitmaskT = uint8_t;		// Instruction Bitmask size (also used as Padding) (8 bits)
	using RawOperandT = uint16_t;		// Instruction Operand size (16 bits)

	inline constexpr size_t BitmaskSz = 8;
	using InstructionMask = std::bitset<BitmaskSz>;


	// AstroAssembly operand types
	enum OperandType : AsTL::BYTE {
		OT_BYTE,
		OT_IDX,

		OT_I16,
		OT_I32,
		OT_F64,
		OT_BOOL,
		OT_VEC3,
		OT_STR
	};


	// AstroAssembly special-purpose register types
	enum SPRType : AsTL::BYTE {
		SPR_VMS,
		SPR_EXC,

		SPRC	// Number of special-purpose registers
	};
	constexpr size_t SPR_COUNT = static_cast<size_t>(SPRType::SPRC);


	// AstroAssembly container type codes
	enum ContainerType : AsTL::BYTE {
		CT_VM_STACK,
		CT_SPR,
		CT_GLOBAL_REG,
		CT_LOCAL_REG,
		CT_CONST_POOL,

		// Non-standard
		NONSTANDARD_CT_DEDICATED_STR_HEAP
	};


	// AstroAssembly bitmask flags
	// The enum has to be unscoped rather than scoped (enum class) because the latter requires unwieldy explicit casting to size_t (e.g., std::bitset::set(static_cast<size_t>(enum)))
	enum Bitflag {
		BITFLAG_READ_ONLY_BIT = 0
	};


	// AstroAssembly standard exit codes
	enum class VMExitCode : AsTL::I16 {
		SUCCESS				=  0,	// Execution signal reached an explicit `Control::Terminate` node and exited cleanly
		CRASHED_CONFIG		= -1,	// Generic VM error that occurs at configuration time
		CRASHED_RT			= -2,	// Generic VM error that occurs at runtime
		EXEC_HALTED			= -3,	// Execution signal hit a closed gate, unlinked execution pin, etc. that prevents it from reaching a `Control::Terminate` node
		VM_STACK_OVERFLOW	= -4,	// The VM stack exceeded its maximum allocated size
		CALL_STACK_OVERFLOW = -5,	// The call stack exceeded its maximum allocated size
		BAD_CAST			= -6,	// A value was casted to or reinterpreted as an incompatible type, or sourced from an incompatible origin
		OUT_OF_BOUNDS		= -7,	// A container was accessed with an out-of-bounds index

		FINISHED_EXEC_TICK	= 1		// The graph has finished execution for this simulation tick
	};


	// AstroAssembly opcodes
	enum Opcode : RawOpcodeT {
		// Data movement
		LOAD_INLINE,
		POP,
		LOAD_SPR,
		STORE_SPR,
		LOAD_CONST,
		LOAD_GL,
		LOAD_LC,
		STORE_GL,
		STORE_LC,
		OVR_GL,
		OVR_LC,
		TO_STR,
		TO_I16,
		TO_I32,
		TO_F64,
		PRINT,
		EXEC_NATIVE,


		// Arithmetic & logic
			// Standard arithmetic instructions
		ADD,
		SUB,
		MUL,
		DIV,
		MOD,
		NEG,

			// String arithmetic instructions
		STR_CAT,

			// Vector arithmetic instructions
		VEC_NORM,
		VEC_MAG,
		VEC_MUL,
		VEC_DIV,
		VEC_MUL_DOT,
		VEC_MUL_CROSS,

			// Comparison instructions
		CMP_LT,
		CMP_LTE,
		CMP_GT,
		CMP_GTE,
		CMP_EQ,
		CMP_NEQ,

			// Logical instructions
		LGC_AND,
		LGC_OR,
		LGC_NOT,


		// Mathematics

			// Trigonometric instructions
		SIN,
		ASIN,
		COS,
		ACOS,
		TAN,
		ATAN,
		ATAN2,
		COT,
		ACOT,

			// Other
		ABS,
		MIN,
		MAX,


		// Control flow
		JUMP,
		JUMP_IF_TRUE,
		JUMP_IF_FALSE,
		CALL,
		CALL_IF_TRUE,
		CALL_IF_FALSE,
		RET,
		TERMINATE
	};


	struct Instruction {
		RawOpcodeT opcode;
		RawBitmaskT bitmask;
		RawOperandT operand;

		FORCE_INLINE Instruction(RawInstructionT instruction) {
			opcode = static_cast<RawOpcodeT>((instruction >> 24) & 0xFF);
			bitmask = static_cast<RawBitmaskT>((instruction >> 16) & 0xFF);
			operand = static_cast<RawOperandT>(instruction & 0xFFFF);
		}

		/* Tests whether a bit is set (True) in the bitmask, or not (False). */
		FORCE_INLINE bool testBit(RawBitmaskT bit) const {
			return (bitmask & (1 << bit)) != 0;
		}
	};


	struct SymbolicInstruction {
		Opcode opcode;

		std::optional<InstructionMask> bitmask;

		std::optional<									// Zero-operand instruction
			std::variant<
				// Single-operand instruction
				AsTL::BYTE,
				AsTL::IDX,
				AsTL::I16,
				
				// Double-operand instruction
				std::pair<AsTL::BYTE, AsTL::BYTE>,
				std::pair<AsTL::BYTE, AsTL::IDX>
			>
		> operand;

		SymbolicInstruction(Opcode op) : opcode(op), bitmask(std::nullopt), operand(std::nullopt) {}

		SymbolicInstruction(Opcode op, decltype(operand) val) : opcode(op), operand(val) {
			setOperandValue();
		}

		SymbolicInstruction(Opcode op, InstructionMask mask, decltype(operand) val) : opcode(op), bitmask(mask), operand(val) {
			setOperandValue();
		}


		void setOperandValue() {
			if (
				// Operand is std::nullopt
				!operand.has_value() ||

				// Operand has already been set to a definitive value
				(operand.has_value() && typeid(operand.value()) != typeid(std::remove_reference_t<decltype(SymbolicInstruction::operand.value())>))
			)
				return;

			std::visit([&](const auto &val) {
				operand = val;
			}, operand.value());
		}
	};


	// NOTE: C++ has a specific specialized syntax for bitfields;
	// see: https://en.cppreference.com/cpp/language/bit_field


	/* Converts an AstroAssembly value type to its Byte representation. */
	inline AsTL::BYTE StackValueToByte(std::type_index type) {
		if (type == AsTL::TID_BYTE)		return static_cast<AsTL::BYTE>(OperandType::OT_BYTE);
		if (type == AsTL::TID_IDX)		return static_cast<AsTL::BYTE>(OperandType::OT_IDX);
		if (type == AsTL::TID_BOOL)		return static_cast<AsTL::BYTE>(OperandType::OT_BOOL);
		if (type == AsTL::TID_I16)		return static_cast<AsTL::BYTE>(OperandType::OT_I16);
		if (type == AsTL::TID_I32)		return static_cast<AsTL::BYTE>(OperandType::OT_I32);
		if (type == AsTL::TID_F64)		return static_cast<AsTL::BYTE>(OperandType::OT_F64);
		if (type == AsTL::TID_VEC3)		return static_cast<AsTL::BYTE>(OperandType::OT_VEC3);
		if (type == AsTL::TID_STR)		return static_cast<AsTL::BYTE>(OperandType::OT_STR);

		return 0xFF;
	}

	inline AsTL::BYTE StackValueToByte(const AsTL::StackValue &stackVal) {
		AsTL::BYTE rv = 0x00;
		std::visit([&](const auto &val) {
			rv = StackValueToByte(typeid(decltype(val)));
		}, stackVal);

		return rv;
	}


	/* Converts the Byte type code of a data type to its String equivalent. */
	inline std::string ByteToString(OperandType opType) {
		switch (opType) {
		case OT_BYTE:	return "BYTE";
		case OT_IDX:	return "IDX";
		case OT_BOOL:	return "BOOL";
		case OT_I16:	return "I16";
		case OT_I32:	return "I32";
		case OT_F64:	return "F64";
		case OT_VEC3:	return "VEC3";
		case OT_STR:	return "STR";
		default:		return "???";
		}
	}

	inline std::string ByteToString(AsTL::BYTE opByteType) {
		return ByteToString(static_cast<OperandType>(opByteType));
	}


	/* Converts a VM exit code to its String representation. */
	inline std::string VMExitCodeToString(VMExitCode exitCode) {
		using enum VMExitCode;
		AsTL::I16 intVal = static_cast<AsTL::I16>(exitCode);
		switch (exitCode) {
		case SUCCESS:				return std::to_string(intVal) + " (SUCCESS)";
		case CRASHED_CONFIG:		return std::to_string(intVal) + " (CRASHED_CONFIG)";
		case CRASHED_RT:			return std::to_string(intVal) + " (CRASHED_RT)";
		case EXEC_HALTED:			return std::to_string(intVal) + " (EXEC_HALTED)";
		case VM_STACK_OVERFLOW:		return std::to_string(intVal) + " (VM_STACK_OVERFLOW)";
		case BAD_CAST:				return std::to_string(intVal) + " (BAD_CAST)";
		case OUT_OF_BOUNDS:			return std::to_string(intVal) + " (OUT_OF_BOUNDS)";
		default:					return std::to_string(intVal) + " (???)";
		}
	}


	/* Converts an encoded Opcode to its decoded String equivalent. */
	inline std::string OpcodeToString(Opcode op) {
		using enum Opcode;

		switch (op) {
		case LOAD_INLINE:		return "LOAD_INLINE";
		case POP:				return "POP";
		case LOAD_SPR:			return "LOAD_SPR";
		case STORE_SPR:			return "STORE_SPR";
		case LOAD_CONST:		return "LOAD_CONST";
		case LOAD_GL:			return "LOAD_GL";
		case LOAD_LC:			return "LOAD_LC";
		case STORE_GL:			return "STORE_GL";
		case STORE_LC:			return "STORE_LC";
		case OVR_GL:			return "OVR_GL";
		case OVR_LC:			return "OVR_LC";
		case TO_STR:			return "TO_STR";
		case TO_I16:			return "TO_I16";
		case TO_I32:			return "TO_I32";
		case TO_F64:			return "TO_F64";
		case PRINT:				return "PRINT";
		case EXEC_NATIVE:		return "EXEC_NATIVE";
		case ADD:				return "ADD";
		case SUB:				return "SUB";
		case MUL:				return "MUL";
		case DIV:				return "DIV";
		case MOD:				return "MOD";
		case NEG:				return "NEG";
		case STR_CAT:			return "STR_CAT";
		case VEC_NORM:			return "VEC_NORM";
		case VEC_MAG:			return "VEC_MAG";
		case VEC_MUL:			return "VEC_MUL";
		case VEC_DIV:			return "VEC_DIV";
		case VEC_MUL_DOT:		return "VEC_MUL_DOT";
		case VEC_MUL_CROSS:		return "VEC_MUL_CROSS";
		case CMP_LT:			return "CMP_LT";
		case CMP_LTE:			return "CMP_LTE";
		case CMP_GT:			return "CMP_GT";
		case CMP_GTE:			return "CMP_GTE";
		case CMP_EQ:			return "CMP_EQ";
		case CMP_NEQ:			return "CMP_NEQ";
		case LGC_AND:			return "LGC_AND";
		case LGC_OR:			return "LGC_OR";
		case LGC_NOT:			return "LGC_NOT";
		case SIN:				return "SIN";
		case ASIN:				return "ASIN";
		case COS:				return "COS";
		case ACOS:				return "ACOS";
		case TAN:				return "TAN";
		case ATAN:				return "ATAN";
		case ATAN2:				return "ATAN2";
		case COT:				return "COT";
		case ACOT:				return "ACOT";
		case ABS:				return "ABS";
		case MIN:				return "MIN";
		case MAX:				return "MAX";
		case JUMP:				return "JUMP";
		case JUMP_IF_TRUE:		return "JUMP_IF_TRUE";
		case JUMP_IF_FALSE:		return "JUMP_IF_FALSE";
		case CALL:				return "CALL";
		case CALL_IF_TRUE:		return "CALL_IF_TRUE";
		case CALL_IF_FALSE:		return "CALL_IF_FALSE";
		case RET:				return "RET";
		case TERMINATE:			return "TERMINATE";

		default:				return "???";
		}
	}
}