#pragma once

#include <span>
#include <cmath>
#include <string>
#include <numbers>
#include <exception>
#include <typeindex>
#include <string_view>
#include <unordered_map>

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/GraphTypes.hpp>
#include <Scripting/Utils/StaticBlock.hpp>

#include "Instruction.hpp"


namespace Compiler {
	struct GraphNodeDescriptor;	// Forward-declaration
	/* Graph node native callback function
		@param ctx: The pointer to any execution context (must be casted back to a concrete context implementation to be usable).
		@param nodeID: The ID of the Node whose callable was invoked.
		@param triggeredPin: The Label of the Execution Pin that was triggered.
		@param args: The arguments passed into the Callable, in the same order as the Input Parameters specified in the Node's Descriptor.
		@param retBuffer: The pointer to the buffer of values, in the same order as the Output Parameters specified in the Node's Descriptor.
				If `N` Execution Output Pins are also specified, the first `N - X` return values must be the Boolean representations of the Output Pins state,
				where `X` is the number of Execution Output Pins that are NOT "EXEC_OUT" (Graph::ExecOutID),
				in the same order as the Execution Output Pins specified in the Node's Descriptor.
	*/
	using GraphNodeCallback = void(*)(
		AsTL::OpaqueExecCtx *ctx,
		const GraphNodeDescriptor *self,
		AsTL::IDX nodeID,
		std::string_view triggeredPin,
		std::span<const AsTL::StackValue> args,
		AsTL::StackValue* retBuffer
	);


	// The description of the graph node
	struct GraphNodeDescriptor {
		enum class NodeClass {
			Action,			// Executable Node: Operation that modifies the environment, change physics states, or otherwise interact with external systems
			ControlFlow,	/* Executable Node: Operation that dictates execution flow; if the execution flow dies before reaching an explicit Terminate node,
												the program definitively ends without re-evaluating the Executable Node. */
			ControlFlowLoop,/* Executable Node: Operation that dictates execution flow; if the execution flow dies before reaching an explicit Terminate node,
												the Executable Node will be re-evaluated and the execution flow restarted from that Node. */

			Getter,			// Non-executable Node: Read-only operation that returns a value
			Constant,		// Non-executable Node: Read-only operation that returns a constant value
			MathAndLogic	// Non-executable Node: Purely mathematical or logical operation
		};

		struct Parameter {
			enum class ParamClass {
				Data,	// Pure Data Darameter
				Combo	// Graph Variable Combo-Box Parameter
			};

			ParamClass paramClass;
			std::string label;				// Parameter label (must correspond to Pin label)
			std::type_index type;			// Parameter data type
		};

		NodeClass nodeClass;
		std::string name;					// Node name
		std::string symbol;					// Symbolic ID
		std::vector<std::string> inExecs;	// Execution Input Pin Labels
		std::vector<std::string> outExecs;	// Execution Output Pin Labels
		std::vector<Parameter> inParams;	// Input parameters (empty if parameterless)
		std::vector<Parameter> outParams;	// Output parameters (empty if return type is void)

		// Callback (can be std::nullopt if only the node signature is available at declaration time; concrete implementation must be added before script compilation)
		// Callback type can either be a native C++ callback or a stream of AstroAssembly instructions
		std::optional<
			std::variant<GraphNodeCallback, Opcode>
		> callback;


		/* Is this graph node categorized as an executable node (i.e., a node with execution pins)? */
		bool isExecutable() const {
			using enum NodeClass;

			switch (nodeClass) {
			case Action:
			case ControlFlow:
			case ControlFlowLoop:
				return true;
			}

			return false;
		}
	};


	using NodeTableT = std::unordered_map<AsTL::IDX, GraphNodeDescriptor>;		// Node Descriptor Table (Node Function Index -> Descriptor)
	using NodeLookupT = std::unordered_map<std::string, AsTL::IDX>;				// Node Lookup Table (Node Name -> Node Function Index)


	class IGraphNodeRegistry {
	public:
		virtual ~IGraphNodeRegistry() = default;

		virtual const NodeTableT &getNodeTable() const = 0;
		virtual const NodeLookupT &getNodeLookup() const = 0;

