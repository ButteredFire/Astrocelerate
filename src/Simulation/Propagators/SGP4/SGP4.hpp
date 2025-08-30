/* SGP4 Implementation - SGP4: Defines the SGP4 class.
    Original author & implementation: David A. Vallado
    Author of THIS implementation: Aholinch (https://github.com/aholinch/sgp4)

    This implementation has been modified to conform to modern programming practices and code consistency of Astrocelerate's codebase.
*/

#pragma once

#include <stdio.h>
#include <cmath>

inline constexpr int wgs72old = 1;
inline constexpr int wgs72 = 2;
inline constexpr int wgs84 = 3;
inline constexpr double pi = 3.14159265358979323846;

inline constexpr double twopi = 2.0 / pi;

#define deg2rad  (pi/180.0)


struct ElsetRec {
    int whichconst;
    char satid[6];
    int epochyr;
    int epochtynumrev;
    int error;
    char operationmode;
    char init;
    char method;    
    double a;
    double altp;
    double alta;
    double epochdays;
    double jdsatepoch;
    double jdsatepochF;
    double nddot;
    double ndot;
    double bstar;
    double rcse;
    double inclo;
    double nodeo;
    double ecco;
    double argpo;
    double mo;
    double no_kozai;
    
    // sgp4fix add new variables from tle
    char classification;
    char intldesg[12];
    int ephtype;
    long elnum;
    long revnum;
    
    // sgp4fix add unkozai'd variable
    double no_unkozai;
    
    // sgp4fix add singly averaged variables
    double am;
    double em;
    double im;
    double Om;
    double om;
    double mm;
    double nm;
    double t;
    
    // sgp4fix add constant parameters to eliminate mutliple calls during execution
    double tumin;
    double mu;
    double radiusearthkm;
    double xke; 
    double j2;
    double j3;
    double j4;
    double j3oj2;
    
    //       Additional elements to capture relevant TLE and object information:       
    long dia_mm; // RSO dia in mm
    double period_sec; // Period in seconds
    char active; // "Active S/C" flag (0=n, 1=y) 
    char not_orbital; // "Orbiting S/C" flag (0=n, 1=y)  
    double rcs_m2; // "RCS (m^2)" storage  

    // temporary variables because the original authors call the same method with different variables
    double ep;
    double inclp;
    double nodep;
    double argpp;
    double mp;


    int isimp;
    double aycof;
    double con41;
    double cc1;
    double cc4;
    double cc5;
    double d2;
    double d3;
    double d4;
    double delmo;
    double eta;
    double argpdot;
    double omgcof;
    double sinmao;
    double t2cof;
    double t3cof;
    double t4cof;
    double t5cof;
    double x1mth2;
    double x7thm1;
    double mdot;
    double nodedot;
    double xlcof;
    double xmcof;
    double nodecf;

    // deep space
    int irez;
    double d2201;
    double d2211;
    double d3210;
    double d3222;
    double d4410;
    double d4422;
    double d5220;
    double d5232;
    double d5421;
    double d5433;
    double dedt;
    double del1;
    double del2;
    double del3;
    double didt;
    double dmdt;
    double dnodt;
    double domdt;
    double e3;
    double ee2;
    double peo;
    double pgho;
    double pho;
    double pinco;
    double plo;
    double se2;
    double se3;
    double sgh2;
    double sgh3;
    double sgh4;
    double sh2;
    double sh3;
    double si2;
    double si3;
    double sl2;
    double sl3;
    double sl4;
    double gsto;
    double xfact;
    double xgh2;
    double xgh3;
    double xgh4;
    double xh2;
    double xh3;
    double xi2;
    double xi3;
    double xl2;
    double xl3;
    double xl4;
    double xlamo;
    double zmol;
    double zmos;
    double atime;
    double xli;
    double xni;
    double snodm;
    double cnodm;
    double sinim;
    double cosim;
    double sinomm;
    double cosomm;
    double day;
    double emsq;
    double gam;
    double rtemsq; 
    double s1;
    double s2;
    double s3;
    double s4;
    double s5;
    double s6;
    double s7; 
    double ss1;
    double ss2;
    double ss3;
    double ss4;
    double ss5;
    double ss6;
    double ss7;
    double sz1;
    double sz2;
    double sz3;
    double sz11;
    double sz12;
    double sz13;
    double sz21;
    double sz22;
    double sz23;
    double sz31;
    double sz32;
    double sz33;
    double z1;
    double z2;
    double z3;
    double z11;
    double z12;
    double z13;
    double z21;
    double z22;
    double z23;
    double z31;
    double z32;
    double z33;
    double argpm;
    double inclm;
    double nodem;
    double dndt;
    double eccsq;
        
