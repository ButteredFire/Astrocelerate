/* Generates camera-relative float trajectory points from classical orbital elements. */

#pragma once

#include <cmath>
#include <vector>

#include <Platform/External/GLM.hpp>

#include <Core/Data/Math.hpp>
#include <Core/Data/Physics.hpp>

#include <Simulation/Algorithms/COE/COE.hpp>


namespace OrbitPointGen {

    struct GenerationConfig {
        uint32_t    pointCount = 512;           // Number of trajectory points to generate
        double      maxRenderDist = 1.0e12;     // Maximum render distance (m); trajectory points beyond this are dropped
        double      minOrbitRadius = 1.0e3;     // Apoapsis below this (m) means orbit is invisible
    };


    /* Computes a single position on the orbit in the ECI frame.
       @param p:    Semi-latus rectum (m)
       @param e:    Eccentricity magnitude
       @param nu:   True anomaly (rad)
       @param R:    Perifocal-to-ECI rotation matrix

       @return      Position in ECI frame (m), or NaN-vector if radius exceeds maxRenderDist
    */
    inline glm::dvec3 OrbitPositionAtNu(double p, double e, double nu, const glm::dmat3 &R) {
        double r_mag = p / (1.0 + e * std::cos(nu));
        glm::dvec3 r_pqw(r_mag * std::cos(nu), r_mag * std::sin(nu), 0.0);
        return R * r_pqw;
    }


    /* Builds the perifocal-to-ECI rotation matrix from COEs.
       @param coe: Classical orbital elements

       @return     3x3 rotation matrix mapping perifocal (PQW) -> ECI (IJK)
    */
    inline glm::dmat3 PerifocalToECI(const COE::Elements &coe) {
        using enum Physics::OrbitGeometry;
        using enum Physics::OrbitInclination;

        double omega = coe.omega;
        double argp = coe.argp;
        double incl = coe.incl;

        // Degenerate substitutions per Vallado
        if (std::isnan(omega)) omega = 0.0;     // Equatorial orbit: RAAN undefined, set to 0
        if (std::isnan(argp))  argp = 0.0;     // Circular orbit: argp undefined, set to 0

        // Rz(-omega)
        glm::dmat3 Rz_omega(
            std::cos(omega),    std::sin(omega),    0.0,
            -std::sin(omega),   std::cos(omega),    0.0,
            0.0,                0.0,                1.0
        );

        // Rx(-incl)
        glm::dmat3 Rx_incl(
            1.0,    0.0,                0.0,
            0.0,    std::cos(incl),     std::sin(incl),
            0.0,    -std::sin(incl),    std::cos(incl)
        );

        // Rz(-argp)
        glm::dmat3 Rz_argp(
            std::cos(argp),     std::sin(argp),     0.0,
            -std::sin(argp),    std::cos(argp),     0.0,
            0.0,                0.0,                1.0
        );

        return Rz_omega * Rx_incl * Rz_argp;
    }


    /* Generates ECI-frame trajectory points for an elliptic or circular orbit.
       Samples uniformly in eccentric anomaly to concentrate points near periapsis
       for high-eccentricity orbits.

       @param coe:     Classical orbital elements
       @param R:       Perifocal-to-ECI rotation matrix
       @param config:  Generation configuration

       @return         ECI positions (m)
    */
    inline std::vector<glm::dvec3> GenerateElliptic(
        const COE::Elements &coe,
        const glm::dmat3 &R,
        const GenerationConfig &config)
    {
        std::vector<glm::dvec3> points;
        points.reserve(config.pointCount + 1);  // +1 to close the loop

        double e = coe.e;
        double p = coe.p;

        // Sample uniformly in eccentric anomaly in range [0, 2pi] to "densify" points near periapsis
        double step = TWOPI / static_cast<double>(config.pointCount);

        for (uint32_t i = 0; i <= config.pointCount; i++) {
            double E = i * step;

            // Eccentric anomaly -> true anomaly
            double cos_nu = (std::cos(E) - e) / (1.0 - e * std::cos(E));
            double sin_nu = (std::sqrt(1.0 - e * e) * std::sin(E)) / (1.0 - e * std::cos(E));
            double nu = std::atan2(sin_nu, cos_nu);

            glm::dvec3 pos = OrbitPositionAtNu(p, e, nu, R);
            points.push_back(pos);
        }

        return points;
    }


