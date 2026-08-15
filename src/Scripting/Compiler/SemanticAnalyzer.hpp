#pragma once

#include <functional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <Scripting/Diagnostics.hpp>
#include <Scripting/GraphTypeRules.hpp>
#include <Scripting/Utils/VariantHelpers.hpp>

#include "GraphNodeRegistry.hpp"


namespace Compiler {

	class SemanticAnalyzer {
	public:
		SemanticAnalyzer(std::reference_wrapper<const IGraphNodeRegistry> nodeRegistry, Diagnostics::DiagReporter& reporter);
		~SemanticAnalyzer() = default;

		void analyze(
			const std::vector<Graph::Variable>& variables,
			const std::vector<Graph::Node>& nodes,
			const std::vector<Graph::Link>& nodeLinks
		);

	private:
		const IGraphNodeRegistry& m_nodeRegistry;
		Diagnostics::DiagReporter& m_reporter;

		std::vector<std::reference_wrapper<const Graph::Variable>> m_variables;

		std::unordered_map<
			Graph::NodeID,
			std::reference_wrapper<const Graph::Node>
		> m_nodes;

		std::vector<Graph::NodeID> m_getterNodeIDs;
		std::vector<Graph::NodeID> m_execOrder;		// Evaluation/Execution order for executable nodes

		using LinkCache = std::unordered_map<
			Graph::NodeID,
			std::vector<
				std::reference_wrapper<const Graph::Link>
			>
		>;
		
		LinkCache m_nodeInLinks;
		LinkCache m_nodeOutLinks;

		// Resolved outputs for a reserved subset of native nodes that accept wildcard values and returns a wildcard output,
		// whose type is dependent on the actual input values (e.g., a Multiply node taking in an I32 and an F64 produces an F64 output: I32 * F64 = F64 due to type promotion)
		std::unordered_map<Graph::NodeID, std::type_index> m_wildcardOutputs;

		// Dirty nodes & pins (to avoid cascading errors)
		std::unordered_map<Graph::NodeID, std::unordered_set<std::string>> m_dirtied;


		/* Traverses the graph, starting from the Entry node.
			@param work: The work to do for each node visited.
		*/
		void traverseGraph(const std::function<void(Graph::NodeID)>& work);
		void traverseGraphAt(Graph::NodeID nodeID, const std::function<void(Graph::NodeID)>& work, std::unordered_set<Graph::NodeID>& visited);

		/* Attempts to deduce the return type of each node, if it is a native math node that accepts wildcard values and returns a wildcard type. */
		void resolveWildcards();

		/* Verifies that all nodes have corresponding descriptors in the node registry. */
		void checkExistence();

		/* Verifies that no descriptors in the node registry are malformed. */
		void checkRegistry();

		/* Checks for circular dependencies. */
		void checkCircularDeps();
		void checkCircularDepsForNode(Graph::NodeID nodeID, std::unordered_set<Graph::NodeID> visited);

		/* Checks literal values of Input Pins. */
		void checkInputLiterals();

		/* Checks for unused variables and orphaned nodes. */
		void checkUnused();

		/* Checks implicit type conversions and potential side effects of widening/narrowing casts. */
		void checkCasting();
		void checkCastingForNode(Graph::NodeID nodeID);


		/* Marks a node (and optionally its pin) as dirty, to avoid cascading errors. */
		void markAsDirtied(Graph::NodeID dirtyNodeID, std::optional<std::string> dirtyPinLabel = std::nullopt);

		/* Is a node (and optionally its pin) dirty/faulty? */
		bool isDirty(Graph::NodeID dirtyNodeID, std::optional<std::string> dirtyPinLabel = std::nullopt);
	};

}
