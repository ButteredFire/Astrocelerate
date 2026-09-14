#pragma once

#include <bit>
#include <any>
#include <array>
#include <cmath>
#include <vector>
#include <limits>
#include <cstdint>
#include <iostream>

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/AsTLMacros.hpp>
#include <Scripting/Utils/Bytes.hpp>
#include <Scripting/Utils/Concepts.hpp>
#include <Scripting/Utils/StaticBlock.hpp>
#include <Scripting/Utils/VariantHelpers.hpp>
#include <Scripting/Compiler/Instruction.hpp>
#include <Scripting/Compiler/ConstantPool.hpp>
#include <Scripting/Compiler/GraphNodeRegistry.hpp>

#include "VMException.hpp"


#define PACK_OPS(OP1, OP2)	((static_cast<Compiler::RawOperandT>(static_cast<AsTL::BYTE>(OP1) & 0xFF) << 8) | \
							(static_cast<Compiler::RawOperandT>(static_cast<AsTL::BYTE>(OP2) & 0xFF) << 0))


/*
	NOTE:
	If the constexpr condition is not met, there is no break statement,
	and the code jumps straight to the `default` case (which throws the exception).
	Since this is intentional behavior, we include the `[[fallthrough]]` attribute
	to tell the compiler (and other programmers) that the behavior is intentional.

	https://en.cppreference.com/cpp/language/attributes/fallthrough
*/
#define MAKE_CASE_1OP(TAG, TYPE, LAMBDA, CONCEPT)							\
    case TAG:																\
	{																		\
		if constexpr (CONCEPT<TYPE>)										\
		{																	\
			LAMBDA<TYPE>(TAG);												\
			break;															\
		}																	\
        [[fallthrough]];													\
	}


#define MAKE_CASE_2OP(L_TAG, L_TYPE, R_TAG, R_TYPE, LAMBDA, BIN_CONCEPT)		\
    case PACK_OPS(L_TAG, R_TAG):												\
	{																			\
		if constexpr (BIN_CONCEPT<L_TYPE, R_TYPE>)								\
		{																		\
			LAMBDA<L_TYPE, R_TYPE>(L_TAG, R_TAG);								\
			break;																\
		}																		\
        [[fallthrough]];														\
	}


/* Generates switch cases for each supported AstroAssembly type.
	@param OP: The raw 1-byte value of the operand
	@param LAMBDA: The callable that will be invoked for each case.
		The callable implementation takes the form: [...]<typename T>(Compiler::OperandType t) -> void {...}
	@param CONCEPT: A requirement (concept) that omits types failing to meet it.
		The concept implementation takes the form: CONCEPT<OPERAND_T>
*/
#define SWITCH_TYPES(OP, LAMBDA, CONCEPT)																									\
    switch (OP) {																															\
        ASTL_TYPE_LIST(MAKE_CASE_1OP, LAMBDA, CONCEPT)																						\
        default:																															\
		{																																	\
			saveVMState(Compiler::VMExitCode::BAD_CAST);																					\
			throw VMRuntimeException("At instruction address 0x{:0>{}X}: Unrecognized operand type 0x{:0>{}X}{}",							\
				m_pc - 1, ADDR_HEX_SZ,																										\
				OP, BYTE_HEX_SZ,																											\
				(static_cast<size_t>(OP) < Compiler::OperandTypeCount) ? std::format(" ({})", Compiler::ByteToString(OP)) : ""				\
			);																																\
		}																																	\
    }