		virtual void addOrSet(const GraphNodeDescriptor &descriptor) = 0;
		virtual void addOrSet(GraphNodeDescriptor &&descriptor) = 0;

		virtual bool contains(const std::string &nodeFuncName) const = 0;
		virtual bool contains(AsTL::IDX nodeFuncIdx) const = 0;

		virtual const std::pair<AsTL::IDX, const GraphNodeDescriptor&> getInfo(const std::string &nodeFuncName) const = 0;
		virtual const GraphNodeDescriptor &getInfo(AsTL::IDX nodeFuncIdx) const = 0;

	protected:
		IGraphNodeRegistry() {
			// Initialize nodes lookup table
			using enum GraphNodeDescriptor::NodeClass;
			using enum GraphNodeDescriptor::Parameter::ParamClass;


			addDefault({
				ControlFlow,
				"On Simulation Tick", Graph::EntryNodeSymbol,
				{}, { Graph::ExecOutID },
				{}, {},
				std::nullopt
			});
			
			addDefault({
				ControlFlow,
				"End Simulation", Graph::TermNodeSymbol,
				{}, { Graph::ExecOutID },
				{}, {},
				Opcode::TERMINATE
			});
			
			addDefault({
				Action,
				"Set Variable", Graph::SetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					{ Combo, Graph::SetterInputComboPin, AsTL::TID_ANY },
					{ Data, Graph::SetterInputDataPin, AsTL::TID_ANY }
				},
				{},
				std::nullopt
			});
			
			addDefault({
				Getter,
				"Get Variable", Graph::GetterNodeSymbol,
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					{ Combo, Graph::GetterInputComboPin, AsTL::TID_ANY }
				},
				{
					{ Data, Graph::GetterOutputDataPin, AsTL::TID_ANY }
				},
				std::nullopt
			});



			// ----- PROPERTIES -----
				// Parameterless
					// General
			addDefault({
				Constant,
				"Pi", "Math::Pi",
				{}, {},
				{},
				{
					{ Data, "Value", AsTL::TID_F64 }
				},
				[](AsTL::OpaqueExecCtx *, const GraphNodeDescriptor *self, AsTL::IDX, std::string_view, std::span<const AsTL::StackValue>, AsTL::StackValue* retBuffer) -> void {
					retBuffer[0] = std::numbers::pi;
				}
			});

