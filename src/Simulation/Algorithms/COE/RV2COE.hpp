/* RV2COE Algorithm
	C++ adaptation of David Vallado's RV2COE algorithm for Astrocelerate.

    Sources:
    - RV2COE Pseudocode: Algorithm 9, Fundamentals of Astrodynamics and Applications
    - RV2COE MATLAB Implementation: https://github.com/jgte/matlab-sgp4/blob/master/rv2coe.m
*/

#pragma once

#include <cmath>

#include <Core/Data/Math.hpp>
#include <Core/Data/Physics.hpp>

#include <Platform/External/GLM.hpp>

#include <Simulation/Algorithms/COE/COE.hpp>


namespace COE {
    /* Computes the classical orbital elements for an orbiting body, given its geocentric equatorial position and velocity vectors.
	    @param r: The position vector, in an inertial reference frame.
	    @param v: The velocity vector, in an inertial reference frame.
	    @param mu: The gravitational parameter of the central body.
	
	    @return The computed elements.
    */
    inline Elements rv2coe(const glm::dvec3 &r, const glm::dvec3 &v, double mu) {
        using namespace std;
        using enum Physics::OrbitGeometry;
        using enum Physics::OrbitInclination;

        Elements coe{};

	    double r_mag = glm::length(r);
	    double v_mag = glm::length(v);
	
	    // Angular momentum
	    glm::dvec3 h = glm::cross(r, v);
	    double h_mag = glm::length(h);

        if (h_mag < R_EPSILON)
            // Orbits are meaningless if angular momentum is infinitesimally small
            return coe;


	    // Eccentricity
	    glm::dvec3 e = ((v_mag * v_mag - mu / r_mag) * r - glm::dot(r, v) * v) / mu;
	    double e_mag = glm::length(e);


        // Specific mechanical energy
        double sme = (v_mag * v_mag) / 2 - mu / r_mag;
    

        // Semi-major axis & Semi-latus rectum
        double a, p;
        if (glm::abs(sme) > R_EPSILON)    a = -mu / (2.0 * sme);
        else                            a = std::numeric_limits<double>::infinity();
        p = (h_mag * h_mag) / mu;


        // Inclination
        double cos_incl = glm::clamp(h.z / h_mag, -1.0, 1.0);
        double incl = acos(cos_incl);
    

        // Line-of-nodes vector (points toward the orbit's ascending node)
        glm::dvec3 n(-h.y, h.x, 0.0);
        double n_mag = glm::length(n);


        // Longitude of ascending node
            // NOTE: For floating-point variables, there is a special NaN macro that can be used to set it as undefined. It's certainly cleaner than std::optional!
            // NOTE: NAN-checking: std::isnan(val)  (defined in <cmath>)
        double omega = NAN;
        if (n_mag > R_EPSILON) {
            double cos_omega = glm::clamp(n.x / n_mag, -1.0, 1.0);
            omega = acos(cos_omega);

            if (n.y < 0.0)
                omega = TWOPI - omega;
        }


        // Orbit type
        Physics::OrbitGeometry orbitGeom = ELLIPTICAL;
        Physics::OrbitInclination orbitIncl = INCLINED;

            // Orbit geometry
        if (e_mag < R_EPSILON)                                          // e = 0
            orbitGeom = CIRCULAR;
        else if (e_mag < 1.0 - R_EPSILON)                               // 0 < e < 1
            orbitGeom = ELLIPTICAL;
        else if (1.0 - R_EPSILON < e_mag && e_mag < 1.0 + R_EPSILON)    // e = 1
            orbitGeom = PARABOLIC;
        else if (e_mag > 1.0 + R_EPSILON)                               // e > 1
            orbitGeom = HYPERBOLIC;

            // Orbit inclination
        if (incl < R_EPSILON || glm::abs(incl - PI) < R_EPSILON)
            orbitIncl = EQUATORIAL;
        else
            orbitIncl = INCLINED;


        // Argument of perigee
        double argp = NAN;
        if (orbitGeom != CIRCULAR && orbitIncl == INCLINED) {
            double cos_argp = glm::clamp(
                glm::dot(glm::normalize(n), glm::normalize(e)), -1.0, 1.0
            );
            argp = acos(cos_argp);

            if (e.z < 0.0)
                argp = TWOPI - argp;
        }


        // True anomaly at epoch
        double nu = NAN;
        if (orbitGeom != CIRCULAR) {
            double cos_nu = glm::clamp(
                glm::dot(glm::normalize(e), glm::normalize(r)), -1.0, 1.0
            );
            nu = acos(cos_nu);
            
            if (glm::dot(r, v) < 0.0)
                nu = TWOPI - nu;
        }


        // Argument of latitude (circular, inclined orbit)
        double m = NAN;
        double arglat = NAN;
        if (orbitGeom == CIRCULAR && orbitIncl == INCLINED) {
            double cos_arglat = glm::clamp(
                glm::dot(glm::normalize(n), glm::normalize(r)), -1.0, 1.0
            );
            arglat = acos(cos_arglat);
            
            if (r.z < 0.0)
                arglat = TWOPI - arglat;

            m = arglat;
        }

    
        // Longitude of periapsis (non-circular, equatorial orbit)
        double lonper = NAN;
        if (e_mag > R_EPSILON && orbitGeom != CIRCULAR && orbitIncl == EQUATORIAL) {
            double cos_omega = glm::clamp(e.x / e_mag, -1.0, 1.0);
            lonper = acos(cos_omega);

            if (e.y < 0.0)
                lonper = TWOPI - lonper;
            if (incl > (PI / 2.0))
                lonper = TWOPI - lonper;
        }


        // True longitude (circular, equatorial orbit)
        double truelon = NAN;
        if (r_mag > R_EPSILON && orbitGeom == CIRCULAR && orbitIncl == EQUATORIAL) {
            double cos_lambda = glm::clamp(r.x / r_mag, -1.0, 1.0);
            truelon = acos(cos_lambda);

            if (r.y < 0.0)
                truelon = TWOPI - truelon;
            if (incl > (PI / 2.0))
                truelon = TWOPI - truelon;

            m = truelon;
        }



        coe.p = p;
        coe.a = a;
        coe.e = e_mag;
        coe.incl = incl;
        coe.omega = omega;
        coe.argp = argp;
        coe.nu = nu;
        coe.m = m;
        coe.arglat = arglat;
        coe.truelon = truelon;
        coe.lonper = lonper;

        coe.orbitGeom = orbitGeom;
        coe.orbitIncl = orbitIncl;

        return coe;
    }
}