/* Generates switch cases with all permutations of supported AstroAssembly types.
	@param L_OP: The raw 1-byte value of the left operand
	@param R_OP: The raw 1-byte value of the right operand
	@param LAMBDA: The callable that will be invoked for each case.
		The callable implementation takes the form: [...]<typename LEFT_T, typename RIGHT_T>
											(Compiler::OperandType lt, Compiler::OperandType rt) -> void {...}
	@param BIN_CONCEPT: A requirement (concept) that omits type combinations failing to meet it.
		The concept implementation takes the form: BIN_CONCEPT<LEFT_OPERAND_T, RIGHT_OPERAND_T>
*/
#define SWITCH_TYPES_PERMUT(L_OP, R_OP, LAMBDA, BIN_CONCEPT)																				\
    switch ((static_cast<Compiler::RawOperandT>(L_OP) << 8) |																				\
			static_cast<Compiler::RawOperandT>(R_OP)) {																						\
        ASTL_TYPE_LIST_PERMUTATIONS(MAKE_CASE_2OP, LAMBDA, BIN_CONCEPT)																		\
        default:																															\
		{																																	\
			saveVMState(Compiler::VMExitCode::BAD_CAST);																					\
			throw VMRuntimeException("At instruction address 0x{:0>{}X}: Unrecognized operand types 0x{:0>{}X}{} and 0x{:0>{}X}{}",			\
				m_pc - 1, ADDR_HEX_SZ,																										\
				L_OP, BYTE_HEX_SZ,																											\
				(static_cast<size_t>(L_OP) < Compiler::OperandTypeCount) ? std::format(" ({})", Compiler::ByteToString(L_OP)) : "",			\
				R_OP, BYTE_HEX_SZ,																											\
				(static_cast<size_t>(R_OP) < Compiler::OperandTypeCount) ? std::format(" ({})", Compiler::ByteToString(R_OP)) : ""			\
			);																																\
		}																																	\
    }


template<typename T>
concept NonVoidReturn = !std::is_void_v<T>;

template <typename T, typename TArg1, typename TArg2>
concept ValidBinaryCallable = requires(T t, TArg1 arg1, TArg2 arg2) {
	{ t.template operator()<TArg1, TArg2>(arg1, arg2) } -> NonVoidReturn;
};


class VirtualMachine {
public:
	VirtualMachine(
		const Compiler::ConstantPool &constPool,
		const std::reference_wrapper<Compiler::IGraphNodeRegistry> nodeRegistry,
		AsTL::OpaqueExecCtx *execCtx,
		const size_t vmStackSzKB = 10,
		const size_t callStackSzKB = 10
	);
	~VirtualMachine() = default;

	void setProgram(const std::vector<Compiler::RawInstructionT>& instructions);

	Compiler::VMExitCode execute(const std::vector<Compiler::RawInstructionT> &instructions);
	Compiler::VMExitCode execute();

	Compiler::VMExitCode resume();

private:
	static constexpr size_t BYTE_HEX_SZ = 2;		// Hexadecimal character size of a byte (1 byte => 2 hex chars)
	static constexpr size_t ADDR_HEX_SZ = 4;		// Hexadecimal character size of an instruction address (2 bytes => 4 hex chars)

	const Compiler::ConstantPool &m_constPool;
	const Compiler::IGraphNodeRegistry &m_nodeRegistry;
	AsTL::OpaqueExecCtx *m_execCtx;

	const std::vector<Compiler::RawInstructionT> *m_instructions;

	std::vector<uint64_t> m_vmStack;
	std::vector<AsTL::StackValue> m_globReg;
	std::array<uint64_t, Compiler::SPRCount> m_SPRs;
	std::vector<AsTL::STR> m_strHeap;			// Dedicated string heap (non-standard)

	std::vector<AsTL::StackValue> m_funcArgs;	// Native function argument list
	std::vector<AsTL::StackValue> m_funcRets;	// Native function return list
	size_t m_funcArgsListSz, m_funcRetsListSz;	// Current max allocated sizes for the native function argument and return lists, in element count

	const size_t m_allocVMStackSzKB;		// Allocated VM stack size (in kilobytes)
	const size_t m_allocCallStackSzKB;		// Allocated call stack size (in kilobytes)