				// Domain-specific
				// NOTE: These should actually be added in a compiler implementation in Engine/
			/*
			addDefault({
				Getter,
				"Simulation Epoch", "Time::Epoch",
				{}, {},
				{},
				{
					{ "Epoch (seconds)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Time Elapsed Since Simulation Start", "Time::Elapsed",
				{}, {},
				{}, 
				{
					{ "Time Elapsed (secs)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Keplerian Orbital Elements", "Orbit::KeplerianElements",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_OBJ), Graph::TID_OBJ }
				},
				{
					{ "Semi-latus Rectum (km)", AsTL::TID_F64 },
					{ "Semi-major Axis (km)", AsTL::TID_F64 },
					{ "Eccentricity", AsTL::TID_F64 },
					{ "Inclination (rad)", AsTL::TID_F64 },
					{ "Longitude of Ascending Node (rad)", AsTL::TID_F64 },
					{ "Argument of Periapsis (rad)", AsTL::TID_F64 },
					{ "True Anomaly at Epoch (rad)", AsTL::TID_F64 },
					{ "Mean Anomaly at Epoch (rad)", AsTL::TID_F64 },
					{ "Argument of Latitude (rad)", AsTL::TID_F64 },
					{ "True Longitude (rad)", AsTL::TID_F64 },
					{ "Longitude of Periapsis (rad)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Position of Object", "Transform::Position",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_OBJ), Graph::TID_OBJ }
				},
				{
					{ "Position (m)", AsTL::TID_VEC3 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Rotation of Object", "Transform::Rotation",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_OBJ), Graph::TID_OBJ }
				},
				{
					{ "Rotation", AsTL::TID_VEC3 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Velocity of Object", "Body::Velocity",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_OBJ), Graph::TID_OBJ }
				},
				{
					{ "Velocity (m/s)", AsTL::TID_VEC3 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Acceleration of Object", "Body::Acceleration",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_OBJ), Graph::TID_OBJ }
				},
				{
					{ "Acceleration (m/s^2)", AsTL::TID_VEC3 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Mass of Object", "Body::Mass",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_OBJ), Graph::TID_OBJ }
				},
				{
					{ "Mass (kg)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Equatorial Radius of Body", "Shape::EquatRadius",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_BODY), Graph::TID_BODY }
				},
				{
					{ "Equatorial Radius (m)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Flattening of Body", "Shape::Flattening",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_BODY), Graph::TID_BODY }
				},
				{
					{ "Flattening", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Gravitational Parameter of Body", "Shape::GravParam",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_BODY), Graph::TID_BODY }
				},
				{
					{ "Gravitational Parameter (m^3/s^-2)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Rotational Velocity of Body", "Shape::RotVelocity",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_BODY), Graph::TID_BODY }
				},
				{
					{ "Rotational Velocity (rad/s)", AsTL::TID_VEC3 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"J2 of Body", "Shape::J2Cf",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_BODY), Graph::TID_BODY }
				},
				{
					{ "J2 Coefficient", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Drag of Spacecraft", "Spacecraft::DragCf",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Drag Coefficient", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Frontal Area of Spacecraft", "Spacecraft::RefArea",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Area (m^2)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Reflectivity of Spacecraft", "Spacecraft::ReflectivityCf",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Reflectivity Coefficient", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Fuel Mass of Thruster", "Spacecraft::Thruster::FuelMass",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Mass (kg)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Maximum Fuel Mass of Thruster", "Spacecraft::Thruster::MaxFuelMass",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Max. Mass (kg)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Maximum Thrust of Thruster", "Spacecraft::Thruster::ThrustMagnitude",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Max. Thrust (N)", AsTL::TID_F64 }
				},
				std::nullopt
			});

			addDefault({
				Getter,
				"Specific Impulse of Thruster", "Spacecraft::Thruster::Isp",
				{}, {},
				{
					{ Graph::PinTypeToRepString(Graph::TID_SPC), Graph::TID_SPC }
				},
				{
					{ "Specific Impulse (s)", AsTL::TID_F64 }
				},
				std::nullopt
			});
			*/


			// ----- FUNCTIONS -----
				// Parameterized
			addDefault({
				ControlFlow,
				"Branch", "Control::Branch",
				{ Graph::ExecInID },
				{ "True", "False" },
				{
					{ Data, "Condition", AsTL::TID_BOOL }
				},
				{},
				[](AsTL::OpaqueExecCtx *, const GraphNodeDescriptor *self, AsTL::IDX, std::string_view, std::span<const AsTL::StackValue> args, AsTL::StackValue* retBuffer) -> void {
					auto condition = std::get<AsTL::BOOL>(args[0]);
					if (condition) {
						// Execute True, do not execute False
						retBuffer[0] = true; 
						retBuffer[1] = false;
					}
					else {
						// Do not execute True, execute False
						retBuffer[0] = false;
						retBuffer[1] = true;
					}
				}
			});

			addDefault({
				ControlFlow,
				"Do Once", "Control::DoOnce",
				{ Graph::ExecInID, "Reset" },
				{ "Out" },
				{
					{ Data, "Start Closed", AsTL::TID_BOOL }
				},
				{},
				[](AsTL::OpaqueExecCtx *, const GraphNodeDescriptor *self, AsTL::IDX nodeID, std::string_view triggeredPin, std::span<const AsTL::StackValue> args, AsTL::StackValue* retBuffer) -> void {
					bool startClosed = std::get<AsTL::BOOL>(args[0]);

					static std::unordered_map<AsTL::IDX, bool> closedStates{};
					bool &isClosed = closedStates[nodeID];

					// Code within STATIC_BLOCKS_XXX is executed ONCE for every Control::DoOnce node in the graph, distinguished by its Node ID
					STATIC_BLOCK_BEGIN(nodeID)
						isClosed = startClosed;
					STATIC_BLOCK_END

					if (triggeredPin == "Reset") {
						isClosed = false;
						retBuffer[0] = false;
						return;
					}

					if (triggeredPin == Graph::ExecInID) {
						if (!isClosed) {
							isClosed = true;
							retBuffer[0] = true;
							return;
						}
					}

					retBuffer[0] = false;
				}
			});

