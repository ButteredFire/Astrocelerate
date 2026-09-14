#pragma once

#include <span>
#include <regex>
#include <vector>
#include <functional>
#include <unordered_map>

#include <Scripting/Diagnostics.hpp>
#include <Scripting/GraphTypes.hpp>
#include <Scripting/Utils/Assembly.hpp>
#include <Scripting/Utils/VariantHelpers.hpp>

#include "Instruction.hpp"
#include "ConstantPool.hpp"
#include "GraphNodeRegistry.hpp"


namespace Compiler {
	/* (Single-use) Bytecode Emitter Implementation */
	class BytecodeEmitter {
	public:
		BytecodeEmitter(const std::reference_wrapper<IGraphNodeRegistry> nodeRegistry, Diagnostics::DiagReporter &reporter, ConstantPool &constPool);
		~BytecodeEmitter() = default;

		/* Generates an array of symbolic instructions.
			@param variables: The graph variables.
			@param nodes: The graph nodes.
			@param nodeLinks: The links between the graph nodes.
			@return An array of symbolic instructions.
		*/
		std::vector<SymbolicInstruction> emitSymbolic(
			const std::vector<Graph::Variable> &variables,
			const std::vector<Graph::Node> &nodes,
			const std::vector<Graph::Link> &nodeLinks
		);


		/* Generates an array of encoded instructions.
			@param variables: The graph variables.
			@param nodes: The graph nodes.
			@param nodeLinks: The links between the graph nodes.
			@return An array of symbolic instructions.
		*/
		std::vector<RawInstructionT> emitEncoded(
			const std::vector<Graph::Variable>& variables,
			const std::vector<Graph::Node>& nodes,
			const std::vector<Graph::Link>& nodeLinks
		);

	private:
		const IGraphNodeRegistry &m_nodeRegistry;
		Diagnostics::DiagReporter &m_reporter;
		ConstantPool &m_constPool;

		std::vector<SymbolicInstruction> m_instructions;
		uint32_t m_line, m_col;

		// Graph variable cache: maps each variable name to its index into the global registry
		std::unordered_map<std::string, AsTL::IDX> m_varGlRegIdxName;
		std::unordered_map<
			Graph::NodeID,
			std::reference_wrapper<const Graph::Variable>
		> m_graphVarCache;

		// Node cache: maps each Node ID to Node data
		std::unordered_map<
			Graph::NodeID,
			std::reference_wrapper<const Graph::Node>
		> m_nodeCache;

		// Node link caches: maps each Node ID to Node Links relevant to it
		using LinkCache = std::unordered_map<
			Graph::NodeID,
			std::vector<
				std::reference_wrapper<const Graph::Link>
			>
		>;

		LinkCache m_outLinkCache;	// Link cache for the outgoing links from a node
		LinkCache m_inLinkCache;	// Link cache for the incoming links into a node


		// Stores direct DATA links from each node's output pin(s) to other nodes' input pins for optimizations
		std::unordered_map<Graph::NodeID, std::unordered_set<Graph::NodeID>> m_directDataLinks;

		// Simulated global registry: map<Node ID, map<Node Pin Label, Global Registry Index>>
		std::unordered_map<Graph::NodeID, std::unordered_map<std::string, AsTL::IDX>> m_glReg;
		AsTL::IDX m_glRegIdx;

		// Node addresses in the instruction stream: maps a node (by ID) to the starting and ending addresses of its instruction stream
		struct Address {
			struct TrueAddress {
				AsTL::IDX start, end;
			};

			struct CompleteAddress {
				AsTL::IDX start, end;
			};

			// A True Address is only concerned with determining the actual compiled instructions of just THIS node,
			// NOT the instructions of other nodes linked to the input pins of THIS node.
			TrueAddress trueAddr;

			// A Complete Address determines the compiled instruction stream of everything necessary to execute THIS node,
			// which includes the instructions of potential Output Nodes linked to the input pins of THIS node.
			CompleteAddress completeAddr;
		};
		std::unordered_map<Graph::NodeID, Address> m_nodeAddresses;

		// Backpatching queue that stores indices of instructions whose address operands need to be resolved
		std::vector<AsTL::IDX> m_backpatchQueue;