	AsTL::IDX m_pc;		// Program counter (pointing to the next instruction to be executed)
	AsTL::I32 m_vsp;	// VM stack pointer (can be negative if stack is empty)
	AsTL::I32 m_csp;	// Call stack pointer (can be negative if stack is empty)

	Compiler::Opcode m_currentOpcode;

	struct VMSRegister {
		Compiler::VMExitCode exitCode;
		AsTL::IDX progCounter;
		AsTL::I16 stackSzKB;
		Compiler::RawBitmaskT insMask;
		Compiler::RawBitmaskT dbgMask;

		
		FORCE_INLINE static VMSRegister Decode(uint64_t sprVal) {
			return VMSRegister{
				.exitCode		= static_cast<decltype(VMSRegister::exitCode)>((sprVal >> 0) & 0xFFFF),
				.progCounter	= static_cast<decltype(VMSRegister::progCounter)>((sprVal >> 16) & 0xFFFF),
				.stackSzKB		= static_cast<decltype(VMSRegister::stackSzKB)>((sprVal >> 32) & 0xFFFF),
				.insMask		= static_cast<decltype(VMSRegister::insMask)>((sprVal >> 48) & 0xFF),
				.dbgMask		= static_cast<decltype(VMSRegister::dbgMask)>((sprVal >> 56) & 0xFF)
			};
		}


		FORCE_INLINE uint64_t encode() const {
			return	((static_cast<uint64_t>(exitCode) & 0xFFFF) << 0) |
					((static_cast<uint64_t>(progCounter) & 0xFFFF) << 16) |
					((static_cast<uint64_t>(stackSzKB) & 0xFFFF) << 32) |
					((static_cast<uint64_t>(insMask) & 0xFF) << 48) |
					((static_cast<uint64_t>(dbgMask) & 0xFF) << 56);
		}
	};
	VMSRegister m_vmsSPR;


	struct StackFrame {
		AsTL::IDX returnAddr;
		std::vector<AsTL::StackValue> m_locReg;
	};
	std::vector<StackFrame> m_callStack;


	/* Start execution at a specific instruction address. */
	Compiler::VMExitCode executeAt(AsTL::IDX insAddress);


	/* Writes to the call stack. */
	FORCE_INLINE void writeToCallStack(StackFrame &&frame);

	/* Pops from the call stack. */
	FORCE_INLINE StackFrame popFromCallStack();

	/* Writes a raw value to the VM stack. */
	FORCE_INLINE void writeToVMStack(const uint64_t val);

	/* Writes a variant to the VM stack.
		@param sourceContainer: The type of the source container that holds the variant,
			if the variant holds a value that is arbitrarily sized (e.g., string) rather than fixed-size (e.g., integral types)
		@param idx: The variant's index into the container 
	*/
	FORCE_INLINE void writeToVMStack(const AsTL::StackValue &variantVal, Compiler::ContainerType sourceContainer, AsTL::IDX idx);

	/* Pops from the VM stack.
		@param cnt (Default: 1): How many elements to pop from the stack.
	*/
	FORCE_INLINE void popFromVMStack(size_t cnt = 1);

	/* Reinterprets the bit representation of the top VM stack slot as a concrete type, then pops it off from the stack. */
	template<typename T> FORCE_INLINE T popFromVMStackAs(Compiler::OperandType opType);

	/* Reinterprets the bit representation of the top VM stack slot as a concrete type.
		@param opType: The type to reinterpret the stack slot as
		@param consumed (Default: nullptr): An output integral value representing how many stack slots were read during the cast.
	*/
	template<typename T> FORCE_INLINE T castFromStack(Compiler::OperandType opType, AsTL::IDX *consumed = nullptr);

	/* Reinterprets the bit representation of a VM stack slot as a concrete type.
		@param sp: The pointer/index to the stack slot
		@param opType: The type to reinterpret the stack slot as
		@param consumed (Default: nullptr): An output integral value representing how many stack slots were read during the cast.
	*/
	template<typename T> FORCE_INLINE T castFromStack(AsTL::IDX sp, Compiler::OperandType opType, AsTL::IDX *consumed = nullptr);

