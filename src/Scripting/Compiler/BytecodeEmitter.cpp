#include "BytecodeEmitter.hpp"


namespace Compiler {
	BytecodeEmitter::BytecodeEmitter(
		const std::reference_wrapper<IGraphNodeRegistry> nodeRegistry,
		Diagnostics::DiagReporter &reporter,
		ConstantPool &constPool
	) :
		m_nodeRegistry(nodeRegistry.get()),
		m_reporter(reporter),
		m_constPool(constPool)
	{
		resetEmitter();
	}


	std::vector<SymbolicInstruction> Compiler::BytecodeEmitter::emitSymbolic(
		const std::vector<Graph::Variable>& variables,
		const std::vector<Graph::Node>& nodes,
		const std::vector<Graph::Link>& nodeLinks
	) {
		using namespace AsTL;

		resetEmitter();

		// Cache node data and create lookup tables
		{
			// Nodes & Graph Variables
			for (const auto& node : nodes) {
				// Cache node
				// std::reference_wrapper doesn't have a default constructor, so we use `emplace` to directly construct it with a value instead of operator[],
				// which attempts to default-construct it first
				m_nodeCache.emplace(node.id, node);

				if (node.symbol == Graph::GetterNodeSymbol && !m_graphVarCache.contains(node.id)) {
					// Find & cache variable from a Getter node
					const auto& comboInPin = std::get<Graph::Node::ComboInPin>(node.inputPins[0]);
					for (const auto &var : variables)
						if (var.name == comboInPin.chosenVar.value()) {
							m_graphVarCache.emplace(node.id, var);
							break;
						}
				}
			}


			// Node Links
			for (const auto &link : nodeLinks) {
				// Cache outgoing links
				if (!m_outLinkCache.contains(link.outNodeID))
					for (const auto& otherLink : nodeLinks)
						if (link.outNodeID == otherLink.outNodeID)
							m_outLinkCache[link.outNodeID].emplace_back(otherLink);

				// Cache incoming links
				if (!m_inLinkCache.contains(link.inNodeID))
					for (const auto& otherLink : nodeLinks)
						if (link.inNodeID == otherLink.inNodeID)
							m_inLinkCache[link.inNodeID].emplace_back(otherLink);

				// Cache data connections
				if (link.linkType == Graph::Link::LinkType::DATA)
					m_directDataLinks[link.outNodeID].insert(link.inNodeID);
			}
		}

		createEvalOrder();
		preallocGraph();
		compileGraph();
		backpatch();

		emitInstruction(SymbolicInstruction(Opcode::TERMINATE, static_cast<I16>(VMExitCode::SUCCESS)));

		return std::move(m_instructions);
	}


	std::vector<RawInstructionT> BytecodeEmitter::emitEncoded(
		const std::vector<Graph::Variable>& variables,
		const std::vector<Graph::Node>& nodes,
		const std::vector<Graph::Link>& nodeLinks
	) {
		std::vector<SymbolicInstruction> instructions = emitSymbolic(variables, nodes, nodeLinks);

		std::vector<RawInstructionT> rawInstructions{};
		rawInstructions.reserve(instructions.size());

		for (const auto& ins : instructions)
			rawInstructions.push_back(CompilerUtils::Assemble(ins));

		/* NOTE: We don't use std::move(rawInstructions) here not because it's unsafe, but because doing so
			prevents the compiler from optimizing it with the Named Return Value Optimization (NRVO) technique.
			When returning a local variable by value, modern compilers use NRVO, which constructs that variable
			directly inside the memory space allocated by the caller without performing any copies/moves.

			However, compilers don't apply this optimization to class member variables such as `m_instructions`, which
			is why we still use `std::move(m_instructions)` in `BytecodeEmitter::emitSymbolic`.
		*/
		return rawInstructions;
	}


	void Compiler::BytecodeEmitter::createEvalOrder() {
		createEvalOrderFrom(Graph::EntryNodeID, Graph::EntryNodeID);
	}


