/* CoordSys - Common data pertaining to coordinate systems, inertial frames, and epochs.
*/

#pragma once


namespace CoordSys {
	// Epochs
	enum class Epoch {
		J2000,
		B1950
	};

	const std::unordered_map<std::string, Epoch> EpochStrToEnumMap = { // Mappings between epoch strings (via YAML files) and their enums
		{ "J2000",		Epoch::J2000 },
		{ "B1950",		Epoch::B1950 }
	};

	const std::unordered_map<Epoch, std::string> EpochToSPICEMap = {	// Mappings between Epoch enum values and their SPICE name equivalents
		{ Epoch::J2000,		"J2000" },
		{ Epoch::B1950,		"B1950" }
	};


	// Frames
	enum class FrameType {
		INERTIAL,
		NON_INERTIAL
	};

	enum class Frame {
		NONE,

		// Inertial frames
		ECI,	// Earth-centered Inertial
		HCI,	// Heliocentric Inertial

		// Non-inertial frames
		ECEF	// Earth-Centered Earth-Fixed
	};

	const std::unordered_map<std::string, Frame> FrameStrToEnumMap = { // Mappings between frame strings (via YAML files) and their enums
		{ "ECI",		Frame::ECI },
		{ "HCI",		Frame::HCI },
		{ "ECEF",		Frame::ECEF }
	};

	const std::unordered_map<Frame, FrameType> FrameToTypeMap = { // Mappings between frames and their types
		{ Frame::ECI,	FrameType::INERTIAL },
		{ Frame::HCI,	FrameType::INERTIAL },
		{ Frame::ECEF,	FrameType::NON_INERTIAL }
	};

	const std::unordered_map<Frame, std::string> FrameEnumToStrMap = { // Mappings between frame enums and their strings (via YAML files)
		{ Frame::ECI,	"ECI" },
		{ Frame::HCI,	"HCI" },
		{ Frame::ECEF,	"ECEF" }
	};

	const std::unordered_map<Frame, std::string> FrameEnumToDisplayStrMap = { // Mappings between frame enums and their display strings
		{ Frame::ECI,	"Earth-Centered Inertial" },
		{ Frame::HCI,	"Heliocentric Inertial" },
		{ Frame::ECEF,	"Earth-Centered Earth-Fixed" }
	};

	const std::unordered_map<FrameType, std::string> FrameTypeToDisplayStrMap = { // Mappings between frame type enums and their display strings
		{ FrameType::INERTIAL,		"Inertial Frame" },
		{ FrameType::NON_INERTIAL,	"Non-Inertial Frame" }
	};
}