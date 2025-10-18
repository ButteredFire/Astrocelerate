/* Earth - Implementation of Earth's properties.
	Mathematical implementation is directly translated to C++ from works by:
		@thkruz ; Theodore Kruczek (https://github.com/thkruz/ootk/blob/master/src/body/Earth.ts)
		@david-rc-dayton (https://github.com/david-rc-dayton/pious_squid/blob/master/lib/src/body/earth.dart)
*/

#pragma once

#include <cmath>
#include <numbers>

#include <vector>

#include "ICelestialBody.hpp"

#include <Core/Data/Math.hpp>
#include <Simulation/NutationCoefficients/IAU1980.hpp>


static const double DEG2RAD = PI / 180.0;
static const double ASEC2RAD = DEG2RAD / 3600.0;

static const double TAU = PI * 2.0;



namespace _Bodies {
    class Earth : public ICelestialBody {
    public:
        CoreComponent::Identifiers getIdentifiers() const override {
            return CoreComponent::Identifiers{
                .entityType = CoreComponent::Identifiers::EntityType::PLANET,
                .spiceID = "EARTH"
            };
        }
        double getGravParam() const override { return 3.986004418e+14; }
        double getEquatRadius() const override { return 6.3781363e+6; }
        glm::dvec3 getRotVelocity() const override { return glm::dvec3(0.0, 0.0, 7.2921159e-5); }
        double getJ2() const override { return 1.08262668355315e-3; }
        double getFlattening() const override { return 0.0033528197; }
        double getMass() const override { return 5.972e+24; }

        std::string getMeshPath() const override {
            return FilePathUtils::JoinPaths(ROOT_DIR, "assets/Models/CelestialBodies/Earth/Earth.gltf");
        }


        /* Computes the time-dependent precession angles for a given epoch. */
        inline glm::dvec3 getPrecessionAngles(double julianDateTT) {
            double t = centuriesSinceJ2000(julianDateTT);

            double zeta = m_zetaPoly.evaluate(t);
            double theta = m_thetaPoly.evaluate(t);
            double zed = m_zedPoly.evaluate(t);

            return glm::dvec3(zeta, theta, zed);
        }


        /* Computes the time-dependent nutation angles for a given epoch. */
        inline PhysicsComponent::NutationAngles getNutationAngles(double julianDateTT, double julianDateUTC) {
            double t = centuriesSinceJ2000(julianDateTT);

			double moonAnom = m_moonAnomPoly.evaluate(t);
			double sunAnom = m_sunAnomPoly.evaluate(t);
			double moonLat = m_moonLatPoly.evaluate(t);
			double sunElong = m_sunElongPoly.evaluate(t);
			double moonRaan = m_moonRaanPoly.evaluate(t);


            double deltaPsi = 0.0;
            double deltaEpsilon = 0.0;
            for (const auto &coeff : IAU1980::Coefficients) {
                double arg =
                    coeff.a1 * moonAnom +
                    coeff.a2 * sunAnom +
                    coeff.a3 * moonLat +
                    coeff.a4 * sunElong +
                    coeff.a5 * moonRaan;

                // Since sin and cos are periodic, large values of arg are mathematically valid since they just mean more revolutions around the unit circle (e.g., sin(pi) = sin(1e1000*pi)). However, it is good practice to reduce large arg values to avoid numerical instability (e.g., sin(pi) = 0, but sin(1e1000*pi) might erroneously be equal to something like 1e-31).
                arg = std::fmod(arg, 2 * PI);

                const double sinC = coeff.ai + coeff.bi * t;
                const double cosC = coeff.ci + coeff.di * t;

                double termPsi = sinC * sin(arg);
                double termEps = cosC * cos(arg);

                deltaPsi += termPsi;        // termPsi
                deltaEpsilon += termEps;    // termEpsilon
            }


            // Convert deltaPsi, deltaEpsilon: asec -> rad
                // NOTE: IAU1980 nutation coefficients (ai, bi, ci, di) are in 0.0001 arcseconds, not arcseconds. We therefore need to convert deltaPsi and deltaEpsilon to arcseconds before converting them to radians.
            deltaPsi *= 1.0e-4 * ASEC2RAD;
            deltaEpsilon *= 1.0e-4 * ASEC2RAD;

            double meanEpsilon = m_meanEpsilonilonPoly.evaluate(t);
            double epsilon = meanEpsilon + deltaEpsilon;


            // NOTE: The `eqEq` calculation from Earth.ts requires `moonRaan` to be in radians
            double eqEquinoxes = deltaPsi * cos(meanEpsilon) + 0.00264 * ASEC2RAD * sin(moonRaan) + 0.000063 * ASEC2RAD * sin(2.0 * moonRaan);


            // Greenwich Apparent Sidereal Time (GAST)
            double dut1 = 0.0947101;    // DUT1 (= UT1 - UTC) for 2025/10/16
            double julianDateUT1 = julianDateUTC + dut1 / 86400.0;
            double gast = gstime(julianDateUT1) + eqEquinoxes;


            PhysicsComponent::NutationAngles angles;
            angles.deltaPsi = deltaPsi;
            angles.deltaEpsilon = deltaEpsilon;
            angles.meanEpsilon = meanEpsilon;
            angles.epsilon = epsilon;
            angles.eqEquinoxes = eqEquinoxes;
            angles.greenwichSiderealTime = gast;

            return angles;
        }

