#pragma once

#include <string>
#include <vector>
#include <typeindex>

#include "AsTLTypes.hpp"


// Astrocelerate's Visual Graph
namespace Graph {
	using NodeID = int32_t;

	// Reserved data
	inline constexpr NodeID EntryNodeID			= 0;			// ID of the entry-point (Start) node
	inline const std::string EntryNodeSymbol	= "_START";		// Symbol of the entry-point (Start) node

	inline const std::string ExecInID			= "EXEC_IN";	// Input Execution Pin ID
	inline const std::string ExecOutID			= "EXEC_OUT";	// Output Execution Pin ID

	inline constexpr NodeID TermNodeID			= -1;			// ID of the Termination node
	inline const std::string TermNodeSymbol		= "_TERM";		// Symbol of the Termination node

	inline const std::string SetterNodeSymbol	= "_VARSET";	// Symbol of the Variable Setter node
	inline const std::string SetterInputComboPin = "_VAR";		// Variable Input Combo-Box Pin ID of the Variable Setter node
	inline const std::string SetterInputDataPin = "_NEWVAL";	// Variable Input Data Pin ID of the Variable Setter node

	inline const std::string GetterNodeSymbol	= "_VARGET";	// Symbol of the Variable Getter node
	inline const std::string GetterInputComboPin = "_VAR";		// Variable Input Combo-Box Pin ID of the Variable Getter node
	inline const std::string GetterOutputDataPin = "_VARVAL";	// Variable Output Combo-Box Pin ID of the Variable Getter node

	inline const std::unordered_set<NodeID> ReservedNodeIDs = {
		EntryNodeID, TermNodeID
	};

	inline const std::unordered_set<std::string> ReservedNodeSymbols = {
		EntryNodeSymbol, TermNodeSymbol,
		SetterNodeSymbol,
		GetterNodeSymbol
	};

	inline const std::unordered_set<std::string> ReservedPinIDs = {
		ExecInID, ExecOutID,
		SetterInputComboPin, SetterInputDataPin,
		GetterInputComboPin, GetterOutputDataPin
	};


	// High-level Pin types
		// Object: all objects in the simulation
		// Underlying implementation is AsTL::IDX referencing a simulation entity ID
	struct ObjectTag {};
	inline const std::type_index TID_OBJ	= typeid(ObjectTag);
	inline const std::string SER_OBJ		= "OBJ";

	// Spacecraft: all spacecraft in the simulation
	// Underlying implementation is AsTL::IDX referencing a simulation entity ID
	struct SpacecraftTag {};
	inline const std::type_index TID_SPC	= typeid(SpacecraftTag);
	inline const std::string SER_SPC		= "SPC";

	// Body: all natural celestial bodies in the simulation
	// Underlying implementation is AsTL::IDX referencing a simulation entity ID
	struct BodyTag {};
	inline const std::type_index TID_BODY	= typeid(BodyTag);
	inline const std::string SER_BODY		= "BODY";

	// SPICE kernels
	// Underlying implementation is AsTL::STR of the path to the SPICE kernels on disk
	struct SPICEKernelTag {};
	inline const std::type_index TID_SPK	= typeid(SPICEKernelTag);
	inline const std::string SER_SPK		= "SPK";

	// Execution: marks the pin as an Execution Pin
	// Underlying implementation is AsTL::BOOL, where True means the Execution signal propagates into/out of the pin, and False means the Execution signal propagates elsewhere
	struct ExecutionTag{};
	inline const std::type_index TID_EXEC	= typeid(ExecutionTag);
	inline const std::string SER_EXEC		= "EXEC";


	/* Is the type a high-level type (True), or a primitive/low-level type (False)? */
	inline bool IsNonPrimitive(std::type_index type) {
		return (
			type == TID_OBJ ||
			type == TID_SPC ||
			type == TID_BODY ||
			type == TID_SPK
		);
	}


	/* Converts a high-level pin type to a low-level primitive. */
	inline std::type_index HighLevelTypeToPrimitive(std::type_index type) {
		if (type == TID_OBJ)		return AsTL::TID_IDX;
		else if (type == TID_SPC)	return AsTL::TID_IDX;
		else if (type == TID_BODY)	return AsTL::TID_IDX;
		else if (type == TID_SPK)	return AsTL::TID_STR;
		else if (type == TID_EXEC)	return AsTL::TID_BOOL;

		else return type;
	}


	/* Converts a serialized string to a pin type. */
	inline static std::type_index StringToPinType(const std::string &type) {
		// High-level types
		if (type == "OBJ")			return TID_OBJ;
		else if (type == "SPC")		return TID_SPC;
		else if (type == "BODY")	return TID_BODY;
		else if (type == "SPK")		return TID_SPK;
		else if (type == "EXEC")	return TID_EXEC;

		// Primitive types
		else return AsTL::StringToStackValue(type);
	}


	/* Converts a pin type to a serialized string. */
	inline static std::string PinTypeToString(std::type_index type) {
		// High-level types
		if (type == TID_OBJ)		return "OBJ";
		else if (type == TID_SPC)	return "SPC";
		else if (type == TID_BODY)	return "BODY";
		else if (type == TID_SPK)	return "SPK";
		else if (type == TID_EXEC)	return "EXEC";

		// Primitive types
		else return AsTL::StackValueToString(type);
	}


	/* Converts a pin type to a representative string. */
	inline static std::string PinTypeToRepString(std::type_index type) {
		// High-level types
		if (type == TID_OBJ)		return "Object";
		else if (type == TID_SPC)	return "Spacecraft";
		else if (type == TID_BODY)	return "Celestial Body";
		else if (type == TID_SPK)	return "SPICE Kernel";
		else if (type == TID_EXEC)	return "Execution Pin";

		// Primitive types
		else return AsTL::StackValueToRepString(type);
	}


	// Graph variable
	// NOTE: Graph variables don't directly exist as graph nodes; only their Getters and Setters do
	struct Variable {
		std::string name;			// Variable names are IDs
		std::type_index type;
		AsTL::StackValue val;		// The variable must always have a value (can start as default value)
	};


	// Standard physical graph node
	struct Node {
		struct DataInPin {
			std::string label;			// Pin labels are IDs
			std::type_index type;
			AsTL::StackValue val;		// Value (could be default value if there is no data link to this pin)
		};

		struct ComboInPin {
			std::string label;						// Pin labels are IDs
			std::type_index comboType;				// The combo type is a variable combo box filter (e.g., type is BOOL => only BOOL variables are shown in the dropdown)
			std::string chosenVar;					// The chosen variable (by name)
		};

		struct DataOutPin {
			std::string label;			// Pin labels are IDs
			std::type_index type;
		};

		NodeID id;
		std::string symbol;

		std::vector<std::string> execInPins;
		std::vector<std::string> execOutPins;
		std::vector<
			std::variant<DataInPin, ComboInPin>
		> inputPins;
		std::vector<DataOutPin> outputPins;
	};


	// Standard physical graph link
	struct Link {
		enum class LinkType {
			EXEC,			// Execution link
			DATA			// Data link
		};

		LinkType linkType;

		NodeID outNodeID;
		std::string outPinID;

		NodeID inNodeID;
		std::string inPinID;
	};


	// Editor-specific: Logical comment-block link
	struct CommentBlockLink {
		std::vector<NodeID> nodeIDs;
		std::string comment;
	};


	// Editor-specific: Node metadata
	struct NodeMeta {
		NodeID id;

		struct { float x; float y; } position;
		std::string comment;
	};
}
