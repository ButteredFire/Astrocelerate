#include "SGP4.hpp"


/*     ----------------------------------------------------------------
*    this file contains the sgp4 procedures for analytical propagation
*    of a satellite. the code was originally released in the 1980 and 1986
*    spacetrack papers. a detailed discussion of the theory and history
*    may be found in the 2006 aiaa paper by vallado, crawford, hujsak,
*    and kelso.
*
*                            companion code for
*               fundamentals of astrodynamics and applications
*                                    2013
*                              by david vallado
*
*     (w) 719-573-2600, email dvallado@agi.com, davallado@gmail.com
*
*    current :
*               7 dec 15  david vallado
*                           fix jd, jdfrac
*    changes :
*               3 nov 14  david vallado
*                           update to msvs2013 c++
*              30 aug 10  david vallado
*                           delete unused variables in initl
*                           replace pow integer 2, 3 with multiplies for speed
*               3 nov 08  david vallado
*                           put returns in for error codes
*              29 sep 08  david vallado
*                           fix atime for faster operation in dspace
*                           add operationmode for afspc (a) or improved (i)
*                           performance mode
*              16 jun 08  david vallado
*                           update small eccentricity check
*              16 nov 07  david vallado
*                           misc fixes for better compliance
*              20 apr 07  david vallado
*                           misc fixes for constants
*              11 aug 06  david vallado
*                           chg lyddane choice back to strn3, constants, misc doc
*              15 dec 05  david vallado
*                           misc fixes
*              26 jul 05  david vallado
*                           fixes for paper
*                           note that each fix is preceded by a
*                           comment with "sgp4fix" and an explanation of
*                           what was changed
*              10 aug 04  david vallado
*                           2nd printing baseline working
*              14 may 01  david vallado
*                           2nd edition baseline
*                     80  norad
*                           original baseline
*       ----------------------------------------------------------------      */



void dpper
    (
    double e3, double ee2, double peo, double pgho, double pho,
    double pinco, double plo, double se2, double se3, double sgh2,
    double sgh3, double sgh4, double sh2, double sh3, double si2,
    double si3, double sl2, double sl3, double sl4, double t,
    double xgh2, double xgh3, double xgh4, double xh2, double xh3,
    double xi2, double xi3, double xl2, double xl3, double xl4,
    double zmol, double zmos,
    char init,
    ElsetRec *rec,
    char opsmode
    ) {
    /* --------------------- local variables ------------------------ */
    double alfdp, betdp, cosip, cosop, dalf, dbet, dls,
        f2, f3, pe, pgh, ph, pinc, pl,
        sel, ses, sghl, sghs, shll, shs, sil,
        sinip, sinop, sinzf, sis, sll, sls, xls,
        xnoh, zf, zm, zel, zes, znl, zns;

        
    /* ---------------------- constants ----------------------------- */
    zns = 1.19459e-5;
    zes = 0.01675;
    znl = 1.5835218e-4;
    zel = 0.05490;

    /* --------------- calculate time varying periodics ----------- */
    zm = zmos + zns * t;
    // be sure that the initial call has time set to zero
    if (init == 'y')
        zm = zmos;
    zf = zm + 2.0 * zes * sin(zm);
    sinzf = sin(zf);
    f2 = 0.5 * sinzf * sinzf - 0.25;
    f3 = -0.5 * sinzf * cos(zf);
    ses = se2* f2 + se3 * f3;
    sis = si2 * f2 + si3 * f3;
    sls = sl2 * f2 + sl3 * f3 + sl4 * sinzf;
    sghs = sgh2 * f2 + sgh3 * f3 + sgh4 * sinzf;
    shs = sh2 * f2 + sh3 * f3;
    zm = zmol + znl * t;
    if (init == 'y')
        zm = zmol;
    zf = zm + 2.0 * zel * sin(zm);
    sinzf = sin(zf);
    f2 = 0.5 * sinzf * sinzf - 0.25;
    f3 = -0.5 * sinzf * cos(zf);
    sel = ee2 * f2 + e3 * f3;
    sil = xi2 * f2 + xi3 * f3;
    sll = xl2 * f2 + xl3 * f3 + xl4 * sinzf;
    sghl = xgh2 * f2 + xgh3 * f3 + xgh4 * sinzf;
    shll = xh2 * f2 + xh3 * f3;
    pe = ses + sel;
    pinc = sis + sil;
    pl = sls + sll;
    pgh = sghs + sghl;
    ph = shs + shll;

    if (init == 'n')
    {
        pe = pe - peo;
        pinc = pinc - pinco;
        pl = pl - plo;
        pgh = pgh - pgho;
        ph = ph - pho;
        rec->inclp = rec->inclp + pinc;
        rec->ep = rec->ep + pe;
        sinip = sin(rec->inclp);
        cosip = cos(rec->inclp);

        /* ----------------- apply periodics directly ------------ */
        //  sgp4fix for lyddane choice
        //  strn3 used original inclination - this is technically feasible
        //  gsfc used perturbed inclination - also technically feasible
        //  probably best to readjust the 0.2 limit value and limit discontinuity
        //  0.2 rad = 11.45916 deg
        //  use next line for original strn3 approach and original inclination
        //  if (inclo >= 0.2)
        //  use next line for gsfc version and perturbed inclination
        if (rec->inclp >= 0.2)
        {
            ph = ph / sinip;
            pgh = pgh - cosip * ph;
            rec->argpp = rec->argpp + pgh;
            rec->nodep = rec->nodep + ph;
            rec->mp = rec->mp + pl;
        }
        else
        {
            /* ---- apply periodics with lyddane modification ---- */
            sinop = sin(rec->nodep);
            cosop = cos(rec->nodep);
            alfdp = sinip * sinop;
            betdp = sinip * cosop;
            dalf = ph * cosop + pinc * cosip * sinop;
            dbet = -ph * sinop + pinc * cosip * cosop;
            alfdp = alfdp + dalf;
            betdp = betdp + dbet;
            rec->nodep = fmod(rec->nodep, twopi);
            //  sgp4fix for afspc written intrinsic functions
            // nodep used without a trigonometric function ahead
            if ((rec->nodep < 0.0) && (opsmode == 'a'))
                rec->nodep = rec->nodep + twopi;
            xls = rec->mp + rec->argpp + cosip * rec->nodep;
            dls = pl + pgh - pinc * rec->nodep * sinip;
            xls = xls + dls;
            xls = fmod(xls,twopi);
            xnoh = rec->nodep;
            rec->nodep = atan2(alfdp, betdp);
            //  sgp4fix for afspc written intrinsic functions
            // nodep used without a trigonometric function ahead
            if ((rec->nodep < 0.0) && (opsmode == 'a'))
                rec->nodep = rec->nodep + twopi;
            if (fabs(xnoh - rec->nodep) > pi)
                if (rec->nodep < xnoh)
                    rec->nodep = rec->nodep + twopi;
                else
                    rec->nodep = rec->nodep - twopi;
            rec->mp = rec->mp + pl;
            rec->argpp = xls - rec->mp - cosip * rec->nodep;
        }
    }   // if init == 'n'

}  // dpper