	void Compiler::BytecodeEmitter::createEvalOrderFrom(Graph::NodeID startNodeID, Graph::NodeID prevNodeID) {
		if (m_evaluatedNodes.contains(startNodeID))
			return;

		if (m_inLinkCache.contains(startNodeID))
			for (const Graph::Link& link : m_inLinkCache.at(startNodeID))
				if (link.linkType == Graph::Link::LinkType::DATA)
					createEvalOrderFrom(link.outNodeID, startNodeID);

		m_nodeEvalOrder.push_back(startNodeID);
		m_evaluatedNodes.insert(startNodeID);

		if (m_outLinkCache.contains(startNodeID))
			for (const Graph::Link& link : m_outLinkCache.at(startNodeID))
				if (link.linkType == Graph::Link::LinkType::EXEC)
					createEvalOrderFrom(link.inNodeID, startNodeID);
	}


	void BytecodeEmitter::preallocGraph() {
		for (size_t i = 0; i < m_nodeEvalOrder.size(); ++i) {
			const Graph::NodeID nodeID = m_nodeEvalOrder[i];

			// Conditionally pre-allocate each output pin in THIS node
			if (m_nodeCache.contains(nodeID)) {
				const Graph::Node& node = m_nodeCache.at(nodeID);

				for (const auto& outPin : node.outputPins) {
					// Decide if the pin's value should be cached in the global variable registry
					const std::string preallocVarName = getPreallocOutPinName(nodeID, outPin.label, outPin.type);

					if (
						// CASE: The node is a Getter node
						(node.symbol == Graph::GetterNodeSymbol) ||

						// CASE: The node is a non-Getter node
						shouldPreallocOutputPin(nodeID, outPin.label)
					) {

						AsTL::IDX poolIdx{};
						if (m_graphVarCache.contains(nodeID))
							// This is a special Getter node; we load the target variable's value from the const pool
							poolIdx = storeInConstPool(m_graphVarCache.at(nodeID).get().val);
						else
							// This is a normal node; we load a default dummy value from the const pool
							poolIdx = storeInConstPool(AsTL::GetDefaultStackValue(outPin.type));

						emitInstruction(SymbolicInstruction(Opcode::LOAD_CONST, poolIdx));

						AsTL::IDX globRegIdx = simulateStoreInGlobReg(
							nodeID,
							outPin.label,
							InstructionMask{},
							StackValueToByte(outPin.type)
						);

						m_varGlRegIdxName.emplace(preallocVarName, globRegIdx);
					}
				}
			}
		}
	}


	std::string BytecodeEmitter::getPreallocOutPinName(Graph::NodeID nodeID, const std::string& pinLabel, std::type_index pinType) {
		return std::format("{}_{}_{}",
			std::to_string(nodeID),
			AsTL::StackValueToString(pinType),
			std::regex_replace(pinLabel, std::regex(" "), "-")
		);
	}


	void BytecodeEmitter::compileGraph() {
		compileGraphFrom(Graph::EntryNodeID, Graph::EntryNodeID);
	}


	void BytecodeEmitter::compileGraphFrom(Graph::NodeID startNodeID, Graph::NodeID prevNodeID) {
		if (
			(!m_outLinkCache.contains(startNodeID) && !m_inLinkCache.contains(startNodeID)) ||
			m_compiledNodes.contains(startNodeID)
		)
			return;

		// Compile node
		if (startNodeID == Graph::EntryNodeID)		compileNode(startNodeID);
		else if (startNodeID == Graph::TermNodeID)	compileNode(startNodeID);
		else
			for (const Graph::Link& link : m_outLinkCache.at(prevNodeID))
				if (link.inNodeID == startNodeID) {
					compileNode(startNodeID);
					break;
				}

		// Continue along execution path to other executable nodes
		if (m_outLinkCache.contains(startNodeID))
			for (const Graph::Link& link : m_outLinkCache.at(startNodeID))
				if (link.linkType == Graph::Link::LinkType::EXEC)
					compileGraphFrom(link.inNodeID, startNodeID);
	}


