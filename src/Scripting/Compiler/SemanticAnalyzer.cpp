#include "SemanticAnalyzer.hpp"


namespace Compiler {

	SemanticAnalyzer::SemanticAnalyzer(
		std::reference_wrapper<const IGraphNodeRegistry> nodeRegistry,
		Diagnostics::DiagReporter& reporter
	) :
		m_nodeRegistry(nodeRegistry),
		m_reporter(reporter)
	{}
	

	void SemanticAnalyzer::analyze(
		const std::vector<Graph::Variable>& variables,
		const std::vector<Graph::Node>& nodes,
		const std::vector<Graph::Link>& nodeLinks
	) {
		for (const auto& var : variables)
			m_variables.emplace_back(var);

		for (const auto& node : nodes) {
			m_nodes.emplace(node.id, node);
			
			if (node.symbol == Graph::GetterNodeSymbol)
				m_getterNodeIDs.emplace_back(node.id);
		}

		for (const auto& link : nodeLinks) {
			m_nodeLinks.emplace_back(link);
			m_nodeOutLinks[link.outNodeID].emplace_back(link);
			m_nodeInLinks[link.inNodeID].emplace_back(link);
		}

		checkExistenceInRegistry();
		checkExistenceInGraph();
		checkRegistry();
		checkCircularDeps();

		resolveWildcards();

		checkInputLiterals();
		checkUnused();
		checkCasting();
	}


	void SemanticAnalyzer::traverseGraph(const std::function<void(Graph::NodeID)>& work) {
		if (m_execOrder.empty()) {
			m_execOrder.reserve(m_nodes.size());

			std::unordered_set<Graph::NodeID> visitedNodeIDs{};
			traverseGraphAt(Graph::EntryNodeID, work, visitedNodeIDs);
		}
		else
			for (const auto& nodeID : m_execOrder)
				work(nodeID);
	}


	void SemanticAnalyzer::traverseGraphAt(Graph::NodeID nodeID, const std::function<void(Graph::NodeID)>& work, std::unordered_set<Graph::NodeID>& visited) {
		if (visited.contains(nodeID))
			return;

		visited.insert(nodeID);
			
		if (nodeID != Graph::EntryNodeID && nodeID != Graph::TermNodeID) {
			m_execOrder.push_back(nodeID);
			work(nodeID);
		}

		if (m_nodeOutLinks.contains(nodeID))
			for (const Graph::Link& link : m_nodeOutLinks.at(nodeID))
				traverseGraphAt(link.inNodeID, work, visited);
	}

	
	void SemanticAnalyzer::resolveWildcards() {
		traverseGraph(
			[&](Graph::NodeID nodeID) -> void {
				resolveWildcardsForNode(nodeID);
			}
		);
	}


	void SemanticAnalyzer::resolveWildcardsForNode(Graph::NodeID nodeID) {
		if (isDirty(nodeID))
			return;

		const Graph::Node& node = m_nodes.at(nodeID);

		std::vector<std::type_index> inPinTypes{};

		for (const auto& inPin : node.inputPins) {
			std::visit(
				[&](const auto& pin) {
					inPinTypes.push_back(pin.type);
					
					if (m_nodeInLinks.contains(nodeID))
						for (const Graph::Link& link : m_nodeInLinks.at(nodeID))
							if (link.inPinID == pin.label)
								resolveWildcardsForNode(link.outNodeID);
				},
				inPin
			);
		}


		// If the node is a native math node that accepts wildcard values and returns wildcard types, attempt to deduce its return type
			// Binary node
		if (
			Graph::BinaryMathTypeRules.HasOperation(node.symbol) &&
			inPinTypes.size() == 2
		) {
			std::optional<std::type_index> retType = Graph::BinaryMathTypeRules.TryGetResultType(inPinTypes[0], inPinTypes[1], node.symbol);

			if (retType.has_value())
				m_wildcardOutputs.emplace(node.id, retType.value());

			else {
				m_reporter.report(
					Diagnostics::Diagnostic::Severity::Error,
					Diagnostics::Diagnostic::DiagType::Semantic,
					node.id,
					std::nullopt,
					"Incompatible math operation between inputs of types {} and {}",
					AsTL::StackValueToRepString(inPinTypes[0]),
					AsTL::StackValueToRepString(inPinTypes[1])
				);

				markAsDirtied(node.id);
			}
		}

			// Unary node
		else if (
			Graph::UnaryMathTypeRules.HasOperation(node.symbol) &&
			inPinTypes.size() == 1
		) {
			std::optional<std::type_index> retType = Graph::UnaryMathTypeRules.TryGetResultType(inPinTypes[0], node.symbol);

			if (retType.has_value())
				m_wildcardOutputs.emplace(node.id, retType.value());

			else {
				m_reporter.report(
					Diagnostics::Diagnostic::Severity::Error,
					Diagnostics::Diagnostic::DiagType::Semantic,
					node.id,
					std::nullopt,
					"Incompatible math operation for input of type {}",
					AsTL::StackValueToRepString(inPinTypes[0])
				);

				markAsDirtied(node.id);
			}
		}
	}


