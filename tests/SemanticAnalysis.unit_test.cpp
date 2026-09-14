#include "catch.hpp"

#include <format>
#include <iostream>

#include <Scripting/Diagnostics.hpp>
#include <Scripting/Compiler/SemanticAnalyzer.hpp>

#include "MockNodeRegistry.hpp"
#include "ScriptingTestData.hpp"


namespace {
	inline void PrintReports(const Diagnostics::DiagReporter& reporter) {
		size_t count = reporter.getDiagCount();

		std::cout << "Semantic analysis finished with " << count << ' ' << ((count == 1) ? "report" : "reports") << ((count) ? ":" : "") << '\n';

		for (const auto& diag : reporter.getDiagnostics())
			std::cout << std::format("\t{}{} @ Node {}{}: {}\n",
				DiagTypeToString(diag.diagType), DiagSeverityToString(diag.severity),
				(diag.faultyNodeID.has_value()) ? std::to_string(diag.faultyNodeID.value()) : "",
				(diag.faultyNodePin.has_value()) ? std::format(", Pin \"{}\"", diag.faultyNodePin.value()) : "",
				diag.message
			);

		std::cout << '\n';
	}
}


TEST_CASE("Semantic Analysis: Simple Graph", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::SimpleGraph.variables,
		TestProgram::SimpleGraph.nodes,
		TestProgram::SimpleGraph.links
	);

	PrintReports(reporter);

	REQUIRE(!reporter.hasDiagnostics());
}


TEST_CASE("Semantic Analysis: Diamond Graph", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::DiamondGraph.variables,
		TestProgram::DiamondGraph.nodes,
		TestProgram::DiamondGraph.links
	);

	PrintReports(reporter);

	REQUIRE(!reporter.hasDiagnostics());
}


TEST_CASE("Semantic Analysis: Fibonacci Sequence", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::FibonacciSequence.variables,
		TestProgram::FibonacciSequence.nodes,
		TestProgram::FibonacciSequence.links
	);

	PrintReports(reporter);

	REQUIRE(!reporter.hasDiagnostics());
}


TEST_CASE("Semantic Analysis: Custom Node", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::CustomNode.variables,
		TestProgram::CustomNode.nodes,
		TestProgram::CustomNode.links
	);

	PrintReports(reporter);

	// Error 1: Missing definition for custom node "Mock::GetApoapsis"
	REQUIRE(reporter.getDiagCount() == 1);
}


TEST_CASE("Semantic Analysis: Simulation Tick", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::SimulationTick.variables,
		TestProgram::SimulationTick.nodes,
		TestProgram::SimulationTick.links
	);

	PrintReports(reporter);

	// Error 1: Missing definition for custom node "Mock::GetEpoch"
	REQUIRE(reporter.getDiagCount() == 1);
}


TEST_CASE("Semantic Analysis: One Hundred Primes", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::OneHundredPrimes.variables,
		TestProgram::OneHundredPrimes.nodes,
		TestProgram::OneHundredPrimes.links
	);

	PrintReports(reporter);

	REQUIRE(!reporter.hasDiagnostics());
}


TEST_CASE("Semantic Analysis: Type Mismatch", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::TypeMismatch.variables,
		TestProgram::TypeMismatch.nodes,
		TestProgram::TypeMismatch.links
	);

	PrintReports(reporter);

	// Error 1: Conversion error between VEC3 and I32 (Pin A)
	// Error 2: Conversion error between VEC3 and I32 (Pin B)
	REQUIRE(reporter.getDiagCount() == 2);
}


TEST_CASE("Semantic Analysis: Undefined Variable", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::UndefinedVariable.variables,
		TestProgram::UndefinedVariable.nodes,
		TestProgram::UndefinedVariable.links
	);

	PrintReports(reporter);

	// Error 1: Variable doesn't exist
	REQUIRE(reporter.getDiagCount() == 1);
}


TEST_CASE("Semantic Analysis: Circular Dependency", __FILE__) {
	Compiler::MockNodeRegistry registry;

	Diagnostics::DiagReporter reporter;

	Compiler::SemanticAnalyzer analyzer(registry, reporter);
	analyzer.analyze(
		TestProgram::CircularDependency.variables,
		TestProgram::CircularDependency.nodes,
		TestProgram::CircularDependency.links
	);

	PrintReports(reporter);

	// Error 1: Circular dependency detected
	REQUIRE(reporter.getDiagCount() == 1);
}