	void BytecodeEmitter::compileNode(Graph::NodeID nodeID) {
		using namespace AsTL;

		if (nodeID == Graph::EntryNodeID) {

			for (const Graph::Link& link : m_outLinkCache.at(nodeID)) {
				if (link.linkType == Graph::Link::LinkType::EXEC) {
					// Store triggered pin label of the next node after the entry node
					IDX valIdx = storeInConstPool(link.inPinID);
					emitInstruction(SymbolicInstruction(Opcode::LOAD_CONST, valIdx));
					emitInstruction(SymbolicInstruction(Opcode::STORE_SPR, SPR_EXC));

					// Call the next node to kickstart the execution flow;
					m_backpatchQueue.emplace_back(m_instructions.size());
					emitInstruction(SymbolicInstruction(Opcode::CALL, static_cast<I16>(link.inNodeID)));
					AsTL::IDX mainCallInsAddr = m_instructions.size() - 1;

					// if any node calls RET in the main function scope, the graph has finished execution for this simulation tick; rerun the graph for the next tick
					// Termination with the special FINISHED_EXEC_TICK exit code triggers a trap that notifies the VM caller of the execution finish for the current simulation tick.
					// The VM caller can then call `VirtualMachine::resume` after updating the simulation tick, whereupon the VM should continue at `CALL [main call instruction address]`
					emitInstruction(SymbolicInstruction(Opcode::TERMINATE, static_cast<I16>(VMExitCode::FINISHED_EXEC_TICK)));
					emitInstruction(SymbolicInstruction(Opcode::CALL, static_cast<IDX>(mainCallInsAddr)));

					return;
				}
			}
			return;
		}
		if (nodeID == Graph::TermNodeID) {

			m_nodeAddresses[nodeID].trueAddr.start = m_instructions.size();
			m_nodeAddresses[nodeID].completeAddr.start = m_instructions.size();
			{
				emitInstruction(SymbolicInstruction(Opcode::TERMINATE, static_cast<I16>(VMExitCode::SUCCESS)));
			}
			m_nodeAddresses[nodeID].trueAddr.end = m_instructions.size() - 1;  // Start and end addresses all point to the same memory block of the instruction
			m_nodeAddresses[nodeID].completeAddr.end = m_instructions.size() - 1;

			return;
		}


		// ========== EMIT INSTRUCTIONS FOR THIS NODE ==========

		const Graph::Node& node = m_nodeCache.at(nodeID);
		const auto& [nodeFuncIdx, nodeDescriptor] = m_nodeRegistry.getInfo(node.symbol);


		if (node.symbol == Graph::GetterNodeSymbol)
			// Default Getter nodes don't need to be compiled, because their values already exist in the global registry;
			// Only the calling node needs to grab them from the registry
			return;


		std::vector<std::type_index> nodeDataInPinTypes{};
		std::vector<std::type_index> nodeDataOutPinTypes{};

		m_nodeAddresses[nodeID].trueAddr.start = m_instructions.size();
		m_nodeAddresses[nodeID].completeAddr.start = m_instructions.size();
		{
			// Process Input Pins
			{
				nodeDataInPinTypes.reserve(node.inputPins.size());


				auto evaluateDataPin = [&](const Graph::Node::DataInPin& dataPin) -> void {
					nodeDataInPinTypes.push_back(dataPin.type);

					// The data pin's value either comes from an incoming link, or is directly specified
					bool valueFromLink = false;
					Graph::Link valueLink{};

					if (m_inLinkCache.contains(nodeID))
						for (const Graph::Link& inLink : m_inLinkCache.at(nodeID))
							if (
								inLink.linkType == Graph::Link::LinkType::DATA &&
								inLink.inPinID == dataPin.label
							) {
								valueFromLink = true;
								valueLink = inLink;

								break;
							}


					if (valueFromLink) {
						// Value comes from a Data Link

						const Graph::Node& outNode = m_nodeCache.at(valueLink.outNodeID);

						if (!m_compiledNodes.contains(valueLink.outNodeID)) {
							// The Output Node has not been compiled it yet; resolve Data Link by compiling it before evaluating load instruction
							compileNode(valueLink.outNodeID);
									
							// Update new starting address of THIS node
							m_nodeAddresses[nodeID].trueAddr.start = m_instructions.size();
						}

						// Evaluate load instruction for this Input Pin
						if (m_constPoolIdxCache.contains(valueLink.outNodeID)) {
							// The value of the Output Node already exists in the constant pool
							AsTL::IDX valIdx = m_constPoolIdxCache.at(valueLink.outNodeID);
							emitInstruction(SymbolicInstruction(Opcode::LOAD_CONST, valIdx));
						}
						else if (
							outNode.symbol == Graph::GetterNodeSymbol ||
							(
								m_glReg.contains(valueLink.outNodeID) &&
								m_glReg.at(valueLink.outNodeID).count(valueLink.outPinID) &&
								shouldPreallocOutputPin(valueLink.outNodeID, valueLink.outPinID)
							)
						) {
							// The value of the Output Node is already stored in the global registry
							auto glRegIdx = m_glReg.at(valueLink.outNodeID).at(valueLink.outPinID);
							emitInstruction(SymbolicInstruction(Opcode::LOAD_GL, glRegIdx));
						}
					}
					else {
						// Value is directly specified; load as constant
						AsTL::IDX valIdx = storeInConstPool(dataPin.val);
						emitInstruction(SymbolicInstruction(Opcode::LOAD_CONST, valIdx));
					}


					// If the input pin value's type does not match the descriptor type of the corresponding input parameter,
					// attempt to cast the value to the descriptor type
					for (const auto& inParam : nodeDescriptor.inParams) {
						if (
							inParam.paramClass == GraphNodeDescriptor::Parameter::ParamClass::Data &&
							inParam.label == dataPin.label &&
							inParam.type != dataPin.type
						) {
							// Wildcard/Non-primitive types
							if (AsTL::IsWildcard(inParam.type) || Graph::IsNonPrimitive(inParam.type)) {
								// Input parameter accepts any type within the wildcard, or accepts a high-level type; skip casting
								// NOTE: For high-level types, the actual input value must be resolved to a concrete primitive type
								// in an earlier compilation stage than this bytecode emission stage
								break;
							}

							// Otherwise, input parameter requires specific type; start conversion
							AsTL::BYTE inParamByteType = StackValueToByte(inParam.type);
							AsTL::BYTE inPinByteType = StackValueToByte(dataPin.type);

							emitCastInstruction(inPinByteType, inParamByteType);
							break;
						}
					}
				};


				auto evaluateComboPin = [&](const Graph::Node::ComboInPin& comboPin) -> void {
					nodeDataInPinTypes.push_back(comboPin.type);

					// The combo-box pin's value either comes from an incoming link, or is directly specified (only possible if it's the default Getter node)
					bool valueFromLink = false;

					Graph::Link valueLink{};
					AsTL::STR varName{};

					if (m_inLinkCache.contains(nodeID))
						for (const Graph::Link& inLink : m_inLinkCache.at(nodeID))
							if (
								inLink.linkType == Graph::Link::LinkType::DATA &&
								inLink.inPinID == comboPin.label
							) {
								valueFromLink = true;
								valueLink = inLink;

								break;
							}


					if (valueFromLink)
						// Value comes from an incoming link
						varName = getPreallocOutPinName(valueLink.outNodeID, valueLink.outPinID, comboPin.type);
					else
						// Value is directly specified (i.e., the node is the default Getter node)
						varName = getPreallocOutPinName(nodeID, Graph::GetterOutputDataPin, comboPin.type);

					emitInstruction(
						SymbolicInstruction(
							Opcode::LOAD_GL,
							m_varGlRegIdxName.at(varName)
						)
					);
				};


				// SPECIAL CASE: Node is a Setter node
				if (node.symbol == Graph::SetterNodeSymbol) {
					// Setter load logic: overwrite a new value into a specified (existing) variable in the global registry

					for (const auto& inPin : node.inputPins) {
						if (std::holds_alternative<Graph::Node::ComboInPin>(inPin)) {
							// Make sure the data pin is evaluated first (so that the value lies on top of the VM stack)
							for (const auto& otherInPin : node.inputPins)
								if (std::holds_alternative<Graph::Node::DataInPin>(otherInPin)) {
									evaluateDataPin(
										std::get<Graph::Node::DataInPin>(otherInPin)
									);
								}


							// Then evaluate combo pin (variable name)
							const auto& comboPin = std::get<Graph::Node::ComboInPin>(inPin);
							for (const auto& [varGetterID, varRef] : m_graphVarCache) {
								if (comboPin.chosenVar.value() == varRef.get().name) {
									AsTL::IDX globRegIdx = m_varGlRegIdxName.at(
										getPreallocOutPinName(varGetterID, Graph::GetterOutputDataPin, comboPin.type)
									);

									// Overwrite the current value in the global registry with the new value
									emitInstruction(SymbolicInstruction(Opcode::OVR_GL, globRegIdx));
								}
							}
						}
					}
				}


				// General Input Pins
				else
					for (const auto& inPin : node.inputPins) {
						std::visit(
							OverloadedVisit{
								[&](const Graph::Node::DataInPin& dataPin)		{ evaluateDataPin(dataPin); },
								[&](const Graph::Node::ComboInPin& comboPin)	{ evaluateComboPin(comboPin); }
							},
							inPin
						);
					}
			}



			// Process Output Pins
			nodeDataOutPinTypes.reserve(node.outputPins.size());
			for (const auto& outPin : node.outputPins)
				nodeDataOutPinTypes.push_back(outPin.type);



			// Process Node Callable
			bool isNativeCallable = false;
			if (nodeDescriptor.callback.has_value()) {
				// Helper to resolve input parameter type
				const auto getInType = [&](size_t index) -> std::type_index {
					return (
						AsTL::IsWildcard(nodeDescriptor.inParams[index].type) ||
						Graph::IsNonPrimitive(nodeDescriptor.inParams[index].type)
					) ?
						nodeDataInPinTypes[index] :
						nodeDescriptor.inParams[index].type;
				};

				// Helper to resolve output parameter type
				const auto getOutType = [&](size_t index) -> std::type_index {
					return (
						AsTL::IsWildcard(nodeDescriptor.outParams[index].type) ||
						Graph::IsNonPrimitive(nodeDescriptor.outParams[index].type)
					) ?
						nodeDataOutPinTypes[index] :
						nodeDescriptor.outParams[index].type;
				};


				// Invoke node callable
				std::visit(
					OverloadedVisit {
						// CASE: Node is an AstroAssembly instruction
						[&](Opcode op) {
							// If the node is the AstroAssembly instruction TERMINATE, emit it and return early
							if (op == Opcode::TERMINATE) {
								emitInstruction(SymbolicInstruction(op, static_cast<I16>(VMExitCode::SUCCESS)));
								return;
							}


							// Check node input parameters and emit instructions accordingly
							switch (nodeDescriptor.inParams.size()) {
							case 0:
								emitInstruction(SymbolicInstruction(op));
								break;

							case 1:
								emitInstruction(
									SymbolicInstruction(
										op,
										StackValueToByte(getInType(0))
									)
								);
								break;

							case 2:
								// Specify stack slot types according to the Input Pin processing logic above
								emitInstruction(
									SymbolicInstruction(
										op,
										std::make_pair(
											StackValueToByte(getInType(0)),
											StackValueToByte(getInType(1))
										)
									)
								);
								break;
							}
						},

						// CASE: Node has a native C++ callback
						[&](GraphNodeCallback callback) {
							/*
							if (nodeDescriptor.nodeClass == GraphNodeDescriptor::NodeClass::Constant) {
								// Optimization: If the Node class is Constant, execute the function now and store the result as a constant
								std::vector<StackValue> retVals = callback(nullptr, &nodeDescriptor, nodeID, "", {});
								AsTL::IDX valIdx = storeInConstPool(retVals[0]);

								m_constPoolIdxCache[nodeID] = valIdx;
							}
							else {
								// Else, let the VM execute the callback
								isNativeCallable = true;
								emitInstruction(SymbolicInstruction(Opcode::LOAD_INLINE, static_cast<AsTL::I16>(nodeID)));
								emitInstruction(SymbolicInstruction(Opcode::EXEC_NATIVE, nodeFuncIdx));
							}
							*/
							isNativeCallable = true;
							emitInstruction(SymbolicInstruction(Opcode::LOAD_INLINE, static_cast<AsTL::I16>(nodeID)));
							emitInstruction(SymbolicInstruction(Opcode::EXEC_NATIVE, nodeFuncIdx));
						}
					},
					nodeDescriptor.callback.value()
				);


				// If the callback returns anything (and the returned values are not in the constant table), we save the return values in the global registry in the correct order, 
				// but ONLY if it is to be used elsewhere rather than only immediately used in the calling node (i.e., only one DATA link for this node)

				// For `k` Execution Output Pins and `m` Parameter Output Pins, Callable return values in the order: [OutExecs[0...k-1], OutParams[0...m-1]]
				// STORE_XXX stores the top stack value into the registry, so we have to store the values in opposite order: [OutParams[m-1...0], OutExecs[k-1...0]]
				for (size_t i = nodeDescriptor.outParams.size(); i-- > 0;) {
					if (shouldStoreIntoGlobReg(nodeID, nodeDescriptor.outParams[i].label)) {
						const std::string preallocVarName = getPreallocOutPinName(nodeID, nodeDescriptor.outParams[i].label, getOutType(i));

						// Output is going to be cached as a (hidden) graph variable; update existing variable in global registry
						emitInstruction(
							SymbolicInstruction(
								Opcode::OVR_GL,
								InstructionMask{},
								m_varGlRegIdxName.at(preallocVarName)
							)
						);
					}
				}
			}



			// Process Output Execution Pins
			if (nodeDescriptor.isExecutable()) {
				bool hasDefaultOutExec = false;
				bool requiresReEvaluation =
					(nodeDescriptor.nodeClass == Compiler::GraphNodeDescriptor::NodeClass::ControlFlowLoop);

				/* For each Output Exec Pin, if it is connected to elsewhere, emit:
					- For EXEC_OUT: An unconditional JUMP/CALL with a placeholder address pointing to the next node's Complete Address
					- For other exec pins: A conditional JUMP_IF_TRUE/CALL_IF_TRUE with a placeholder address pointing to the next node's Complete Address

					This approach could create situations where evaluated boolean results for inactive execution branches remain on the VM
						if any but the last jump/call instruction actually results in a jump.

					To solve this:
					1) Process the Output Exec Pins and record the instruction offset for the jump/call instructions (and emit them as usual)
					2) Emit the fallback JUMP/RET instruction as usual
					3) For each replaced jump instruction
						+ emit `POP <y>` and point the jump instruction to it (where `y` is the number of remaining Output Exec booleans)
						+ emit the real jump instruction pointing to the real next address

					The placeholders will later be replaced via backpatching.
				*/

				// vector<pair<Instruction Address, Placeholder Address>>
				struct RealInstruction {
					size_t remainingExecs;
					size_t insAddr;
					I16 placeholderAddr;
				};
				std::vector<RealInstruction> realInstructions{};


				for (size_t i = nodeDescriptor.outExecs.size(); i-- > 0;) {
					const std::string &execPinID = nodeDescriptor.outExecs[i];

					bool isLinkedPin = false;

					if (m_outLinkCache.contains(nodeID))
						for (const Graph::Link& link : m_outLinkCache.at(nodeID)) {

							if (link.linkType == Graph::Link::LinkType::EXEC && link.outPinID == execPinID) {
								isLinkedPin = true;

								// For each exec pin:
								// 1. Set Register EXC to the label of the triggered Input Exec Pin of the Input Node connected to THIS node's output exec pin
								// 2. Emit the status of the triggered Output Exec Pin of THIS node

								// Set EXC
								IDX valIdx = storeInConstPool(link.inPinID);
								emitInstruction(SymbolicInstruction(Opcode::LOAD_CONST, valIdx));
								emitInstruction(SymbolicInstruction(Opcode::STORE_SPR, SPR_EXC));

								// Placeholder address is the node of the Input node connected to THIS node's Output Exec Pin
								I16 placeholderAddr = static_cast<I16>(link.inNodeID);
								{
									realInstructions.push_back(RealInstruction{ i, m_instructions.size(), placeholderAddr });

									if (execPinID == Graph::ExecOutID) {
										emitInstruction(SymbolicInstruction(Opcode::JUMP, placeholderAddr));
										hasDefaultOutExec = true;
									}
									else
										emitInstruction(
											SymbolicInstruction(
												requiresReEvaluation ?
													Opcode::CALL_IF_TRUE :
													Opcode::JUMP_IF_TRUE,

												placeholderAddr
											)
										);
								}


								break;
							}
						}

					if (!isLinkedPin && execPinID != Graph::ExecOutID) {
						// If the execution pin is not connected to anything, we pop its boolean state off the VM stack
						emitInstruction(SymbolicInstruction(Opcode::POP, static_cast<I16>(1)));
					}
				}


				// If Node does not have the default EXEC_OUT pin (and no specific output exec pin was triggered), the execution flow stops at THIS node
				if (!hasDefaultOutExec) {
					if (requiresReEvaluation)
						// Node requires re-evaluation; jump back to its starting address
						emitInstruction(SymbolicInstruction(Opcode::JUMP, m_nodeAddresses[nodeID].completeAddr.start));
					else
						emitInstruction(SymbolicInstruction(Opcode::RET));
				}


				// Resolution for inactive execution branches' boolean states remaining on the VM stack
				for (const auto& ins : realInstructions) {
					if (ins.remainingExecs > 0) {
						IDX popAddr = m_instructions.size();

						m_instructions[ins.insAddr].operand = popAddr;

						emitInstruction(
							SymbolicInstruction(
								Opcode::POP,
								static_cast<I16>(ins.remainingExecs)
							)
						);

						m_backpatchQueue.emplace_back(m_instructions.size());
						emitInstruction(SymbolicInstruction(Opcode::JUMP, ins.placeholderAddr));

						continue;
					}
					
					m_backpatchQueue.emplace_back(ins.insAddr);
				}
			}
		}
		m_nodeAddresses[nodeID].trueAddr.end = m_instructions.size();
		m_nodeAddresses[nodeID].completeAddr.end = m_instructions.size();

		
		auto &[startAddr, endAddr] = m_nodeAddresses[nodeID].trueAddr;
		if (startAddr == endAddr) {
			// No instructions were emitted for this node, so it should not have an address in the instruction stream
			m_nodeAddresses.erase(nodeID);
		}
		else {
			// The node's ending address should be decremented by 1, since the next instruction address starts at `m_instructions.size()`
			--endAddr;
			--m_nodeAddresses[nodeID].completeAddr.end;

			m_compiledNodes.insert(nodeID);
		}
	}


