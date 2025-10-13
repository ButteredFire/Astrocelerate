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

#include <Simulation/NutationCoefficients/IAU1980.hpp>


static constexpr double DEG2RAD = std::numbers::pi / 180.0;
static constexpr double ASEC2RAD = DEG2RAD / 3600.0;

static constexpr double TAU = std::numbers::pi * 2.0;



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
        inline glm::dvec3 getPrecessionAngles(double julianDate) {
            // Julian centuries past J2000.0 (J2000.0 is Julian Date 2451545.0)
            double t = (julianDate - 2451545.0) / 36525.0;

            double zeta = evalPolynomial(m_zetaPolyCfs, t);
            double theta = evalPolynomial(m_thetaPolyCfs, t);
            double zed = evalPolynomial(m_zedPolyCfs, t);

            return glm::dvec3(zeta, theta, zed);
        }


        /* Computes the time-dependent nutation angles for a given epoch. */
        inline PhysicsComponent::NutationAngles getNutationAngles(double julianDate, double julianDateUTC) {
            double t = (julianDate - 2451545.0) / 36525.0;

            double moonAnom = evalPolynomial(m_moonAnomPolyCfs, t);
            double sunAnom = evalPolynomial(m_sunAnomPolyCfs, t);
            double moonLat = evalPolynomial(m_moonLatPolyCfs, t);
            double sunElong = evalPolynomial(m_sunElongPolyCfs, t);
            double moonRaan = evalPolynomial(m_moonRaanPolyCfs, t);

            double deltaPsi = 0.0;
            double deltaEpsilon = 0.0;

            for (const auto &coeff : IAU1980::Coefficients) {
                const double arg =
                    coeff.a1 * moonAnom +
                    coeff.a2 * sunAnom +
                    coeff.a3 * moonLat +
                    coeff.a4 * sunElong +
                    coeff.a5 * moonRaan;

                const double sinC = coeff.ai + coeff.bi * t;
                const double cosC = coeff.ci + coeff.di * t;

                deltaPsi += sinC * sin(arg);
                deltaEpsilon += cosC * cos(arg);
            }

            deltaPsi *= ASEC2RAD;
            deltaEpsilon *= ASEC2RAD;

            double meanEpsilon = evalPolynomial(m_meanEpsilonilonPolyCfs, t);
            double epsilon = meanEpsilon + deltaEpsilon;

            // NOTE: The `eqEq` calculation from Earth.ts requires `moonRaan` to be in radians
            double eqEquinoxes = deltaPsi * cos(meanEpsilon) + 0.00264 * ASEC2RAD * sin(moonRaan) + 0.000063 * ASEC2RAD * sin(2.0 * moonRaan);

            // Greenwich Apparent Sidereal Time (GAST)
            //double greenwichSiderealTime = (6.6974243245 + 1.000021359 * t) * DEG2RAD + eqEquinoxes;
            double gast = gstime(julianDateUTC);

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
        /* Evaluates a polynomial function for a given input.
            @param cfs: Coefficients of the polynomial: [constant, linear, quadratic, cubic].
            @param input: The input value.

            @return The evaluated polynomial value.
        */
        static double evalPolynomial(const std::vector<double> &cfs, double input) {
            if (cfs.size() == 0)
                return 0.0;

            double val = 0.0;
            for (size_t i = 0; i < cfs.size(); i++)
                val = val * input + cfs[i];

            return val;
        }


        // ----- EARTH PRECESSION POLYNOMIAL COEFFICIENTS -----
        inline static const std::vector<double> m_zetaPolyCfs = {
            0.017998 * ASEC2RAD,
            0.30188 * ASEC2RAD,
            2306.2181 * ASEC2RAD,
            0.0,
        };

        inline static const std::vector<double> m_thetaPolyCfs = {
            -0.041833 * ASEC2RAD,
            -0.42665 * ASEC2RAD,
            2004.3109 * ASEC2RAD,
            0.0,
        };

        inline static const std::vector<double> m_zedPolyCfs = {
            0.018203 * ASEC2RAD,
            1.09468 * ASEC2RAD,
            2306.2181 * ASEC2RAD,
            0,
        };


        // ----- OTHER POLYNOMIAL COEFFICIENTS -----
        inline static const std::vector<double> m_moonAnomPolyCfs = {
            1.4343e-5 * DEG2RAD,
            0.0088553 * DEG2RAD,
            (1325.0 * 360.0 + 198.8675605) * DEG2RAD,
            134.96340251 * DEG2RAD,
        };

        inline static const std::vector<double> m_sunAnomPolyCfs = {
            3.8e-8 * DEG2RAD,
            -0.0001537 * DEG2RAD,
            (99.0 * 360.0 + 359.0502911) * DEG2RAD,
            357.52910918 * DEG2RAD,
        };

        inline static const std::vector<double> m_moonLatPolyCfs = {
            -2.88e-7 * DEG2RAD,
            -0.003542 * DEG2RAD,
            (1342.0 * 360.0 + 82.0174577) * DEG2RAD,
            93.27209062 * DEG2RAD,
        };

        inline static const std::vector<double> m_sunElongPolyCfs = {
            -1.55e-7 * DEG2RAD,
            -0.0003022 * DEG2RAD,
            (1236.0 * 360.0 + 297.8503632) * DEG2RAD,
            297.8503632 * DEG2RAD,
        };

        inline static const std::vector<double> m_moonRaanPolyCfs = {
            2.139e-6 * DEG2RAD,
            0.0020756 * DEG2RAD,
            -(5.0 * 360.0 + 134.1361851) * DEG2RAD,
            125.04455501 * DEG2RAD,
        };

        inline static const std::vector<double> m_meanEpsilonilonPolyCfs = {
        0.001813 * ASEC2RAD,
        -0.00059 * ASEC2RAD,
        -46.815 * ASEC2RAD,
        84381.448 * ASEC2RAD,
        };
    };
}