			addDefault({
				ControlFlowLoop,
				"Sequence", "Control::Sequence",
				{ Graph::ExecInID },
				{ "Then 0", "Then 1", "Then 2" },
				{}, {},
				[](AsTL::OpaqueExecCtx *, const GraphNodeDescriptor *self, AsTL::IDX nodeID, std::string_view triggeredPin, std::span<const AsTL::StackValue> args, AsTL::StackValue* retBuffer) -> void {
					static std::unordered_map<AsTL::IDX,
						std::pair<std::vector<AsTL::StackValue>, size_t>
					> outExecLists{};
					std::vector<AsTL::StackValue> &outList = outExecLists[nodeID].first;
					size_t &currentOutPin = outExecLists[nodeID].second;

					STATIC_BLOCK_BEGIN(nodeID)
					{
						currentOutPin = 0;

						outList.resize(self->outExecs.size());
						outList[0] = true;
						for (size_t i = 1; i < self->outExecs.size(); ++i)
							outList[i] = false;
					}
					STATIC_BLOCK_END

					if (currentOutPin > 0) {
						for (size_t i = 1; i < outList.size(); ++i) {
							if (i == currentOutPin) {
								outList[i - 1] = false;
								outList[i] = true;
							}
						}
					}

					++currentOutPin;

					/* NOTE: `retBuffer = outList.data()` does NOT work!!
						When a native callback is invoked, `retBuffer` is created as a local variable that points to the return list managed by the VM.
						This means pointer reassignments like `retBuffer = outList.data()` merely change what `retBuffer` points to, resulting in
						the returning list not being updated at all, since `retBuffer` is no longer pointing to it.

						The solution is to copy the data from the local container into `retBuffer`. For contiguous containers:
							`memcpy(retBuffer, outList.data(), outList.size() * sizeof(AsTL::StackValue))`

						Or the idiomatic C++ alternative (safer, because it does the byte size calculations for you, but only accepts STL containers as the source):
							`std::copy(outList.begin(), outList.end(), retBuffer);`
					*/
					std::copy(outList.begin(), outList.end(), retBuffer);
				}
			});


				// Descriptors mappable to AstroAssembly instructions
			addDefault({
				Action,
				"Print to Console", "Console::Print",
				{ Graph::ExecInID },
				{ Graph::ExecOutID },
				{
					{ Data, "String", AsTL::TID_STR }
				},
				{},
				Opcode::PRINT
			});

			addDefault({
				MathAndLogic,
				"Concatenate String", "StringUtils::Concat",
				{}, {},
				{
					{ Data, "A", AsTL::TID_STR },
					{ Data, "B", AsTL::TID_STR }
				},
				{
					{ Data, "", AsTL::TID_STR }
				},
				Opcode::STR_CAT
			});

			addDefault({
				MathAndLogic,
				"Add", "Math::Add",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ADD
			});

			addDefault({
				MathAndLogic,
				"Subtract", "Math::Subtract",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::SUB
			});

			addDefault({
				MathAndLogic,
				"Multiply", "Math::Multiply",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::MUL
			});

			addDefault({
				MathAndLogic,
				"Modulo", "Math::Modulo",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::MOD
			});

			addDefault({
				MathAndLogic,
				"Divide", "Math::Divide",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::DIV
			});

			addDefault({
				MathAndLogic,
				"Greater Than", "Math::GT",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::CMP_GT
			});

			addDefault({
				MathAndLogic,
				"Greater Than or Equal To", "Math::GTEq",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::CMP_GTE
			});

			addDefault({
				MathAndLogic,
				"Less Than", "Math::LT",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::CMP_LT
			});

			addDefault({
				MathAndLogic,
				"Less Than or Equal To", "Math::LTEq",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::CMP_LTE
			});

			addDefault({
				MathAndLogic,
				"Equal To", "Math::Eq",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::CMP_EQ
			});

			addDefault({
				MathAndLogic,
				"Not Equal To", "Math::NEq",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::CMP_NEQ
			});