void dscom
    (
    double epoch, double ep, double argpp, double tc, double inclp,
    double nodep, double np,
    ElsetRec *rec
    ) {
    /* -------------------------- constants ------------------------- */
    const double zes = 0.01675;
    const double zel = 0.05490;
    const double c1ss = 2.9864797e-6;
    const double c1l = 4.7968065e-7;
    const double zsinis = 0.39785416;
    const double zcosis = 0.91744867;
    const double zcosgs = 0.1945905;
    const double zsings = -0.98088458;

    /* --------------------- local variables ------------------------ */
    int lsflg;
    double a1, a2, a3, a4, a5, a6, a7,
        a8, a9, a10, betasq, cc, ctem, stem,
        x1, x2, x3, x4, x5, x6, x7,
        x8, xnodce, xnoi, zcosg, zcosgl, zcosh, zcoshl,
        zcosi, zcosil, zsing, zsingl, zsinh, zsinhl, zsini,
        zsinil, zx, zy;

    rec->nm = np;
    rec->em = ep;
    rec->snodm = sin(nodep);
    rec->cnodm = cos(nodep);
    rec->sinomm = sin(argpp);
    rec->cosomm = cos(argpp);
    rec->sinim = sin(inclp);
    rec->cosim = cos(inclp);
    rec->emsq = rec->em * rec->em;
    betasq = 1.0 - rec->emsq;
    rec->rtemsq = sqrt(betasq);

    /* ----------------- initialize lunar solar terms --------------- */
    rec->peo = 0.0;
    rec->pinco = 0.0;
    rec->plo = 0.0;
    rec->pgho = 0.0;
    rec->pho = 0.0;
    rec->day = epoch + 18261.5 + tc / 1440.0;
    xnodce = fmod(4.5236020 - 9.2422029e-4 * rec->day, twopi);
    stem = sin(xnodce);
    ctem = cos(xnodce);
    zcosil = 0.91375164 - 0.03568096 * ctem;
    zsinil = sqrt(1.0 - zcosil * zcosil);
    zsinhl = 0.089683511 * stem / zsinil;
    zcoshl = sqrt(1.0 - zsinhl * zsinhl);
    rec->gam = 5.8351514 + 0.0019443680 * rec->day;
    zx = 0.39785416 * stem / zsinil;
    zy = zcoshl * ctem + 0.91744867 * zsinhl * stem;
    zx = atan2(zx, zy);
    zx = rec->gam + zx - xnodce;
    zcosgl = cos(zx);
    zsingl = sin(zx);

    /* ------------------------- do solar terms --------------------- */
    zcosg = zcosgs;
    zsing = zsings;
    zcosi = zcosis;
    zsini = zsinis;
    zcosh = rec->cnodm;
    zsinh = rec->snodm;
    cc = c1ss;
    xnoi = 1.0 / rec->nm;

    for (lsflg = 1; lsflg <= 2; lsflg++)
    {
        a1 = zcosg * zcosh + zsing * zcosi * zsinh;
        a3 = -zsing * zcosh + zcosg * zcosi * zsinh;
        a7 = -zcosg * zsinh + zsing * zcosi * zcosh;
        a8 = zsing * zsini;
        a9 = zsing * zsinh + zcosg * zcosi * zcosh;
        a10 = zcosg * zsini;
        a2 = rec->cosim * a7 + rec->sinim * a8;
        a4 = rec->cosim * a9 + rec->sinim * a10;
        a5 = -rec->sinim * a7 + rec->cosim * a8;
        a6 = -rec->sinim * a9 + rec->cosim * a10;

        x1 = a1 * rec->cosomm + a2 * rec->sinomm;
        x2 = a3 * rec->cosomm + a4 * rec->sinomm;
        x3 = -a1 * rec->sinomm + a2 * rec->cosomm;
        x4 = -a3 * rec->sinomm + a4 * rec->cosomm;
        x5 = a5 * rec->sinomm;
        x6 = a6 * rec->sinomm;
        x7 = a5 * rec->cosomm;
        x8 = a6 * rec->cosomm;

        rec->z31 = 12.0 * x1 * x1 - 3.0 * x3 * x3;
        rec->z32 = 24.0 * x1 * x2 - 6.0 * x3 * x4;
        rec->z33 = 12.0 * x2 * x2 - 3.0 * x4 * x4;
        rec->z1 = 3.0 *  (a1 * a1 + a2 * a2) + rec->z31 * rec->emsq;
        rec->z2 = 6.0 *  (a1 * a3 + a2 * a4) + rec->z32 * rec->emsq;
        rec->z3 = 3.0 *  (a3 * a3 + a4 * a4) + rec->z33 * rec->emsq;
        rec->z11 = -6.0 * a1 * a5 + rec->emsq *  (-24.0 * x1 * x7 - 6.0 * x3 * x5);
        rec->z12 = -6.0 *  (a1 * a6 + a3 * a5) + rec->emsq *
            (-24.0 * (x2 * x7 + x1 * x8) - 6.0 * (x3 * x6 + x4 * x5));
        rec->z13 = -6.0 * a3 * a6 + rec->emsq * (-24.0 * x2 * x8 - 6.0 * x4 * x6);
        rec->z21 = 6.0 * a2 * a5 + rec->emsq * (24.0 * x1 * x5 - 6.0 * x3 * x7);
        rec->z22 = 6.0 *  (a4 * a5 + a2 * a6) + rec->emsq *
            (24.0 * (x2 * x5 + x1 * x6) - 6.0 * (x4 * x7 + x3 * x8));
        rec->z23 = 6.0 * a4 * a6 + rec->emsq * (24.0 * x2 * x6 - 6.0 * x4 * x8);
        rec->z1 = rec->z1 + rec->z1 + betasq * rec->z31;
        rec->z2 = rec->z2 + rec->z2 + betasq * rec->z32;
        rec->z3 = rec->z3 + rec->z3 + betasq * rec->z33;
        rec->s3 = cc * xnoi;
        rec->s2 = -0.5 * rec->s3 / rec->rtemsq;
        rec->s4 = rec->s3 * rec->rtemsq;
        rec->s1 = -15.0 * rec->em * rec->s4;
        rec->s5 = x1 * x3 + x2 * x4;
        rec->s6 = x2 * x3 + x1 * x4;
        rec->s7 = x2 * x4 - x1 * x3;

        /* ----------------------- do lunar terms ------------------- */
        if (lsflg == 1)
        {
            rec->ss1 = rec->s1;
            rec->ss2 = rec->s2;
            rec->ss3 = rec->s3;
            rec->ss4 = rec->s4;
            rec->ss5 = rec->s5;
            rec->ss6 = rec->s6;
            rec->ss7 = rec->s7;
            rec->sz1 = rec->z1;
            rec->sz2 = rec->z2;
            rec->sz3 = rec->z3;
            rec->sz11 = rec->z11;
            rec->sz12 = rec->z12;
            rec->sz13 = rec->z13;
            rec->sz21 = rec->z21;
            rec->sz22 = rec->z22;
            rec->sz23 = rec->z23;
            rec->sz31 = rec->z31;
            rec->sz32 = rec->z32;
            rec->sz33 = rec->z33;
            zcosg = zcosgl;
            zsing = zsingl;
            zcosi = zcosil;
            zsini = zsinil;
            zcosh = zcoshl * rec->cnodm + zsinhl * rec->snodm;
            zsinh = rec->snodm * zcoshl - rec->cnodm * zsinhl;
            cc = c1l;
        }
    }

    rec->zmol = fmod(4.7199672 + 0.22997150  * rec->day - rec->gam, twopi);
    rec->zmos = fmod(6.2565837 + 0.017201977 * rec->day, twopi);

    /* ------------------------ do solar terms ---------------------- */
    rec->se2 = 2.0 * rec->ss1 * rec->ss6;
    rec->se3 = 2.0 * rec->ss1 * rec->ss7;
    rec->si2 = 2.0 * rec->ss2 * rec->sz12;
    rec->si3 = 2.0 * rec->ss2 * (rec->sz13 - rec->sz11);
    rec->sl2 = -2.0 * rec->ss3 * rec->sz2;
    rec->sl3 = -2.0 * rec->ss3 * (rec->sz3 - rec->sz1);
    rec->sl4 = -2.0 * rec->ss3 * (-21.0 - 9.0 * rec->emsq) * zes;
    rec->sgh2 = 2.0 * rec->ss4 * rec->sz32;
    rec->sgh3 = 2.0 * rec->ss4 * (rec->sz33 - rec->sz31);
    rec->sgh4 = -18.0 * rec->ss4 * zes;
    rec->sh2 = -2.0 * rec->ss2 * rec->sz22;
    rec->sh3 = -2.0 * rec->ss2 * (rec->sz23 - rec->sz21);

    /* ------------------------ do lunar terms ---------------------- */
    rec->ee2 = 2.0 * rec->s1 * rec->s6;
    rec->e3 = 2.0 * rec->s1 * rec->s7;
    rec->xi2 = 2.0 * rec->s2 * rec->z12;
    rec->xi3 = 2.0 * rec->s2 * (rec->z13 - rec->z11);
    rec->xl2 = -2.0 * rec->s3 * rec->z2;
    rec->xl3 = -2.0 * rec->s3 * (rec->z3 - rec->z1);
    rec->xl4 = -2.0 * rec->s3 * (-21.0 - 9.0 * rec->emsq) * zel;
    rec->xgh2 = 2.0 * rec->s4 * rec->z32;
    rec->xgh3 = 2.0 * rec->s4 * (rec->z33 - rec->z31);
    rec->xgh4 = -18.0 * rec->s4 * zel;
    rec->xh2 = -2.0 * rec->s2 * rec->z22;
    rec->xh3 = -2.0 * rec->s2 * (rec->z23 - rec->z21);

}  // dscom


