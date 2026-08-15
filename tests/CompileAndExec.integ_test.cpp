#include "catch.hpp"

#include <cmath>
#include <format>
#include <chrono>
#include <sstream>
#include <iostream>

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/GraphTypes.hpp>
#include <Scripting/Diagnostics.hpp>
#include <Scripting/Utils/Assembly.hpp>
#include <Scripting/Utils/VariantHelpers.hpp>
#include <Scripting/Compiler/Instruction.hpp>
#include <Scripting/Compiler/BytecodeEmitter.hpp>

#include <Scripting/VM/VirtualMachine.hpp>

#include "MockNodeRegistry.hpp"
#include "ScriptingTestData.hpp"


#define ENABLE_PROFILING 1

#define PROFILE_COMPILE_START													\
		auto cts = std::chrono::high_resolution_clock::now();

#define PROFILE_COMPILE_END														\
		auto cte = std::chrono::high_resolution_clock::now();

#define PROFILE_EXEC_START														\
		auto exts = std::chrono::high_resolution_clock::now();

#define PROFILE_EXEC_END														\
		auto exte = std::chrono::high_resolution_clock::now();

#define PRINT_PROFILING_STAT(EXIT_CODE)																							\
	std::chrono::duration<double> ct = cte - cts;																				\
	std::chrono::duration<double> ext = exte - exts;																			\
	if (ENABLE_PROFILING) {																										\
		std::cout << "\nGraph compiled in " << ct.count() << "s\n"																\
			<< "Executed in " << ext.count() << "s with exit code " << Compiler::VMExitCodeToString(EXIT_CODE) << "\n";			\
	}


namespace {
	std::string valueToString(const AsTL::StackValue &val) {
		using namespace AsTL;

		std::string retVal = "???";

		std::visit(OverloadedVisit {
			[&retVal](IDX arg)					{ retVal = std::to_string(arg); },
			[&retVal](I32 arg)					{ retVal = std::to_string(arg); },
			[&retVal](F64 arg)					{ retVal = std::to_string(arg); },
			[&retVal](BOOL arg)					{ retVal = std::to_string(arg); },
			[&retVal](VEC3 arg)					{ retVal = std::format("[{}, {}, {}]", arg.x, arg.y, arg.z); },
			[&retVal](const std::string &arg)	{ retVal = arg; }
		}, val);

		return retVal;
	}

	std::string valueTypeToString(const AsTL::StackValue &val) {
		using namespace AsTL;

		std::string retVal = "???";

		std::visit([&retVal](const auto &val) {
			retVal = StackValueToRepString(typeid(decltype(val)));
		}, val);

		return retVal;
	}

