#include "CoordinateSystem.hpp"


CoordinateSystem::CoordinateSystem() {
	reset();

	// Configure SPICE to return from any functions that failed to execute
	// (thus allowing us to check the execution status with `failed_c` and handle failures gracefully).
	erract_c("SET", 0, const_cast<SpiceChar *>("RETURN"));
	SPICEUtils::CheckFailure(true);


	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


CoordinateSystem::~CoordinateSystem() {
	reset();
}


void CoordinateSystem::reset() {
	kclear_c();
	SPICEUtils::CheckFailure(true);
}


void CoordinateSystem::init(const std::vector<std::string> &kernelPaths, CoordSys::Frame frame, CoordSys::Epoch epoch, const std::string &epochFormat) {
	// Load kernels
	for (const auto &path : kernelPaths) {
		furnsh_c(path.c_str());

		SPICEUtils::CheckFailure(true, false,
			[path](const std::string errMsg) {
				unload_c(path.c_str());  // Unload the kernel that failed to load (to prevent it being erroneously flagged as "not found" and thus not loading when the coordinate system is re-initialized)
				SPICEUtils::CheckFailure(true);
			}
		);

		Log::Print(Log::T_INFO, __FUNCTION__, "Loaded kernel " + FilePathUtils::GetFileName(path) + ".");
	}


	// "string to ET" - Converts epoch format string to Ephemeris Time (ET)
	str2et_c(epochFormat.c_str(), &m_epochET);
	SPICEUtils::CheckFailure(true);


	m_observerName = CoordSys::FrameProperties.at(frame).spiceName;
	m_frameName = CoordSys::EpochToSPICEMap.at(epoch);
	m_epochFormat = epochFormat;
}


std::array<double, 6> CoordinateSystem::getBodyState(const std::string &targetName, double ephTime) {
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
	spkezr_c(targetName.c_str(), ephTime, m_frameName.c_str(), "NONE", m_observerName.c_str(), state.data(), &lightTime);
	SPICEUtils::CheckFailure(false, true);


	// Convert state vector from km and km/s to m and m/s respectively
	for (size_t i = 0; i < state.size(); i++)
		state[i] *= 1e3;

	return state;
}


glm::mat3 CoordinateSystem::getRotationMatrix(const std::string &targetFrame, double ephTime) {
	double rotMat[3][3];

	// "Position X-form": Used for transforming position vectors (3 components). "X-form" is an abbreviation for "transformation".
	pxform_c(m_frameName.c_str(), targetFrame.c_str(), ephTime, rotMat);
	SPICEUtils::CheckFailure(false, true);

	return glm::mat3(
		rotMat[0][0], rotMat[0][1], rotMat[0][2],
		rotMat[1][0], rotMat[1][1], rotMat[1][2],
		rotMat[2][0], rotMat[2][1], rotMat[2][2]
	);
}


std::array<double, 6> CoordinateSystem::TEMEToThisFrame(const std::array<double, 6> &stateVector, double ephTime) {
	// Convert ET -> ...
		// JED (Julian Ephemeris Date)
	double julianDate = unitim_c(ephTime, "ET", "JED");
		
		// UT1 (UTC Julian Date)
	double julianDateUTC = 0.0;
	char utcStrBuf[50];
	char *endptr;
	uint32_t precision = 14;
	et2utc_c(ephTime, "J", precision, 50, utcStrBuf);
	julianDateUTC = strtod(utcStrBuf + 3, &endptr);  // Skips the "JD " prefix and convert the rest of the string to a double

	std::cout << "Julian date: " << julianDate << " (JED); " << julianDateUTC << " (UTC/UT1)\n";

	// Calculate Earth orientation parameters & prepare nutation parameters
	glm::dvec3 precession = Body::Earth.getPrecessionAngles(julianDate);
	auto [pZeta, pTheta, pZed] = std::tuple<double, double, double>(precession.x, precession.y, precession.z);	// Convert (x, y, z) to (ζ, θ, z)
	 
	PhysicsComponent::NutationAngles nutation = Body::Earth.getNutationAngles(julianDate, julianDateUTC);

	double epsilon = nutation.meanEpsilon + nutation.deltaEpsilon;			// True obliquity of ecliptic
	double dPsiCosEps = nutation.deltaPsi * glm::cos(epsilon);				// Nutation in longitude * cos(obliquity)


	// Nutation correction (TEME -> MOD)
	glm::dvec3 axisX = glm::dvec3(1.0, 0.0, 0.0);
	glm::dvec3 axisY = glm::dvec3(0.0, 1.0, 0.0);
	glm::dvec3 axisZ = glm::dvec3(0.0, 0.0, 1.0);

	glm::dmat4 nutationMatrix(1.0);
	nutationMatrix = glm::rotate(nutationMatrix, -nutation.meanEpsilon, axisX);		// Remove mean obliquity
	nutationMatrix = glm::rotate(nutationMatrix, nutation.deltaPsi, axisZ);			// Apply nutation in longitude
	nutationMatrix = glm::rotate(nutationMatrix, epsilon, axisX);					// Rotate by true obliquity
	nutationMatrix = glm::rotate(nutationMatrix, -dPsiCosEps, axisZ);				// Remove equation of equinoxes


	// Precession correction (MOD -> J2000)
	glm::dmat4 precessionMatrix(1.0);
	precessionMatrix = glm::rotate(precessionMatrix, pZeta, axisZ);			// Rotation about the final Z-axis
	precessionMatrix = glm::rotate(precessionMatrix, -pTheta, axisY);		// Rotation about the intermediate Y-axis (negative angle)
	precessionMatrix = glm::rotate(precessionMatrix, pZed, axisZ);			// Rotation about the Z-axis


	// Final transformation matrix from TEME to J2000 is a combination of nutation and precession.
	glm::dmat3 transformationMatrix = precessionMatrix * nutationMatrix;

	glm::dvec3 position(stateVector[0], stateVector[1], stateVector[2]);
	position = transformationMatrix * position;

	glm::dvec3 velocity(stateVector[3], stateVector[4], stateVector[5]);
	velocity = transformationMatrix * velocity;


	return std::array<double, 6>{
		position.x, position.y, position.z,
		velocity.x, velocity.y, velocity.z
	};
}