    private:
        /* Computes centuries since J2000. 
            @param julianDateTT: Julian Date (Terrestrial Time).

            @return Centuries past J2000.
        */
        static double centuriesSinceJ2000(double julianDateTT) {
            return (julianDateTT - 2451545.0) / 36525.0;
        }


        // ----- EARTH PRECESSION POLYNOMIAL COEFFICIENTS -----
            // Source: https://iers-conventions.obspm.fr/archive/2003/tn32.pdf ("Precession Developments compatible with the IAU2000 Model", P.45)
		inline static const Math::Polynomial<double, 4> m_zetaPoly = {
			0.0,
			2306.2181 * ASEC2RAD,
			0.30188 * ASEC2RAD,
			0.017998 * ASEC2RAD
		};

        inline static const Math::Polynomial<double, 4> m_thetaPoly = {
            0.0,
            2004.3109 * ASEC2RAD,
            -0.42665 * ASEC2RAD,
            -0.041833 * ASEC2RAD
        };

        inline static const Math::Polynomial<double, 4> m_zedPoly = {
            0.0,
            2306.2181 * ASEC2RAD,
            1.09468 * ASEC2RAD,
            0.018203 * ASEC2RAD
        };


        // ----- OTHER POLYNOMIAL COEFFICIENTS -----
        inline static const Math::Polynomial<double, 4> m_moonAnomPoly = {
            134.96340251 * DEG2RAD,
            (1325.0 * 360.0 + 198.8675605) * DEG2RAD,
            0.0088553 * DEG2RAD,
            1.4343e-5 * DEG2RAD
        };

        inline static const Math::Polynomial<double, 4> m_sunAnomPoly = {
            357.52910918 * DEG2RAD,
            (99.0 * 360.0 + 359.0502911) * DEG2RAD,
            -0.0001537 * DEG2RAD,
            3.8e-8 * DEG2RAD
        };

        inline static const Math::Polynomial<double, 4> m_moonLatPoly = {
            93.27209062 * DEG2RAD,
            (1342.0 * 360.0 + 82.0174577) * DEG2RAD,
            -0.003542 * DEG2RAD,
            -2.88e-7 * DEG2RAD
        };

        inline static const Math::Polynomial<double, 4> m_sunElongPoly = {
            297.8503632 * DEG2RAD,
            (1236.0 * 360.0 + 297.8503632) * DEG2RAD,
            -0.0003022 * DEG2RAD,
            -1.55e-7 * DEG2RAD
        };

        inline static const Math::Polynomial<double, 4> m_moonRaanPoly = {
            125.04455501 * DEG2RAD,
            -(5.0 * 360.0 + 134.1361851) * DEG2RAD,
            0.0020756 * DEG2RAD,
            2.139e-6 * DEG2RAD
        };

        inline static const Math::Polynomial<double, 4> m_meanEpsilonilonPoly = {
            84381.448 * ASEC2RAD,
            -46.815 * ASEC2RAD,
            -0.00059 * ASEC2RAD,
            0.001813 * ASEC2RAD
        };
    };
}