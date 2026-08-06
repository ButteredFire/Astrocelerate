/*
    Implementation of "static blocks" that allow for arbitrary one-time code execution inside functions (akin to Java's `static {...}` block).
*/

#pragma once

#include <unordered_set>


// Concatenates two tokens together
// NOTE: Since the token-pasting operator (##) inhibits macro expansion of its immediate operands,
// we need 3 layers of macro indirection to make sure both operands are fully evaluated before being concatenated
#define CONCAT_EVAL_2(a, b) a ## b
#define CONCAT_EVAL_1(a, b) CONCAT_EVAL_2(a, b)
#define CONCAT(a, b)        CONCAT_EVAL_1(a, b)

#define TOKEN_PREFIX _static_block_

// Begins a static block
#define STATIC_BLOCK_BEGIN(...) \
    static std::unordered_set<size_t> CONCAT(TOKEN_PREFIX, __LINE__); \
    size_t const CONCAT(_seed_, __LINE__) = __VA_OPT__(__VA_ARGS__ +) 0; \
    if (CONCAT(TOKEN_PREFIX, __LINE__).insert(CONCAT(_seed_, __LINE__)).second) {

// Ends a static block
#define STATIC_BLOCK_END }