    // for initl
    double ainv;
    double ao;
    double con42;
    double cosio;
    double cosio2;
    double omeosq;
    double posq;
    double rp;
    double rteosq;
    double sinio;
};


/* SUMMARY: Provides deep space contributions to mean motion dot due to geopotential resonance with half day and one day orbits. *//* -----------------------------------------------------------------------------
    *
    *                           procedure dpper
    *
    *  this procedure provides deep space long period periodic contributions
    *    to the mean elements.  by design, these periodics are zero at epoch.
    *    this used to be dscom which included initialization, but it's really a
    *    recurring function.
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    e3          -
    *    ee2         -
    *    peo         -
    *    pgho        -
    *    pho         -
    *    pinco       -
    *    plo         -
    *    se2 , se3 , sgh2, sgh3, sgh4, sh2, sh3, si2, si3, sl2, sl3, sl4 -
    *    t           -
    *    xh2, xh3, xi2, xi3, xl2, xl3, xl4 -
    *    zmol        -
    *    zmos        -
    *    ep          - eccentricity                           0.0 - 1.0
    *    inclo       - inclination - needed for lyddane modification
    *    nodep       - right ascension of ascending node
    *    argpp       - argument of perigee
    *    mp          - mean anomaly
    *
    *  outputs       :
    *    ep          - eccentricity                           0.0 - 1.0
    *    inclp       - inclination
    *    nodep        - right ascension of ascending node
    *    argpp       - argument of perigee
    *    mp          - mean anomaly
    *
    *  locals        :
    *    alfdp       -
    *    betdp       -
    *    cosip  , sinip  , cosop  , sinop  ,
    *    dalf        -
    *    dbet        -
    *    dls         -
    *    f2, f3      -
    *    pe          -
    *    pgh         -
    *    ph          -
    *    pinc        -
    *    pl          -
    *    sel   , ses   , sghl  , sghs  , shl   , shs   , sil   , sinzf , sis   ,
    *    sll   , sls
    *    xls         -
    *    xnoh        -
    *    zf          -
    *    zm          -
    *
    *  coupling      :
    *    none.
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
void dpper(double e3, double ee2, double peo, double pgho, double pho,
           double pinco, double plo, double se2, double se3, double sgh2,
           double sgh3, double sgh4, double sh2, double sh3, double si2,
           double si3, double sl2, double sl3, double sl4, double t,
           double xgh2, double xgh3, double xgh4, double xh2, double xh3,
           double xi2, double xi3, double xl2, double xl3, double xl4,
           double zmol, double zmos, char init,
           ElsetRec *rec, char opsmode);


/* SUMMARY: Provides deep space common items used by both the secular and periodics subroutines. */
    /*-----------------------------------------------------------------------------
    *
    *                           procedure dscom
    *
    *  this procedure provides deep space common items used by both the secular
    *    and periodics subroutines.  input is provided as shown. this routine
    *    used to be called dpper, but the functions inside weren't well organized.
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    epoch       -
    *    ep          - eccentricity
    *    argpp       - argument of perigee
    *    tc          -
    *    inclp       - inclination
    *    nodep       - right ascension of ascending node
    *    np          - mean motion
    *
    *  outputs       :
    *    sinim  , cosim  , sinomm , cosomm , snodm  , cnodm
    *    day         -
    *    e3          -
    *    ee2         -
    *    em          - eccentricity
    *    emsq        - eccentricity squared
    *    gam         -
    *    peo         -
    *    pgho        -
    *    pho         -
    *    pinco       -
    *    plo         -
    *    rtemsq      -
    *    se2, se3         -
    *    sgh2, sgh3, sgh4        -
    *    sh2, sh3, si2, si3, sl2, sl3, sl4         -
    *    s1, s2, s3, s4, s5, s6, s7          -
    *    ss1, ss2, ss3, ss4, ss5, ss6, ss7, sz1, sz2, sz3         -
    *    sz11, sz12, sz13, sz21, sz22, sz23, sz31, sz32, sz33        -
    *    xgh2, xgh3, xgh4, xh2, xh3, xi2, xi3, xl2, xl3, xl4         -
    *    nm          - mean motion
    *    z1, z2, z3, z11, z12, z13, z21, z22, z23, z31, z32, z33         -
    *    zmol        -
    *    zmos        -
    *
    *  locals        :
    *    a1, a2, a3, a4, a5, a6, a7, a8, a9, a10         -
    *    betasq      -
    *    cc          -
    *    ctem, stem        -
    *    x1, x2, x3, x4, x5, x6, x7, x8          -
    *    xnodce      -
    *    xnoi        -
    *    zcosg  , zsing  , zcosgl , zsingl , zcosh  , zsinh  , zcoshl , zsinhl ,
    *    zcosi  , zsini  , zcosil , zsinil ,
    *    zx          -
    *    zy          -
    *
    *  coupling      :
    *    none.
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
void dscom(double epoch, double ep, double argpp, double tc, double inclp,
           double nodep, double np, ElsetRec *rec);


/* SUMMARY: Initializes the SGP4 model with the given operation mode and satellite record. */
/*-----------------------------------------------------------------------------
    *
    *                           procedure dsinit
    *
    *  this procedure provides deep space contributions to mean motion dot due
    *    to geopotential resonance with half day and one day orbits.
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    xke         - reciprocal of tumin
    *    cosim, sinim-
    *    emsq        - eccentricity squared
    *    argpo       - argument of perigee
    *    s1, s2, s3, s4, s5      -
    *    ss1, ss2, ss3, ss4, ss5 -
    *    sz1, sz3, sz11, sz13, sz21, sz23, sz31, sz33 -
    *    t           - time
    *    tc          -
    *    gsto        - greenwich sidereal time                   rad
    *    mo          - mean anomaly
    *    mdot        - mean anomaly dot (rate)
    *    no          - mean motion
    *    nodeo       - right ascension of ascending node
    *    nodedot     - right ascension of ascending node dot (rate)
    *    xpidot      -
    *    z1, z3, z11, z13, z21, z23, z31, z33 -
    *    eccm        - eccentricity
    *    argpm       - argument of perigee
    *    inclm       - inclination
    *    mm          - mean anomaly
    *    xn          - mean motion
    *    nodem       - right ascension of ascending node
    *
    *  outputs       :
    *    em          - eccentricity
    *    argpm       - argument of perigee
    *    inclm       - inclination
    *    mm          - mean anomaly
    *    nm          - mean motion
    *    nodem       - right ascension of ascending node
    *    irez        - flag for resonance           0-none, 1-one day, 2-half day
    *    atime       -
    *    d2201, d2211, d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433    -
    *    dedt        -
    *    didt        -
    *    dmdt        -
    *    dndt        -
    *    dnodt       -
    *    domdt       -
    *    del1, del2, del3        -
    *    ses  , sghl , sghs , sgs  , shl  , shs  , sis  , sls
    *    theta       -
    *    xfact       -
    *    xlamo       -
    *    xli         -
    *    xni
    *
    *  locals        :
    *    ainv2       -
    *    aonv        -
    *    cosisq      -
    *    eoc         -
    *    f220, f221, f311, f321, f322, f330, f441, f442, f522, f523, f542, f543  -
    *    g200, g201, g211, g300, g310, g322, g410, g422, g520, g521, g532, g533  -
    *    sini2       -
    *    temp        -
    *    temp1       -
    *    theta       -
    *    xno2        -
    *
    *  coupling      :
    *    getgravconst- no longer used
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
void dsinit(double tc, double xpidot, ElsetRec *rec);


/* SUMMARY: Provides deep space long period periodic contributions to the mean elements. */
/*-----------------------------------------------------------------------------
    *
    *                           procedure dspace
    *
    *  this procedure provides deep space contributions to mean elements for
    *    perturbing third body.  these effects have been averaged over one
    *    revolution of the sun and moon.  for earth resonance effects, the
    *    effects have been averaged over no revolutions of the satellite.
    *    (mean motion)
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    d2201, d2211, d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433 -
    *    dedt        -
    *    del1, del2, del3  -
    *    didt        -
    *    dmdt        -
    *    dnodt       -
    *    domdt       -
    *    irez        - flag for resonance           0-none, 1-one day, 2-half day
    *    argpo       - argument of perigee
    *    argpdot     - argument of perigee dot (rate)
    *    t           - time
    *    tc          -
    *    gsto        - gst
    *    xfact       -
    *    xlamo       -
    *    no          - mean motion
    *    atime       -
    *    em          - eccentricity
    *    ft          -
    *    argpm       - argument of perigee
    *    inclm       - inclination
    *    xli         -
    *    mm          - mean anomaly
    *    xni         - mean motion
    *    nodem       - right ascension of ascending node
    *
    *  outputs       :
    *    atime       -
    *    em          - eccentricity
    *    argpm       - argument of perigee
    *    inclm       - inclination
    *    xli         -
    *    mm          - mean anomaly
    *    xni         -
    *    nodem       - right ascension of ascending node
    *    dndt        -
    *    nm          - mean motion
    *
    *  locals        :
    *    delt        -
    *    ft          -
    *    theta       -
    *    x2li        -
    *    x2omi       -
    *    xl          -
    *    xldot       -
    *    xnddt       -
    *    xndt        -
    *    xomi        -
    *
    *  coupling      :
    *    none        -
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
void dspace(double tc, ElsetRec *rec);


/* SUMMARY: Initializes the SGP4 model with the given epoch and satellite record. */
/*-----------------------------------------------------------------------------
    *
    *                           procedure initl
    *
    *  this procedure initializes the spg4 propagator. all the initialization is
    *    consolidated here instead of having multiple loops inside other routines.
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    satn        - satellite number - not needed, placed in satrec
    *    xke         - reciprocal of tumin
    *    j2          - j2 zonal harmonic
    *    ecco        - eccentricity                           0.0 - 1.0
    *    epoch       - epoch time in days from jan 0, 1950. 0 hr
    *    inclo       - inclination of satellite
    *    no          - mean motion of satellite
    *
    *  outputs       :
    *    ainv        - 1.0 / a
    *    ao          - semi major axis
    *    con41       -
    *    con42       - 1.0 - 5.0 cos(i)
    *    cosio       - cosine of inclination
    *    cosio2      - cosio squared
    *    eccsq       - eccentricity squared
    *    method      - flag for deep space                    'd', 'n'
    *    omeosq      - 1.0 - ecco * ecco
    *    posq        - semi-parameter squared
    *    rp          - radius of perigee
    *    rteosq      - square root of (1.0 - ecco*ecco)
    *    sinio       - sine of inclination
    *    gsto        - gst at time of observation               rad
    *    no          - mean motion of satellite
    *
    *  locals        :
    *    ak          -
    *    d1          -
    *    del         -
    *    adel        -
    *    po          -
    *
    *  coupling      :
    *    getgravconst- no longer used
    *    gstime      - find greenwich sidereal time from the julian date
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
void initl(double epoch, ElsetRec *rec);


/* SUMMARY: Initializes the SGP4 model with the given operation mode and satellite record. */
/*-----------------------------------------------------------------------------
    *
    *                             procedure sgp4init
    *
    *  this procedure initializes variables for sgp4.
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    opsmode     - mode of operation afspc or improved 'a', 'i'
    *    whichconst  - which set of constants to use  72, 84
    *    satn        - satellite number
    *    bstar       - sgp4 type drag coefficient              kg/m2er
    *    ecco        - eccentricity
    *    epoch       - epoch time in days from jan 0, 1950. 0 hr
    *    argpo       - argument of perigee (output if ds)
    *    inclo       - inclination
    *    mo          - mean anomaly (output if ds)
    *    no          - mean motion
    *    nodeo       - right ascension of ascending node
    *
    *  outputs       :
    *    satrec      - common values for subsequent calls
    *    return code - non-zero on error.
    *                   1 - mean elements, ecc >= 1.0 or ecc < -0.001 or a < 0.95 er
    *                   2 - mean motion less than 0.0
    *                   3 - pert elements, ecc < 0.0  or  ecc > 1.0
    *                   4 - semi-latus rectum < 0.0
    *                   5 - epoch elements are sub-orbital
    *                   6 - satellite has decayed
    *
    *  locals        :
    *    cnodm  , snodm  , cosim  , sinim  , cosomm , sinomm
    *    cc1sq  , cc2    , cc3
    *    coef   , coef1
    *    cosio4      -
    *    day         -
    *    dndt        -
    *    em          - eccentricity
    *    emsq        - eccentricity squared
    *    eeta        -
    *    etasq       -
    *    gam         -
    *    argpm       - argument of perigee
    *    nodem       -
    *    inclm       - inclination
    *    mm          - mean anomaly
    *    nm          - mean motion
    *    perige      - perigee
    *    pinvsq      -
    *    psisq       -
    *    qzms24      -
    *    rtemsq      -
    *    s1, s2, s3, s4, s5, s6, s7          -
    *    sfour       -
    *    ss1, ss2, ss3, ss4, ss5, ss6, ss7         -
    *    sz1, sz2, sz3
    *    sz11, sz12, sz13, sz21, sz22, sz23, sz31, sz32, sz33        -
    *    tc          -
    *    temp        -
    *    temp1, temp2, temp3       -
    *    tsi         -
    *    xpidot      -
    *    xhdot1      -
    *    z1, z2, z3          -
    *    z11, z12, z13, z21, z22, z23, z31, z32, z33         -
    *
    *  coupling      :
    *    getgravconst-
    *    initl       -
    *    dscom       -
    *    dpper       -
    *    dsinit      -
    *    sgp4        -
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
bool sgp4init(char opsmode, ElsetRec *satrec);


/* SUMMARY: Propagates the satellite state vector using the SGP4 model for a given time since epoch. */
/*-----------------------------------------------------------------------------
    *
    *                             procedure sgp4
    *
    *  this procedure is the sgp4 prediction model from space command. this is an
    *    updated and combined version of sgp4 and sdp4, which were originally
    *    published separately in spacetrack report #3. this version follows the
    *    methodology from the aiaa paper (2006) describing the history and
    *    development of the code.
    *
    *  author        : david vallado                  719-573-2600   28 jun 2005
    *
    *  inputs        :
    *    satrec     - initialised structure from sgp4init() call.
    *    tsince     - time since epoch (minutes)
    *
    *  outputs       :
    *    r           - position vector                     km
    *    v           - velocity                            km/sec
    *  return code - non-zero on error.
    *                   1 - mean elements, ecc >= 1.0 or ecc < -0.001 or a < 0.95 er
    *                   2 - mean motion less than 0.0
    *                   3 - pert elements, ecc < 0.0  or  ecc > 1.0
    *                   4 - semi-latus rectum < 0.0
    *                   5 - epoch elements are sub-orbital
    *                   6 - satellite has decayed
    *
    *  locals        :
    *    am          -
    *    axnl, aynl        -
    *    betal       -
    *    cosim   , sinim   , cosomm  , sinomm  , cnod    , snod    , cos2u   ,
    *    sin2u   , coseo1  , sineo1  , cosi    , sini    , cosip   , sinip   ,
    *    cosisq  , cossu   , sinsu   , cosu    , sinu
    *    delm        -
    *    delomg      -
    *    dndt        -
    *    eccm        -
    *    emsq        -
    *    ecose       -
    *    el2         -
    *    eo1         -
    *    eccp        -
    *    esine       -
    *    argpm       -
    *    argpp       -
    *    omgadf      -c
    *    pl          -
    *    r           -
    *    rtemsq      -
    *    rdotl       -
    *    rl          -
    *    rvdot       -
    *    rvdotl      -
    *    su          -
    *    t2  , t3   , t4    , tc
    *    tem5, temp , temp1 , temp2  , tempa  , tempe  , templ
    *    u   , ux   , uy    , uz     , vx     , vy     , vz
    *    inclm       - inclination
    *    mm          - mean anomaly
    *    nm          - mean motion
    *    nodem       - right asc of ascending node
    *    xinc        -
    *    xincp       -
    *    xl          -
    *    xlm         -
    *    mp          -
    *    xmdf        -
    *    xmx         -
    *    xmy         -
    *    nodedf      -
    *    xnode       -
    *    nodep       -
    *    np          -
    *
    *  coupling      :
    *    getgravconst- no longer used. Variables are conatined within satrec
    *    dpper
    *    dpspace
    *
    *  references    :
    *    hoots, roehrich, norad spacetrack report #3 1980
    *    hoots, norad spacetrack report #6 1986
    *    hoots, schumacher and glover 2004
    *    vallado, crawford, hujsak, kelso  2006
    ----------------------------------------------------------------------------*/