	void BytecodeEmitter::backpatch() {
		for (const auto idx : m_backpatchQueue) {
			const Graph::NodeID nodeID = static_cast<Graph::NodeID>(
				std::get<AsTL::I16>(m_instructions[idx].operand.value())
			);

			m_instructions[idx].operand = m_nodeAddresses[nodeID].completeAddr.start;
		}
	}


	void BytecodeEmitter::emitCastInstruction(AsTL::BYTE startType, AsTL::BYTE destType) {
		using enum OperandType;
		switch (destType) {
		case OT_STR:
			emitInstruction(SymbolicInstruction(Opcode::TO_STR, startType));
			break;
		case OT_I16:
			emitInstruction(SymbolicInstruction(Opcode::TO_I16, startType));
			break;
		case OT_I32:
			emitInstruction(SymbolicInstruction(Opcode::TO_I32, startType));
			break;
		case OT_F64:
			emitInstruction(SymbolicInstruction(Opcode::TO_F64, startType));
			break;

			// default: input pin value cannot be casted to parameter type; this should've been caught in semantic analysis
		}
	}


	bool BytecodeEmitter::shouldLoadFromGlobReg(Graph::NodeID outNodeID, const std::string& outPinLabel, Graph::NodeID inNodeID) {
		const Graph::Node& outNode = m_nodeCache.at(outNodeID).get();

		if (
			!m_directDataLinks.contains(outNodeID) ||
			!m_nodeAddresses.contains(outNodeID) ||
			!m_nodeAddresses.contains(inNodeID) ||
			!m_nodeRegistry.contains(outNode.symbol)
		)
			return false;

		//const auto& [_, descriptor] = m_nodeRegistry.getInfo(outNode.symbol);
		//
		//std::string potentialVarName{};
		//for (const auto& outPin : outNode.outputPins)
		//	if (outPin.label == outPinLabel)
		//		potentialVarName = getPreallocOutPinName(outNodeID, outPinLabel, outPin.type);

		return (
			m_glReg.contains(outNodeID) &&
			m_glReg.at(outNodeID).count(outPinLabel) &&

			// The instruction block of the Output Node is NOT next to that of the Input Node
			// (because in that case, the value of the Output Node should already be at the top of the VM stack
			// by the time the Input Node is evaluated)
			m_nodeAddresses[outNodeID].trueAddr.end + 1 != m_nodeAddresses[inNodeID].trueAddr.start

			/*
			// CASE: The Output Node's callback function is a native C++ implementation rather than an AstroAssembly instruction
			// Native callbacks are allowed to have arbitrary return lists, which can affect the VM stack layout;
			// in contrast, AstroAssembly instructions that return anything only push one return value to the VM stack
			(descriptor.callback.has_value() && std::holds_alternative<GraphNodeCallback>(descriptor.callback.value())) ||

			// CASE: The pin exists as a Graph variable (explicit variable or hidden/generated)
			(m_varGlRegIdxName.contains(potentialVarName)) ||

			// Degenerate case conditions
			(
				// The value of the Output Node is going to be reused for multiple Input Nodes
				m_directDataLinks[outNodeID].size() > 1 &&

				// The Output Node has already been compiled
				m_compiledNodes.contains(outNodeID) &&

				// The instruction block of the Output Node is NOT next to that of the Input Node
				// (because in that case, the value of the Output Node should already be at the top of the VM stack
				// by the time the Input Node is evaluated)
				m_nodeAddresses[outNodeID].trueAddr.end + 1 != m_nodeAddresses[inNodeID].trueAddr.start
			)
			*/
		);
	}


