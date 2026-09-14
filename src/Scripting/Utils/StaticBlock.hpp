/*
    Implementation of "static blocks" that allow for arbitrary one-time code execution inside functions (akin to Java's `static {...}` block).
*/

#pragma once

#include <string>
#include <functional>
#include <unordered_set>


#define STATIC_BLOCK_IMPL


// Keeps track of all active static block sets to make state-resetting possible later
#define STATIC_BLOCK_TYPE           std::unordered_set<size_t>
#define STATIC_BLOCK_TRACKER        _static_block_tracker

// The actual block resetting can only be done by the virtual machine
// (this implementation assumes there is only one active VM at all times, which is the intended execution model for Astrocelerate)
class VirtualMachine;
class StaticBlockTracker {
public:
    friend class VirtualMachine;

    inline void add(const std::string &blockName, STATIC_BLOCK_TYPE& block) {
        if (m_blockNames.contains(blockName))
            return;

        m_blockNames.insert(blockName);
        m_blocks.push_back(block);
    }

private:
    std::unordered_set<std::string> m_blockNames;
    std::vector<std::reference_wrapper<STATIC_BLOCK_TYPE>> m_blocks;

    /* Resets the states of all active static blocks so that all arbitrary code declared inside them can be re-run after the fact. */
    inline void resetStaticBlocks() {
        for (STATIC_BLOCK_TYPE& block : m_blocks)
            block.clear();

        m_blockNames.clear();
        m_blocks.clear();
    }
};

inline StaticBlockTracker STATIC_BLOCK_TRACKER;

// Concatenates two tokens together
// NOTE: Since the token-pasting operator (##) inhibits macro expansion of its immediate operands,
// we need 3 layers of macro indirection to make sure both operands are fully evaluated before being concatenated
#define CONCAT_EVAL_2(a, b) a ## b
#define CONCAT_EVAL_1(a, b) CONCAT_EVAL_2(a, b)
#define CONCAT(a, b)        CONCAT_EVAL_1(a, b)

#define MAKE_STR_EXPAND(x)  #x
#define MAKE_STR(x)         MAKE_STR_EXPAND(x)

#define BLOCK_NAME CONCAT(_static_block_, __LINE__)


#define IMPL_STATIC_BLOCK_BEGIN(...)                                                    \
    static STATIC_BLOCK_TYPE BLOCK_NAME;                                                \
    STATIC_BLOCK_TRACKER.add(                                                           \
        MAKE_STR(BLOCK_NAME),                                                           \
        BLOCK_NAME                                                                      \
    );                                                                                  \
    size_t const CONCAT(_seed_, __LINE__) = __VA_OPT__(__VA_ARGS__ +) 0;                \
    if (BLOCK_NAME.insert(CONCAT(_seed_, __LINE__)).second) {

#define IMPL_STATIC_BLOCK_END }


#undef STATIC_BLOCK_IMPL


// Begins a static block
#define STATIC_BLOCK_BEGIN(...) IMPL_STATIC_BLOCK_BEGIN(__VA_ARGS__)

// Ends a static block
#define STATIC_BLOCK_END        IMPL_STATIC_BLOCK_END
