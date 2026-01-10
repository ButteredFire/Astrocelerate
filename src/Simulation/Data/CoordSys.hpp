/* CoordSys - Common data pertaining to coordinate systems, inertial frames, and epochs.
*/

#pragma once

#include <string>
#include <unordered_map>


namespace CoordSys {
	// ----- EPOCHS -----
	enum class Epoch {
		J2000,
		B1950
	};

		// Mappings between epoch SPICE names and their enums
	const std::unordered_map<std::string, Epoch> EpochStrToEnumMap = {
		{ "J2000",		Epoch::J2000 },
		{ "B1950",		Epoch::B1950 }
	};

		// Mappings between Epoch enum values and their SPICE names
	const std::unordered_map<Epoch, std::string> EpochToSPICEMap = {
		{ Epoch::J2000,		"J2000" },
		{ Epoch::B1950,		"B1950" }
	};



	// ----- REFERENCE FRAMES -----
	enum class FrameType {
		INERTIAL,
		NON_INERTIAL
	};

	enum class Frame {
		NONE,

		// Inertial frames
		ECI,	// Earth-centered Inertial
		HCI,	// Heliocentric Inertial
		SSB,	// Solar System Barycenter

		// Non-inertial frames
		ECEF	// Earth-Centered Earth-Fixed
	};

	struct _FrameProps {
		std::string spiceName;		// The frame's SPICE name (observer name)
		std::string yamlValue;		// The frame's YAML value
		std::string displayName;	// The frame's display name
		FrameType frameType;		// The frame type
	};

		// Mappings between coordinate systems (frames) and their properties
	const std::unordered_map<Frame, _FrameProps> FrameProperties = {
		{ Frame::ECI,	{ "EARTH",
						"ECI",
						"Earth-Centered Inertial",
						FrameType::INERTIAL }
		},

		{ Frame::HCI,	{ "SUN",
						"HCI",
						"Heliocentric Inertial",
						FrameType::INERTIAL }
		},

		{ Frame::SSB,	{ "SSB",
						"SSB",
						"Solar System Barycenter",
						FrameType::INERTIAL }
		},

		{ Frame::ECEF,	{ "IAU_EARTH",
						"ECEF",
						"Earth-Centered Earth-Fixed",
						FrameType::NON_INERTIAL }
		}
	};

		// Mappings between frames (as YAML value strings) and frames (as enums)
	const std::unordered_map<std::string, Frame> FrameYAMLToEnumMap = {
		{ "ECI", Frame::ECI },
		{ "HCI", Frame::HCI },
		{ "SSB", Frame::SSB },

		{ "ECEF", Frame::ECEF }
	};

		// Mappings between frame type enums and their display strings
	const std::unordered_map<FrameType, std::string> FrameTypeToDisplayStrMap = {
		{ FrameType::INERTIAL,		"Inertial Frame" },
		{ FrameType::NON_INERTIAL,	"Non-Inertial Frame" }
	};
}