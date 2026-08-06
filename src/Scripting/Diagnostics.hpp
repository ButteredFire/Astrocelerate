#pragma once

#include <vector>
#include <string>
#include <format>
#include <optional>
#include <string_view>


namespace Diagnostics {
    // Diagnostic carrying compiler-emitted errors and warnings.
    struct Diagnostic {
        enum class Severity {
            Error,
            Warning
        };

        enum class DiagType {
            Lexical,    // e.g., invalid characters, unclosed strings
            Syntactic,  // e.g., missing closing parenthesis, unexpected tokens
            Semantic,   // e.g., undefined identifiers, type mismatches
            Codegen,    // e.g., jump offsets out of bounds, stack limit exceeded
            Internal    // e.g., compiler crashes or invariant violations
        };


        Severity severity;
        DiagType diagType;

        std::string message;

        int32_t faultyNodeID;

        std::optional<std::string> faultyNodePin;

        // Line and column in disassembled AstroAssembly (optional)
        std::optional<uint32_t> line;
        std::optional<uint32_t> col;
    };


    inline std::string DiagSeverityToString(Diagnostic::Severity severity) {
        using enum Diagnostic::Severity;
        switch (severity) {
        case Warning:
            return "Warning";
        case Error:
        default:
            return "Error";
        }
    }


	inline std::string DiagTypeToString(Diagnostic::DiagType diagType) {
		using enum Diagnostic::DiagType;
		switch (diagType) {
		case Lexical:
			return "Lexical";
		case Syntactic:
			return "Syntactic";
		case Semantic:
			return "Semantic";
		case Codegen:
			return "Codegen";
		case Internal:
		default:
			return "Internal";
		}
	}


    class DiagReporter {
    public:
        DiagReporter() = default;
        ~DiagReporter() = default;

        template<typename... Args>
        void report(
            Diagnostic::Severity severity,
            Diagnostic::DiagType diagType,
            int32_t nodeID,
            std::optional<std::string> nodePin,
            std::optional<uint32_t> disassemblyLine,
            std::optional<uint32_t> disassemblyCol,
            const std::string_view messageFmt, Args&&... args
        ) {
            m_diagnostics.push_back({
                severity,
                diagType,
                std::vformat(messageFmt, std::make_format_args(args...)),
                nodeID,
                nodePin,
                disassemblyLine,
                disassemblyCol
            });
        }

        bool hasDiagnostics() const { return !m_diagnostics.empty(); }
        size_t getDiagCount() const { return m_diagnostics.size(); }

        const std::vector<Diagnostic> &getDiagnostics() const { return m_diagnostics; }

    private:
        std::vector<Diagnostic> m_diagnostics;
    };
}