void dsinit
    (
    double tc, double xpidot,
    ElsetRec *rec
    ) {
    /* --------------------- local variables ------------------------ */

    double ainv2, aonv = 0.0, cosisq, eoc, f220, f221, f311,
        f321, f322, f330, f441, f442, f522, f523,
        f542, f543, g200, g201, g211, g300, g310,
        g322, g410, g422, g520, g521, g532, g533,
        ses, sgs, sghl, sghs, shs, shll, sis,
        sini2, sls, temp, temp1, theta, xno2, q22,
        q31, q33, root22, root44, root54, rptim, root32,
        root52, x2o3, znl, emo, zns, emsqo;

    q22 = 1.7891679e-6;
    q31 = 2.1460748e-6;
    q33 = 2.2123015e-7;
    root22 = 1.7891679e-6;
    root44 = 7.3636953e-9;
    root54 = 2.1765803e-9;
    rptim = 4.37526908801129966e-3; // this equates to 7.29211514668855e-5 rad/sec
    root32 = 3.7393792e-7;
    root52 = 1.1428639e-7;
    x2o3 = 2.0 / 3.0;
    znl = 1.5835218e-4;
    zns = 1.19459e-5;

    // sgp4fix identify constants and allow alternate values
    // just xke is used here so pass it in rather than have multiple calls
    // getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );

    /* -------------------- deep space initialization ------------ */
    rec->irez = 0;
    if ((rec->nm < 0.0052359877) && (rec->nm > 0.0034906585))
        rec->irez = 1;
    if ((rec->nm >= 8.26e-3) && (rec->nm <= 9.24e-3) && (rec->em >= 0.5))
        rec->irez = 2;

    /* ------------------------ do solar terms ------------------- */
    ses = rec->ss1 * zns * rec->ss5;
    sis = rec->ss2 * zns * (rec->sz11 + rec->sz13);
    sls = -zns * rec->ss3 * (rec->sz1 + rec->sz3 - 14.0 - 6.0 * rec->emsq);
    sghs = rec->ss4 * zns * (rec->sz31 + rec->sz33 - 6.0);
    shs = -zns * rec->ss2 * (rec->sz21 + rec->sz23);
    // sgp4fix for 180 deg incl
    if ((rec->inclm < 5.2359877e-2) || (rec->inclm > pi - 5.2359877e-2))
        shs = 0.0;
    if (rec->sinim != 0.0)
        shs = shs / rec->sinim;
    sgs = sghs - rec->cosim * shs;

    /* ------------------------- do lunar terms ------------------ */
    rec->dedt = ses + rec->s1 * znl * rec->s5;
    rec->didt = sis + rec->s2 * znl * (rec->z11 + rec->z13);
    rec->dmdt = sls - znl * rec->s3 * (rec->z1 + rec->z3 - 14.0 - 6.0 * rec->emsq);
    sghl = rec->s4 * znl * (rec->z31 + rec->z33 - 6.0);
    shll = -znl * rec->s2 * (rec->z21 + rec->z23);
    // sgp4fix for 180 deg incl
    if ((rec->inclm < 5.2359877e-2) || (rec->inclm > pi - 5.2359877e-2))
        shll = 0.0;
    rec->domdt = sgs + sghl;
    rec->dnodt = shs;
    if (rec->sinim != 0.0)
    {
        rec->domdt = rec->domdt - rec->cosim / rec->sinim * shll;
        rec->dnodt = rec->dnodt + shll / rec->sinim;
    }

    /* ----------- calculate deep space resonance effects -------- */
    rec->dndt = 0.0;
    theta = fmod(rec->gsto + tc * rptim, twopi);
    rec->em = rec->em + rec->dedt * rec->t;
    rec->inclm = rec->inclm + rec->didt * rec->t;
    rec->argpm = rec->argpm + rec->domdt * rec->t;
    rec->nodem = rec->nodem + rec->dnodt * rec->t;
    rec->mm = rec->mm + rec->dmdt * rec->t;
    //   sgp4fix for negative inclinations
    //   the following if statement should be commented out
    //if (inclm < 0.0)
    //  {
    //    inclm  = -inclm;
    //    argpm  = argpm - pi;
    //    nodem = nodem + pi;
    //  }

    /* -------------- initialize the resonance terms ------------- */
    if (rec->irez != 0)
    {
        aonv = pow(rec->nm / rec->xke, x2o3);

        /* ---------- geopotential resonance for 12 hour orbits ------ */
        if (rec->irez == 2)
        {
            cosisq = rec->cosim * rec->cosim;
            emo = rec->em;
            rec->em = rec->ecco;
            emsqo = rec->emsq;
            rec->emsq = rec->eccsq;
            eoc = rec->em * rec->emsq;
            g201 = -0.306 - (rec->em - 0.64) * 0.440;

            if (rec->em <= 0.65)
            {
                g211 = 3.616 - 13.2470 * rec->em + 16.2900 * rec->emsq;
                g310 = -19.302 + 117.3900 * rec->em - 228.4190 * rec->emsq + 156.5910 * eoc;
                g322 = -18.9068 + 109.7927 * rec->em - 214.6334 * rec->emsq + 146.5816 * eoc;
                g410 = -41.122 + 242.6940 * rec->em - 471.0940 * rec->emsq + 313.9530 * eoc;
                g422 = -146.407 + 841.8800 * rec->em - 1629.014 * rec->emsq + 1083.4350 * eoc;
                g520 = -532.114 + 3017.977 * rec->em - 5740.032 * rec->emsq + 3708.2760 * eoc;
            }
            else
            {
                g211 = -72.099 + 331.819 * rec->em - 508.738 * rec->emsq + 266.724 * eoc;
                g310 = -346.844 + 1582.851 * rec->em - 2415.925 * rec->emsq + 1246.113 * eoc;
                g322 = -342.585 + 1554.908 * rec->em - 2366.899 * rec->emsq + 1215.972 * eoc;
                g410 = -1052.797 + 4758.686 * rec->em - 7193.992 * rec->emsq + 3651.957 * eoc;
                g422 = -3581.690 + 16178.110 * rec->em - 24462.770 * rec->emsq + 12422.520 * eoc;
                if (rec->em > 0.715)
                    g520 = -5149.66 + 29936.92 * rec->em - 54087.36 * rec->emsq + 31324.56 * eoc;
                else
                    g520 = 1464.74 - 4664.75 * rec->em + 3763.64 * rec->emsq;
            }
            if (rec->em < 0.7)
            {
                g533 = -919.22770 + 4988.6100 * rec->em - 9064.7700 * rec->emsq + 5542.21  * eoc;
                g521 = -822.71072 + 4568.6173 * rec->em - 8491.4146 * rec->emsq + 5337.524 * eoc;
                g532 = -853.66600 + 4690.2500 * rec->em - 8624.7700 * rec->emsq + 5341.4  * eoc;
            }
            else
            {
                g533 = -37995.780 + 161616.52 * rec->em - 229838.20 * rec->emsq + 109377.94 * eoc;
                g521 = -51752.104 + 218913.95 * rec->em - 309468.16 * rec->emsq + 146349.42 * eoc;
                g532 = -40023.880 + 170470.89 * rec->em - 242699.48 * rec->emsq + 115605.82 * eoc;
            }

            sini2 = rec->sinim * rec->sinim;
            f220 = 0.75 * (1.0 + 2.0 * rec->cosim + cosisq);
            f221 = 1.5 * sini2;
            f321 = 1.875 * rec->sinim  *  (1.0 - 2.0 * rec->cosim - 3.0 * cosisq);
            f322 = -1.875 * rec->sinim  *  (1.0 + 2.0 * rec->cosim - 3.0 * cosisq);
            f441 = 35.0 * sini2 * f220;
            f442 = 39.3750 * sini2 * sini2;
            f522 = 9.84375 * rec->sinim * (sini2 * (1.0 - 2.0 * rec->cosim - 5.0 * cosisq) +
                0.33333333 * (-2.0 + 4.0 * rec->cosim + 6.0 * cosisq));
            f523 = rec->sinim * (4.92187512 * sini2 * (-2.0 - 4.0 * rec->cosim +
                10.0 * cosisq) + 6.56250012 * (1.0 + 2.0 * rec->cosim - 3.0 * cosisq));
            f542 = 29.53125 * rec->sinim * (2.0 - 8.0 * rec->cosim + cosisq *
                (-12.0 + 8.0 * rec->cosim + 10.0 * cosisq));
            f543 = 29.53125 * rec->sinim * (-2.0 - 8.0 * rec->cosim + cosisq *
                (12.0 + 8.0 * rec->cosim - 10.0 * cosisq));
            xno2 = rec->nm * rec->nm;
            ainv2 = aonv * aonv;
            temp1 = 3.0 * xno2 * ainv2;
            temp = temp1 * root22;
            rec->d2201 = temp * f220 * g201;
            rec->d2211 = temp * f221 * g211;
            temp1 = temp1 * aonv;
            temp = temp1 * root32;
            rec->d3210 = temp * f321 * g310;
            rec->d3222 = temp * f322 * g322;
            temp1 = temp1 * aonv;
            temp = 2.0 * temp1 * root44;
            rec->d4410 = temp * f441 * g410;
            rec->d4422 = temp * f442 * g422;
            temp1 = temp1 * aonv;
            temp = temp1 * root52;
            rec->d5220 = temp * f522 * g520;
            rec->d5232 = temp * f523 * g532;
            temp = 2.0 * temp1 * root54;
            rec->d5421 = temp * f542 * g521;
            rec->d5433 = temp * f543 * g533;
            rec->xlamo = fmod(rec->mo + rec->nodeo + rec->nodeo - theta - theta, twopi);
            rec->xfact = rec->mdot + rec->dmdt + 2.0 * (rec->nodedot + rec->dnodt - rptim) - rec->no_unkozai;
            rec->em = emo;
            rec->emsq = emsqo;
        }

        /* ---------------- synchronous resonance terms -------------- */
        if (rec->irez == 1)
        {
            g200 = 1.0 + rec->emsq * (-2.5 + 0.8125 * rec->emsq);
            g310 = 1.0 + 2.0 * rec->emsq;
            g300 = 1.0 + rec->emsq * (-6.0 + 6.60937 * rec->emsq);
            f220 = 0.75 * (1.0 + rec->cosim) * (1.0 + rec->cosim);
            f311 = 0.9375 * rec->sinim * rec->sinim * (1.0 + 3.0 * rec->cosim) - 0.75 * (1.0 + rec->cosim);
            f330 = 1.0 + rec->cosim;
            f330 = 1.875 * f330 * f330 * f330;
            rec->del1 = 3.0 * rec->nm * rec->nm * aonv * aonv;
            rec->del2 = 2.0 * rec->del1 * f220 * g200 * q22;
            rec->del3 = 3.0 * rec->del1 * f330 * g300 * q33 * aonv;
            rec->del1 = rec->del1 * f311 * g310 * q31 * aonv;
            rec->xlamo = fmod(rec->mo + rec->nodeo + rec->argpo - theta, twopi);
            rec->xfact = rec->mdot + xpidot - rptim + rec->dmdt + rec->domdt + rec->dnodt - rec->no_unkozai;
        }

        /* ------------ for sgp4, initialize the integrator ---------- */
        rec->xli = rec->xlamo;
        rec->xni = rec->no_unkozai;
        rec->atime = 0.0;
        rec->nm = rec->no_unkozai + rec->dndt;
    }

}  // dsinit


