#pragma once

#include <cmath>
#include <limits>
#include <numbers>

#include <Core/Data/Physics.hpp>


namespace COE {
    /* Classical orbital elements */
    struct Elements {
        double p = NAN;
		double a = NAN;
		double e = NAN;
		double incl = NAN;
		double omega = NAN;
		double argp = NAN;
		double nu = NAN;
		double m = NAN;
		double arglat = NAN;
		double truelon = NAN;
		double lonper = NAN;

        Physics::OrbitGeometry orbitGeom;
        Physics::OrbitInclination orbitIncl;
    };
}