	void SemanticAnalyzer::checkExistenceInRegistry() {
		for (const auto& [nodeID, nodeRef] : m_nodes) {
			const Graph::Node& node = nodeRef.get();

			if (!m_nodeRegistry.contains(node.symbol)) {
				m_reporter.report(
					Diagnostics::Diagnostic::Severity::Error,
					Diagnostics::Diagnostic::DiagType::Semantic,
					nodeRef.get().id,
					std::nullopt,
					"Cannot resolve definition for Node Symbol \"{}\"",
					node.symbol
				);

				markAsDirtied(node.id);
			}
		}
	}


	void SemanticAnalyzer::checkExistenceInGraph() {
		bool hasFullyInvalidLinks = false;

		const auto processLink = [&](const Graph::Link& link) -> void {
			if (link.outNodeID == Graph::EntryNodeID || link.inNodeID == Graph::TermNodeID)
				return;

			const bool inNodeInvalid = !m_nodes.contains(link.inNodeID);
			const bool outNodeInvalid = !m_nodes.contains(link.outNodeID);

			if (inNodeInvalid && outNodeInvalid) {
				hasFullyInvalidLinks = true;
				return;
			}

			Graph::NodeID traceNodeID{};
			Impl::DirtyPin dirtyPin{};

			
			if (inNodeInvalid) {
				traceNodeID = link.outNodeID;

				dirtyPin.type = Impl::DirtyPin::OUT_PIN;
				dirtyPin.label = link.outPinID;
			}
			if (outNodeInvalid) {
				traceNodeID = link.inNodeID;

				dirtyPin.type = Impl::DirtyPin::IN_PIN;
				dirtyPin.label = link.inPinID;
			}

			if (inNodeInvalid || outNodeInvalid) {
				m_reporter.report(
					Diagnostics::Diagnostic::Severity::Error,
					Diagnostics::Diagnostic::DiagType::Semantic,
					traceNodeID,
					dirtyPin.label,
					"Node pin is connected to an invalid or corrupted node"
				);

				markAsDirtied(traceNodeID, dirtyPin);
			}
		};


		for (const Graph::Link& link : m_nodeLinks)
			processLink(link);
		

		if (hasFullyInvalidLinks) {
			// Since the links are not connected to any valid node in the graph, graph traversal will not go through them;
			// therefore, the program won't crash. This is why the report will be a warning instead of an error.
			m_reporter.report(
				Diagnostics::Diagnostic::Severity::Warning,
				Diagnostics::Diagnostic::DiagType::Semantic,
				std::nullopt,
				std::nullopt,
				"One or more invalid or corrupted inert links have been identified; they will be ignored during program execution"
			);
		}
	}