	void dumpDisassembly(
		const Compiler::MockNodeRegistry &nodeRegistry,
		const Compiler::ConstantPool::ConstantPoolLookupT &poolLookupTable,
		const Diagnostics::DiagReporter &reporter,
		const std::vector<Compiler::SymbolicInstruction> &symbolicInstructions,
		const std::ostringstream &output
	) {
		static constexpr int IDX_HEX_W = sizeof(Compiler::RawOperandT) * 2;    // sizeof(T) returns the size in bytes; 2 hex digits = 1 byte

		{
			static constexpr int MAX_IDX_W = 10;
			static constexpr int MAX_FSYMBOL_W = 50;
			static constexpr int MAX_FNAME_W = 50;

			std::cout << "========== NODE CALLABLE REGISTRY ==========\n\n";
			std::cout << std::format(" {:<{}} | {:<{}} | {:<{}} \n{}+{}+{}\n",
				"Index", MAX_IDX_W,
				"Symbol", MAX_FSYMBOL_W,
				"Name", MAX_FNAME_W,
				std::string(MAX_IDX_W + 2, '-'),
				std::string(MAX_FSYMBOL_W + 2, '-'),
				std::string(MAX_FNAME_W + 2, '-')
			);
			for (const auto &[funcName, funcIdx] : nodeRegistry.getNodeLookup()) {
				const auto &funcDesc = nodeRegistry.getInfo(funcIdx);

				std::cout << std::format(" 0x{:0>{}X}{}| {:<{}} | {:<{}} \n",
					funcIdx, IDX_HEX_W, std::string(MAX_IDX_W - IDX_HEX_W - 1, ' '),
					funcDesc.symbol, MAX_FSYMBOL_W,
					funcDesc.name, MAX_FNAME_W
				);
			}

			std::cout << '\n';
		}

		if (reporter.hasDiagnostics()) {
			using namespace Diagnostics;

			std::cout << "========== REPORTED DIAGNOSTICS ==========\n\n";
			for (const auto &diag : reporter.getDiagnostics()) {
				std::cout << std::format("{} {} @ Node_{}{}: {}\n",
					DiagTypeToString(diag.diagType), DiagSeverityToString(diag.severity),
					(diag.faultyNodeID.has_value()) ? std::to_string(diag.faultyNodeID.value()) : "",
					(diag.faultyNodePin.has_value()) ? std::format(", Pin \"{}\"", diag.faultyNodePin.value()) : "",
					diag.message
				);
			}
			std::cout << '\n';
		}
		
		if (!poolLookupTable.empty()) {
			static constexpr int MAX_IDX_W = 10;    // sizeof(T) returns the size in bytes; 2 hex digits = 1 byte
			static constexpr int MAX_TYPE_W = 20;

			static const int MAX_VAL_W = [&pool = poolLookupTable]() -> int {
				size_t length = 10;
				for (const auto &[val, _] : pool)
					length = (std::max)(length, valueToString(val).size());

				return static_cast<int>(length);
			}();

			std::cout << "========== CONSTANT POOL ==========\n\n";
			std::cout << std::format(" {:<{}} | {:<{}} | {:<{}} \n{}+{}+{}\n",
				"Index", MAX_IDX_W,
				"Type", MAX_TYPE_W,
				"Value", MAX_VAL_W,
				std::string(MAX_IDX_W + 2, '-'),
				std::string(MAX_TYPE_W + 2, '-'),
				std::string(MAX_VAL_W + 2, '-')
			);
			for (const auto &[val, idx] : poolLookupTable)
				std::cout << std::format(" 0x{:0>{}X}{}| {:<{}} | {:<{}} \n",
					idx, IDX_HEX_W, std::string(MAX_IDX_W - IDX_HEX_W - 1, ' '),
					valueTypeToString(val), MAX_TYPE_W,
					valueToString(val), MAX_VAL_W
				);

			std::cout << '\n';
		}


		std::cout << "========== DISASSEMBLY ==========\n\n";
		AsTL::IDX address = 0;

		for (const auto &line : CompilerUtils::FormatDisassembly(symbolicInstructions))
			std::cout << std::format("0x{:0>{}X} |\t{}\n",
				address++, IDX_HEX_W,
				line
			);


		std::cout << "\n========== OUTPUT ==========\n\n";
		std::cout << output.str() << '\n';
	}


	/* Verifies the VM output against the expected otuput. */
	void AssertOutput(const std::ostringstream& realOut, const std::string& actualOut) {
		REQUIRE_THAT(realOut.str(), Catch::Matchers::Equals(actualOut));
	}
}


TEST_CASE("Compilation & Execution Test: Simple Graph", __FILE__) {
	Compiler::MockNodeRegistry registry;
	Diagnostics::DiagReporter reporter;
	Compiler::ConstantPool pool;

	std::vector<Compiler::SymbolicInstruction> symbolicInstructions{};
	std::vector<Compiler::RawInstructionT> rawInstructions{};

	// Compilation
	PROFILE_COMPILE_START
	{
		symbolicInstructions = Compiler::BytecodeEmitter(registry, reporter, pool).emitSymbolic(
			TestProgram::SimpleGraph.variables,
			TestProgram::SimpleGraph.nodes,
			TestProgram::SimpleGraph.links
		);

		rawInstructions.reserve(symbolicInstructions.size());
		for (const auto& ins : symbolicInstructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));
	}
	PROFILE_COMPILE_END

	//std::ostringstream dummy{};
	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, dummy);

	Compiler::VMExitCode exitCode{};

	// Redirect std::cout's output to a string stream instead of the console
	// (the VM prints outputs via std::cout)
	std::ostringstream output{};
	std::streambuf* oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(output.rdbuf());

	// Execution
	PROFILE_EXEC_START
	{
		VirtualMachine vm(pool, registry, nullptr);
		exitCode = vm.execute(rawInstructions);
	}
	PROFILE_EXEC_END


	std::cout.rdbuf(oldBuf);


	REQUIRE(exitCode == Compiler::VMExitCode::SUCCESS);
	AssertOutput(
		output,
		"Got here from the True branch!\n"
		"62.831853\n"
	);


	PRINT_PROFILING_STAT(exitCode)
}


