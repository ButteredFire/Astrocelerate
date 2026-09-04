#include "VirtualMachine.hpp"


VirtualMachine::VirtualMachine(
	const Compiler::ConstantPool &constPool,
	const std::reference_wrapper<Compiler::IGraphNodeRegistry> nodeRegistry,
	AsTL::OpaqueExecCtx *execCtx,
	const size_t vmStackSzKB,
	const size_t callStackSzKB
) :
	m_allocVMStackSzKB(vmStackSzKB),
	m_allocCallStackSzKB(callStackSzKB),
	m_constPool(constPool),
	m_nodeRegistry(nodeRegistry.get()),
	m_execCtx(execCtx),
	m_instructions(nullptr),
	m_SPRs(), m_vmsSPR(),
	m_pc(0x00),
	m_vsp(-1), // The stack pointer is zero-indexed, so it would point to -1 for empty stacks
	m_csp(-1),
	m_currentOpcode()
{
	size_t vmStackSzBytes = m_allocVMStackSzKB * 1000;
	size_t callStackSzBytes = m_allocCallStackSzKB * 1000;

	// Stack size must be CompilerUtils::Divisible by the size of each stack element
	if (vmStackSzBytes % sizeof(uint64_t) != 0) {
		saveVMState(Compiler::VMExitCode::CRASHED_CONFIG);
		throw VMConfigException("Virtual machine configuration error: allocated VM stack size must be a multiple of {} bytes",
			std::to_string(sizeof(uint64_t))
		);
	}
	if (callStackSzBytes % sizeof(uint64_t) != 0) {
		saveVMState(Compiler::VMExitCode::CRASHED_CONFIG);
		throw VMConfigException("Virtual machine configuration error: allocated call stack size must be a multiple of {} bytes",
			std::to_string(sizeof(uint64_t))
		);
	}

	// VM Stack size must be less than 512 KB
	if (m_allocVMStackSzKB > 512) {
		saveVMState(Compiler::VMExitCode::CRASHED_CONFIG);
		throw VMConfigException("Virtual machine configuration error: allocated VM stack size must not exceed 512 KB");
	}

	// Call Stack size must be less than 1000 KB (1 MB)
	if (m_allocCallStackSzKB > 1000) {
		saveVMState(Compiler::VMExitCode::CRASHED_CONFIG);
		throw VMConfigException("Virtual machine configuration error: allocated call stack size must not exceed 1000 KB");
	}

	m_vmStack.reserve(vmStackSzBytes / sizeof(uint64_t));
	m_callStack.reserve(callStackSzBytes / sizeof(uint64_t));

	m_allocFuncListSz = 20;
	m_funcArgs.resize(m_allocFuncListSz);
	m_funcRets.resize(m_allocFuncListSz);

	// Default fallback exit code is EXEC_HALTED.
	// It should be updated to a concrete exit code upon VM exceptions or `TERMINATE` instructions
	m_vmsSPR.exitCode = Compiler::VMExitCode::EXEC_HALTED;
}


void VirtualMachine::setProgram(const std::vector<Compiler::RawInstructionT>& instructions) {
	m_instructions = &instructions;
}


Compiler::VMExitCode VirtualMachine::execute(const std::vector<Compiler::RawInstructionT>& instructions) {
	m_pc = 0;
	m_vsp = -1;

	STATIC_BLOCK_TRACKER.resetStaticBlocks();
	
	m_instructions = &instructions;

	return executeAt(m_pc);
}


Compiler::VMExitCode VirtualMachine::execute() {
	m_pc = 0;
	m_vsp = -1;

	STATIC_BLOCK_TRACKER.resetStaticBlocks();

	return executeAt(m_pc);
}


Compiler::VMExitCode VirtualMachine::resume() {
	return executeAt(m_pc);
}