void dspace(double tc, ElsetRec *rec)
{
    int iretn;
    double delt, ft, theta, x2li, x2omi, xl, xldot, xnddt, xndt, xomi, g22, g32,
        g44, g52, g54, fasx2, fasx4, fasx6, rptim, step2, stepn, stepp;
        
    xndt = 0;
    xnddt = 0;
    xldot = 0;
        
    fasx2 = 0.13130908;
    fasx4 = 2.8843198;
    fasx6 = 0.37448087;
    g22 = 5.7686396;
    g32 = 0.95240898;
    g44 = 1.8014998;
    g52 = 1.0508330;
    g54 = 4.4108898;
    rptim = 4.37526908801129966e-3; // this equates to 7.29211514668855e-5 rad/sec
    stepp = 720.0;
    stepn = -720.0;
    step2 = 259200.0;

    /* ----------- calculate deep space resonance effects ----------- */
    rec->dndt = 0.0;
    theta = fmod(rec->gsto + tc * rptim, twopi);
    rec->em = rec->em + rec->dedt * rec->t;

    rec->inclm = rec->inclm + rec->didt * rec->t;
    rec->argpm = rec->argpm + rec->domdt * rec->t;
    rec->nodem = rec->nodem + rec->dnodt * rec->t;
    rec->mm = rec->mm + rec->dmdt * rec->t;

    //   sgp4fix for negative inclinations
    //   the following if statement should be commented out
    //  if (inclm < 0.0)
    // {
    //    inclm = -inclm;
    //    argpm = argpm - pi;
    //    nodem = nodem + pi;
    //  }

    /* - update resonances : numerical (euler-maclaurin) integration - */
    /* ------------------------- epoch restart ----------------------  */
    //   sgp4fix for propagator problems
    //   the following integration works for negative time steps and periods
    //   the specific changes are unknown because the original code was so convoluted

    // sgp4fix take out atime = 0.0 and fix for faster operation
    ft = 0.0;
    if (rec->irez != 0)
    {
        // sgp4fix streamline check
        if ((rec->atime == 0.0) || (rec->t * rec->atime <= 0.0) || (fabs(rec->t) < fabs(rec->atime)))
        {
            rec->atime = 0.0;
            rec->xni = rec->no_unkozai;
            rec->xli = rec->xlamo;
        }
        // sgp4fix move check outside loop
        if (rec->t > 0.0)
            delt = stepp;
        else
            delt = stepn;

        iretn = 381; // added for do loop
        while (iretn == 381)
        {
            /* ------------------- dot terms calculated ------------- */
            /* ----------- near - synchronous resonance terms ------- */
            if (rec->irez != 2)
            {
                xndt = rec->del1 * sin(rec->xli - fasx2) + rec->del2 * sin(2.0 * (rec->xli - fasx4)) +
                        rec->del3 * sin(3.0 * (rec->xli - fasx6));
                xldot = rec->xni + rec->xfact;
                xnddt = rec->del1 * cos(rec->xli - fasx2) +
                    2.0 * rec->del2 * cos(2.0 * (rec->xli - fasx4)) +
                    3.0 * rec->del3 * cos(3.0 * (rec->xli - fasx6));
                xnddt = xnddt * xldot;
            }
            else
            {
                /* --------- near - half-day resonance terms -------- */
                xomi = rec->argpo + rec->argpdot * rec->atime;
                x2omi = xomi + xomi;
                x2li = rec->xli + rec->xli;
                xndt = rec->d2201 * sin(x2omi + rec->xli - g22) + rec->d2211 * sin(rec->xli - g22) +
                        rec->d3210 * sin(xomi + rec->xli - g32) + rec->d3222 * sin(-xomi + rec->xli - g32) +
                        rec->d4410 * sin(x2omi + x2li - g44) + rec->d4422 * sin(x2li - g44) +
                        rec->d5220 * sin(xomi + rec->xli - g52) + rec->d5232 * sin(-xomi + rec->xli - g52) +
                        rec->d5421 * sin(xomi + x2li - g54) + rec->d5433 * sin(-xomi + x2li - g54);
                xldot = rec->xni + rec->xfact;
                xnddt = rec->d2201 * cos(x2omi + rec->xli - g22) + rec->d2211 * cos(rec->xli - g22) +
                        rec->d3210 * cos(xomi + rec->xli - g32) + rec->d3222 * cos(-xomi + rec->xli - g32) +
                        rec->d5220 * cos(xomi + rec->xli - g52) + rec->d5232 * cos(-xomi + rec->xli - g52) +
                    2.0 * (rec->d4410 * cos(x2omi + x2li - g44) +
                            rec->d4422 * cos(x2li - g44) + rec->d5421 * cos(xomi + x2li - g54) +
                            rec->d5433 * cos(-xomi + x2li - g54));
                xnddt = xnddt * xldot;
            }

            /* ----------------------- integrator ------------------- */
            // sgp4fix move end checks to end of routine
            if (fabs(rec->t - rec->atime) >= stepp)
            {
                iretn = 381;
            }
            else // exit here
            {
                ft = rec->t - rec->atime;
                iretn = 0;
            }

            if (iretn == 381)
            {
                rec->xli = rec->xli + xldot * delt + xndt * step2;
                rec->xni = rec->xni + xndt * delt + xnddt * step2;
                rec->atime = rec->atime + delt;
            }
        }  // while iretn = 381

            
        rec->nm = rec->xni + xndt * ft + xnddt * ft * ft * 0.5;
        xl = rec->xli + xldot * ft + xndt * ft * ft * 0.5;
        if (rec->irez != 1)
        {
            rec->mm = xl - 2.0 * rec->nodem + 2.0 * theta;
            rec->dndt = rec->nm - rec->no_unkozai;
        }
        else
        {
            rec->mm = xl - rec->nodem - rec->argpm + theta;
            rec->dndt = rec->nm - rec->no_unkozai;
        }
        rec->nm = rec->no_unkozai + rec->dndt;
    }

}  // dsspace


