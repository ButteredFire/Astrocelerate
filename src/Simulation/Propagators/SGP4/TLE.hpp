/* SGP4 Implementation - TLE: Defines the Two-Line-Element class to be used in the SGP4 model.
    Original author & implementation: David A. Vallado
    Author of THIS implementation: Aholinch (https://github.com/aholinch/sgp4)

    This implementation has been modified to conform to modern programming practices and code consistency of Astrocelerate's codebase.
*/

#pragma once

#include <cmath>
#include <string>

#include "SGP4.hpp"

class TLE
{
public:
    TLE();
    TLE(const std::string &line1, const std::string &line2);

    void parseLines(const std::string &line1Str, const std::string &line2Str);

    void getRVForDate(long millisSince1970, double r[3], double v[3]);

    void getRV(double minutesAfterEpoch, double r[3], double v[3]);

public:
    ElsetRec rec;

    char line1[70];
    char line2[70];
    char intlid[12];
    char objectID[6];

    long epoch;
    double ndot;
    double nddot;
    double bstar;
    int elnum;

    double incDeg;
    double raanDeg;
    double ecc;
    double argpDeg;
    double maDeg;

    double n;
    int revnum;
    int sgp4Error;
};