Compiler::VMExitCode VirtualMachine::executeAt(AsTL::IDX insAddress) {
	try {
		if (!m_instructions) {
			saveVMState(Compiler::VMExitCode::CRASHED_RT);
			throw VMRuntimeException("Cannot begin or resume execution: No program has been set");
		}

		const auto& instructions = *m_instructions;
		
		if (instructions.size() >= std::numeric_limits<AsTL::IDX>::max()) {
			saveVMState(Compiler::VMExitCode::CRASHED_RT);
			throw VMRuntimeException("Cannot begin or resume execution: Program size is larger than the virtual machine's addressable byte range");
		}
		
		/* Is the instruction's operation read-only? */
		const auto insReadOnly = [&]() -> bool {
			return testBit(m_vmsSPR.insMask, Compiler::BITFLAG_READ_ONLY_BIT);
		};

		AsTL::BYTE leftOp{}, rightOp{};  // VM stack: [leftOp, rightOp]

		while (true) {
			if (m_pc >= instructions.size()) {
				saveVMState(Compiler::VMExitCode::EXEC_HALTED);
				return m_vmsSPR.exitCode;
			}

			Compiler::Instruction ins(instructions[m_pc]);
			++m_pc;

			m_vmsSPR.insMask = ins.bitmask;
			m_currentOpcode = static_cast<Compiler::Opcode>(ins.opcode);

			// Also reinterpret the instruction operand as 2 operands
			leftOp = static_cast<AsTL::BYTE>((ins.operand >> 8) & 0xFF);
			rightOp = static_cast<AsTL::BYTE>(ins.operand & 0xFF);

			using enum Compiler::Opcode;
			switch (m_currentOpcode) {
			case LOAD_INLINE:
				writeToVMStack(std::bit_cast<AsTL::I16>(ins.operand), Compiler::CT_VM_STACK, m_vmStack.size());
				break;

			case POP:
			{
				popFromVMStack(std::bit_cast<AsTL::I16>(ins.operand));
				break;
			}

			case LOAD_SPR:
			{
				checkBoundsFor(m_SPRs, rightOp);
				writeToVMStack(m_SPRs[rightOp]);
				break;
			}

			case STORE_SPR:
			{
				if (m_vmStack.empty()) {
					saveVMState(Compiler::VMExitCode::OUT_OF_BOUNDS);
					throw VMRuntimeException("At instruction address 0x{:0>{}X}: Invalid pop from empty VM stack",
						m_pc - 1, ADDR_HEX_SZ
					);
				}

				checkBoundsFor(m_SPRs, rightOp);
				m_SPRs[rightOp] = m_vmStack.back();

				if (!insReadOnly())
					popFromVMStack();
				
				break;
			}

			case LOAD_CONST:
			{
				AsTL::IDX idx = std::bit_cast<AsTL::IDX>(ins.operand);
				const auto &val = m_constPool.getValue(idx);
				writeToVMStack(val, Compiler::CT_CONST_POOL, idx);

				break;
			}

			case LOAD_GL:
			{
				AsTL::IDX idx = std::bit_cast<AsTL::IDX>(ins.operand);

				checkBoundsFor(m_globReg, idx);
				const auto &val = m_globReg[idx];

				writeToVMStack(val, Compiler::CT_GLOBAL_REG, idx);

				break;
			}

			case LOAD_LC:
				// TODO
				break;

			case STORE_GL:
			{
				AsTL::IDX consumed = 0;
				m_globReg.push_back(
					castFromStack<AsTL::StackValue>(static_cast<Compiler::OperandType>(rightOp), &consumed)
				);

				if (!insReadOnly())
					popFromVMStack(consumed);

				break;
			}

			case STORE_LC:
				// TODO
				break;

			case OVR_GL:
			{
				AsTL::IDX idx = std::bit_cast<AsTL::IDX>(ins.operand);
				AsTL::IDX consumed = 0;

				checkBoundsFor(m_globReg, idx);
				m_globReg[idx] = castFromStack<AsTL::StackValue>(
					static_cast<Compiler::OperandType>(
						Compiler::StackValueToByte(m_globReg[idx])
					),
					&consumed
				);

				if (!insReadOnly())
					popFromVMStack(consumed);
				
				break;
			}

			case OVR_LC:
				// TODO
				break;

			case TO_STR:
			{
				AsTL::IDX consumed = 0;
				const auto &val = castFromStack<AsTL::StackValue>(static_cast<Compiler::OperandType>(rightOp), &consumed);

				m_strHeap.push_back(variantToString(val));
				popFromVMStack(consumed);
				writeToVMStack(m_strHeap.back(), Compiler::NONSTANDARD_CT_DEDICATED_STR_HEAP, m_strHeap.size() - 1);

				break;
			}

			case TO_I16:
			{
				AsTL::IDX consumed = 0;
				const auto &val = castFromStack<AsTL::StackValue>(static_cast<Compiler::OperandType>(rightOp), &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(
					numericVariantToStackElem<AsTL::I16>(val)
				);

				break;
			}

			case TO_I32:
			{
				AsTL::IDX consumed = 0;
				const auto &val = castFromStack<AsTL::StackValue>(static_cast<Compiler::OperandType>(rightOp), &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(
					numericVariantToStackElem<AsTL::I32>(val)
				);

				break;
			}

			case TO_F64:
			{
				AsTL::IDX consumed = 0;
				const auto &val = castFromStack<AsTL::StackValue>(static_cast<Compiler::OperandType>(rightOp), &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(
					numericVariantToStackElem<AsTL::F64>(val)
				);

				break;
			}

			case PRINT:
			{
				AsTL::StackValue val = castFromStack<AsTL::StackValue>(static_cast<Compiler::OperandType>(rightOp));
				AsTL::STR str = variantToString(val);

				std::cout << str << '\n';

				break;
			}

			case EXEC_NATIVE:
			{
				const Compiler::GraphNodeDescriptor &desc = m_nodeRegistry.getInfo(std::bit_cast<AsTL::IDX>(ins.operand));

				size_t retListSz = desc.outExecs.size() + desc.outParams.size();

				if (retListSz > m_allocFuncListSz) {
					m_allocFuncListSz = retListSz;
					m_funcArgs.resize(m_allocFuncListSz);
					m_funcRets.resize(m_allocFuncListSz);
				}

				// Node ID
				AsTL::IDX nodeIDConsumed{};
				Graph::NodeID nodeID = static_cast<Graph::NodeID>(
					castFromStack<AsTL::I16>(Compiler::OT_I16, &nodeIDConsumed)
				);

				// Arguments
				AsTL::IDX offset = nodeIDConsumed;
				for (size_t i = desc.inParams.size(); i-- > 0;) {
					AsTL::IDX slotsConsumed = 0;

					std::type_index argType = desc.inParams[i].type;
					if (Graph::IsNonPrimitive(argType))
						argType = Graph::HighLevelTypeToPrimitive(argType);

					m_funcArgs[i] = castFromStack<AsTL::StackValue>(
						m_vsp - offset,
						static_cast<Compiler::OperandType>(Compiler::StackValueToByte(argType)),
						&slotsConsumed
					);

					offset += slotsConsumed;
				}

				// Triggered pin
				std::string triggeredPin = decodeString(m_SPRs[Compiler::SPR_EXC]);

				// Callback invocation
				std::get<Compiler::GraphNodeCallback>(desc.callback.value())(
					m_execCtx, &desc, nodeID, triggeredPin, m_funcArgs, m_funcRets.data()
				);


				// Stack updates
				if (!insReadOnly())
					popFromVMStack(offset);

				for (size_t i = 0; i < retListSz; ++i) {
					const auto& val = m_funcRets[i];

					if (std::holds_alternative<AsTL::STR>(val)) {
						m_strHeap.push_back(std::get<AsTL::STR>(val));
						writeToVMStack(val, Compiler::NONSTANDARD_CT_DEDICATED_STR_HEAP, m_strHeap.size() - 1);
					}
					else
						writeToVMStack(val, Compiler::CT_VM_STACK, m_vmStack.size() - 1);
				}

				break;
			}

			case ADD:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::Addable);
				break;
			}

			case SUB:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::Subtractable);
				break;
			}


			case MUL:
			case VEC_MUL:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::Multipliable);
				break;
			}

			case DIV:
			case VEC_DIV:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::Divisible);
				break;
			}

			case MOD:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CanDoModulo);
				break;
			}
			
			case NEG:
			{
				SWITCH_TYPES(rightOp, unarySwitchIns, CompilerUtils::Negatable);
				break;
			}

			case STR_CAT:
			{
				AsTL::IDX consumed1{}, consumed2{};

				AsTL::StackValue slot1 = castFromStack<AsTL::StackValue>(m_vsp, static_cast<Compiler::OperandType>(leftOp), &consumed1);
				AsTL::StackValue slot2 = castFromStack<AsTL::StackValue>(m_vsp - 1, static_cast<Compiler::OperandType>(rightOp), &consumed2);

				if (!insReadOnly())
					popFromVMStack(consumed1 + consumed2);

				AsTL::STR str1 = variantToString(slot1);
				AsTL::STR str2 = variantToString(slot2);

				// Concatenation order follows the same FIFO principle as outlined in the ISA
				m_strHeap.push_back(str2 + str1);

				writeToVMStack(m_strHeap.back(), Compiler::NONSTANDARD_CT_DEDICATED_STR_HEAP, m_strHeap.size() - 1);

				break;
			}

			case VEC_NORM:
			{
				AsTL::IDX consumed{};
				AsTL::VEC3 vec = castFromStack<AsTL::VEC3>(Compiler::OT_VEC3, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(vec.norm(), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case VEC_MAG:
			{
				AsTL::IDX consumed{};
				AsTL::VEC3 vec = castFromStack<AsTL::VEC3>(Compiler::OT_VEC3, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(vec.mag(), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case VEC_MUL_DOT:
			{
				AsTL::IDX consumed1{}, consumed2{};
				AsTL::VEC3 vec1 = castFromStack<AsTL::VEC3>(m_vsp, Compiler::OT_VEC3, &consumed1);
				AsTL::VEC3 vec2 = castFromStack<AsTL::VEC3>(m_vsp - 1, Compiler::OT_VEC3, &consumed2);

				if (!insReadOnly())
					popFromVMStack(consumed1 + consumed2);

				writeToVMStack(vec1.dot(vec2), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case VEC_MUL_CROSS:
			{
				AsTL::IDX consumed1{}, consumed2{};
				AsTL::VEC3 vec1 = castFromStack<AsTL::VEC3>(m_vsp, Compiler::OT_VEC3, &consumed1);
				AsTL::VEC3 vec2 = castFromStack<AsTL::VEC3>(m_vsp - 1, Compiler::OT_VEC3, &consumed2);

				if (!insReadOnly())
					popFromVMStack(consumed1 + consumed2);

				writeToVMStack(vec1.cross(vec2), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case CMP_LT:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CompLessThan);
				break;
			}

			case CMP_LTE:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CompLessThanEqualTo);
				break;
			}

			case CMP_GT:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CompGreaterThan);
				break;
			}

			case CMP_GTE:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CompGreaterThanEqualTo);
				break;
			}

			case CMP_EQ:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CompEqualTo);
				break;
			}

			case CMP_NEQ:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::CompNotEqualTo);
				break;
			}

			case LGC_AND:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::LogicalAnd);
				break;
			}

			case LGC_OR:
			{
				SWITCH_TYPES_PERMUT(leftOp, rightOp, binarySwitchIns, CompilerUtils::LogicalOr);
				break;
			}

			case LGC_NOT:
			{
				SWITCH_TYPES(rightOp, unarySwitchIns, CompilerUtils::LogicalNot);
				break;
			}

			case SIN:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::sin(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case ASIN:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::asin(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case COS:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::cos(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case ACOS:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::acos(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case TAN:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::tan(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case ATAN:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::atan(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case ATAN2:
			{
				AsTL::IDX consumed1{}, consumed2{};
				AsTL::F64 x = castFromStack<AsTL::F64>(m_vsp, Compiler::OT_F64, &consumed1);
				AsTL::F64 y = castFromStack<AsTL::F64>(m_vsp - 1, Compiler::OT_F64, &consumed2);

				if (!insReadOnly())
					popFromVMStack(consumed1 + consumed2);

				writeToVMStack(std::atan2(y, x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case COT:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack((1.0 / std::tan(x)), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case ACOT:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::atan2(1, x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case ABS:
			{
				AsTL::IDX consumed{};
				AsTL::F64 x = castFromStack<AsTL::F64>(Compiler::OT_F64, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				writeToVMStack(std::abs(x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case MIN:
			{
				AsTL::IDX consumed1{}, consumed2{};
				AsTL::F64 x = castFromStack<AsTL::F64>(m_vsp, Compiler::OT_F64, &consumed1);
				AsTL::F64 y = castFromStack<AsTL::F64>(m_vsp - 1, Compiler::OT_F64, &consumed2);

				if (!insReadOnly())
					popFromVMStack(consumed1 + consumed2);

				writeToVMStack((std::min)(y, x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case MAX:
			{
				AsTL::IDX consumed1{}, consumed2{};
				AsTL::F64 x = castFromStack<AsTL::F64>(m_vsp, Compiler::OT_F64, &consumed1);
				AsTL::F64 y = castFromStack<AsTL::F64>(m_vsp - 1, Compiler::OT_F64, &consumed2);

				if (!insReadOnly())
					popFromVMStack(consumed1 + consumed2);

				writeToVMStack((std::max)(y, x), Compiler::CT_GLOBAL_REG, m_vmStack.size() - 1);

				break;
			}

			case JUMP:
			{
				setProgramCounterRT(std::bit_cast<AsTL::IDX>(ins.operand));
				break;
			}

			case JUMP_IF_TRUE:
			{
				AsTL::IDX consumed{};
				AsTL::BOOL cond = castFromStack<AsTL::BOOL>(Compiler::OT_BOOL, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				if (cond)
					setProgramCounterRT(std::bit_cast<AsTL::IDX>(ins.operand));

				break;
			}

			case JUMP_IF_FALSE:
			{
				AsTL::IDX consumed{};
				AsTL::BOOL cond = castFromStack<AsTL::BOOL>(Compiler::OT_BOOL, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				if (!cond)
					setProgramCounterRT(std::bit_cast<AsTL::IDX>(ins.operand));

				break;
			}

			case CALL:
			{
				writeToCallStack(StackFrame{
					.returnAddr = m_pc
				});

				setProgramCounterRT(std::bit_cast<AsTL::IDX>(ins.operand));

				break;
			}

			case CALL_IF_TRUE:
			{
				AsTL::IDX consumed{};
				AsTL::BOOL cond = castFromStack<AsTL::BOOL>(Compiler::OT_BOOL, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				if (cond) {
					writeToCallStack(StackFrame{
						.returnAddr = m_pc
					});

					setProgramCounterRT(std::bit_cast<AsTL::IDX>(ins.operand));
				}

				break;
			}

			case CALL_IF_FALSE:
			{
				AsTL::IDX consumed{};
				AsTL::BOOL cond = castFromStack<AsTL::BOOL>(Compiler::OT_BOOL, &consumed);

				if (!insReadOnly())
					popFromVMStack(consumed);

				if (!cond) {
					writeToCallStack(StackFrame{
						.returnAddr = m_pc
					});
					
					setProgramCounterRT(std::bit_cast<AsTL::IDX>(ins.operand));
				}

				break;
			}

			case RET:
			{
				StackFrame frame = popFromCallStack();
				setProgramCounterRT(frame.returnAddr);

				break;
			}

			case TERMINATE:
			{
				Compiler::VMExitCode exitCode = static_cast<Compiler::VMExitCode>(static_cast<AsTL::I16>(ins.operand));

				saveVMState(exitCode);
				return m_vmsSPR.exitCode;
			}

			default:
			{
				saveVMState(Compiler::VMExitCode::CRASHED_RT);
				throw VMRuntimeException("At instruction address 0x{:0>{}X}: Invalid opcode in instruction stream (hex: 0x{:0>{}X})",
					m_pc - 1, ADDR_HEX_SZ,
					ins.opcode, BYTE_HEX_SZ
				);
			}
			}
		}
	}
	catch (const VMRuntimeException &) {
		// Propagate exception
		throw;
	}
	catch (const VMFinishedExecTick &) {
		throw;
	}
	catch (const std::exception &e) {
		saveVMState(Compiler::VMExitCode::CRASHED_RT);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Unhandled exception during execution ({})",
			m_pc - 1, ADDR_HEX_SZ,
			e.what()
		);
	}


	//return m_vmsSPR.exitCode;
	// The execution loop is an infinite loop that only exits when the program counter reaches a TERMINATE instruction, or an exception is thrown.
}


FORCE_INLINE void VirtualMachine::writeToCallStack(StackFrame &&frame) {
	if ((m_callStack.size() + 1) * sizeof(uint64_t) > m_allocCallStackSzKB * 1000) {
		saveVMState(Compiler::VMExitCode::CALL_STACK_OVERFLOW);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Encountered call stack overflow",
			m_pc - 1, ADDR_HEX_SZ
		);
	}

	m_callStack.push_back(frame);
	++m_csp;
}


FORCE_INLINE VirtualMachine::StackFrame VirtualMachine::popFromCallStack() {
	StackFrame frame = m_callStack.back();
	m_callStack.pop_back();
	--m_csp;

	return frame;
}


FORCE_INLINE void VirtualMachine::writeToVMStack(uint64_t val) {
	if ((m_vmStack.size() + 1) * sizeof(uint64_t) > m_allocVMStackSzKB * 1000) {
		saveVMState(Compiler::VMExitCode::VM_STACK_OVERFLOW);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Encountered virtual machine stack overflow",
			m_pc - 1, ADDR_HEX_SZ
		);
	}

	m_vmStack.push_back(val);
	++m_vsp;
}


FORCE_INLINE void VirtualMachine::writeToVMStack(const AsTL::StackValue &variantVal, Compiler::ContainerType sourceContainer, AsTL::IDX idx) {
	std::visit(CompilerUtils::OverloadedVisit {
		[&](const AsTL::BOOL &val)	{ writeToVMStack(integralToStackElem(val)); },
		[&](const AsTL::IDX &val)	{ writeToVMStack(integralToStackElem(val)); },
		[&](const AsTL::I16 &val)	{ writeToVMStack(integralToStackElem(val)); },
		[&](const AsTL::I32 &val)	{ writeToVMStack(integralToStackElem(val)); },
		[&](const AsTL::F64 &val)	{ writeToVMStack(floatingPointToStackElem(val)); },
		[&](const AsTL::VEC3 &val)	{
			// VEC3 VM stack order: [x, y, z]
			writeToVMStack(floatingPointToStackElem(val.x));
			writeToVMStack(floatingPointToStackElem(val.y));
			writeToVMStack(floatingPointToStackElem(val.z));
		},
		[&](const AsTL::STR &val) {
			uint64_t packedVal =	static_cast<uint64_t>((integralToStackElem(static_cast<AsTL::BYTE>(sourceContainer)) & 0xFF) << 56) |
									static_cast<uint64_t>((integralToStackElem(idx) & 0xFFFF) << 0);

			writeToVMStack(packedVal);
		}
	}, variantVal);
}


FORCE_INLINE void VirtualMachine::popFromVMStack(size_t cnt) {
	checkBoundsFor(m_vmStack, m_vmStack.size() - cnt);

	for (size_t i = 0; i < cnt; ++i) {
		m_vmStack.pop_back();
	}
	m_vsp -= cnt;
}


FORCE_INLINE AsTL::StackValue VirtualMachine::castFromStack_Impl(AsTL::IDX sp, Compiler::OperandType opType, AsTL::IDX *consumed) {
	if (sp >= m_vmStack.size()) {
		saveVMState(Compiler::VMExitCode::OUT_OF_BOUNDS);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Out-of-bounds VM stack access at index {}",
			m_pc - 1, ADDR_HEX_SZ,
			sp
		);
	}

	uint64_t stackVal = m_vmStack[sp];

	auto setConsumed = [&](AsTL::IDX v) -> void {
		if (consumed)
			*consumed = v;
	};


	using enum Compiler::OperandType;
	switch (opType) {
	case OT_BYTE:
	{
		setConsumed(1);
		return stackElemToIntegral<AsTL::BYTE>(stackVal);
	}
	case OT_BOOL:
	{
		setConsumed(1);
		return stackElemToIntegral<AsTL::BOOL>(stackVal);
	}
	case OT_IDX:
	{
		setConsumed(1);
		return stackElemToIntegral<AsTL::IDX>(stackVal);
	}
	case OT_I16:
	{
		setConsumed(1);
		return stackElemToIntegral<AsTL::I16>(stackVal);
	}
	case OT_I32:
	{
		setConsumed(1);
		return stackElemToIntegral<AsTL::I32>(stackVal);
	}
	case OT_F64:
	{
		setConsumed(1);
		return stackElemToFloatingPoint<AsTL::F64>(stackVal);
	}

	case OT_VEC3:
	{
		setConsumed(3);

		// VEC3 VM stack order: [x, y, z]
		return AsTL::VEC3(
			castFromStack<AsTL::F64>(sp - 2, Compiler::OT_F64),
			castFromStack<AsTL::F64>(sp - 1, Compiler::OT_F64),
			castFromStack<AsTL::F64>(sp, Compiler::OT_F64)
		);
	}
	case OT_STR:
	{
		setConsumed(1);

		// Strings are stored in the stack as an index into another container
		return decodeString(stackVal);
	}

	default:
	{
		saveVMState(Compiler::VMExitCode::CRASHED_RT);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Invalid stack cast to type \"{}\"",
			m_pc - 1, ADDR_HEX_SZ,
			Compiler::ByteToString(opType)
		);
	}
	}
}


template<typename Container, typename Idx>
requires CompilerUtils::HasSizeMethod<Container> && std::is_integral_v<Idx>
FORCE_INLINE void VirtualMachine::checkBoundsFor(const Container& container, Idx index) {
	if (index >= container.size()) {
		goto throw_err;
	}

	if constexpr (std::is_signed_v<Idx>) {
		if (index < 0) {
			goto throw_err;
		}
	}

	return;

throw_err:
	saveVMState(Compiler::VMExitCode::OUT_OF_BOUNDS);
	throw VMRuntimeException("At instruction address 0x{:0>{}X}: Invalid container access at index 0x{:0>{}X}",
		m_pc - 1, ADDR_HEX_SZ,
		index, BYTE_HEX_SZ
	);
}


FORCE_INLINE void VirtualMachine::setProgramCounterRT(AsTL::IDX newAddr) {
	if (newAddr >= m_instructions->size()) {
		saveVMState(Compiler::VMExitCode::OUT_OF_BOUNDS);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Attempted jump to invalid instruction at address 0x{:0>{}X}",
			m_pc - 1, ADDR_HEX_SZ,
			newAddr, ADDR_HEX_SZ
		);
	}

	m_pc = newAddr;
}


FORCE_INLINE AsTL::STR VirtualMachine::decodeString(uint64_t val) {
	Compiler::ContainerType ct = static_cast<Compiler::ContainerType>((val >> 56) & 0xFF);
	AsTL::IDX idx = stackElemToIntegral<AsTL::IDX>(val);

	try {
		switch (ct) {
		case Compiler::CT_GLOBAL_REG:
			return std::get<AsTL::STR>(m_globReg[idx]);

		case Compiler::CT_CONST_POOL:
			return std::get<AsTL::STR>(m_constPool.getValue(idx));

		case Compiler::CT_SPR:
			return decodeString(m_SPRs[Compiler::SPR_EXC]);

		
		case Compiler::NONSTANDARD_CT_DEDICATED_STR_HEAP:
			return m_strHeap[idx];


		// Unsupported source container types
		case Compiler::CT_LOCAL_REG:
		case Compiler::CT_VM_STACK:
		default:
			saveVMState(Compiler::VMExitCode::BAD_CAST);
			throw VMRuntimeException("At instruction address 0x{:0>{}X}: String value is sourced from an unsupported container (type code: 0x{:0>{}X})",
				m_pc - 1, ADDR_HEX_SZ,
				static_cast<AsTL::BYTE>(ct), BYTE_HEX_SZ
			);
		}
	}
	catch (const std::bad_variant_access &) {
		saveVMState(Compiler::VMExitCode::BAD_CAST);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Element at index {} of source container does not contain a string",
			m_pc - 1, ADDR_HEX_SZ,
			idx
		);
	}
}


template <typename T>
void VirtualMachine::unarySwitchIns(Compiler::OperandType t) {
	AsTL::IDX consumed{};
	T val = castFromStack<T>(t, &consumed);

	if (!testBit(m_vmsSPR.insMask, Compiler::BITFLAG_READ_ONLY_BIT))
		popFromVMStack(consumed);

	using enum Compiler::Opcode;
	switch (m_currentOpcode) {
	case NEG:
		if constexpr (CompilerUtils::Negatable<T>) {
			writeToVMStack(-val, Compiler::CT_VM_STACK, m_vmStack.size() - 1);
			break;
		}
		goto bad_unary_op;
		break;  // Break anyway to prevent fallthroughs

	case LGC_NOT:
		if constexpr (CompilerUtils::LogicalNot<T>) {
			writeToVMStack(!val, Compiler::CT_VM_STACK, m_vmStack.size() - 1);
			break;
		}
		goto bad_unary_op;
		break;

	default:
		saveVMState(Compiler::VMExitCode::CRASHED_RT);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Illegal unary operation", m_pc - 1, ADDR_HEX_SZ);
	}

	return;

bad_unary_op:
	saveVMState(Compiler::VMExitCode::BAD_CAST);
	throw VMRuntimeException("At instruction address 0x{:0>{}X}: Invalid operand type {} for unary operation",
		m_pc - 1, ADDR_HEX_SZ,
		Compiler::ByteToString(t)
	);
}


template <typename LEFT_T, typename RIGHT_T>
void VirtualMachine::binarySwitchIns(Compiler::OperandType lt, Compiler::OperandType rt) {
	AsTL::IDX consumed1{}, consumed2{};
	RIGHT_T slot1 = castFromStack<RIGHT_T>(m_vsp, rt, &consumed1);		// Top VM stack slot
	LEFT_T slot2 = castFromStack<LEFT_T>(m_vsp - 1, lt, &consumed2);	// Second-to-top VM stack slot

	if (!testBit(m_vmsSPR.insMask, Compiler::BITFLAG_READ_ONLY_BIT))
		popFromVMStack(consumed1 + consumed2);

	using enum Compiler::Opcode;
	switch (m_currentOpcode) {
	case ADD:
	{
		if constexpr (CompilerUtils::Addable<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 + slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case SUB:
	{
		if constexpr (CompilerUtils::Subtractable<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 - slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case MUL:
	{
		if constexpr (CompilerUtils::Multipliable<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 * slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case DIV:
	{
		if constexpr (CompilerUtils::Divisible<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 / slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case MOD:
	{
		if constexpr (CompilerUtils::CanDoModulo<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 % slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case CMP_LT:
	{
		if constexpr (CompilerUtils::CompLessThan<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 < slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case CMP_LTE:
	{
		if constexpr (CompilerUtils::CompLessThanEqualTo<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 <= slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case CMP_GT:
	{
		if constexpr (CompilerUtils::CompGreaterThan<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 > slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case CMP_GTE:
	{
		if constexpr (CompilerUtils::CompGreaterThanEqualTo<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 >= slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case CMP_EQ:
	{
		if constexpr (CompilerUtils::CompEqualTo<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 == slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case CMP_NEQ:
	{
		if constexpr (CompilerUtils::CompNotEqualTo<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 != slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case LGC_AND:
	{
		if constexpr (CompilerUtils::LogicalAnd<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 && slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	case LGC_OR:
	{
		if constexpr (CompilerUtils::LogicalOr<LEFT_T, RIGHT_T>) {
			writeToVMStack(slot2 || slot1, Compiler::CT_VM_STACK, m_vmStack.size());
			break;
		}
		goto bad_binary_op;
		break;
	}

	default:
		saveVMState(Compiler::VMExitCode::CRASHED_RT);
		throw VMRuntimeException("At instruction address 0x{:0>{}X}: Illegal binary operation", m_pc - 1, ADDR_HEX_SZ);
	}

	return;

bad_binary_op:
	saveVMState(Compiler::VMExitCode::BAD_CAST);
	throw VMRuntimeException("At instruction address 0x{:0>{}X}: Invalid operand types (left: {}, right: {}) for binary operation",
		m_pc - 1, ADDR_HEX_SZ,
		Compiler::ByteToString(lt), Compiler::ByteToString(rt)
	);
}


template <typename FirstT>
FORCE_INLINE uint64_t VirtualMachine::numericVariantToStackElem(const AsTL::StackValue &val) {
	uint64_t stackVal{};

	std::visit([&](const auto &v) {
		// Because the type of `v` is evaluated at compile time, but std::visit dynamically changes the type depending on the variant,
		// 
		/*
			At compile time, the compiler generates all overloads for std::visit and runs compile-time checks for each one (`if constexpr...`).
			However, since AsTL::StackValue includes AsTL::STR, `std::is_arithmetic_v<AsTL::STR>` fails, which results in compile errors.

			To solve this, we force a compile-time re-evaluation of the type of `v` with `ValueType`.
			That re-evaluation is a side effect of `std::decay_t`, which strips a type of references, cv-qualifiers, etc. to return the bare type.
		*/
		using ValueType = std::decay_t<decltype(v)>;

		if constexpr (std::is_arithmetic_v<ValueType>) {
			if constexpr (std::is_floating_point_v<FirstT> && sizeof(FirstT) <= sizeof(uint64_t)) {
				stackVal = floatingPointToStackElem(static_cast<FirstT>(v));
			}
			else if constexpr (std::is_integral_v<FirstT> && sizeof(FirstT) <= sizeof(uint64_t)) {
				stackVal = integralToStackElem(static_cast<FirstT>(v));
			}
			else {
				saveVMState(Compiler::VMExitCode::BAD_CAST);
				throw VMRuntimeException("At instruction address 0x{:0>{}X}: Unable to serialize value casted to type {} as a VM stack value",
					m_pc - 1, ADDR_HEX_SZ,
					Compiler::ByteToString(
						Compiler::StackValueToByte(typeid(FirstT))
					)
				);
			}
		}
		else {
			saveVMState(Compiler::VMExitCode::BAD_CAST);
			throw VMRuntimeException("At instruction address 0x{:0>{}X}: Unable to serialize value of type {} as a VM stack value",
				m_pc - 1, ADDR_HEX_SZ,
				Compiler::ByteToString(
					Compiler::StackValueToByte(typeid(ValueType))
				)
			);
		}
	}, val);

	return stackVal;
}


FORCE_INLINE void VirtualMachine::saveVMState(std::optional<Compiler::VMExitCode> exitCode) {
	if (exitCode.has_value())
		m_vmsSPR.exitCode = exitCode.value();

	m_vmsSPR.progCounter = m_pc;
	m_vmsSPR.stackSzKB = m_vmStack.size() * sizeof(uint64_t) / 1000;

	m_SPRs[Compiler::SPR_VMS] = m_vmsSPR.encode();
}


FORCE_INLINE bool VirtualMachine::testBit(const Compiler::RawBitmaskT bitmask, const int bit) const {
	return (bitmask & (1 << bit)) != 0;
}


std::string VirtualMachine::variantToString(const AsTL::StackValue &val) const {
	AsTL::STR str = "???";
	std::visit(CompilerUtils::OverloadedVisit {
		[&](const AsTL::BOOL &v) { str = v ? "True" : "False"; },
		[&](const AsTL::IDX &v) { str = std::format("0x{:0>{}X}", v, ADDR_HEX_SZ); },
		[&](const AsTL::I16 &v) { str = std::to_string(v); },
		[&](const AsTL::I32 &v) { str = std::to_string(v); },
		[&](const AsTL::F64 &v) { str = std::to_string(v); },
		[&](const AsTL::STR &val) { str = val; },

		[&](const AsTL::VEC3 &v) {
			// math vector notation
			str = std::format("({}, {}, {})", v.x, v.y, v.z);
		}
	}, val);

	return str;
}