			addDefault({
				MathAndLogic,
				"And", "Math::And",
				{}, {},
				{
					{ Data, "A", AsTL::TID_BOOL },
					{ Data, "B", AsTL::TID_BOOL }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::LGC_AND
			});

			addDefault({
				MathAndLogic,
				"Or", "Math::Or",
				{}, {},
				{
					{ Data, "A", AsTL::TID_BOOL },
					{ Data, "B", AsTL::TID_BOOL }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::LGC_OR
			});

			addDefault({
				MathAndLogic,
				"Not", "Math::Not",
				{}, {},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				{
					{ Data, "", AsTL::TID_BOOL }
				},
				Opcode::LGC_NOT
			});

			addDefault({
				MathAndLogic,
				"Sine", "Math::Sin",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::SIN
			});

			addDefault({
				MathAndLogic,
				"Arc-sine", "Math::Asin",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ASIN
			});

			addDefault({
				MathAndLogic,
				"Cosine", "Math::Cos",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::COS
			});

			addDefault({
				MathAndLogic,
				"Arc-cosine", "Math::Acos",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ACOS
			});

			addDefault({
				MathAndLogic,
				"Tangent", "Math::Tan",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::TAN
			});

			addDefault({
				MathAndLogic,
				"Arc-tangent of Single Ratio", "Math::Atan",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ATAN
			});

			addDefault({
				MathAndLogic,
				"Arc-tangent", "Math::Atan2",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC },
					{ Data, "Y", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ATAN2
			});

			addDefault({
				MathAndLogic,
				"Cotangent", "Math::Cot",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::COT
			});

			addDefault({
				MathAndLogic,
				"Arc-cotangent", "Math::Acot",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ACOT
			});

			addDefault({
				MathAndLogic,
				"Absolute Value", "Math::Abs",
				{}, {},
				{
					{ Data, "X", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::ABS
			});

			addDefault({
				MathAndLogic,
				"Minimum Value", "Math::Min",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::MIN
			});

			addDefault({
				MathAndLogic,
				"Maximum Value", "Math::Max",
				{}, {},
				{
					{ Data, "A", AsTL::TID_NUMERIC },
					{ Data, "B", AsTL::TID_NUMERIC }
				},
				{
					{ Data, "", AsTL::TID_NUMERIC }
				},
				Opcode::MAX
			});
		}

		/* Adds a new node or modifies a current node (reserved for IGraphNodeRegistry and derived classes). */
		void addOrModifyInternal(NodeTableT &table, NodeLookupT &lookup, const GraphNodeDescriptor &descriptor) const {
			// Output parameters must be pure data parameters; combo-box outputs are not semantically correct
			for (const auto& outParam : descriptor.outParams)
				if (outParam.paramClass != GraphNodeDescriptor::Parameter::ParamClass::Data)
					throw std::runtime_error("Cannot register descriptor for node \"" + descriptor.symbol + "\": Output parameter class is restricted to the Data class");


			AsTL::IDX idx = IDX_NAN;

			if (lookup.contains(descriptor.symbol)) {
				// Modify existing node
				idx = lookup[descriptor.symbol];
			}
			else {
				// Add new node
				idx = static_cast<AsTL::IDX>(table.size());
				lookup[descriptor.symbol] = idx;
			}

			// GraphNodeDescriptor has a std::type_index field, which does not have a default constructor,
			// so we can't just use the map's [] operator, as that attempts to default-construct GraphNodeDescriptor
			table.insert_or_assign(idx, descriptor);
		}

		/* Moves the node table data from the IGraphNodeRegistry base class to a derived class. */
		NodeTableT &&transferTable() { return std::move(m_nodeTable); }

		/* Moves the lookup table data from the IGraphNodeRegistry base class to a derived class. */
		NodeLookupT &&transferLookup() { return std::move(m_nodeLookup); }

	private:
		AsTL::IDX IDX_NAN = std::numeric_limits<AsTL::IDX>::max();

		NodeTableT m_nodeTable{};	// Node Table
		NodeLookupT m_nodeLookup{};	// Lookup Table mapping a node symbol to its corresponding entry in the Node Table

		/* Adds a default node. */
		void addDefault(GraphNodeDescriptor &&desc) {
			addOrModifyInternal(m_nodeTable, m_nodeLookup, std::forward<GraphNodeDescriptor>(desc));
		}
	};
}