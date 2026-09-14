#pragma once

#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>

#include <Scripting/Utils/Concepts.hpp>

#include "AsTLMacros.hpp"
#include "GraphIdentifiers.hpp"


namespace Graph::Impl {
    class UnaryMathTypeRules {
    public:
        UnaryMathTypeRules() {
            m_supportedOps = {
                MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Negate)
            };
        }
        ~UnaryMathTypeRules() = default;


        /* Is the math operation defined in the rule table? */
        bool HasOperation(const std::string& mathOpID) const { return m_supportedOps.contains(mathOpID); }


        /* Tries to determine the resulting type of a unary math operation.
            @param operand: The type of second operand of the math operation.
            @param mathOpID: The math operation's node descriptor identifier.

            @return The type of the operation if it is a valid operation, std::nullopt otherwise.
        */
        std::optional<std::type_index> TryGetResultType(std::type_index operand, const std::string& mathOpID) const {
            // Unary Negation operation: return the same type
            if (mathOpID == MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Negate))
                return operand;

            return std::nullopt;
        }

    private:
        std::unordered_set<std::string> m_supportedOps;
    };


    class BinaryMathTypeRules {
    public:
        BinaryMathTypeRules() {
            #define X(L_TAG, L_TYPE, R_TAG, R_TYPE, BIN_CONCEPT, MATH_OP, OP_TOKEN)                                                 \
                m_supportedOps.insert(MATH_OP);                                                                                     \
                if constexpr (BIN_CONCEPT<L_TYPE, R_TYPE>) {                                                                        \
                    /*
                        For some reason, even with the outer `if constexpr` check, code like this:

                        ```
                            struct CustomType { int x, y, z; }  // No operator+ overload

                            if constexpr (CompilerUtils::Addable<int, CustomType>)
                                auto res = int() + CustomType();
                        ```

                        ... still fails. CompilerUtils::Addable<T1, T2> explicitly requires T1 and T2 to be CompilerUtils::Addable, so why does the compiler still
                        insist that the line `auto res = int() + CustomType()` is illegal because `CustomType` has no `operator+`,
                        when that conditional branch would logically be discarded at compile time?

                        That's because the compiler is dumb (MSVC MSVC MSVC MSVC MSVC).

                        Even with the `if constexpr` check, everything inside of its block is technically still evaluated for syntax errors
                        at compile time, so the compiler errors out when seeing `CustomType` having no `operator+`.

                        The solution is to wrap the `auto res = int() + CustomType()` line in a templated function with an additional
                        `if constexpr` check:

                        ```
                            template<typename A, typename B>
                            void doWork(A a, B b) {
                                if constexpr (CompilerUtils::Addable<A, B>)
                                    auto res = a + b;
                            }

                            if constexpr (CompilerUtils::Addable<int, CustomType>)
                                doWork(int(), CustomType());
                        ```

                        For some reason, templated functions suppress compile-time evaluation of an `if constexpr` block.
                        That tricks the compiler into not peeking at the actual work inside the outer `if constexpr` statement.

                        Here, we use templated immediately invoked lambdas instead, but the logic is the same.
                    */                                                                                                              \
                    [&]<typename LEFT_T, typename RIGHT_T>() {                                                                      \
                        if constexpr (BIN_CONCEPT<L_TYPE, R_TYPE>) {                                                                \
                            std::type_index _res_type = typeid(decltype(std::declval<LEFT_T>() OP_TOKEN std::declval<RIGHT_T>()));  \
                                                                                                                                    \
                            /* C++ integer promotion rules may yield an `int` return type for integer sizes smaller than `int`
                                (`int` is usually 32 bits, so types like AsTL::I16 may be implicitly widened to `int`).
                                I'm not sure if typeid(int) == typeid(AsTL::I32), so I'll do the check below just in case
                             */                                                                                                     \
                            if (_res_type == typeid(int)) {                                                                         \
                                _res_type = AsTL::TID_I32;                                                                          \
                            }                                                                                                       \
                                                                                                                                    \
                            m_ruleTable.try_emplace(                                                                                \
                                std::tuple{ std::type_index(typeid(LEFT_T)), std::type_index(typeid(RIGHT_T)), MATH_OP },           \
                                _res_type                                                                                           \
                            );                                                                                                      \
                        }                                                                                                           \
                    }.template operator()<L_TYPE, R_TYPE>();                                                                        \
                }

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::Addable,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Add), +
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::Subtractable,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Subtract), -
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::Multipliable,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Multiply), *
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::Divisible,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Divide), /
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CanDoModulo,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Modulo), %
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CompGreaterThan,
                    MakeQualifiedID(ClassScope::Math, CatScope::Logic, FuncScope::GreaterThan), >
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CompGreaterThanEqualTo,
                    MakeQualifiedID(ClassScope::Math, CatScope::Logic, FuncScope::GreaterThanEqualTo), >=
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CompLessThan,
                    MakeQualifiedID(ClassScope::Math, CatScope::Logic, FuncScope::LessThan), <
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CompLessThanEqualTo,
                    MakeQualifiedID(ClassScope::Math, CatScope::Logic, FuncScope::LessThanEqualTo), <=
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CompEqualTo,
                    MakeQualifiedID(ClassScope::Math, CatScope::Logic, FuncScope::EqualTo), ==
                )

                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::CompNotEqualTo,
                    MakeQualifiedID(ClassScope::Math, CatScope::Logic, FuncScope::NotEqualTo), !=
                )


                // min(A, B), max(A, B), and A + B all return the same type, so we're using `+` as a hack
                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::Addable,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Minimum), +
                )
                ASTL_TYPE_LIST_PERMUTATIONS(
                    X, CompilerUtils::Addable,
                    MakeQualifiedID(ClassScope::Math, CatScope::Arithmetic, FuncScope::Maximum), +
                )
            #undef X
        }
        ~BinaryMathTypeRules() = default;


        /* Is the math operation defined in the rule table? */
        bool HasOperation(const std::string& mathOpID) const { return m_supportedOps.contains(mathOpID); }


        /* Tries to determine the resulting type of a binary math operation.
            @param firstOperand: The type of first operand of the math operation.
            @param secondOperand: The type of second operand of the math operation.
            @param mathOpID: The math operation's node descriptor identifier.

            @return The type of the operation if it is a valid operation, std::nullopt otherwise.
        */
        std::optional<std::type_index> TryGetResultType(std::type_index firstOperand, std::type_index secondOperand, const std::string &mathOpID) const {
            auto key = std::tuple{ firstOperand, secondOperand, mathOpID };

            if (m_ruleTable.contains(key))
                return m_ruleTable.at(key);

            return std::nullopt;
        }

    private:
        struct TupleHash {
            template <class T1, class T2, class T3>
            std::size_t operator()(const std::tuple<T1, T2, T3>& t) const {
                auto h1 = std::hash<T1>{}(std::get<0>(t));
                auto h2 = std::hash<T2>{}(std::get<1>(t));
                auto h3 = std::hash<T3>{}(std::get<2>(t));
                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };

        std::unordered_map<
            std::tuple<std::type_index, std::type_index, std::string>,
            std::type_index,
            TupleHash
        > m_ruleTable{};

        std::unordered_set<std::string> m_supportedOps;
    };
}


namespace Graph {
    // Lookup table for type combinations of wildcard unary operations: (Type Index, MathOp) -> MathOp(Type Index)::Type
    // Example: (F64, Negate) -> F64
    inline Impl::UnaryMathTypeRules UnaryMathTypeRules;

    // Lookup table for type combinations of wildcard binary operations: (Type Index 1, Type Index 2, MathOp) -> MathOp(Type Index 1, Type Index 2)::Type
    // Example: (I32, F64, Multiply) -> F64
    // Example: (I32, I32, Divide) -> F64
    inline Impl::BinaryMathTypeRules BinaryMathTypeRules;
}