void initl(double epoch, ElsetRec *rec)
{
    /* --------------------- local variables ------------------------ */
    double ak, d1, del, adel, po, x2o3;

    // sgp4fix use old way of finding gst
    double ds70;
    double ts70, tfrac, c1, thgr70, fk5r, c1p2p;
        
    /* ----------------------- earth constants ---------------------- */
    // sgp4fix identify constants and allow alternate values
    // only xke and j2 are used here so pass them in directly
    // getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );
    x2o3 = 2.0 / 3.0;

    /* ------------- calculate auxillary epoch quantities ---------- */
    rec->eccsq = rec->ecco * rec->ecco;
    rec->omeosq = 1.0 - rec->eccsq;
    rec->rteosq = sqrt(rec->omeosq);
    rec->cosio = cos(rec->inclo);
    rec->cosio2 = rec->cosio * rec->cosio;

    /* ------------------ un-kozai the mean motion ----------------- */
    ak = pow(rec->xke / rec->no_kozai, x2o3);
    d1 = 0.75 * rec->j2 * (3.0 * rec->cosio2 - 1.0) / (rec->rteosq * rec->omeosq);
    del = d1 / (ak * ak);
    adel = ak * (1.0 - del * del - del *
        (1.0 / 3.0 + 134.0 * del * del / 81.0));
    del = d1 / (adel * adel);
    rec->no_unkozai = rec->no_kozai / (1.0 + del);

    rec->ao = pow(rec->xke / (rec->no_unkozai), x2o3);
    rec->sinio = sin(rec->inclo);
    po = rec->ao * rec->omeosq;
    rec->con42 = 1.0 - 5.0 * rec->cosio2;
    rec->con41 = -rec->con42 - rec->cosio2 - rec->cosio2;
    rec->ainv = 1.0 / rec->ao;
    rec->posq = po * po;
    rec->rp = rec->ao * (1.0 - rec->ecco);
    rec->method = 'n';

    // sgp4fix modern approach to finding sidereal time
    //   if (opsmode == 'a')
    //      {
    // sgp4fix use old way of finding gst
    // count integer number of days from 0 jan 1970
    ts70 = epoch - 7305.0;
    ds70 = floor(ts70 + 1.0e-8);
    tfrac = ts70 - ds70;
    // find greenwich location at epoch
    c1 = 1.72027916940703639e-2;
    thgr70 = 1.7321343856509374;
    fk5r = 5.07551419432269442e-15;
    c1p2p = c1 + twopi;
    double gsto1 = fmod(thgr70 + c1*ds70 + c1p2p*tfrac + ts70*ts70*fk5r, twopi);
    if (gsto1 < 0.0)
        gsto1 = gsto1 + twopi;
    //    }
    //    else
    rec->gsto = gstime(epoch + 2433281.5);

}  // initl