	/* castFromStack implementation */
	FORCE_INLINE AsTL::StackValue castFromStack_Impl(AsTL::IDX sp, Compiler::OperandType opType, AsTL::IDX *consumed = nullptr);

	/* Performs a boundary check for a given index-based container at a given index. */
	template<typename Container, typename Idx>
	requires CompilerUtils::HasSizeMethod<Container>&& std::is_integral_v<Idx>
	FORCE_INLINE void checkBoundsFor(const Container& container, Idx index);

	/* Sets the program counter (at runtime). */
	FORCE_INLINE void setProgramCounterRT(AsTL::IDX newAddr);

	/* Decodes a string from an encoded VM stack value. */
	FORCE_INLINE AsTL::STR decodeString(uint64_t val);

	/* Logic for all unary arithmetic and logical operations */
	template <typename T>
	void unarySwitchIns(Compiler::OperandType t);

	/* Logic for all binary arithmetic and logical operations */
	template <typename LEFT_T, typename RIGHT_T>
	void binarySwitchIns(Compiler::OperandType lt, Compiler::OperandType rt);

	/* Reinterprets the binary representation of a (smaller- or equal-size) floating-point value as that of a stack element value (uint64_t).
		The reinterpretation preserves the original value's binary pattern, and performs zero-extension if necessary.
	*/
	template <typename T>
	requires std::is_floating_point_v<T> && (sizeof(T) <= sizeof(uint64_t))
	FORCE_INLINE uint64_t floatingPointToStackElem(T val) const;

	/* Reinterprets the binary representation of a (smaller- or equal-size) integral value as that of a stack element value (uint64_t).
		The reinterpretation preserves the original value's binary pattern, and performs zero-extension if necessary.
	*/
	template <typename T>
	requires std::is_integral_v<T> && (sizeof(T) <= sizeof(uint64_t))
	FORCE_INLINE uint64_t integralToStackElem(T val) const;

	/* Reinterprets the binary representation of a (smaller- or equal-size) numeric variant value as that of a stack element value (uint64_t).
		@tparam FirstT: The numeric type to first cast the variant's value to before reinterpreting that value as a stack element value.
	*/
	template <typename FirstT>
	FORCE_INLINE uint64_t numericVariantToStackElem(const AsTL::StackValue &val);

	/* Reinterprets the binary representation of a stack element value (uint64_t) as that of a (smaller- or equal-size) floating-point value. */
	template <typename T>
	requires std::is_floating_point_v<T> && (sizeof(T) <= sizeof(uint64_t))
	FORCE_INLINE T stackElemToFloatingPoint(uint64_t val) const;

	/* Reinterprets the binary representation of a stack element value (uint64_t) as that of a (smaller- or equal-size) integral value. */
	template <typename T>
	requires std::is_integral_v<T> && (sizeof(T) <= sizeof(uint64_t))
	FORCE_INLINE T stackElemToIntegral(uint64_t val) const;

	/* Saves the VM state. */
	FORCE_INLINE void saveVMState(std::optional<Compiler::VMExitCode> exitCode = std::nullopt);

	/* Tests whether a bit in a bitmask is flipped to 1 (True), or 0 (False). */
	FORCE_INLINE bool testBit(const Compiler::RawBitmaskT bitmask, const int bit) const;

	/* Converts a value to its string representation. */
	std::string variantToString(const AsTL::StackValue &val) const;

	/* Clears specific VM memory containers in preparation for program execution. */
	void resetMemPreExec();
};


template<typename T>
inline FORCE_INLINE T VirtualMachine::popFromVMStackAs(Compiler::OperandType opType) {
	AsTL::IDX consumed{};
	const T v = castFromStack<T>(opType, &consumed);

	popFromVMStack(consumed);

	return v;
}


