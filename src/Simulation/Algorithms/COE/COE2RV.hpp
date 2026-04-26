/* COE2RV Algorithm
    C++ adaptation of David Vallado's COE2RV algorithm for Astrocelerate.

    Sources:
    - COE2RV Pseudocode: Algorithm 10, Fundamentals of Astrodynamics and Applications
*/

#pragma once

#include <array>
#include <cmath>

#include <Platform/External/GLM.hpp>

#include <Core/Data/Math.hpp>

#include <Simulation/Algorithms/COE/COE.hpp>


namespace COE {
    /* Computes the state vector of an orbiting body in an inertial reference frame, given its classical orbital elements.
        @param elements: The orbital elements.
	    @param mu: The gravitational parameter of the central body.

        @return The state vector, in an inertial reference frame.
    */
    std::array<double, 6> coe2rv(const COE::Elements &elements, double mu) {
        using namespace std;
        using enum Physics::OrbitGeometry;
        using enum Physics::OrbitInclination;

        std::array<double, 6> nullState{};
        nullState.fill(NAN);

        if (elements.p < R_EPSILON)
            // Degenerate: orbit is a straight-line trajectory
            return nullState;


        double argp = elements.argp;
        double omega = elements.omega;
        double nu = elements.nu;


        // Circular
        if (elements.orbitGeom == CIRCULAR) {
            argp = 0.0;
            
            if (elements.orbitIncl == EQUATORIAL) {
                omega = 0.0;
                nu = elements.truelon;
            }
            else if (elements.orbitIncl == INCLINED) {
                nu = elements.arglat;
            }
        }

        // Non-circular, equatorial
        else if (elements.orbitIncl == EQUATORIAL) {
            omega = 0.0;
            nu = elements.lonper;
        }


        // State vector in Perifocal (PQW) reference frame
        double denom = 1.0 + elements.e * cos(nu);
        double root = sqrt(mu / elements.p);

        if (denom < R_EPSILON)
			// Degenerate: radius is infinite at the given true anomaly
            return nullState;

        glm::dvec3 r_pqw(
            elements.p * cos(nu) / denom,
            elements.p * sin(nu) / denom,
            0.0
        );

        glm::dvec3 v_pqw(
            -root * sin(nu),
            root * (elements.e + cos(nu)),
            0.0
        );


        // PQW -> IJK transformation matrix
        double &cO = argp;
        double &lO = omega;
        double i = elements.incl;

            // NOTE: GLM matrices are column-major by default
        glm::dmat3 transform(
            cos(cO)*cos(lO)-sin(cO)*sin(lO)*cos(i),     sin(cO)*cos(lO)+cos(cO)*sin(lO)*cos(i),     sin(lO)*sin(i),
            -cos(cO)*sin(lO)-sin(cO)*cos(lO)*cos(i),    -sin(cO)*sin(lO)+cos(cO)*cos(lO)*cos(i),    cos(lO)*sin(i),
            sin(cO)*sin(i),                             -cos(cO)*sin(i),                            cos(i)
        );


        // State vector in an inertial reference frame
        glm::dvec3 r_ijk = transform * r_pqw;
        glm::dvec3 v_ijk = transform * v_pqw;


        return {
            r_ijk.x, r_ijk.y, r_ijk.z,
            v_ijk.x, v_ijk.y, v_ijk.z
        };
    }
}