	void SemanticAnalyzer::checkRegistry() {
		using namespace Diagnostics;
		using enum Diagnostic::Severity;
		using enum Diagnostic::DiagType;

		for (const auto& [funcIdx, descriptor] : m_nodeRegistry.getNodeTable()) {
			if (Graph::ReservedNodeSymbols.contains(descriptor.symbol))
				continue;

			// Check alignment with node categorization
			const std::string diagTitle = std::format("Malformed definition: Node \"{}\"", descriptor.name);
			{
				// Emit a warning if there are faulty descriptors in the registry, but whose instantiations are not present in the graph.
				// If the faulty node is present in the graph, elevate severity to Error.
				Diagnostic::Severity severity = Warning;
				std::optional<Graph::NodeID> faultyNodeID;
				for (const auto& [nodeID, nodeRef] : m_nodes) {
					const Graph::Node& node = nodeRef.get();

					if (node.symbol == descriptor.symbol) {
						severity = Error;
						faultyNodeID = node.id;

						break;
					}
				}

				// GENERAL
				{
					// Node descriptors must have callbacks
					if (!descriptor.callback.has_value()) {
						if (!Graph::ReservedNodeSymbols.contains(descriptor.symbol))
							m_reporter.report(
								severity,
								Semantic,
								faultyNodeID,
								std::nullopt,
								"{} does not have any bound native or AstroAssembly callback",
								diagTitle
							);
					}
					else if (
						// Callback is an AstroAssembly instruction
						std::holds_alternative<Opcode>(descriptor.callback.value()) &&

						// Callback parameter list size exceeds 2 (as per the ISA, AstroAssembly instructions are allowed to have at most 2 operands)
						descriptor.inParams.size() > 2
					)
						m_reporter.report(
							severity,
							Semantic,
							faultyNodeID,
							std::nullopt,
							"{} has an AstroAssembly instruction callback, but the input parameter list size (size: {}) exceeds 2",
							diagTitle, descriptor.inParams.size()
						);
				}


				// EXECUTABLE NODES
				if (descriptor.isExecutable()) {
					// Executable nodes MUST have at least one Input Execution Pin
					if (descriptor.inExecs.empty())
						m_reporter.report(
							severity,
							Semantic,
							faultyNodeID,
							std::nullopt,
							"{} is executable, but has no Input Execution Pin",
							diagTitle
						);


					// Executable nodes MUST NOT have duplicate EXEC_IN and EXEC_OUT pins
					{
						bool hasExecIn = false;
						for (const auto& exec : descriptor.inExecs)
							if (exec == Graph::ExecInID) {
								if (!hasExecIn)
									hasExecIn = true;
								else
									m_reporter.report(
										severity,
										Semantic,
										faultyNodeID,
										std::nullopt,
										"{} cannot have multiple Default Input Execution Pins",
										diagTitle
									);
							}

						bool hasExecOut = false;
						for (const auto& exec : descriptor.outExecs)
							if (exec == Graph::ExecOutID) {
								if (!hasExecOut)
									hasExecOut = true;
								else
									m_reporter.report(
										severity,
										Semantic,
										faultyNodeID,
										std::nullopt,
										"{} cannot have multiple Default Output Execution Pins",
										diagTitle
									);
							}
					}
				}


				// NON-EXECUTABLE NODES
				else {
					// Non-executable nodes MUST NOT have any execution pins
					if (!descriptor.inExecs.empty() || !descriptor.outExecs.empty())
						m_reporter.report(
							severity,
							Semantic,
							faultyNodeID,
							std::nullopt,
							"{} is non-executable, but has one or more Execution Pins",
							diagTitle
						);
				}
			}
		}
	}


	void SemanticAnalyzer::checkCircularDeps() {
		traverseGraph(
			[&](Graph::NodeID nodeID) {
				checkCircularDepsForNode(nodeID, {});
			}
		);
	}


	void SemanticAnalyzer::checkCircularDepsForNode(Graph::NodeID nodeID, std::unordered_set<Graph::NodeID> visited) {
		if (visited.contains(nodeID)) {
			m_reporter.report(
				Diagnostics::Diagnostic::Severity::Error,
				Diagnostics::Diagnostic::DiagType::Semantic,
				nodeID,
				std::nullopt,
				"A circular dependency is formed at this node"
			);

			markAsDirtied(nodeID);

			return;
		}

		visited.insert(nodeID);

		const Graph::Node& node = m_nodes.at(nodeID);
		for (const auto& inPin : node.inputPins)
			std::visit(
				[&](const auto& pin) {
					if (m_nodeInLinks.contains(nodeID))
						for (const Graph::Link& link : m_nodeInLinks.at(nodeID))
							if (link.inPinID == pin.label) {
								checkCircularDepsForNode(link.outNodeID, visited);
								break;
							}
				},
				inPin
			);
	}


