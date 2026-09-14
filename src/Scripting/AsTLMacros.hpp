#pragma once

#include <Scripting/Compiler/Instruction.hpp>

#include "AsTLTypes.hpp"


// AstroAssembly Data-type List as X-Macros
#define ASTL_TYPE_LIST(X, ...)											\
	X(Compiler::OperandType::OT_BOOL, AsTL::BOOL,	##__VA_ARGS__)		\
	X(Compiler::OperandType::OT_IDX, AsTL::IDX,		##__VA_ARGS__)		\
	X(Compiler::OperandType::OT_I16, AsTL::I16,		##__VA_ARGS__)		\
	X(Compiler::OperandType::OT_I32, AsTL::I32,		##__VA_ARGS__)		\
	X(Compiler::OperandType::OT_F64, AsTL::F64,		##__VA_ARGS__)		\
	X(Compiler::OperandType::OT_VEC3, AsTL::VEC3,	##__VA_ARGS__)


// AstroAssembly Data-type List Permutations
#define ASTL_TYPE_LIST_PERMUTATIONS(OP, ...)											\
    ASTL_TYPE_LIST(OP, Compiler::OperandType::OT_BOOL, AsTL::BOOL,	##__VA_ARGS__)		\
    ASTL_TYPE_LIST(OP, Compiler::OperandType::OT_IDX, AsTL::IDX,	##__VA_ARGS__)		\
    ASTL_TYPE_LIST(OP, Compiler::OperandType::OT_I16, AsTL::I16,	##__VA_ARGS__)		\
    ASTL_TYPE_LIST(OP, Compiler::OperandType::OT_I32, AsTL::I32,	##__VA_ARGS__)		\
    ASTL_TYPE_LIST(OP, Compiler::OperandType::OT_F64, AsTL::F64,	##__VA_ARGS__)		\
    ASTL_TYPE_LIST(OP, Compiler::OperandType::OT_VEC3, AsTL::VEC3,	##__VA_ARGS__)