TEST_CASE("Compilation & Execution Test: Diamond Graph", __FILE__) {
	Compiler::MockNodeRegistry registry;
	Diagnostics::DiagReporter reporter;
	Compiler::ConstantPool pool;

	std::vector<Compiler::SymbolicInstruction> symbolicInstructions{};
	std::vector<Compiler::RawInstructionT> rawInstructions{};

	// Compilation
	PROFILE_COMPILE_START
	{
		symbolicInstructions = Compiler::BytecodeEmitter(registry, reporter, pool).emitSymbolic(
			TestProgram::DiamondGraph.variables,
			TestProgram::DiamondGraph.nodes,
			TestProgram::DiamondGraph.links
		);

		rawInstructions.reserve(symbolicInstructions.size());
		for (const auto& ins : symbolicInstructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));
	}
	PROFILE_COMPILE_END

	//std::ostringstream dummy{};
	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, dummy);

	Compiler::VMExitCode exitCode{};

	// Redirect std::cout's output to a string stream instead of the console
	// (the VM prints outputs via std::cout)
	std::ostringstream output{};
	std::streambuf* oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(output.rdbuf());

	// Execution
	PROFILE_EXEC_START
	{
		VirtualMachine vm(pool, registry, nullptr);
		exitCode = vm.execute(rawInstructions);
	}
	PROFILE_EXEC_END


	std::cout.rdbuf(oldBuf);

	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, output);


	REQUIRE(exitCode == Compiler::VMExitCode::SUCCESS);
	AssertOutput(
		output,
R"(Path A: True
Converged!
Path A: True
Converged!
)"
	);


	PRINT_PROFILING_STAT(exitCode)
}


TEST_CASE("Compilation & Execution Test: Fibonacci Sequence", __FILE__) {
	Compiler::MockNodeRegistry registry;
	Diagnostics::DiagReporter reporter;
	Compiler::ConstantPool pool;

	std::vector<Compiler::SymbolicInstruction> symbolicInstructions{};
	std::vector<Compiler::RawInstructionT> rawInstructions{};

	// Compilation
	PROFILE_COMPILE_START
	{
		symbolicInstructions = Compiler::BytecodeEmitter(registry, reporter, pool).emitSymbolic(
			TestProgram::FibonacciSequence.variables,
			TestProgram::FibonacciSequence.nodes,
			TestProgram::FibonacciSequence.links
		);

		rawInstructions.reserve(symbolicInstructions.size());
		for (const auto& ins : symbolicInstructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));
	}
	PROFILE_COMPILE_END

	//std::ostringstream dummy{};
	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, dummy);

	Compiler::VMExitCode exitCode{};

	// Redirect std::cout's output to a string stream instead of the console
	// (the VM prints outputs via std::cout)
	std::ostringstream output{};
	std::streambuf* oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(output.rdbuf());

	// Execution
	PROFILE_EXEC_START
	{
		VirtualMachine vm(pool, registry, nullptr);
		exitCode = vm.execute(rawInstructions);
	}
	PROFILE_EXEC_END


	std::cout.rdbuf(oldBuf);


	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, output);


	REQUIRE(exitCode == Compiler::VMExitCode::SUCCESS);
	AssertOutput(
		output,
R"(1
2
3
5
8
13
21
34
55
89
)"
	);


	PRINT_PROFILING_STAT(exitCode)
}