bool sgp4init ( char opsmode,ElsetRec *satrec)
{
    /* --------------------- local variables ------------------------ */
        
    double cc1sq,
        cc2, cc3, coef, coef1, cosio4,
        eeta, etasq, perige, pinvsq, psisq, qzms24,
        sfour,tc, temp, temp1, temp2, temp3, tsi, xpidot,
        xhdot1,qzms2t, ss, x2o3, r[3], v[3],
        delmotemp, qzms2ttemp, qzms24temp;
        
                       
    double epoch = (satrec->jdsatepoch + satrec->jdsatepochF) - 2433281.5;
        
    /* ------------------------ initialization --------------------- */
    // sgp4fix divisor for divide by zero check on inclination
    // the old check used 1.0 + cos(pi-1.0e-9), but then compared it to
    // 1.5 e-12, so the threshold was changed to 1.5e-12 for consistency
    const double temp4 = 1.5e-12;

    /* ----------- set all near earth variables to zero ------------ */
    satrec->isimp = 0;   satrec->method = 'n'; satrec->aycof = 0.0;
    satrec->con41 = 0.0; satrec->cc1 = 0.0; satrec->cc4 = 0.0;
    satrec->cc5 = 0.0; satrec->d2 = 0.0; satrec->d3 = 0.0;
    satrec->d4 = 0.0; satrec->delmo = 0.0; satrec->eta = 0.0;
    satrec->argpdot = 0.0; satrec->omgcof = 0.0; satrec->sinmao = 0.0;
    satrec->t = 0.0; satrec->t2cof = 0.0; satrec->t3cof = 0.0;
    satrec->t4cof = 0.0; satrec->t5cof = 0.0; satrec->x1mth2 = 0.0;
    satrec->x7thm1 = 0.0; satrec->mdot = 0.0; satrec->nodedot = 0.0;
    satrec->xlcof = 0.0; satrec->xmcof = 0.0; satrec->nodecf = 0.0;

    /* ----------- set all deep space variables to zero ------------ */
    satrec->irez = 0;   satrec->d2201 = 0.0; satrec->d2211 = 0.0;
    satrec->d3210 = 0.0; satrec->d3222 = 0.0; satrec->d4410 = 0.0;
    satrec->d4422 = 0.0; satrec->d5220 = 0.0; satrec->d5232 = 0.0;
    satrec->d5421 = 0.0; satrec->d5433 = 0.0; satrec->dedt = 0.0;
    satrec->del1 = 0.0; satrec->del2 = 0.0; satrec->del3 = 0.0;
    satrec->didt = 0.0; satrec->dmdt = 0.0; satrec->dnodt = 0.0;
    satrec->domdt = 0.0; satrec->e3 = 0.0; satrec->ee2 = 0.0;
    satrec->peo = 0.0; satrec->pgho = 0.0; satrec->pho = 0.0;
    satrec->pinco = 0.0; satrec->plo = 0.0; satrec->se2 = 0.0;
    satrec->se3 = 0.0; satrec->sgh2 = 0.0; satrec->sgh3 = 0.0;
    satrec->sgh4 = 0.0; satrec->sh2 = 0.0; satrec->sh3 = 0.0;
    satrec->si2 = 0.0; satrec->si3 = 0.0; satrec->sl2 = 0.0;
    satrec->sl3 = 0.0; satrec->sl4 = 0.0; satrec->gsto = 0.0;
    satrec->xfact = 0.0; satrec->xgh2 = 0.0; satrec->xgh3 = 0.0;
    satrec->xgh4 = 0.0; satrec->xh2 = 0.0; satrec->xh3 = 0.0;
    satrec->xi2 = 0.0; satrec->xi3 = 0.0; satrec->xl2 = 0.0;
    satrec->xl3 = 0.0; satrec->xl4 = 0.0; satrec->xlamo = 0.0;
    satrec->zmol = 0.0; satrec->zmos = 0.0; satrec->atime = 0.0;
    satrec->xli = 0.0; satrec->xni = 0.0;

    /* ------------------------ earth constants ----------------------- */
    // sgp4fix identify constants and allow alternate values
    // this is now the only call for the constants
    getgravconst(satrec->whichconst, satrec);

    //-------------------------------------------------------------------------

    satrec->error = 0;
    satrec->operationmode = opsmode;


    // single averaged mean elements
    satrec->am = satrec->em = satrec->im = satrec->Om = satrec->mm = satrec->nm = 0.0;

    /* ------------------------ earth constants ----------------------- */
    // sgp4fix identify constants and allow alternate values no longer needed
    // getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );
    ss = 78.0 / satrec->radiusearthkm + 1.0;
    // sgp4fix use multiply for speed instead of pow
    qzms2ttemp = (120.0 - 78.0) / satrec->radiusearthkm;
    qzms2t = qzms2ttemp * qzms2ttemp * qzms2ttemp * qzms2ttemp;
    x2o3 = 2.0 / 3.0;

    satrec->init = 'y';
    satrec->t = 0.0;

    // sgp4fix remove satn as it is not needed in initl
        
    initl(epoch,satrec);
        
    satrec->a = pow(satrec->no_unkozai * satrec->tumin, (-2.0 / 3.0));
    satrec->alta = satrec->a * (1.0 + satrec->ecco) - 1.0;
    satrec->altp = satrec->a * (1.0 - satrec->ecco) - 1.0;
    satrec->error = 0;

    // sgp4fix remove this check as it is unnecessary
    // the mrt check in sgp4 handles decaying satellite cases even if the starting
    // condition is below the surface of te earth
    //     if (rp < 1.0)
    //       {
    //         satrec->error = 5;
    //       }

    if ((satrec->omeosq >= 0.0) || (satrec->no_unkozai >= 0.0))
    {
        satrec->isimp = 0;
        if (satrec->rp < (220.0 / satrec->radiusearthkm + 1.0))
            satrec->isimp = 1;
        sfour = ss;
        qzms24 = qzms2t;
        perige = (satrec->rp - 1.0) * satrec->radiusearthkm;

        /* - for perigees below 156 km, s and qoms2t are altered - */
        if (perige < 156.0)
        {
            sfour = perige - 78.0;
            if (perige < 98.0)
                sfour = 20.0;
            // sgp4fix use multiply for speed instead of pow
            qzms24temp = (120.0 - sfour) / satrec->radiusearthkm;
            qzms24 = qzms24temp * qzms24temp * qzms24temp * qzms24temp;
            sfour = sfour / satrec->radiusearthkm + 1.0;
        }
        pinvsq = 1.0 / satrec->posq;

        tsi = 1.0 / (satrec->ao - sfour);
        satrec->eta = satrec->ao * satrec->ecco * tsi;
        etasq = satrec->eta * satrec->eta;
        eeta = satrec->ecco * satrec->eta;
        psisq = fabs(1.0 - etasq);
        coef = qzms24 * pow(tsi, 4.0);
        coef1 = coef / pow(psisq, 3.5);
        cc2 = coef1 * satrec->no_unkozai * (satrec->ao * (1.0 + 1.5 * etasq + eeta *
            (4.0 + etasq)) + 0.375 * satrec->j2 * tsi / psisq * satrec->con41 *
            (8.0 + 3.0 * etasq * (8.0 + etasq)));
        satrec->cc1 = satrec->bstar * cc2;
        cc3 = 0.0;
        if (satrec->ecco > 1.0e-4)
            cc3 = -2.0 * coef * tsi * satrec->j3oj2 * satrec->no_unkozai * satrec->sinio / satrec->ecco;
        satrec->x1mth2 = 1.0 - satrec->cosio2;
        satrec->cc4 = 2.0* satrec->no_unkozai * coef1 * satrec->ao * satrec->omeosq *
            (satrec->eta * (2.0 + 0.5 * etasq) + satrec->ecco *
            (0.5 + 2.0 * etasq) - satrec->j2 * tsi / (satrec->ao * psisq) *
            (-3.0 * satrec->con41 * (1.0 - 2.0 * eeta + etasq *
            (1.5 - 0.5 * eeta)) + 0.75 * satrec->x1mth2 *
            (2.0 * etasq - eeta * (1.0 + etasq)) * cos(2.0 * satrec->argpo)));
        satrec->cc5 = 2.0 * coef1 * satrec->ao * satrec->omeosq * (1.0 + 2.75 *
            (etasq + eeta) + eeta * etasq);
        cosio4 = satrec->cosio2 * satrec->cosio2;
        temp1 = 1.5 * satrec->j2 * pinvsq * satrec->no_unkozai;
        temp2 = 0.5 * temp1 * satrec->j2 * pinvsq;
        temp3 = -0.46875 * satrec->j4 * pinvsq * pinvsq * satrec->no_unkozai;
        satrec->mdot = satrec->no_unkozai + 0.5 * temp1 * satrec->rteosq * satrec->con41 + 0.0625 *
            temp2 * satrec->rteosq * (13.0 - 78.0 * satrec->cosio2 + 137.0 * cosio4);
        satrec->argpdot = -0.5 * temp1 * satrec->con42 + 0.0625 * temp2 *
            (7.0 - 114.0 * satrec->cosio2 + 395.0 * cosio4) +
            temp3 * (3.0 - 36.0 * satrec->cosio2 + 49.0 * cosio4);
        xhdot1 = -temp1 * satrec->cosio;
        satrec->nodedot = xhdot1 + (0.5 * temp2 * (4.0 - 19.0 * satrec->cosio2) +
            2.0 * temp3 * (3.0 - 7.0 * satrec->cosio2)) * satrec->cosio;
        xpidot = satrec->argpdot + satrec->nodedot;
        satrec->omgcof = satrec->bstar * cc3 * cos(satrec->argpo);
        satrec->xmcof = 0.0;
        if (satrec->ecco > 1.0e-4)
            satrec->xmcof = -x2o3 * coef * satrec->bstar / eeta;
        satrec->nodecf = 3.5 * satrec->omeosq * xhdot1 * satrec->cc1;
        satrec->t2cof = 1.5 * satrec->cc1;
        // sgp4fix for divide by zero with xinco = 180 deg
        if (fabs(satrec->cosio + 1.0) > 1.5e-12)
            satrec->xlcof = -0.25 * satrec->j3oj2 * satrec->sinio * (3.0 + 5.0 * satrec->cosio) / (1.0 + satrec->cosio);
        else
            satrec->xlcof = -0.25 * satrec->j3oj2 * satrec->sinio * (3.0 + 5.0 * satrec->cosio) / temp4;
        satrec->aycof = -0.5 * satrec->j3oj2 * satrec->sinio;
        // sgp4fix use multiply for speed instead of pow
        delmotemp = 1.0 + satrec->eta * cos(satrec->mo);
        satrec->delmo = delmotemp * delmotemp * delmotemp;
        satrec->sinmao = sin(satrec->mo);
        satrec->x7thm1 = 7.0 * satrec->cosio2 - 1.0;

        /* --------------- deep space initialization ------------- */
        if ((2 * pi / satrec->no_unkozai) >= 225.0)
        {
            satrec->method = 'd';
            satrec->isimp = 1;
            tc = 0.0;
            satrec->inclm = satrec->inclo;

            dscom(epoch, satrec->ecco, satrec->argpo, tc, satrec->inclo, satrec->nodeo, satrec->no_unkozai,satrec);
                
                
            satrec->ep=satrec->ecco;
            satrec->inclp=satrec->inclo;
            satrec->nodep=satrec->nodeo;
            satrec->argpp=satrec->argpo;
            satrec->mp=satrec->mo;

                
            dpper(satrec->e3, satrec->ee2, satrec->peo, satrec->pgho,
                satrec->pho, satrec->pinco, satrec->plo, satrec->se2,
                satrec->se3, satrec->sgh2, satrec->sgh3, satrec->sgh4,
                satrec->sh2, satrec->sh3, satrec->si2, satrec->si3,
                satrec->sl2, satrec->sl3, satrec->sl4, satrec->t,
                satrec->xgh2, satrec->xgh3, satrec->xgh4, satrec->xh2,
                satrec->xh3, satrec->xi2, satrec->xi3, satrec->xl2,
                satrec->xl3, satrec->xl4, satrec->zmol, satrec->zmos, satrec->init,satrec,
                satrec->operationmode);


            satrec->ecco=satrec->ep;
            satrec->inclo=satrec->inclp;
            satrec->nodeo=satrec->nodep;
            satrec->argpo=satrec->argpp;
            satrec->mo=satrec->mp;


            satrec->argpm = 0.0;
            satrec->nodem = 0.0;
            satrec->mm = 0.0;
                
            dsinit(tc, xpidot, satrec);
        }

        /* ----------- set variables if not deep space ----------- */
        if (satrec->isimp != 1)
        {
            cc1sq = satrec->cc1 * satrec->cc1;
            satrec->d2 = 4.0 * satrec->ao * tsi * cc1sq;
            temp = satrec->d2 * tsi * satrec->cc1 / 3.0;
            satrec->d3 = (17.0 * satrec->ao + sfour) * temp;
            satrec->d4 = 0.5 * temp * satrec->ao * tsi * (221.0 * satrec->ao + 31.0 * sfour) * satrec->cc1;
            satrec->t3cof = satrec->d2 + 2.0 * cc1sq;
            satrec->t4cof = 0.25 * (3.0 * satrec->d3 + satrec->cc1 *
                (12.0 * satrec->d2 + 10.0 * cc1sq));
            satrec->t5cof = 0.2 * (3.0 * satrec->d4 +
                12.0 * satrec->cc1 * satrec->d3 +
                6.0 * satrec->d2 * satrec->d2 +
                15.0 * cc1sq * (2.0 * satrec->d2 + cc1sq));
        }
    } // if omeosq = 0 ...

    /* finally propogate to zero epoch to initialize all others. */
    // sgp4fix take out check to let satellites process until they are actually below earth surface
    //       if(satrec->error == 0)
        
        
    sgp4(satrec, 0.0, r, v);

    satrec->init = 'n';

    //sgp4fix return boolean. satrec->error contains any error codes
    return true;
}  // sgp4init


