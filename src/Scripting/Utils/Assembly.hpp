#pragma once

#include <bit>
#include <vector>
#include <string>
#include <format>
#include <sstream>

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/Compiler/Instruction.hpp>

#include "Bytes.hpp"
#include "VariantHelpers.hpp"


namespace CompilerUtils {

	/* Disassembles an encoded AstroAssembly instruction.
		@param rawInstruction: The encoded AstroAssembly instruction.
		@return The decoded instruction.
	*/
	inline Compiler::SymbolicInstruction Disassemble(Compiler::RawInstructionT rawInstruction) {
		return Compiler::SymbolicInstruction(
			static_cast<Compiler::Opcode>((rawInstruction >> 24) & 0xFF),
			Compiler::InstructionMask((rawInstruction >> 16) & 0xFF),
			static_cast<AsTL::IDX>(rawInstruction & 0xFFFF)
		);
	}


	/* Assembles a symbolic AstroAssembly instruction.
		@param instruction: The symbolic AstroAssembly instruction.
		@return The encoded instruction.
	*/
	inline Compiler::RawInstructionT Assemble(const Compiler::SymbolicInstruction &instruction) {
		Compiler::RawOpcodeT  opcode  = static_cast<Compiler::RawOpcodeT>(instruction.opcode);
		Compiler::RawBitmaskT bitmask = static_cast<Compiler::RawBitmaskT>(	instruction.bitmask.has_value() ?
																			instruction.bitmask.value().to_ulong() :
																			Compiler::InstructionMask(0x00).to_ulong());

		Compiler::RawOperandT operand{ 0 };

		if (instruction.operand.has_value()) {
			const auto &v = instruction.operand.value();

			std::visit(CompilerUtils::OverloadedVisit {
				[&](AsTL::BYTE val)		{ operand = val; },
				[&](AsTL::IDX val)		{ operand = val; },
				[&](AsTL::I16 val)		{ operand = val; },
				[&](std::pair<AsTL::BYTE, AsTL::BYTE> val) {
					const auto &[high, low] = val;
					operand =	(static_cast<Compiler::RawOperandT>(high) << 8) | 
								(static_cast<Compiler::RawOperandT>(low) << 0);
				}
			}, v);
		}

		return	((static_cast<Compiler::RawInstructionT>(opcode) & 0xFF) << 24) |
				((static_cast<Compiler::RawInstructionT>(bitmask) & 0xFF) << 16) |
				((static_cast<Compiler::RawInstructionT>(operand) & 0xFFFF) << 0);
	}


	/* Formats symbolic instructions into corresponding strings for disassembly output.
		@param instructions: Symbolic instructions to format.
		@return Formatted strings representing the disassembled instructions.
	*/
	inline std::vector<std::string> FormatDisassembly(const std::vector<Compiler::SymbolicInstruction>& instructions) {
		static constexpr int MAX_OPCODE_W = 15;
		static constexpr int BITMASK_W = Compiler::BitmaskSz * 2 / 8;
		static constexpr int FULL_OPERAND_W = sizeof(Compiler::RawOperandT) * 2;
		static constexpr int HALF_OPERAND_W = FULL_OPERAND_W / 2;

		std::vector<std::string> lines{};
		lines.reserve(instructions.size());

		for (const auto &instruction : instructions) {
			std::string operandStr{};

			if (instruction.operand.has_value()) {
				std::visit(CompilerUtils::OverloadedVisit {
					[&](AsTL::BYTE val) {
						operandStr = std::format("0x{:0>{}X}", val, HALF_OPERAND_W);
					},
					[&](AsTL::IDX val) {
						operandStr = std::format("0x{:0>{}X}", val, FULL_OPERAND_W);
					},
					[&](AsTL::I16 val) {
						operandStr = std::format("{}", val);
					},
					[&](std::pair<AsTL::BYTE, AsTL::BYTE> val) {
						operandStr = std::format("0x{:0>{}X}, 0x{:0>{}X}",
							val.first, HALF_OPERAND_W,
							val.second, HALF_OPERAND_W
						);
					}
				}, instruction.operand.value());
			}

			std::ostringstream oss;

			// Opcode
			std::string opcodeStr = Compiler::OpcodeToString(instruction.opcode);
			oss << std::format("{:<{}}", opcodeStr, MAX_OPCODE_W);

			// Bitmask
			if (instruction.bitmask.has_value()) {
				oss << std::format("  0x{:0>{}X}",
					instruction.bitmask.value().to_ulong(), BITMASK_W
				);
			}

			// Operand
			if (!operandStr.empty()) {
				oss << "  " << operandStr;
			}

			lines.push_back(oss.str());
		}

		return lines;
	}

} // namespace CompilerUtils
