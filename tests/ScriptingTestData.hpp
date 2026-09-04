#pragma once

#include <vector>

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/GraphTypes.hpp>


namespace TestProgram::Impl {
	using enum Graph::Link::LinkType;
	using enum Graph::ClassScope;
	using enum Graph::CatScope;
	using enum Graph::FuncScope;


	struct TestInput {
		std::vector<Graph::Variable> variables;
		std::vector<Graph::Node> nodes;
		std::vector<Graph::Link> links;
	};

	inline const TestInput SimpleGraph = {
		.variables = {},

		.nodes = {
			Graph::Node{
				1,
				Graph::MakeQualifiedID(Math, Constant, Pi),
				{}, {}, {},
				{
					{ "Value", AsTL::TID_F64 }
				}
			},
			Graph::Node{
				2,
				Graph::MakeQualifiedID(Math, Arithmetic, Multiply),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32, 20 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},
			Graph::Node{
				3,
				Graph::MakeQualifiedID(Math, Logic, GreaterThan),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},
			Graph::Node{
				4,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},
			Graph::Node{
				5,
				Graph::MakeQualifiedID(Control, DoOnce),
				{ Graph::ExecInID, "Reset" },
				{ "Out" },
				{
					Graph::Node::DataInPin{ "Start Closed", AsTL::TID_BOOL, false }
				},
				{}
			},
			Graph::Node{
				6,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR, "Got here from the True branch!" }
				},
				{}
			},
			Graph::Node{
				7,
				Graph::MakeQualifiedID(Control, DoOnce),
				{ Graph::ExecInID, "Reset" },
				{ "Out" },
				{
					Graph::Node::DataInPin{ "Start Closed", AsTL::TID_BOOL, false }
				},
				{}
			},
			Graph::Node{
				8,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR, "Got here from the False branch!" }
				},
				{}
			},

			Graph::Node{
				9,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_F64 }
				},
				{}
			}
		},


		.links = {
			// Execution Links
			Graph::Link{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 4, Graph::ExecInID },	// START				 [EXEC_OUT]  ->  [EXEC_IN] Control::Branch (4)
			Graph::Link{ EXEC, 4, "True", 5, Graph::ExecInID },								// Control::Branch (4)	     [True]  ->  [EXEC_IN] Control::DoOnce (5)
			Graph::Link{ EXEC, 4, "False", 7, Graph::ExecInID },							// Control::Branch (4)		[False]  ->  [EXEC_IN] Control::DoOnce (7)
			Graph::Link{ EXEC, 5, "Out", 6, Graph::ExecInID, },								// Control::DoOnce (5)	 [EXEC_OUT]  ->  [EXEC_IN] Console::Print  (6)
			Graph::Link{ EXEC, 6, Graph::ExecOutID, 9, Graph::ExecInID },					// Console::Print (6)	 [EXEC_OUT]  ->  [EXEC_IN] Console::Print  (9)
			Graph::Link{ EXEC, 9, Graph::ExecOutID, Graph::TermNodeID, Graph::ExecInID },	// Console::Print (9)    [EXEC_OUT]  ->  [EXEC_IN] TERMINATE
			Graph::Link{ EXEC, 7, "Out", 8, Graph::ExecInID },								// Control::DoOnce (7)	 [EXEC_OUT]  ->  [EXEC_IN] Console::Print  (8)
			Graph::Link{ EXEC, 8, Graph::ExecOutID, Graph::TermNodeID, Graph::ExecInID },	// Console::Print (9)    [EXEC_OUT]  ->  [EXEC_IN] TERMINATE


			// Data Links
			Graph::Link{ DATA, 1, "Value", 2, "A" },										// Math::Pi (1)             <Value>  ->  <A>         Math::Multiply  (2)
			Graph::Link{ DATA, 2, "", 3, "A" },												// Math::Multiply (2)       <(out)>  ->  <A>         Math::GT        (3)
			Graph::Link{ DATA, 1, "Value", 3, "B" },										// Math::Pi (1)             <Value>  ->  <B>         Math::GT        (3)
			Graph::Link{ DATA, 3, "", 4, "Condition" },										// Math::GT (3)             <(out)>  ->  <Condition> Control::Branch (4)
			Graph::Link{ DATA, 2, "", 9, "String"}											// Math::Multiply (2)       <(out)>  ->  <String>	 Console::Print	 (9)
		}
	};


	inline const TestInput DiamondGraph = {
		.variables = {
			{ "Second Branch Condition", AsTL::TID_BOOL, true }
		},

		.nodes = {
			Graph::Node{
				1,
				Graph::MakeQualifiedID(Math, Constant, Pi),
				{}, {},
				{},
				{
					{ "Value", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				2,
				Graph::MakeQualifiedID(Math, Arithmetic, Multiply),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64, 0.5 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				3,
				Graph::MakeQualifiedID(Math, Arithmetic, Sine),
				{}, {},
				{
					Graph::Node::DataInPin{ "X", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				4,
				Graph::MakeQualifiedID(Math, Arithmetic, Cosine),
				{}, {},
				{
					Graph::Node::DataInPin{ "X", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				5,
				Graph::MakeQualifiedID(Math, Arithmetic, Add),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				6,
				Graph::MakeQualifiedID(Math, Logic, GreaterThan),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64, 0.0 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				7,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				8,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR, "Path A: True" }
				},
				{}
			},

			Graph::Node{
				9,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR, "Path B: False" }
				},
				{}
			},

			Graph::Node{
				10,
				Graph::MakeQualifiedID(Control, DoOnce),
				{ Graph::ExecInID, "Reset" },
				{ "Out" },
				{
					Graph::Node::DataInPin{ "Start Closed", AsTL::TID_BOOL, false }
				},
				{}
			},

			Graph::Node{
				11,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR, "Converged!" }
				},
				{}
			},

			Graph::Node{
				12,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, AsTL::TID_BOOL, "Second Branch Condition" }
				},
				{
					{ Graph::GetterOutputDataPin, AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				13,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				14,
				Graph::MakeQualifiedID(Control, Sequence),
				{ Graph::ExecInID },
				{ "Then 0", "Then 1", "Then 2" },
				{}, {}
			},

			Graph::Node{
				15,
				Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::ComboInPin{ Graph::SetterInputComboPin, AsTL::TID_BOOL, "Second Branch Condition" },
					Graph::Node::DataInPin{ Graph::SetterInputDataPin, AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				16,
				Graph::MakeQualifiedID(Math, Logic, Not),
				{}, {},
				{
					Graph::Node::DataInPin{ "", AsTL::TID_BOOL }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			}
		},


		.links = {
			// Execution Links
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 7, Graph::ExecInID }, // START -> Branch 1
			{ EXEC, 7, "True", 8, Graph::ExecInID },							// Branch 1 [True] -> Print A
			{ EXEC, 7, "False", 9, Graph::ExecInID },							// Branch 1 [False] -> Print B
			{ EXEC, 8, Graph::ExecOutID, 10, Graph::ExecInID },					// Print A [EXEC_IN] -> DoOnce
			{ EXEC, 9, Graph::ExecOutID, 10, "Reset" },							// Print B [EXEC_IN] -> DoOnce
			{ EXEC, 10, "Out", 11, Graph::ExecInID },							// DoOnce [EXEC_OUT] -> Print Converged
			{ EXEC, 11, Graph::ExecOutID, 13, Graph::ExecInID },				// Print Converged [EXEC_OUT] -> Branch 2
			{ EXEC, 13, "False", Graph::TermNodeID, Graph::ExecInID },			// Branch 2 [False] -> TERMINATE
			{ EXEC, 13, "True", 14, Graph::ExecInID },							// Branch 2 [True] -> Sequence
			{ EXEC, 14, "Then 0", 10, "Reset" },								// Sequence [Then 0] -> DoOnce
			{ EXEC, 14, "Then 1", 15, Graph::ExecInID },						// Sequence	[Then 1] -> Variable Set
			{ EXEC, 14, "Then 2", 7, Graph::ExecInID },							// Sequence [Then 2] -> Branch 1

			// Data Links
			{ DATA, 1, "Value", 2, "A" },										// Pi -> Multiply
			{ DATA, 2, "", 3, "X" },											// Multiply -> Sin
			{ DATA, 2, "", 4, "X" },											// Multiply -> Cos
			{ DATA, 3, "", 5, "A" },											// Sin -> Add
			{ DATA, 4, "", 5, "B" },											// Cos -> Add
			{ DATA, 5, "", 6, "A" },											// Add -> GT
			{ DATA, 6, "", 7, "Condition" },									// GT -> Branch 1
			{ DATA, 6, "", 16, "" },											// GT -> Not
			{ DATA, 12, Graph::GetterOutputDataPin, 13, "Condition" },			// Variable Get -> Branch 2
			{ DATA, 16, "", 15, Graph::SetterInputDataPin }						// Not -> Variable Set
		}
	};


	inline const TestInput FibonacciSequence = {
		.variables = {
			{ "X - 2", AsTL::TID_I32, 0 },
			{ "X - 1", AsTL::TID_I32, 1 }
		},


		.nodes = {
			Graph::Node{
				1,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, AsTL::TID_I32, "X - 2" }
				},
				{
					{ Graph::GetterOutputDataPin, AsTL::TID_I32 }
				}
			},

			Graph::Node{
				2,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, AsTL::TID_I32, "X - 1" }
				},
				{
					{ Graph::GetterOutputDataPin, AsTL::TID_I32 }
				}
			},

			Graph::Node{
				3,
				Graph::MakeQualifiedID(Math, Arithmetic, Add),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				4,
				Graph::MakeQualifiedID(Math, Logic, LessThan),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32, 100 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				5,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				6,
				Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::ComboInPin{ Graph::SetterInputComboPin, AsTL::TID_I32, "X - 2" },
					Graph::Node::DataInPin{ Graph::SetterInputDataPin, AsTL::TID_I32 }
				},
				{}
			},

			Graph::Node{
				7,
				Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::ComboInPin{ Graph::SetterInputComboPin, AsTL::TID_I32, "X - 1" },
					Graph::Node::DataInPin{ Graph::SetterInputDataPin, AsTL::TID_I32 }
				},
				{}
			},

			Graph::Node{
				8,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_I32 } // I32 instead of STR because this pin is connected to the Variable Getter, which returns I32
				},
				{}
			}
		},


		.links = {
			// Execution links
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 5, Graph::ExecInID },		// START -> Branch
			{ EXEC, 5, "False", Graph::TermNodeID, Graph::ExecInID },				// Branch (False) -> TERMINATE
			{ EXEC, 5, "True", 6, Graph::ExecInID },								// Branch (True) -> Variable Set "X - 2"
			{ EXEC, 6, Graph::ExecOutID, 7, Graph::ExecInID },						// Variable Set "X - 2" -> Variable Set "X - 1"
			{ EXEC, 7, Graph::ExecOutID, 8, Graph::ExecInID },						// Variable Set "X - 1" -> Print
			{ EXEC, 8, Graph::ExecOutID, 5, Graph::ExecInID },						// Print -> Branch

			// Data links
			{ DATA, 1, Graph::GetterOutputDataPin, 3, "A" },						// Variable Get "X - 2" -> Add (A)
			{ DATA, 2, Graph::GetterOutputDataPin, 3, "B" },						// Variable Get "X - 1" -> Add (B)
			{ DATA, 3, "", 4, "A" },												// Add (<result>) -> Less Than (A)
			{ DATA, 4, "", 5, "Condition" },										// Less Than (<result>) -> Branch (Condition)
			{ DATA, 2, Graph::GetterOutputDataPin, 6, Graph::SetterInputDataPin },	// Variable Get "X - 1" -> Variable Set "X - 2"
			{ DATA, 3, "", 7, Graph::SetterInputDataPin },							// Add (<result>) -> Variable Set "X - 1"
			{ DATA, 3, "", 8, "String" }											// Add (<result>) -> Print
		}
	};


	//inline const std::type_index SpcPrimitive = Graph::HighLevelTypeToPrimitive(Graph::TID_SPC); // Primitive type of the high-level Spacecraft type: IDX
	// NOTE: The definition of SpcPrimitive above is actually UB: the initialization order of static variables across different translation units is not defined in standard C++.
	// Concretely, SpcPrimitive calls HighLevelTypeToPrimitive with TID_SPC, both of which are defined in GraphTypes.hpp, a different header file.
	// While the function call happens after `main`, TID_SPC initialization does not, which means while this line expects TID_SPC to be defined before SpcPrimitive, that behavior is not guaranteed.
	// This is known as the Static Initialization Order Fiasco.
	
	// The solution is to use the Meyers' Singleton design pattern (i.e., Construct on First Use idiom), where you wrap variables with dependencies as static variables inside a function.
	// Due to lazy evaluation, these variables are only initialized after `main` because they have static storage.
	inline std::type_index SpcPrimitive() {
		static const std::type_index val = Graph::HighLevelTypeToPrimitive(Graph::TID_SPC);
		return val;
	}
	
	inline const TestInput CustomNode = {
		.variables = {
			{ "Chosen Satellite ID", SpcPrimitive(), 0}
		},

		.nodes = {
			Graph::Node{
				1,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, SpcPrimitive(), "Chosen Satellite ID" }
				},
				{
					{ Graph::GetterOutputDataPin, SpcPrimitive() }
				}
			},

			Graph::Node{
				2,
				"Mock::GetApoapsis",
				{}, {},
				{
					Graph::Node::ComboInPin{ "Satellite", SpcPrimitive() }
				},
				{
					{ "Node Symbol", AsTL::TID_STR },
					{ "Satellite Name", AsTL::TID_STR },
					{ "Position", AsTL::TID_VEC3 }
				}
			},

			Graph::Node{
				3,
				Graph::MakeQualifiedID(Math, Arithmetic, Vec3Magnitude),
				{},
				{},
				{
					Graph::Node::DataInPin{ "Vector", AsTL::TID_VEC3 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				4,
				Graph::MakeQualifiedID(Math, Logic, GreaterThan),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64, 2000e3 }  // Low-earth orbit: 2000 km above Earth - { 2000e3, 2000e3, 2000e3 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				5,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				6,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR },
					Graph::Node::DataInPin{ "B", AsTL::TID_STR, ": Callable invoked with " }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				7,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR, "Satellite " },
					Graph::Node::DataInPin{ "B", AsTL::TID_STR }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				8,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR, " at position " },
					Graph::Node::DataInPin{ "B", AsTL::TID_VEC3 }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				9,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR, " at position " },
					Graph::Node::DataInPin{ "B", AsTL::TID_STR }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				10,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR },
					Graph::Node::DataInPin{ "B", AsTL::TID_STR }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				11,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR }
				},
				{}
			},

			Graph::Node{
				12,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR },
					Graph::Node::DataInPin{ "B", AsTL::TID_STR, " has escaped Low-Earth Orbit!" }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				13,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR }
				},
				{}
			},
		},

		.links = {
			// Execution links
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 5, Graph::ExecInID },
			{ EXEC, 5, "False", 11, Graph::ExecInID },
			{ EXEC, 11, Graph::ExecOutID, 5, Graph::ExecInID },
			{ EXEC, 5, "True", 13, Graph::ExecInID },
			{ EXEC, 13, Graph::ExecOutID, Graph::TermNodeID, Graph::ExecInID },

			// Data links
			{ DATA, 1, Graph::GetterOutputDataPin, 2, "Satellite" },
			{ DATA, 2, "Node Symbol", 6, "A" },
			{ DATA, 2, "Satellite Name", 7, "B"},
			{ DATA, 2, "Position", 3, "Vector" },
			{ DATA, 2, "Position", 8, "B" },
			{ DATA, 3, "", 4, "A" },
			{ DATA, 4, "", 5, "Condition" },
			{ DATA, 6, "", 9, "A" },
			{ DATA, 7, "", 9, "B" },
			{ DATA, 9, "", 10, "A" },
			{ DATA, 8, "", 10, "B" },
			{ DATA, 10, "", 11, "String" },
			{ DATA, 7, "", 12, "A" },
			{ DATA, 12, "", 13, "String" }
		}
	};


	inline const TestInput SimulationTick = {
		.variables = {},

		.nodes = {
			Graph::Node{
				1,
				"Mock::GetEpoch",
				{}, {},
				{},
				{
					{ "Epoch", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				2,
				Graph::MakeQualifiedID(Misc, StringConcat),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_STR, "Current Epoch: " },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_STR }
				}
			},

			Graph::Node{
				3,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_STR }
				},
				{}
			},

			Graph::Node{
				4,
				Graph::MakeQualifiedID(Math, Logic, GreaterThan),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64, 10.0 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				5,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},
		},

		.links = {
			// Execution links
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 3, Graph::ExecInID },		// START -> Print
			{ EXEC, 3, Graph::ExecOutID, 5, Graph::ExecInID },						// Print -> Branch
			{ EXEC, 5, "True", Graph::TermNodeID, Graph::ExecInID},					// Branch (True) -> TERMINATE
																					// Branch (False) is not connected to anything;
																					// upon hitting the False pin, the graph finishes execution for THIS simulation tick;
																					// the VM caller should increment the simulation tick (via the custom node) and rerun the graph

			// Data links
			{ DATA, 1, "Epoch", 2, "B" },					// GetEpoch -> (B) String Concat
			{ DATA, 1, "Epoch", 4, "A" },					// GetEpoch -> (A) GT
			{ DATA, 2, "", 3, "String" },					// String Concat (<result>) -> (String) Print
			{ DATA, 4, "", 5, "Condition" }					// GT (<result>) -> (Condition) Branch
		}
	};


	inline const TestInput OneHundredPrimes = {
		.variables = {
			{ "N", AsTL::TID_I32, 545 },
			{ "Is Prime", AsTL::TID_BOOL, false }
		},

		.nodes = {
			Graph::Node{
				1,
				Graph::MakeQualifiedID(Control, WhileLoop),
				{ Graph::ExecInID, "Break" },
				{ "In Loop", "Completed" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL, true }
				},
				{
					{ "Index", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				2,
				Graph::MakeQualifiedID(Math, Arithmetic, Add),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32, 2 }
				},
				{
					{ "", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				3,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, AsTL::TID_I32, "N" }
				},
				{
					{ Graph::GetterOutputDataPin, AsTL::TID_I32 }
				}
			},

			Graph::Node{
				4,
				Graph::MakeQualifiedID(Math, Logic, GreaterThanEqualTo),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				5,
				Graph::MakeQualifiedID(Math, Logic, LessThan),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32, 2 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				6,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				7,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				8,
				Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::ComboInPin{ Graph::SetterInputComboPin, AsTL::TID_BOOL, "Is Prime" },
					Graph::Node::DataInPin{ Graph::SetterInputDataPin, AsTL::TID_BOOL, true }
				},
				{}
			},

			Graph::Node{
				9,
				Graph::MakeQualifiedID(Control, WhileLoop),
				{ Graph::ExecInID, "Break" },
				{ "In Loop", "Completed" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL, true }
				},
				{
					{ "Index", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				10,
				Graph::MakeQualifiedID(Math, Arithmetic, Add),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32, 2 }
				},
				{
					{ "", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				11,
				Graph::MakeQualifiedID(Math, Arithmetic, Multiply),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				12,
				Graph::MakeQualifiedID(Math, Logic, LessThanEqualTo),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				13,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				14,
				Graph::MakeQualifiedID(Math, Arithmetic, Modulo),
				{},
				{},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				15,
				Graph::MakeQualifiedID(Math, Logic, EqualTo),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				16,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				17,
				Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::ComboInPin{ Graph::SetterInputComboPin, AsTL::TID_BOOL, "Is Prime" },
					Graph::Node::DataInPin{ Graph::SetterInputDataPin, AsTL::TID_BOOL, false }
				},
				{}
			},

			Graph::Node{
				18,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, AsTL::TID_BOOL, "Is Prime" }
				},
				{
					{ Graph::GetterOutputDataPin, AsTL::TID_BOOL }
				}
			},

			Graph::Node{
				19,
				Graph::MakeQualifiedID(Control, Branch),
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					Graph::Node::DataInPin{ "Condition", AsTL::TID_BOOL }
				},
				{}
			},

			Graph::Node{
				20,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_I32 }
				},
				{}
			}
		},

		.links = {
			// Execution links
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 1, Graph::ExecInID },			// START -> Outer Loop
			{ EXEC, 1, "Completed", Graph::TermNodeID, Graph::ExecInID },				// Outer Loop (Completed) -> TERMINATE
			{ EXEC, 1, "In Loop", 6, Graph::ExecInID },									// Outer Loop (In Loop) -> Branch 1
			{ EXEC, 6, "True", 1, "Break" },											// Branch 1 (True) -> (Break) Outer Loop
			{ EXEC, 6, "False", 7, Graph::ExecInID },									// Branch 1 (False) -> Branch 2
		//{ EXEC, 7, "True", 8, Graph::ExecInID },										// Branch 2 (True) -> Outer Loop (Continue loop...)
			{ EXEC, 7, "False", 8, Graph::ExecInID },									// Branch 2 (False) -> Set "Is Prime" to True
			{ EXEC, 8, Graph::ExecOutID, 9, Graph::ExecInID },							// Set "Is Prime" to True -> Inner Loop
			{ EXEC, 9, "In Loop", 13, Graph::ExecInID },								// Inner Loop (In Loop) -> Branch 3
			{ EXEC, 9, "Completed", 19, Graph::ExecInID },								// Inner Loop (Completed) -> Branch 5
			{ EXEC, 13, "False", 9, "Break" },											// Branch 3 (False) -> (Break) Inner Loop
			{ EXEC, 13, "True", 16, Graph::ExecInID },									// Branch 3 (True) -> Branch 4
			{ EXEC, 16, "True", 17, Graph::ExecInID },									// Branch 4 (True) -> Set "Is Prime" to False
		//{ EXEC, 16, "False", 9, Graph::ExecInID },									// Branch 4 (False) -> Inner Loop (Continue loop...)
			{ EXEC, 17, Graph::ExecOutID, 9, "Break" },									// Set "Is Prime" to False -> (Break) Inner Loop
		//{ EXEC, 19, "False", 1, Graph::ExecInID },									// Branch 5 (False) -> Outer Loop (Continue loop...)
			{ EXEC, 19, "True", 20, Graph::ExecInID },									// Branch 5 (True) -> Print Prime Number
		//{ EXEC, 20, Graph::ExecOutID, 1, Graph::ExecInID },							// Print Prime Number -> Outer Loop (Continue loop...)

			// Data links
			{ DATA, 1, "Index", 2, "A" },												// Outer Loop (Index) -> (A) Add Outer Index
			{ DATA, 2, "", 4, "A" },													// Add Outer Index (<result>) -> (A) GTEq
			{ DATA, 2, "", 5, "A" },													// Add Outer Index (<result>) -> (A) LT
			{ DATA, 2, "", 12, "B" },													// Add Outer Index (<result>) -> (B) LTEq
			{ DATA, 2, "", 14, "A" },													// Add Outer Index (<result>) -> (A) Modulo
			{ DATA, 2, "", 20, "String" },												// Add Outer Index (<result>) -> (String) Print Prime Number
			{ DATA, 3, Graph::GetterOutputDataPin, 4, "B" },							// Get "N" -> (B) GTEq
			{ DATA, 4, "", 6, "Condition" },											// GTEq (<result>) -> (Condition) Branch 1
			{ DATA, 5, "", 7, "Condition" },											// LT (<result>) -> (Condition) Branch 2
			{ DATA, 9, "Index", 10, "A" },												// Inner Loop (Index) -> (A) Add Inner Index
			{ DATA, 10, "", 11, "A" },													// Add Inner Index (<result>) -> (A) Multiply
			{ DATA, 10, "", 11, "B" },													// Add Inner Index (<result>) -> (B) Multiply
			{ DATA, 10, "", 14, "B" },													// Add Inner Index (<result>) -> (B) Modulo
			{ DATA, 11, "", 12, "A" },													// Multiply (<result>) -> (A) LTEq
			{ DATA, 12, "", 13, "Condition" },											// LTEq (<result>) -> (Condition) Branch 3
			{ DATA, 14, "", 15, "A" },													// Modulo (<result>) -> (A) Eq
			{ DATA, 15, "", 16, "Condition" },											// Eq (<result>) -> (Condition) Branch 4
			{ DATA, 18, Graph::GetterOutputDataPin, 19, "Condition" },					// Get "Is Prime" -> (Condition) Branch 5
		}
	};



	inline const TestInput BrokenTypeMismatch = {
		.variables = {
			{ "Vector", AsTL::TID_VEC3, AsTL::VEC3(3.14) }
		},

		.nodes = {
			Graph::Node{
				1,
				Graph::GetterNodeSymbol,
				{}, {},
				{
					Graph::Node::ComboInPin{ Graph::GetterInputComboPin, AsTL::TID_VEC3, "Vector" }
				},
				{
					{ Graph::GetterOutputDataPin, AsTL::TID_VEC3 }
				}
			},

			Graph::Node{
				2,
				Graph::MakeQualifiedID(Math, Arithmetic, Add),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_I32 },
					Graph::Node::DataInPin{ "B", AsTL::TID_I32 }
				},
				{
					{ "", AsTL::TID_I32 }
				}
			},

			Graph::Node{
				3,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_I32 }
				},
				{}
			}
		},

		.links = {
			// Execution Links
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 3, Graph::ExecInID },
			{ EXEC, 3, Graph::ExecOutID, Graph::TermNodeID, Graph::ExecInID },

			// Data Links
			{ DATA, 1, Graph::GetterOutputDataPin, 2, "A" },	// Vector Get (VEC3) -> I32 Input
			{ DATA, 1, Graph::GetterOutputDataPin, 2, "B" },	// Vector Get (VEC3) -> I32 Input
			{ DATA, 2, "", 3, "String" }
		}
	};



	inline const TestInput BrokenUndefinedVariable = {
		.variables = {},

		.nodes = {
			Graph::Node{
				1,
				Graph::MakeQualifiedID(Math, Constant, Pi),
				{}, {},
				{},
				{
					{ "Value", AsTL::TID_F64 }
				}
			},
			Graph::Node{
				2,
				Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::ComboInPin{ Graph::SetterInputComboPin, AsTL::TID_ANY, "NonExistentVar" },  // This variable doesn't exist
					Graph::Node::DataInPin{ Graph::SetterInputDataPin, AsTL::TID_F64 }
				},
				{}
			}
		},

		.links = {
			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 2, Graph::ExecInID },
			{ EXEC, 2, Graph::ExecOutID, Graph::TermNodeID, Graph::ExecInID },

			{ DATA, 1, "Value", 2, Graph::SetterInputDataPin }
		}
	};



	inline const TestInput BrokenCircularDependency = {
		.variables = {},

		.nodes = {
			Graph::Node{
				1,
				Graph::MakeQualifiedID(Math, Arithmetic, Add),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				2,
				Graph::MakeQualifiedID(Math, Arithmetic, Multiply),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			Graph::Node{
				3,
				Graph::MakeQualifiedID(Math, Arithmetic, Subtract),
				{}, {},
				{
					Graph::Node::DataInPin{ "A", AsTL::TID_F64 },
					Graph::Node::DataInPin{ "B", AsTL::TID_F64 }
				},
				{
					{ "", AsTL::TID_F64 }
				}
			},

			// We need an executable node to link one of the circular nodes to, so that the semantic analyzer can trace the links and report the error
			Graph::Node{
				4,
				Graph::MakeQualifiedID(Console, Print),
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					Graph::Node::DataInPin{ "String", AsTL::TID_F64 }
				},
				{}
			}
		},

		.links = {
			// Circular dependency: 1 depends on 2, 2 depends on 3, 3 depends on 1
			{ DATA, 1, "", 4, "String" },	// Node 1 output -> Print
			{ DATA, 1, "", 3, "A" },		// Node 1 output -> Node 3
			{ DATA, 3, "", 2, "A" },		// Node 3 output -> Node 2
			{ DATA, 2, "", 1, "A" },		// Node 2 output -> Node 1

			{ EXEC, Graph::EntryNodeID, Graph::ExecOutID, 4, Graph::ExecInID }
		}
	};
}


namespace TestProgram {
	// Working programs
	inline const Impl::TestInput& SimpleGraph = Impl::SimpleGraph;
	inline const Impl::TestInput& DiamondGraph = Impl::DiamondGraph;
	inline const Impl::TestInput& FibonacciSequence = Impl::FibonacciSequence;
	inline const Impl::TestInput& CustomNode = Impl::CustomNode;
	inline const Impl::TestInput& SimulationTick = Impl::SimulationTick;
	inline const Impl::TestInput& OneHundredPrimes = Impl::OneHundredPrimes;
	

	// Broken programs
	inline const Impl::TestInput& TypeMismatch = Impl::BrokenTypeMismatch;
	inline const Impl::TestInput& UndefinedVariable = Impl::BrokenUndefinedVariable;
	inline const Impl::TestInput& CircularDependency = Impl::BrokenCircularDependency;
}