	void SemanticAnalyzer::checkInputLiterals() {
		// Helper to check if an input pin's value is provided as a literal, or sourced from elsewhere via a data link
		auto pinContainsLiteral = [&](Graph::NodeID nodeID, const std::string& pinLabel) -> bool {
			if (m_nodeInLinks.contains(nodeID))
				for (const Graph::Link& link : m_nodeInLinks.at(nodeID))
					if (link.inPinID == pinLabel)
						return false;

			return true;
		};


		for (const auto& [nodeID, nodeRef] : m_nodes) {
			const Graph::Node& node = nodeRef.get();
			if (isDirty(node.id))
				continue;

			for (const auto& inPin : node.inputPins)
				std::visit(
					CompilerUtils::OverloadedVisit {
						// Variable combo boxes
						[&](const Graph::Node::ComboInPin& comboPin) {
							if (!pinContainsLiteral(node.id, comboPin.label))
								return;

							if (comboPin.chosenVar.has_value()) {
								for (const Graph::Variable& var : m_variables)
									if (var.name == comboPin.chosenVar.value()) {
										if (var.type != comboPin.type) {
											m_reporter.report(
												Diagnostics::Diagnostic::Severity::Error,
												Diagnostics::Diagnostic::DiagType::Semantic,
												node.id,
												comboPin.label,
												"Selected variable \"{}\" is of type {}, but the expected input type is {}",
												var.name,
												AsTL::StackValueToRepString(var.type),
												AsTL::StackValueToRepString(comboPin.type)
											);

											markAsDirtied(node.id, Impl::DirtyPin{ Impl::DirtyPin::IN_PIN, comboPin.label });
										}

										return;
									}

								// Not reaching the `return` in the loop means no matching variable is found
								m_reporter.report(
									Diagnostics::Diagnostic::Severity::Error,
									Diagnostics::Diagnostic::DiagType::Semantic,
									node.id,
									comboPin.label,
									"Selected variable \"{}\" does not exist",
									comboPin.chosenVar.value()
								);

								markAsDirtied(node.id, Impl::DirtyPin{ Impl::DirtyPin::IN_PIN, comboPin.label });
							}
							else {
								// No variable has been selected
								m_reporter.report(
									Diagnostics::Diagnostic::Severity::Error,
									Diagnostics::Diagnostic::DiagType::Semantic,
									node.id,
									comboPin.label,
									"No variable has been selected"
								);

								markAsDirtied(node.id, Impl::DirtyPin{ Impl::DirtyPin::IN_PIN, comboPin.label });
							}
						},


						// Data pins with literal values
						[&](const Graph::Node::DataInPin& dataPin) {
							if (!pinContainsLiteral(node.id, dataPin.label))
								return;

							std::visit(
								[&](const auto& val) {
									std::type_index literalType = typeid(std::decay_t<decltype(val)>);

									// If the literal type is a numeric type, it may be interpreted as BYTE or IDX, which are internal types;
									// in this case, we promote it to the smallest non-internal type: I16
									if (literalType == AsTL::TID_BYTE || literalType == AsTL::TID_IDX)
										literalType = AsTL::TID_I16;

									if (
										AsTL::ValidConversionMap.contains(literalType) &&

										// The type of the literal value cannot be converted to the required type
										!AsTL::ValidConversionMap.at(literalType).contains(dataPin.type)
									) {
										m_reporter.report(
											Diagnostics::Diagnostic::Severity::Error,
											Diagnostics::Diagnostic::DiagType::Semantic,
											node.id,
											dataPin.label,
											"Provided value is of type {}, but the expected input type is {}",
											AsTL::StackValueToRepString(literalType),
											AsTL::StackValueToRepString(dataPin.type)
										);

										markAsDirtied(node.id, Impl::DirtyPin{ Impl::DirtyPin::IN_PIN, dataPin.label });
									}
								},
								dataPin.val
							);
						}
					},
					inPin
				);
		}
	}