		// Maps IDs of Constant-class Nodes to their values' indices in the constant pool
		std::unordered_map<Graph::NodeID, AsTL::IDX> m_constPoolIdxCache;

		//std::unordered_set<Graph::NodeID> m_preallocNodes;
		std::unordered_set<Graph::NodeID> m_compiledNodes;

		std::vector<Graph::NodeID> m_nodeEvalOrder;
		std::unordered_set<Graph::NodeID> m_evaluatedNodes;


		/* Evaluation pass: The emitter traverses the graph to determine the node evaluation flow. */
		void createEvalOrder();
		void createEvalOrderFrom(Graph::NodeID startNodeID, Graph::NodeID prevNodeID);


		/* Pre-allocation pass: The emitter traverses the graph to determine which node outputs are reused (i.e., connected to the inputs of more than one node),
			so that the results can be pre-allocated in the global registry.
		*/
		void preallocGraph();


		/* Gets the generated variable name for a pre-allocated output node pin. */
		std::string getPreallocOutPinName(Graph::NodeID nodeID, const std::string &pinLabel, std::type_index pinType);


		/* Compilation pass: The emitter traverses and compiles the graph. */
		void compileGraph();

		/* Traverses the graph starting from a particular node.
			@param startNodeID: The ID of the starting node.
			@param prevNodeID: The ID of the node that immediately precedes the starting node in execution order.
			@param execPath: The path the compiler took, from the entry node, to get to the starting node.
		*/
		void compileGraphFrom(Graph::NodeID startNodeID, Graph::NodeID prevNodeID, std::vector<Graph::NodeID> execPath);

		/* Backpatching pass: The emitter resolves emitted. placeholder memory addresses. */
		void backpatch();

		/* Compiles a node.
			@param nodeID: The ID of the node to be compiled.
			@param execPath: The path the compiler took, from the entry node, to get to the node to be compiled.
		*/
		void compileNode(Graph::NodeID nodeID, std::vector<Graph::NodeID> execPath);

		/* Emits a cast instruction for the top VM stack slot. */
		void emitCastInstruction(AsTL::BYTE startType, AsTL::BYTE destType);

		/* Should LOAD_GL be emitted to get the value from an output pin of an output node?
			@note This function cannot be used before or during the pre-allocation pass

			@param outNodeID: The ID of the Output node.
			@param outPinLabel: The ID/Label of the Output pin.
			@param inNodeID: The ID of the Input node, one of whose Input pins is connected to the Output node.

			@return True if LOAD_GL should be emitted to get the value of the Output node,
					False otherwise (e.g., value is already in constant pool (requiring LOAD_CONST), value is already at the top of VM stack)
		*/
		bool shouldLoadFromGlobReg(Graph::NodeID outNodeID, const std::string &outPinLabel, Graph::NodeID inNodeID);


		/* Should STORE_GL be emitted to store the value from an output pin of an output node into the global registry?
			@note This function cannot be used before or during the pre-allocation pass

			@param nodeID: The ID of the Output node.
			@param outPinLabel: The ID/Label of the Output pin.

			@return True if STORE_GL should be emitted to get the value of the Output node,
					False otherwise (e.g., value is already in constant pool (requiring LOAD_CONST), value is already at the top of VM stack)
		*/
		bool shouldStoreIntoGlobReg(Graph::NodeID nodeID, const std::string& outPinLabel);


		bool shouldPreallocOutputPin(Graph::NodeID nodeID, const std::string& outPinLabel);


		AsTL::IDX storeInConstPool(const AsTL::StackValue &val);


		/* Simulates a value-store into the Virtual Machine's Global Registry.
			@param nodeID: The ID of the node.
			@param pinLabel: The label of the Pin whose value is about to be stored.
			@param mask: The bitmask to alter the behavior of STORE_XXX instructions
			@param type: The data type that the value is about to be stored as.

			@return The index into the Global Registry of the stored value.
		*/
		AsTL::IDX simulateStoreInGlobReg(Graph::NodeID nodeID, const std::string &pinLabel, const InstructionMask bitmask, AsTL::BYTE type);


		void emitInstruction(SymbolicInstruction &&instruction);
	};
}
