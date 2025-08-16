#include "ECIFrame.hpp"


ECIFrame::ECIFrame() {

}


ECIFrame::~ECIFrame() {
	kclear_c();		// Unload kernels
}


void ECIFrame::init(const std::vector<std::string> &kernelPaths, CoordSys::Epoch epoch, const std::string &epochFormat) {
	// Loads kernels
	for (const auto &path : kernelPaths)
		furnsh_c(path.c_str());

	// "string to ET" - Converts epoch format string to Ephemeris Time (ET)
	str2et_c(epochFormat.c_str(), &m_epochET);


	m_epochName = CoordSys::EpochToSPICEMap.at(epoch);
	m_epochFormat = epochFormat;
}


std::array<double, 6> ECIFrame::getBodyState(const std::string &targetName, double ephTime) {
	if (!SPICEUtils::IsObjectAvailable(targetName)) {
		Log::Print(Log::T_ERROR, __FUNCTION__, "Target body " + enquote(targetName) + " is not available in the SPICE kernels!");
		return {};
	}

	// NOTE: A state vector contains 6 components: the first 3 for the position vector (x, y, z), the last 3 for the velocity vector (vx, vy, vz).
	std::array<double, 6> state{};
	double lightTime{};		// Light time between observer (e.g., "EARTH") and target

	/* "Spacecraft and Planet Kernel (SPK) easy-read"
		Returns the state (position and velocity, 6 components) of a target body relative to an observing body,
		optionally corrected for light time (planetary aberration) and stellar aberration.
	*/
	spkezr_c(targetName.c_str(), ephTime, m_frameName.c_str(), "NONE", "EARTH", state.data(), &lightTime);


	// Convert to meters
	for (size_t i = 0; i < state.size(); i++)
		state[i] *= 1e3;

	return state;
}


glm::mat3 ECIFrame::getRotationMatrix(const std::string &targetFrame, double ephTime) {
	double rotMat[3][3];

	/* "Position X-form"
		Used for transforming position vectors (3 components). "xform" is an abbreviation for "transformation". My god, that's fucking stupid. ARE THE PEOPLE AT NASA OBSESSED WITH ABBREVIATIONS OR SOMETHING????????
		NOTE: Similarly, sxform_c ("State X-form") is used for transforming state vectors (position and velocity, 6 components).
	*/
	pxform_c(m_frameName.c_str(), targetFrame.c_str(), ephTime, rotMat);

	return glm::mat3(
		rotMat[0][0], rotMat[0][1], rotMat[0][2],
		rotMat[1][0], rotMat[1][1], rotMat[1][2],
		rotMat[2][0], rotMat[2][1], rotMat[2][2]
	);
}