	void SemanticAnalyzer::checkUnused() {
		// Unused variables
		for (const Graph::Variable& var : m_variables) {
			bool unused = true;

			for (Graph::NodeID nodeID : m_getterNodeIDs) {
				const Graph::Node& node = m_nodes.at(nodeID);

				for (const auto& inPin : node.inputPins)
					std::visit(
						CompilerUtils::OverloadedVisit {
							[&](const Graph::Node::ComboInPin& comboPin) {
								if (comboPin.chosenVar.has_value() && comboPin.chosenVar.value() == var.name)
									unused = false;
							},

							[&](const auto& val) {}
						},
						inPin
					);
			}


			if (unused)
				m_reporter.report(
					Diagnostics::Diagnostic::Severity::Warning,
					Diagnostics::Diagnostic::DiagType::Semantic,
					std::nullopt,
					std::nullopt,
					"Variable \"{}\" is unused",
					var.name
				);
		}


		// Unused nodes
		for (const auto& [nodeID, nodeRef] : m_nodes) {
			const Graph::Node& node = nodeRef.get();

			if (!m_nodeRegistry.contains(node.symbol) || isDirty(node.id))
				continue;

			const auto& [_, descriptor] = m_nodeRegistry.getInfo(node.symbol);

			if (!m_nodeInLinks.contains(node.id) && !m_nodeOutLinks.contains(node.id)) {
				m_reporter.report(
					Diagnostics::Diagnostic::Severity::Warning,
					Diagnostics::Diagnostic::DiagType::Semantic,
					node.id,
					std::nullopt,
					"Node \"{}\" is unused",
					descriptor.name
				);

				markAsDirtied(node.id);
			}
		}
	}


	void SemanticAnalyzer::checkCasting() {
		traverseGraph(
			[&](Graph::NodeID nodeID) {
				checkCastingForNode(nodeID);
			}
		);
	}

	
	void SemanticAnalyzer::checkCastingForNode(Graph::NodeID nodeID) {
		if (isDirty(nodeID))
			return;

		const Graph::Node& node = m_nodes.at(nodeID);

		// For each input pin, check if the data link is compatible (i.e., the type of the source data can be converted to THIS node's input pin type)
		for (const auto& inPin : node.inputPins) {
			std::visit(
				[&](const auto& pin) {
					// Get source type
					std::type_index sourceType = pin.type;

					if (m_nodeInLinks.contains(node.id))
						for (const Graph::Link& inLink : m_nodeInLinks.at(node.id))
							if (inLink.inPinID == pin.label) {
								checkCastingForNode(inLink.outNodeID);

								for (const auto& outPin : m_nodes.at(inLink.outNodeID).get().outputPins)
									if (outPin.label == inLink.outPinID) {
										if (m_wildcardOutputs.contains(inLink.outNodeID))
											sourceType = m_wildcardOutputs.at(inLink.outNodeID);
										else
											sourceType = outPin.type;
								
										break;
									}

								break;
							}


					if (
						// Only begin the check if the output pin's type is a different type than the input pin's;
						// otherwise, the two pins have the same type, so no conversion will be performed (and thus, no need for checking)
						sourceType == pin.type
					)
						return;

					std::type_index destType = pin.type;

					if (Graph::IsNonPrimitive(destType))
						destType = Graph::HighLevelTypeToPrimitive(destType);

					if (
						AsTL::ValidConversionMap.contains(destType) &&
						!AsTL::ValidConversionMap.at(destType).contains(sourceType)
					) {
						m_reporter.report(
							Diagnostics::Diagnostic::Severity::Error,
							Diagnostics::Diagnostic::DiagType::Semantic,
							node.id,
							pin.label,
							"Incompatible data link to Input Pin \"{}\": No conversion exists between output type {} and input type {}",
							pin.label,
							AsTL::StackValueToRepString(sourceType),
							AsTL::StackValueToRepString(destType)
						);

						markAsDirtied(node.id, Impl::DirtyPin{ Impl::DirtyPin::IN_PIN, pin.label });
					}
				},
				inPin
			);
		}
	}


	void SemanticAnalyzer::markAsDirtied(Graph::NodeID dirtyNodeID, std::optional<Impl::DirtyPin> dirtyPin) {
		if (dirtyPin.has_value())
			m_dirtied[dirtyNodeID].insert(dirtyPin.value());
		else if (!m_dirtied.contains(dirtyNodeID))
			m_dirtied[dirtyNodeID] = {};
	}


	bool SemanticAnalyzer::isDirty(Graph::NodeID dirtyNodeID, std::optional<Impl::DirtyPin> dirtyPin) const {
		if (!m_dirtied.contains(dirtyNodeID))
			return false;

		// Check if node's pin is dirty
		if (dirtyPin.has_value())
			return m_dirtied.at(dirtyNodeID).contains(dirtyPin.value());
		
		// Check if node is dirty (true here, due to the guardrail at the start)
		return true;
	}

} // namespace Compiler