bool sgp4(ElsetRec *satrec, double tsince, double *r, double *v);


/* -----------------------------------------------------------------------------
    *
    *                           function getgravconst
    *
    *  this function gets constants for the propagator. note that mu is identified to
    *    facilitiate comparisons with newer models. the common useage is wgs72.
    *
    *  author        : david vallado                  719-573-2600   21 jul 2006
    *
    *  inputs        :
    *    whichconst  - which set of constants to use  wgs72old, wgs72, wgs84
    *
    *  outputs       :
    *    tumin       - minutes in one time unit
    *    mu          - earth gravitational parameter
    *    radiusearthkm - radius of the earth in km
    *    xke         - reciprocal of tumin
    *    j2, j3, j4  - un-normalized zonal harmonic values
    *    j3oj2       - j3 divided by j2
    *
    *  locals        :
    *
    *  coupling      :
    *    none
    *
    *  references    :
    *    norad spacetrack report #3
    *    vallado, crawford, hujsak, kelso  2006
    --------------------------------------------------------------------------- */
void getgravconst(int whichconst, ElsetRec *rec);


/* -----------------------------------------------------------------------------
    *
    *                           function gstime
    *
    *  this function finds the greenwich sidereal time.
    *
    *  author        : david vallado                  719-573-2600    1 mar 2001
    *
    *  inputs          description                    range / units
    *    jdut1       - julian date in ut1             days from 4713 bc
    *
    *  outputs       :
    *    gstime      - greenwich sidereal time        0 to 2pi rad
    *
    *  locals        :
    *    temp        - temporary variable for doubles   rad
    *    tut1        - julian centuries from the
    *                  jan 1, 2000 12 h epoch (ut1)
    *
    *  coupling      :
    *    none
    *
    *  references    :
    *    vallado       2013, 187, eq 3-45
    * --------------------------------------------------------------------------- */
double gstime(double jdut1);


/* -----------------------------------------------------------------------------
    *
    *                           procedure jday
    *
    *  this procedure finds the julian date given the year, month, day, and time.
    *    the julian date is defined by each elapsed day since noon, jan 1, 4713 bc.
    *
    *  algorithm     : calculate the answer in one step for efficiency
    *
    *  author        : david vallado                  719-573-2600    1 mar 2001
    *
    *  inputs          description                    range / units
    *    year        - year                           1900 .. 2100
    *    mon         - month                          1 .. 12
    *    day         - day                            1 .. 28,29,30,31
    *    hr          - universal time hour            0 .. 23
    *    min         - universal time min             0 .. 59
    *    sec         - universal time sec             0.0 .. 59.999
    *
    *  outputs       :
    *    jd          - julian date                    days from 4713 bc
    *    jdfrac      - julian date fraction into day  days from 4713 bc
    *
    *  locals        :
    *    none.
    *
    *  coupling      :
    *    none.
    *
    *  references    :
    *    vallado       2013, 183, alg 14, ex 3-4
    * --------------------------------------------------------------------------- */
void jday(int year, int mon, int day, int hr, int minute, double sec, double *jd, double *jdfrac);