	bool BytecodeEmitter::shouldStoreIntoGlobReg(Graph::NodeID nodeID, const std::string& outPinLabel) {
		const Graph::Node& outNode = m_nodeCache.at(nodeID).get();

		if (
			!m_directDataLinks.contains(nodeID) ||
			!m_nodeAddresses.contains(nodeID) ||
			!m_nodeRegistry.contains(outNode.symbol)
		)
			return false;

		//const auto& [_, descriptor] = m_nodeRegistry.getInfo(outNode.symbol);

		std::string potentialVarName{};

		for (const auto& outPin : outNode.outputPins)
			if (outPin.label == outPinLabel)
				potentialVarName = getPreallocOutPinName(nodeID, outPinLabel, outPin.type);


		return (
			// The pin exists as a Graph variable (explicit variable or hidden/generated)
			(m_varGlRegIdxName.contains(potentialVarName)) &&

			shouldPreallocOutputPin(nodeID, outPinLabel)
		);
	}
	

	bool BytecodeEmitter::shouldPreallocOutputPin(Graph::NodeID nodeID, const std::string& outPinLabel) {
		if (!m_nodeCache.contains(nodeID))
			return false;

		const Graph::Node& outNode = m_nodeCache.at(nodeID).get();

		size_t pinLinkCnt = 0;
		bool onlyLinkIsToNextNode = false;
		std::string potentialVarName{};

		for (const auto& outPin : outNode.outputPins) {
			if (outPin.label == outPinLabel)
				potentialVarName = getPreallocOutPinName(nodeID, outPinLabel, outPin.type);


			// Get number of outgoing data wires from this output pin
			size_t evalIdx = 0;
			for (size_t i = 0; i < m_nodeEvalOrder.size(); ++i)
				if (m_nodeEvalOrder[i] == nodeID) {
					evalIdx = i;
					break;
				}

			for (const Graph::Link& link : m_outLinkCache.at(nodeID))
				if (link.linkType == Graph::Link::LinkType::DATA &&
					link.outPinID == outPin.label
					) {
					++pinLinkCnt;

					if (evalIdx + 1 < m_nodeEvalOrder.size() && m_nodeEvalOrder[evalIdx + 1] == link.inNodeID)
						onlyLinkIsToNextNode = true;
				}
		}

		return (
			// Output pin is not already stored in the constant pool
			!m_constPoolIdxCache.contains(nodeID) &&
			(
				// Output pin is connected to multiple nodes
				pinLinkCnt > 1 ||

				// Output pin is connected to one single node, and that node is not the closest parent to THIS node
				(pinLinkCnt == 1 && !onlyLinkIsToNextNode) ||

				// Output pin has at least one Output Execution Pin
				// (Execution Pins dictate control flow, and thus dictate the VM stack state;
				// it's safer to always cache output data pins if the condition is true, rather than assume they always remain on top of the VM stack
				// even after the control flow redirection caused by the pins)
				!outNode.execOutPins.empty()
			)
		);
	}


	AsTL::IDX BytecodeEmitter::storeInConstPool(const AsTL::StackValue &val) {
		return m_constPool.getOrCreateIndex(val);
	}


	AsTL::IDX BytecodeEmitter::simulateStoreInGlobReg(Graph::NodeID nodeID, const std::string &pinLabel, const InstructionMask bitmask, AsTL::BYTE type) {
		m_glReg[nodeID][pinLabel] = m_glRegIdx;
		emitInstruction(SymbolicInstruction(Opcode::STORE_GL, bitmask, type));
		return m_glRegIdx++;
	}


	void BytecodeEmitter::emitInstruction(SymbolicInstruction &&instruction) {
		m_instructions.emplace_back(std::forward<SymbolicInstruction>(instruction));
		++m_line;
	}


	void Compiler::BytecodeEmitter::resetEmitter() {
		m_line = 1;
		m_col = 1;

		m_instructions.clear();

		m_varGlRegIdxName.clear();
		m_nodeCache.clear();
		m_outLinkCache.clear();
		m_inLinkCache.clear();

		m_glReg.clear();
		m_glRegIdx = 0;

		m_compiledNodes.clear();
	}
}