bool sgp4
    (
    ElsetRec *satrec, double tsince,
    double *r, double *v
    ) {
        
    double axnl, aynl, betal, cnod,
        cos2u, coseo1, cosi, cosip, cosisq, cossu, cosu,
        delm, delomg, ecose, el2, eo1,
        esine, argpdf, pl, mrt = 0.0,
        mvt, rdotl, rl, rvdot, rvdotl,
        sin2u, sineo1, sini, sinip, sinsu, sinu,
        snod, su, t2, t3, t4, tem5, temp,
        temp1, temp2, tempa, tempe, templ, u, ux,
        uy, uz, vx, vy, vz,
        xinc, xincp, xl, xlm,
        xmdf, xmx, xmy, nodedf, xnode, tc,
        x2o3, vkmpersec, delmtemp;
        
    int ktr;

    /* ------------------ set mathematical constants --------------- */
    // sgp4fix divisor for divide by zero check on inclination
    // the old check used 1.0 + cos(pi-1.0e-9), but then compared it to
    // 1.5 e-12, so the threshold was changed to 1.5e-12 for consistency
    const double temp4 = 1.5e-12;
    x2o3 = 2.0 / 3.0;
    // sgp4fix identify constants and allow alternate values
    // getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );
    vkmpersec = satrec->radiusearthkm * satrec->xke / 60.0;

    /* --------------------- clear sgp4 error flag ----------------- */
    satrec->t = tsince;
    satrec->error = 0;

    /* ------- update for secular gravity and atmospheric drag ----- */
    xmdf = satrec->mo + satrec->mdot * satrec->t;
    argpdf = satrec->argpo + satrec->argpdot * satrec->t;
    nodedf = satrec->nodeo + satrec->nodedot * satrec->t;
    satrec->argpm = argpdf;
    satrec->mm = xmdf;
    t2 = satrec->t * satrec->t;
    satrec->nodem = nodedf + satrec->nodecf * t2;
    tempa = 1.0 - satrec->cc1 * satrec->t;
    tempe = satrec->bstar * satrec->cc4 * satrec->t;
    templ = satrec->t2cof * t2;

    delomg = 0;
    delmtemp = 0;
    delm = 0;
    temp = 0;
    t3 = 0;
    t4 = 0;
    mrt = 0;
        
    if (satrec->isimp != 1)
    {
        delomg = satrec->omgcof * satrec->t;
        // sgp4fix use mutliply for speed instead of pow
        delmtemp = 1.0 + satrec->eta * cos(xmdf);
        delm = satrec->xmcof *
            (delmtemp * delmtemp * delmtemp -
            satrec->delmo);
        temp = delomg + delm;
        satrec->mm = xmdf + temp;
        satrec->argpm = argpdf - temp;
        t3 = t2 * satrec->t;
        t4 = t3 * satrec->t;
        tempa = tempa - satrec->d2 * t2 - satrec->d3 * t3 -
            satrec->d4 * t4;
        tempe = tempe + satrec->bstar * satrec->cc5 * (sin(satrec->mm) -satrec->sinmao);
        templ = templ + satrec->t3cof * t3 + t4 * (satrec->t4cof + satrec->t * satrec->t5cof);
    }

        
    tc = 0;
    satrec->nm = satrec->no_unkozai;
    satrec->em = satrec->ecco;
    satrec->inclm = satrec->inclo;
    if (satrec->method == 'd')
    {
        tc = satrec->t;
        dspace(tc,satrec);        
    } // if method = d

    if (satrec->nm <= 0.0)
    {
        satrec->error = 2;
        // sgp4fix add return
        return false;
    }
        
    satrec->am = pow((satrec->xke / satrec->nm), x2o3) * tempa * tempa;
    satrec->nm = satrec->xke / pow(satrec->am, 1.5);
    satrec->em = satrec->em - tempe;

    // fix tolerance for error recognition
    // sgp4fix am is fixed from the previous nm check
    if ((satrec->em >= 1.0) || (satrec->em < -0.001)/* || (am < 0.95)*/)
    {
        satrec->error = 1;
        // sgp4fix to return if there is an error in eccentricity
        return false;
    }
    // sgp4fix fix tolerance to avoid a divide by zero
    if (satrec->em < 1.0e-6)
        satrec->em = 1.0e-6;
    satrec->mm = satrec->mm + satrec->no_unkozai * templ;
    xlm = satrec->mm + satrec->argpm + satrec->nodem;
    satrec->emsq = satrec->em * satrec->em;
    temp = 1.0 - satrec->emsq;

    satrec->nodem = fmod(satrec->nodem, twopi);
    satrec->argpm = fmod(satrec->argpm, twopi);
    xlm = fmod(xlm, twopi);
    satrec->mm = fmod(xlm - satrec->argpm - satrec->nodem, twopi);

    // sgp4fix recover singly averaged mean elements
    satrec->am = satrec->am;
    satrec->em = satrec->em;
    satrec->im = satrec->inclm;
    satrec->Om = satrec->nodem;
    satrec->om = satrec->argpm;
    satrec->mm = satrec->mm;
    satrec->nm = satrec->nm;

    /* ----------------- compute extra mean quantities ------------- */
    satrec->sinim = sin(satrec->inclm);
    satrec->cosim = cos(satrec->inclm);

    /* -------------------- add lunar-solar periodics -------------- */
    satrec->ep = satrec->em;
    xincp = satrec->inclm;
    satrec->inclp = satrec->inclm;
    satrec->argpp = satrec->argpm;
    satrec->nodep = satrec->nodem;
    satrec->mp = satrec->mm;
    sinip = satrec->sinim;
    cosip = satrec->cosim;
    if (satrec->method == 'd')
    {
        dpper(satrec->e3, satrec->ee2, satrec->peo, satrec->pgho,
                satrec->pho, satrec->pinco, satrec->plo, satrec->se2,
                satrec->se3, satrec->sgh2, satrec->sgh3, satrec->sgh4,
                satrec->sh2, satrec->sh3, satrec->si2, satrec->si3,
                satrec->sl2, satrec->sl3, satrec->sl4, satrec->t,
                satrec->xgh2, satrec->xgh3, satrec->xgh4, satrec->xh2,
                satrec->xh3, satrec->xi2, satrec->xi3, satrec->xl2,
                satrec->xl3, satrec->xl4, satrec->zmol, satrec->zmos, 
                'n', satrec, satrec->operationmode);
            
        xincp = satrec->inclp;
        if (xincp < 0.0)
        {
            xincp = -xincp;
            satrec->nodep = satrec->nodep + pi;
            satrec->argpp = satrec->argpp - pi;
        }
        if ((satrec->ep < 0.0) || (satrec->ep > 1.0))
        {
            satrec->error = 3;
            // sgp4fix add return
            return false;
        }
    } // if method = d

    /* -------------------- long period periodics ------------------ */
    if (satrec->method == 'd')
    {
        sinip = sin(xincp);
        cosip = cos(xincp);
        satrec->aycof = -0.5*satrec->j3oj2*sinip;
        // sgp4fix for divide by zero for xincp = 180 deg
        if (fabs(cosip + 1.0) > 1.5e-12)
            satrec->xlcof = -0.25 * satrec->j3oj2 * sinip * (3.0 + 5.0 * cosip) / (1.0 + cosip);
        else
            satrec->xlcof = -0.25 * satrec->j3oj2 * sinip * (3.0 + 5.0 * cosip) / temp4;
    }
    axnl = satrec->ep * cos(satrec->argpp);
    temp = 1.0 / (satrec->am * (1.0 - satrec->ep * satrec->ep));
    aynl = satrec->ep* sin(satrec->argpp) + temp * satrec->aycof;
    xl = satrec->mp + satrec->argpp + satrec->nodep + temp * satrec->xlcof * axnl;

    /* --------------------- solve kepler's equation --------------- */
    u = fmod(xl - satrec->nodep, twopi);
    eo1 = u;
    tem5 = 9999.9;
    ktr = 1;
    sineo1 = 0;
    coseo1 = 0;
    //   sgp4fix for kepler iteration
    //   the following iteration needs better limits on corrections
    while ((fabs(tem5) >= 1.0e-12) && (ktr <= 10))
    {
        sineo1 = sin(eo1);
        coseo1 = cos(eo1);
        tem5 = 1.0 - coseo1 * axnl - sineo1 * aynl;
        tem5 = (u - aynl * coseo1 + axnl * sineo1 - eo1) / tem5;
        if (fabs(tem5) >= 0.95)
            tem5 = tem5 > 0.0 ? 0.95 : -0.95;
        eo1 = eo1 + tem5;
        ktr = ktr + 1;
    }

    /* ------------- short period preliminary quantities ----------- */
    ecose = axnl*coseo1 + aynl*sineo1;
    esine = axnl*sineo1 - aynl*coseo1;
    el2 = axnl*axnl + aynl*aynl;
    pl = satrec->am*(1.0 - el2);
    if (pl < 0.0)
    {
        satrec->error = 4;
        // sgp4fix add return
        return false;
    }
    else
    {
        rl = satrec->am * (1.0 - ecose);
        rdotl = sqrt(satrec->am) * esine / rl;
        rvdotl = sqrt(pl) / rl;
        betal = sqrt(1.0 - el2);
        temp = esine / (1.0 + betal);
        sinu = satrec->am / rl * (sineo1 - aynl - axnl * temp);
        cosu = satrec->am / rl * (coseo1 - axnl + aynl * temp);
        su = atan2(sinu, cosu);
        sin2u = (cosu + cosu) * sinu;
        cos2u = 1.0 - 2.0 * sinu * sinu;
        temp = 1.0 / pl;
        temp1 = 0.5 * satrec->j2 * temp;
        temp2 = temp1 * temp;

        /* -------------- update for short period periodics ------------ */
        if (satrec->method == 'd')
        {
            cosisq = cosip * cosip;
            satrec->con41 = 3.0*cosisq - 1.0;
            satrec->x1mth2 = 1.0 - cosisq;
            satrec->x7thm1 = 7.0*cosisq - 1.0;
        }
        mrt = rl * (1.0 - 1.5 * temp2 * betal * satrec->con41) +
            0.5 * temp1 * satrec->x1mth2 * cos2u;
        su = su - 0.25 * temp2 * satrec->x7thm1 * sin2u;
        xnode = satrec->nodep + 1.5 * temp2 * cosip * sin2u;
        xinc = xincp + 1.5 * temp2 * cosip * sinip * cos2u;
        mvt = rdotl - satrec->nm * temp1 * satrec->x1mth2 * sin2u / satrec->xke;
        rvdot = rvdotl + satrec->nm * temp1 * (satrec->x1mth2 * cos2u +
            1.5 * satrec->con41) / satrec->xke;

        /* --------------------- orientation vectors ------------------- */
        sinsu = sin(su);
        cossu = cos(su);
        snod = sin(xnode);
        cnod = cos(xnode);
        sini = sin(xinc);
        cosi = cos(xinc);
        xmx = -snod * cosi;
        xmy = cnod * cosi;
        ux = xmx * sinsu + cnod * cossu;
        uy = xmy * sinsu + snod * cossu;
        uz = sini * sinsu;
        vx = xmx * cossu - cnod * sinsu;
        vy = xmy * cossu - snod * sinsu;
        vz = sini * cossu;

        /* --------- position and velocity (in km and km/sec) ---------- */
        r[0] = (mrt * ux)* satrec->radiusearthkm;
        r[1] = (mrt * uy)* satrec->radiusearthkm;
        r[2] = (mrt * uz)* satrec->radiusearthkm;
        v[0] = (mvt * ux + rvdot * vx) * vkmpersec;
        v[1] = (mvt * uy + rvdot * vy) * vkmpersec;
        v[2] = (mvt * uz + rvdot * vz) * vkmpersec;
    }  // if pl > 0

    // sgp4fix for decaying satellites
    if (mrt < 1.0)
    {
        satrec->error = 6;
        return false;
    }

    return true;
}  // sgp4


