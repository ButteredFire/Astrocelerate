#pragma once

#include <cmath>
#include <limits>
#include <numbers>

#include <Core/Data/Physics.hpp>


namespace COE {
    /* Classical orbital elements */
    struct Elements {
        double p = NAN;         // [km]  Semi-latus rectum
        double a = NAN;         // [km]  Semi-major axis
        double e = NAN;         // Eccentricity
        double incl = NAN;      // [rad] Inclination
        double omega = NAN;     // [rad] Longitude of ascending node (Ω)
        double argp = NAN;      // [rad] Argument of periapsis (ω)
        double nu = NAN;        // [rad] True anomaly at epoch (ν)
        double m = NAN;         // [rad] Mean anomaly at epoch (M)
        double arglat = NAN;    // [rad] Argument of latitude
        double truelon = NAN;   // [rad] True longitude
        double lonper = NAN;    // [rad] Longitude of periapsis

        Physics::OrbitGeometry orbitGeom = Physics::OrbitGeometry::ELLIPTICAL;          // Orbit geometry type
        Physics::OrbitInclination orbitIncl = Physics::OrbitInclination::INCLINED;      // Orbit inclination type
    };
}