TEST_CASE("Compilation & Execution Test: Custom Node", __FILE__) {
	Compiler::MockNodeRegistry registry;
	Diagnostics::DiagReporter reporter;
	Compiler::ConstantPool pool;


	// Custom mock context and node to get the position of a satellite
	struct MockSimulationContext {
		struct SatInfo {
			std::string name;
			AsTL::VEC3 position;
		};
		std::unordered_map<AsTL::IDX, SatInfo> satPositions;
	};
	MockSimulationContext mockSimCtx{};
	mockSimCtx.satPositions = {
		{ 0, { "VNREDSat-1A", { 100e3, 200e3, 100e3 } }}
	};

	Compiler::GraphNodeDescriptor mockDescriptor{};
	mockDescriptor.nodeClass = Compiler::GraphNodeDescriptor::NodeClass::Getter;
	mockDescriptor.name = "Get Apoapsis of Satellite";
	mockDescriptor.symbol = "Mock::GetApoapsis";
	mockDescriptor.inExecs = {};
	mockDescriptor.outExecs = {};
	mockDescriptor.inParams = {
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Combo,
			"Satellite",
			Graph::TID_SPC
		}
	};
	mockDescriptor.outParams = {
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Data,
			"Node Symbol",
			AsTL::TID_STR
		},
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Data,
			"Satellite Name",
			AsTL::TID_STR
		},
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Data,
			"Position",
			AsTL::TID_VEC3
		}
	};
	mockDescriptor.callback = [](
		AsTL::OpaqueExecCtx* ctx,
		const Compiler::GraphNodeDescriptor* self,
		AsTL::IDX nodeID,
		std::string_view triggeredPin,
		std::span<const AsTL::StackValue> args,
		AsTL::StackValue* retBuffer
	) -> void {
		
		MockSimulationContext* simCtx = reinterpret_cast<MockSimulationContext*>(ctx);
		const AsTL::IDX satID = std::get<AsTL::IDX>(args[0]);

		const AsTL::VEC3& oldPos = simCtx->satPositions[satID].position;

		// Simulate satellite altitude ascension
		simCtx->satPositions[satID].position = simCtx->satPositions[satID].position * 2;

		retBuffer[0] = self->symbol;
		retBuffer[1] = simCtx->satPositions[satID].name;
		retBuffer[2] = oldPos;
	};

	registry.addOrSet(mockDescriptor);


	std::vector<Compiler::SymbolicInstruction> symbolicInstructions{};
	std::vector<Compiler::RawInstructionT> rawInstructions{};

	// Compilation
	PROFILE_COMPILE_START
	{
		symbolicInstructions = Compiler::BytecodeEmitter(registry, reporter, pool).emitSymbolic(
			TestProgram::CustomNode.variables,
			TestProgram::CustomNode.nodes,
			TestProgram::CustomNode.links
		);

		rawInstructions.reserve(symbolicInstructions.size());
		for (const auto& ins : symbolicInstructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));
	}
		PROFILE_COMPILE_END


	//std::ostringstream dummy{};
	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, dummy);


	Compiler::VMExitCode exitCode{};

	// Redirect std::cout's output to a string stream instead of the console
	// (the VM prints outputs via std::cout)
	std::ostringstream output{};
	std::streambuf* oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(output.rdbuf());

	// Execution
	PROFILE_EXEC_START
	{
		VirtualMachine vm(pool, registry, reinterpret_cast<AsTL::OpaqueExecCtx*>(&mockSimCtx));
		exitCode = vm.execute(rawInstructions);
	}
	PROFILE_EXEC_END


	std::cout.rdbuf(oldBuf);


	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, output);


	REQUIRE(exitCode == Compiler::VMExitCode::SUCCESS);
	AssertOutput(
		output,
		R"(Mock::GetApoapsis: Callable invoked with Satellite VNREDSat-1A at position (2e+05, 4e+05, 2e+05)
Mock::GetApoapsis: Callable invoked with Satellite VNREDSat-1A at position (4e+05, 8e+05, 4e+05)
Mock::GetApoapsis: Callable invoked with Satellite VNREDSat-1A at position (8e+05, 1600000, 8e+05)
Mock::GetApoapsis: Callable invoked with Satellite VNREDSat-1A at position (1600000, 3200000, 1600000)
Satellite VNREDSat-1A has escaped Low-Earth Orbit!
)"
	);


	PRINT_PROFILING_STAT(exitCode)
}