    /* Generates ECI-frame trajectory points for a hyperbolic orbit.
       Sweeps true anomaly within the physical bounds defined by the asymptote,
       and clamps points exceeding maxRenderDist.

       @param coe:     Classical orbital elements
       @param R:       Perifocal-to-ECI rotation matrix
       @param config:  Generation configuration

       @return         ECI positions (m), may be fewer than pointCount if clamped
    */
    inline std::vector<glm::dvec3> GenerateHyperbolic(
        const COE::Elements &coe,
        const glm::dmat3 &R,
        const GenerationConfig &config)
    {
        std::vector<glm::dvec3> points;
        points.reserve(config.pointCount);

        double e = coe.e;
        double p = coe.p;

        // Asymptote angle — true anomaly at which r -> infinity
        double nu_asymptote = std::acos(-1.0 / e);
        double nu_margin = 0.01;                         // Radians away from asymptote — prevents r -> inf
        double nu_max = nu_asymptote - nu_margin;
        double nu_min = -nu_max;

        double step = (nu_max - nu_min) / static_cast<double>(config.pointCount - 1);

        for (uint32_t i = 0; i < config.pointCount; i++) {
            double nu = nu_min + i * step;
            double r_mag = p / (1.0 + e * std::cos(nu));

            // Clamp at render distance — drop remaining points
            if (r_mag > config.maxRenderDist)
                break;

            glm::dvec3 pos = OrbitPositionAtNu(p, e, nu, R);
            points.push_back(pos);
        }

        return points;
    }


    /* Generates ECI-frame trajectory points for a parabolic orbit.
       Treated as a limiting case of hyperbolic (e clamped just above 1).
       Same distance clamping applies.

       @param coe:     Classical orbital elements (e ≈ 1)
       @param R:       Perifocal-to-ECI rotation matrix
       @param config:  Generation configuration

       @return         ECI positions (m)
    */
    inline std::vector<glm::dvec3> GenerateParabolic(
        const COE::Elements &coe,
        const glm::dmat3 &R,
        const GenerationConfig &config)
    {
        // Parabola has nu_asymptote = π — treat as hyperbolic with e slightly above 1
        COE::Elements coe_hyperbolic = coe;
        coe_hyperbolic.e = 1.0 + 1e-6;
        return GenerateHyperbolic(coe_hyperbolic, R, config);
    }


    /* Generates trajectory points for an orbit.
       @param coe:             Classical orbital elements describing the orbit
       @param bodyMeshRadius:  Radius of the parent body's mesh (m)
       @param bodyAbsPos:      The parent body's absolute position in simulation space (m)
       @param config:          Generation configuration

       @return                 Camera-relative float positions, or empty if culled
    */
    inline std::vector<glm::dvec3> GenerateTrajectoryPoints(
        const COE::Elements     &coe,
        double                  bodyMeshRadius,
        const glm::dvec3        &bodyAbsPos,
        const GenerationConfig  &config)
    {
        using enum Physics::OrbitGeometry;

        // Don't generate anything if COE/orbit is invalid
        if (std::isnan(coe.p) || coe.p <= 0.0)
            return {};


        // Apoapsis culling: don't generate anything if entire orbit is inside the body mesh
        double r_apoapsis;
        if (coe.orbitGeom == HYPERBOLIC || coe.orbitGeom == PARABOLIC) {
            r_apoapsis = std::numeric_limits<double>::infinity();    // Open conic (no apoapsis)
        }
        else {
            r_apoapsis = coe.a * (1.0 + coe.e);
        }

        if (r_apoapsis < bodyMeshRadius)
            return {};


        // Minimum orbit radius cull
        if (r_apoapsis < config.minOrbitRadius)
            return {};


        // Build rotation matrix
        glm::dmat3 R = PerifocalToECI(coe);


        // Generate points in ECI
        std::vector<glm::dvec3> eciPoints;

        switch (coe.orbitGeom) {
        case CIRCULAR:
        case ELLIPTICAL:
            eciPoints = GenerateElliptic(coe, R, config);
            break;

        case HYPERBOLIC:
            eciPoints = GenerateHyperbolic(coe, R, config);
            break;

        case PARABOLIC:
            eciPoints = GenerateParabolic(coe, R, config);
            break;
        }

        if (eciPoints.empty())
            return {};


        // Offset points by the orbiting body's absolute position
        std::vector<glm::dvec3> result;
        result.reserve(eciPoints.size());

        for (const glm::dvec3 &pos : eciPoints) {
            result.push_back(pos + bodyAbsPos);
        }

        return result;
    }

}