template<typename T>
inline FORCE_INLINE T VirtualMachine::castFromStack(Compiler::OperandType opType, AsTL::IDX *consumed) {
	const auto &v = castFromStack_Impl(m_vsp, opType, consumed);
	if constexpr (CompilerUtils::AlternativeOf<T, AsTL::StackValue>)
		return std::get<T>(v);
	if constexpr (std::is_same_v<T, AsTL::StackValue>)
		return v;

	saveVMState(Compiler::VMExitCode::BAD_CAST);
	throw VMRuntimeException("At instruction address 0x{:0>{}X}: Attempted to cast VM stack value to unrecognized type ({})",
		m_pc - 1, ADDR_HEX_SZ,
		typeid(T).name()
	);
}


template<typename T>
inline FORCE_INLINE T VirtualMachine::castFromStack(AsTL::IDX sp, Compiler::OperandType opType, AsTL::IDX *consumed) {
	const auto &v = castFromStack_Impl(sp, opType, consumed);
	if constexpr (CompilerUtils::AlternativeOf<T, AsTL::StackValue>)
		return std::get<T>(v);
	if constexpr (std::is_same_v<T, AsTL::StackValue>)
		return v;

	saveVMState(Compiler::VMExitCode::BAD_CAST);
	throw VMRuntimeException("At instruction address 0x{:0>{}X}: Attempted to cast VM stack value to unrecognized type ({})",
		m_pc - 1, ADDR_HEX_SZ,
		typeid(T).name()
	);
}


template<typename T>
requires std::is_floating_point_v<T> && (sizeof(T) <= sizeof(uint64_t))
inline FORCE_INLINE uint64_t VirtualMachine::floatingPointToStackElem(T val) const {
	// Bit-cast the floating point type to a size-equivalent unsigned integral type, then cast that to uint64_t for zero-extension

	using SameSizeUint = std::conditional_t<
		sizeof(T) == 4, uint32_t,
		std::conditional_t<
			sizeof(T) == 8, uint64_t,
			uint16_t
		>
	>;

	SameSizeUint bits = std::bit_cast<SameSizeUint>(val);

	return static_cast<uint64_t>(bits);
}


template<typename T>
requires std::is_integral_v<T> && (sizeof(T) <= sizeof(uint64_t))
inline FORCE_INLINE uint64_t VirtualMachine::integralToStackElem(T val) const {
	if constexpr (!std::is_same_v<T, AsTL::BOOL>)
		// For a non-boolean integral type, cast it to an equivalent unsigned type (to preserve the binary pattern),
		// then cast that to uint64_t for zero-extension 
		return static_cast<uint64_t>(static_cast<std::make_unsigned_t<decltype(val)>>(val));

	// For booleans, just cast them to uint64_t directly
	return static_cast<uint64_t>(val);
}


template<typename T>
requires std::is_floating_point_v<T> && (sizeof(T) <= sizeof(uint64_t))
inline FORCE_INLINE T VirtualMachine::stackElemToFloatingPoint(uint64_t val) const {
	// Cast the stack element type to an unsigned integral type with the same size as the destination floating-point type
	// (which also truncates the higher bits), then bit-cast the result to the floating-point type

	using SameSizeUint = std::conditional_t<
		sizeof(T) == 4, uint32_t,
		std::conditional_t<
			sizeof(T) == 8, uint64_t,
			uint16_t
		>
	>;

	auto narrowed = static_cast<SameSizeUint>(val);

	return std::bit_cast<T>(narrowed);
}


template<typename T>
requires std::is_integral_v<T> && (sizeof(T) <= sizeof(uint64_t))
inline FORCE_INLINE T VirtualMachine::stackElemToIntegral(uint64_t val) const {
	if constexpr (!std::is_same_v<T, AsTL::BOOL>)
		return std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(val));

	return static_cast<T>(val);
}