void getgravconst(int whichconst, ElsetRec *rec) {
    rec->whichconst = whichconst;
    switch (whichconst)
    {
        // -- wgs-72 low precision str#3 constants --
    case wgs72old:
        rec->mu = 398600.79964;        // in km3 / s2
        rec->radiusearthkm = 6378.135;     // km
        rec->xke = 0.0743669161;        // reciprocal of tumin
        rec->tumin = 1.0 / rec->xke;
        rec->j2 = 0.001082616;
        rec->j3 = -0.00000253881;
        rec->j4 = -0.00000165597;
        rec->j3oj2 = rec->j3 / rec->j2;
        break;
        // ------------ wgs-72 constants ------------
    case wgs72:
        rec->mu = 398600.8;            // in km3 / s2
        rec->radiusearthkm = 6378.135;     // km
        rec->xke = 60.0 / sqrt(rec->radiusearthkm*rec->radiusearthkm*rec->radiusearthkm / rec->mu);
        rec->tumin = 1.0 / rec->xke;
        rec->j2 = 0.001082616;
        rec->j3 = -0.00000253881;
        rec->j4 = -0.00000165597;
        rec->j3oj2 = rec->j3 / rec->j2;
        break;
    default:
    case wgs84:
        // ------------ wgs-84 constants ------------
        rec->mu = 398600.5;            // in km3 / s2
        rec->radiusearthkm = 6378.137;     // km
        rec->xke = 60.0 / sqrt(rec->radiusearthkm*rec->radiusearthkm*rec->radiusearthkm / rec->mu);
        rec->tumin = 1.0 / rec->xke;
        rec->j2 = 0.00108262998905;
        rec->j3 = -0.00000253215306;
        rec->j4 = -0.00000161098761;
        rec->j3oj2 = rec->j3 / rec->j2;
        break;
    }

}   // getgravconst


double gstime(double jdut1) {
    double       temp, tut1;

    tut1 = (jdut1 - 2451545.0) / 36525.0;
    temp = -6.2e-6* tut1 * tut1 * tut1 + 0.093104 * tut1 * tut1 +
        (876600.0 * 3600 + 8640184.812866) * tut1 + 67310.54841;  // sec
    temp = std::fmod(temp * deg2rad / 240.0, twopi); //360/86400 = 1/240, to deg, to rad
        
    // ------------------------ check quadrants ---------------------
    if (temp < 0.0)
        temp += twopi;

    return temp;
}  // gstime
    
    
void jday(int year, int mon, int day, int hr, int minute, double sec, double *jd, double *jdfrac) {

    *jd = 367.0 * year -
        floor((7 * (year + floor((mon + 9) / 12.0))) * 0.25) +
        floor(275 * mon / 9.0) +
        day + 1721013.5;  // use - 678987.0 to go to mjd directly
    *jdfrac = (sec + minute * 60.0 + hr * 3600.0) / 86400.0;

    // check that the day and fractional day are correct
    if (fabs(*jdfrac) > 1.0)
    {
        double dtt = floor(*jdfrac);
        *jd = *jd + dtt;
        *jdfrac = *jdfrac - dtt;
    }

    return;
}  // jday