TEST_CASE("Compilation & Execution Test: Simulation Tick Exiting", __FILE__) {
	Compiler::MockNodeRegistry registry;
	Diagnostics::DiagReporter reporter;
	Compiler::ConstantPool pool;


	// Custom mock context and node to get the current simulation epoch
	struct MockSimulationContext {
		AsTL::F64 epoch;
	};
	MockSimulationContext mockSimCtx{};
	mockSimCtx.epoch = 0.0;

	Compiler::GraphNodeDescriptor mockDescriptor{};
	mockDescriptor.nodeClass = Compiler::GraphNodeDescriptor::NodeClass::Getter;
	mockDescriptor.name = "Get Simulation Epoch";
	mockDescriptor.symbol = "Mock::GetEpoch";
	mockDescriptor.inExecs = {};
	mockDescriptor.outExecs = {};
	mockDescriptor.inParams = {};
	mockDescriptor.outParams = {
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Data,
			"Epoch",
			AsTL::TID_F64
		}
	};
	mockDescriptor.callback = [](
		AsTL::OpaqueExecCtx* ctx,
		const Compiler::GraphNodeDescriptor* self,
		AsTL::IDX nodeID,
		std::string_view triggeredPin,
		std::span<const AsTL::StackValue> args,
		AsTL::StackValue* retBuffer
	) -> void {
		
		MockSimulationContext* simCtx = reinterpret_cast<MockSimulationContext*>(ctx);
		const AsTL::F64 currentEpoch = simCtx->epoch;

		simCtx->epoch += 0.5;

		retBuffer[0] = currentEpoch;
	};

	registry.addOrSet(mockDescriptor);


	std::vector<Compiler::SymbolicInstruction> symbolicInstructions{};
	std::vector<Compiler::RawInstructionT> rawInstructions{};

	// Compilation
	PROFILE_COMPILE_START
	{
		symbolicInstructions = Compiler::BytecodeEmitter(registry, reporter, pool).emitSymbolic(
			TestProgram::SimulationTick.variables,
			TestProgram::SimulationTick.nodes,
			TestProgram::SimulationTick.links
		);

		rawInstructions.reserve(symbolicInstructions.size());
		for (const auto& ins : symbolicInstructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));
	}
	PROFILE_COMPILE_END


	//std::ostringstream dummy{};
	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, dummy);

	// Intentionally invalid exit code (see the execution loop below)
	// Otherwise, `exitCode` will be initialized to the exit code (0x00 - SUCCESS)
	Compiler::VMExitCode exitCode = static_cast<Compiler::VMExitCode>(0xFF); 

	// Redirect std::cout's output to a string stream instead of the console
	// (the VM prints outputs via std::cout)
	std::ostringstream output{};
	std::streambuf* oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(output.rdbuf());

	// Cap simulation ticks to prevent TLEs
	constexpr int MAX_SIM_TICKS = 100;
	int simTicks = 0;

	// Execution
	PROFILE_EXEC_START
	{
		VirtualMachine vm(pool, registry, reinterpret_cast<AsTL::OpaqueExecCtx*>(&mockSimCtx));
		vm.setProgram(rawInstructions);

		exitCode = vm.execute();

		while (exitCode == Compiler::VMExitCode::FINISHED_EXEC_TICK && ++simTicks < MAX_SIM_TICKS)
			exitCode = vm.resume();
	}
	PROFILE_EXEC_END


	std::cout.rdbuf(oldBuf);


	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, output);


	REQUIRE(simTicks < MAX_SIM_TICKS);
	REQUIRE(exitCode == Compiler::VMExitCode::SUCCESS);
	AssertOutput(
		output,
		R"(Current Epoch: 0.000000
Current Epoch: 0.500000
Current Epoch: 1.000000
Current Epoch: 1.500000
Current Epoch: 2.000000
Current Epoch: 2.500000
Current Epoch: 3.000000
Current Epoch: 3.500000
Current Epoch: 4.000000
Current Epoch: 4.500000
Current Epoch: 5.000000
Current Epoch: 5.500000
Current Epoch: 6.000000
Current Epoch: 6.500000
Current Epoch: 7.000000
Current Epoch: 7.500000
Current Epoch: 8.000000
Current Epoch: 8.500000
Current Epoch: 9.000000
Current Epoch: 9.500000
Current Epoch: 10.000000
Current Epoch: 10.500000
)"
	);


	PRINT_PROFILING_STAT(exitCode)
}


TEST_CASE("Compilation & Execution Test: One Hundred Primes", __FILE__) {
	Compiler::MockNodeRegistry registry;
	Diagnostics::DiagReporter reporter;
	Compiler::ConstantPool pool;


	// Custom Loop node
	Compiler::GraphNodeDescriptor mockDescriptor{};

	mockDescriptor.nodeClass = Compiler::GraphNodeDescriptor::NodeClass::ControlFlow;
	
	mockDescriptor.name = "Loop";
	mockDescriptor.symbol = "Control::Loop";
	mockDescriptor.inExecs = { Graph::ExecInID, "Break" };
	mockDescriptor.outExecs = { "In Loop", "Completed" };
	mockDescriptor.inParams = {
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Data,
			"Condition",
			AsTL::TID_BOOL
		}
	};
	mockDescriptor.outParams = {
		{
			Compiler::GraphNodeDescriptor::Parameter::ParamClass::Data,
			"Index",  // Index (starting from 0)
			AsTL::TID_I32
		}
	};
	mockDescriptor.callback = [](
		AsTL::OpaqueExecCtx* ctx,
		const Compiler::GraphNodeDescriptor* self,
		AsTL::IDX nodeID,
		std::string_view triggeredPin,
		std::span<const AsTL::StackValue> args,
		AsTL::StackValue* retBuffer
	) -> void {
		
		static std::unordered_map<AsTL::IDX, AsTL::BOOL> conditionsOfLoops{};
		static std::unordered_map<AsTL::IDX, AsTL::I32> indicesOfLoops{};

		AsTL::BOOL& maintainLoop = conditionsOfLoops[nodeID];
		AsTL::I32& thisLoopIdx = indicesOfLoops[nodeID];

		maintainLoop = std::get<AsTL::BOOL>(args[0]);
		STATIC_BLOCK_BEGIN(nodeID)
			thisLoopIdx = 0;
		STATIC_BLOCK_END

		AsTL::I32 lastLoopIdx = thisLoopIdx;

		if (maintainLoop && triggeredPin != "Break") {
			retBuffer[0] = true;			// Exec Out: In Loop
			retBuffer[1] = false;			// Exec Out: Completed
			retBuffer[2] = thisLoopIdx++;	// Data Out: Index

			return;
		}

		else if (!maintainLoop) {
			thisLoopIdx = 0;

			retBuffer[0] = false;			// Exec Out: In Loop
			retBuffer[1] = false;			// Exec Out: Completed
			retBuffer[2] = lastLoopIdx;		// Data Out: Index

			return;
		}

		maintainLoop = false;
		thisLoopIdx = 0;

		retBuffer[0] = false;			// Exec Out: In Loop
		retBuffer[1] = true;			// Exec Out: Completed
		retBuffer[2] = lastLoopIdx;		// Data Out: Index
	};

	registry.addOrSet(mockDescriptor);


	std::vector<Compiler::SymbolicInstruction> symbolicInstructions{};
	std::vector<Compiler::RawInstructionT> rawInstructions{};

	// Compilation
	PROFILE_COMPILE_START
	{
		symbolicInstructions = Compiler::BytecodeEmitter(registry, reporter, pool).emitSymbolic(
			TestProgram::OneHundredPrimes.variables,
			TestProgram::OneHundredPrimes.nodes,
			TestProgram::OneHundredPrimes.links
		);

		rawInstructions.reserve(symbolicInstructions.size());
		for (const auto& ins : symbolicInstructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));
	}
		PROFILE_COMPILE_END


	//std::ostringstream dummy{};
	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, dummy);


	Compiler::VMExitCode exitCode{};

	// Redirect std::cout's output to a string stream instead of the console
	// (the VM prints outputs via std::cout)
	std::ostringstream output{};
	std::streambuf* oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(output.rdbuf());

	// Execution
	PROFILE_EXEC_START
	{
		VirtualMachine vm(pool, registry, nullptr);
		exitCode = vm.execute(rawInstructions);
	}
	PROFILE_EXEC_END


	std::cout.rdbuf(oldBuf);


	//dumpDisassembly(registry, pool.getPoolLookup(), reporter, symbolicInstructions, output);

	
	REQUIRE(exitCode == Compiler::VMExitCode::SUCCESS);
	AssertOutput(
		output,
		"2\n3\n5\n7\n11\n13\n17\n19\n23\n29\n31\n37\n41\n43\n47\n53\n59\n61\n67\n71\n73\n79\n83\n89\n97\n101\n103\n107\n109\n113\n127\n131\n137\n139\n149\n151\n157\n163\n167\n173\n179\n181\n191\n193\n197\n199\n211\n223\n227\n229\n233\n239\n241\n251\n257\n263\n269\n271\n277\n281\n283\n293\n307\n311\n313\n317\n331\n337\n347\n349\n353\n359\n367\n373\n379\n383\n389\n397\n401\n409\n419\n421\n431\n433\n439\n443\n449\n457\n461\n463\n467\n479\n487\n491\n499\n503\n509\n521\n523\n541\n"
	);


	PRINT_PROFILING_STAT(exitCode)
}

