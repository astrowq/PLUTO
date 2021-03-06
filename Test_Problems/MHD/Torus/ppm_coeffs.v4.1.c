#include "pluto.h"

#if INTERPOLATION == WENO3
 #undef PPM_VERSION
 #define PPM_VERSION PPM3
#endif
#define PPM_VERSION PPM_ORDER
#define PPM3 3
#define PPM4 4
#define PPM5 5

/* -----------------------------------------------------
    Static local variables
   ----------------------------------------------------- */

static double ***Wp3D, ***Wm3D, **hp3D, **hm3D;


static void PPM_CartCoeff(double **, double **, int, int);
static void PPM_FindWeights(double **, double **, int, Grid *);

/* ********************************************************************* */
void PPM_CoefficientsSet(Grid *grid)
/*!
 * Compute interpolation coefficients for PPM in every coordinate system.
 *
 *********************************************************************** */
{
  int    i, iL, iR, n, beg, end, d;
  int    uniform_grid[3];
  double i1, i2, rp, dr, den;
  double **wp, **wm;
  
  if (Wp3D == NULL){
    Wp3D = ArrayBox(0, DIMENSIONS-1, 0, NMAX_POINT-1, -2, 2);
    Wm3D = ArrayBox(0, DIMENSIONS-1, 0, NMAX_POINT-1, -2, 2);
    hp3D = ARRAY_2D(DIMENSIONS, NMAX_POINT, double);
    hm3D = ARRAY_2D(DIMENSIONS, NMAX_POINT, double);
  }

/* -------------------------------------------------------
    Check whether the grid is regularly spaced.
    This is always true when chombo is used, but not
    necessarily so with the static grid version.
   ------------------------------------------------------- */

  #ifdef CHOMBO
   for (d = 0; d < DIMENSIONS; d++) uniform_grid[d] = 1;
  #else
   for (d = 0; d < DIMENSIONS; d++) {
     uniform_grid[d] = 1;
     for (i = 0; i < grid[d].np_tot-1; i++){
       if (fabs(grid[d].dx[i+1]/grid[d].dx[i]-1.0) > 1.e-9){
         uniform_grid[d] = 0;
         break;
       }
     }
   }
  #endif

/* -----------------------------------------------------
    set the local (iL, iR) and global (beg, end) 
    reconstruction stencils and the order of accuracy n
   ----------------------------------------------------- */

  #if PPM_VERSION == PPM3
   iL = 1; iR = 1;
  #elif PPM_VERSION == PPM4
   iL = 1; iR = 2;
  #elif PPM_VERSION == PPM5
   iL = 2; iR = 2;
  #endif
  n = iR + iL + 1;

/* ----------------------------------------------------------
    Loop over dimensions
   ---------------------------------------------------------- */

  for (d = 0; d < DIMENSIONS; d++){

    wp = Wp3D[d];
    wm = Wm3D[d];

    beg = iL;
    end = grid[d].np_tot - 1 - iR;

  /* -------------------------------------------------------------
      Interpolation weights for Cartesian geometry (called anyway
      to initialize cofficients to zero where necessary).
     ------------------------------------------------------------- */

    PPM_CartCoeff(wp, wm, beg, end);
  
  /* -------------------------------------------------------------
      Interpolation weights for cylindrical/polar geometries
     ------------------------------------------------------------- */

    #if GEOMETRY == CYLINDRICAL || GEOMETRY == POLAR
     if (d == IDIR) for (i = beg; i <= end; i++){
       rp  = grid[IDIR].xr[i];
       dr  = grid[IDIR].dx[i];
       i1  = rp/dr;
//      i1  = (double)(i + (grid[dir].beg-IBEG) - IBEG + 1);
       i2  = i1*i1;
       #if PPM_VERSION == PPM3
        den = 12.0*POLY_3(1.0, -1.0, -3.0, 2.0, i1);

        wp[i][-2] = 0.0;
        wp[i][-1] = POLY_3( -3.0,   2.0,   6.0, -4.0, i1)/den;
        wp[i][ 0] = POLY_3( 11.0, -13.0, -28.0, 20.0, i1)/den;
        wp[i][ 1] = POLY_3(  4.0,  -1.0, -14.0,  8.0, i1)/den;
        wp[i][ 2] = 0.0;

        wm[i][-2] = 0.0;
        wm[i][-1] = POLY_3(  3.0,  -5.0, -10.0,  8.0, i1)/den;
        wm[i][ 0] = POLY_3( 10.0,  -9.0, -32.0, 20.0, i1)/den;
        wm[i][ 1] = POLY_3( -1.0,   2.0,   6.0, -4.0, i1)/den;
        wm[i][ 2] = 0.0;
       #elif PPM_VERSION == PPM4
        den = 24.0*POLY_2(4.0, -15.0, 5.0, i2);

        wp[i][-2] = 0.0;
        wp[i][-1] = POLY_4(-12.0,  -1.0,   30.0,  -1.0, -10.0, i1)/den;
        wp[i][ 0] = POLY_4( 60.0, -27.0, -210.0,  13.0,  70.0, i1)/den;
        wp[i][ 1] = POLY_4( 60.0,  27.0, -210.0, -13.0,  70.0, i1)/den;
        wp[i][ 2] = POLY_4(-12.0,   1.0,   30.0,   1.0, -10.0, i1)/den;

       #elif PPM_VERSION == PPM5
        den = 120.0*(2.0*i1-1.0)*POLY_4(12.0, 16.0, -13.0, -6.0, 3.0, i1);

        wp[i][-2] = POLY_5(  -80,   32,   200,   -80,   -60,   24, i1)/den;
        wp[i][-1] = POLY_5(  492, -193, -1230,   535,   384, -156, i1)/den;
        wp[i][ 0] = POLY_5(-1276, 1157,  4090, -2075, -1332,  564, i1)/den;
        wp[i][ 1] = POLY_5( -684,   27,  2610,  -885,  -888,  324, i1)/den;
        wp[i][ 2] = POLY_5(  108,  -63,  -270,   105,    96,  -36, i1)/den;

        wm[i][-2] = POLY_5(   60,  -84,  -261,   129,    84,   -36, i1)/den;
        wm[i][-1] = POLY_5( -504,  660,  2133, -1197,  -732,   324, i1)/den;
        wm[i][ 0] = POLY_5(-1128,  604,  4487, -1763, -1488,   564, i1)/den;
        wm[i][ 1] = POLY_5(  168, -292, -1119,   511,   396,  -156, i1)/den;
        wm[i][ 2] = POLY_5(  -36,   72,   160,   -80,   -60,    24, i1)/den;
       #endif
     }
    #endif 

  /* -----------------------------------------------------
     Interpolation weights for spherical geometry
   ------------------------------------------------------- */
    
    #if GEOMETRY == SPHERICAL
     if (d == IDIR) for (i = beg; i <= end; i++){
       rp  = grid[IDIR].xr[i];
       dr  = grid[IDIR].dx[i];
       i1  = fabs(rp/dr);
/*      i1  = (double)(i + (grid[dir].beg-IBEG) - IBEG + 1); */
       i2  = i1*i1;
       #if PPM_VERSION == PPM3
        den = 18.0*POLY_6(4.0, -6.0, -9.0, 20.0, 15.0, -30.0, 10.0, i1);

        wp[i][-2] =  0.0;
        wp[i][-1] =  -POLY_2(7.0, -9.0, 3.0, i1)
                     *POLY_2(3.0, -9.0, 10.0, i2)/den;
        wp[i][ 0] =   POLY_2(1.0, -3.0, 3.0, i1)
                     *POLY_4(69.0, 96.0, -63.0, -90.0, 50.0, i1)/den;
        wp[i][ 1] = 2.0*POLY_2( 1.0,   3.0,  3.0, i1)
                       *POLY_4(12.0, -48.0, 72.0, -45.0, 10.0, i1)/den;
        wp[i][ 2] =  0.0;

        wm[i][-2] =  0.0;
        wm[i][-1] =  2.0*POLY_2(7.0, -9.0, 3.0, i1)
                        *POLY_4(1.0, -1.0, -3.0, 5.0, 10.0, i1)/den;
        wm[i][ 0] =  POLY_2(1.0, -3.0, 3.0, i1)
                    *POLY_4(62.0, 100.0, -33.0, -110.0, 50.0, i1)/den;
        wm[i][ 1] = -POLY_2(1.0, 3.0, 3.0, i1)
                    *POLY_4(4.0, -22.0, 51.0, -40.0, 10.0, i1)/den;
        wm[i][ 2] =  0.0;
       #elif PPM_VERSION == PPM4
        den = 36.0*POLY_4(16.0, -60.0, 150.0, -85.0, 15.0, i2);

        wp[i][-2] =  0.0;
        wp[i][-1] = -POLY_2( 7, -9,  3, i1)/den
                    *POLY_6(12, 16, -30, -48.0, 23, 48, 15, i1);
        wp[i][ 0] =  POLY_2( 1, -3, 3, i1)/den
                    *POLY_6(372, 1008.0, 510, -720, -487, 144, 105, i1);
        wp[i][ 1] =  POLY_2( 1,  3, 3.0, i1)/den
                    *POLY_6(372, -1008, 510, 720, -487, -144, 105, i1);
        wp[i][ 2] = -POLY_2( 7,  9, 3, i1)/den
                    *POLY_6(12, -16, -30, 48, 23, -48, 15, i1);
       #elif PPM_VERSION == PPM5
        den = POLY_10(  48.0,  -48.0, -164.0, 200.0, 390.0, 
                      -399.0, -161.0,  210.0,   0.0, -35.0, 7.0, i1);
        den *= 180.0;
     
        wp[i][-2] = 2.0*POLY_2(19., -15.,  3., i1)/den
                       *POLY_4(16., -60., 94., -45., 7., i2);

        wp[i][-1]  = -POLY_2(7.0, -9.0, 3.0, i1)/den;
        wp[i][-1] *=  POLY_8(508.,  240., -1740., -795., 2417.,
                             930., -780.,  -175.,   91., i1);
        
        wp[i][0]  = POLY_2(1.0, -3.0, 3.0, i1)/den;
        wp[i][0] *= POLY_8(8132., 15120., -5700., -20325., 3863., 
                           8670., -1800., -1225.,    329., i1);

        wp[i][1]  = POLY_2(1.0, 3.0, 3.0, i1)/den;
        wp[i][1] *= POLY_8(4212., -15120., 16560., 1275., -11517.,
                           4350.,   1620., -1225.,  189., i1);

        wp[i][2]  = -POLY_2(7.0, 9.0, 3.0, i1)/den;
        wp[i][2] *=  POLY_8( 108., -240., -120., 645., -223.,
                            -510.,  510., -175.,  21., i1);
        
        wm[i][-2]  = -POLY_2( 19., -15., 3., i1)/den;
        wm[i][-2] *=  POLY_8( 16.,  -16., -60., 96., 222., 
                             -51., -127.,   7., 21., i1);
             
        wm[i][-1]  = POLY_2(  7., -9., 3., i1)/den;
        wm[i][-1] *= POLY_8( 344., -164., -1350., 1184., 4888., 
                            1071., -1663., -287.,  189., i1);

        wm[i][0]  = POLY_2(1.0, -3.0, 3.0, i1)/den;
        wm[i][0] *= POLY_8(7064., 15196.,  -310., -21376., 368., 
                           9431., -1163., -1407.,    329., i1);
                     
        wm[i][1]  = -POLY_2(1.0, 3.0, 3.0, i1)/den;
        wm[i][1] *=  POLY_8( 696., -3516., 6850., -1544., -4388., 
                            2329.,   543., -553.,    91., i1);

        wm[i][2]  = 2.0*POLY_2(7.0, 9.0, 3.0, i1)/den;
        wm[i][2] *= POLY_8(  12., -42.,  25., 132., -91., 
                           -122., 151., -56.,   7., i1);
       #endif /* PPM_VERSION */
     } else if (d == JDIR) {
   
       PPM_FindWeights(wp, wm, d, grid);

     } else if (d == KDIR){
   
       PPM_CartCoeff(wp, wm, beg, end);
     
     }
    #endif /* GEOMETRY == SPHERICAL */

  /* ------------------------------------------------------
      Compute Q6 coefficients
     ------------------------------------------------------ */

    PPM_Q6_Coeffs(hp3D[d], hm3D[d], d, grid);

  } /* End main loop on dimensions */

}

double BetaTheta(double x);
static double gx0;
static int    gk;
/* ********************************************************************* */
void PPM_FindWeights(double **wp, double **wm, int dir, Grid *grid)
/*!
 *   Find the coefficients numerically by inverting the beta matrix
 *   We define the matrix beta[0...n][0...j] so we need to
 *   index it as beta[k][j-jb]
 *
 *********************************************************************** */
{
  int    i, j, k, n;
  int    jb, je, iL, iR, beg, end;
  int    indx[16];
  double rp, rc, rm, vol, d, a[16];
  static double **beta;

/* ----------------------------------------------------
    set the reconstruction stencil & order of accuracy
   ---------------------------------------------------- */

  #if PPM_VERSION == PPM3
   iL = 1; iR = 1;
  #elif PPM_VERSION == PPM4
   iL = 1; iR = 2;
  #elif PPM_VERSION == PPM5
   iL = 2; iR = 2;
  #endif
  n = iR + iL + 1;

  beg = iL;
  end = grid[dir].np_tot - 1 - iR;

  if (beta == NULL) beta = ARRAY_2D(8, 8, double);
  
/* -----------------------------------------------------------------
                Start main loop
   ----------------------------------------------------------------- */

  for (i = beg; i <= end; i++){
  
    rc = grid[dir].x[i];
    jb = i - iL; je = i + iR;
    
    if (dir == IDIR){
      double m;
      #if GEOMETRY == CARTESIAN
       m = 0;
      #elif GEOMETRY == CYLINDRICAL || GEOMETRY == POLAR
       m = 1;
      #elif GEOMETRY == SPHERICAL
       m = 2;
      #endif

      for (j = jb; j <= je; j++){       /* -- stencil loop -- */
        rp = grid[dir].xr[j];
        rm = grid[dir].xl[j];
        vol = (pow(rp,m+1) - pow(rm,m+1))/(m+1.0);
        for (k = 0; k < n; k++) {    /* -- order loop -- */
/*    beta[k][j-jb] = (pow(rp-ri,k+m+1) - pow(rm-ri,k+m+1))/(k+m+1.0)/vol; */

          #if GEOMETRY == CARTESIAN
           beta[k][j-jb] = (pow(rp-rc,k+1) - pow(rm-rc,k+1))/(k+1.0)/vol;
          #elif GEOMETRY == CYLINDRICAL || GEOMETRY == POLAR
           beta[k][j-jb] =  pow(rp-rc,k+1)*((k+1.0)*rp + rc)
                           -pow(rm-rc,k+1)*((k+1.0)*rm + rc);
           beta[k][j-jb] /= (k+2.0)*(k+1.0)*vol;
          #elif GEOMETRY == SPHERICAL
           beta[k][j-jb] =  pow(rp-rc,k+1)*((k*k + 3.0*k + 2.0)*rp*rp
                                            + 2.0*rc*(k+1.0)*rp + 2.0*rc*rc)
                           -pow(rm-rc,k+1)*((k*k + 3.0*k + 2.0)*rm*rm
                                            + 2.0*rc*(k+1.0)*rm + 2.0*rc*rc);

           beta[k][j-jb] /= (k+3.0)*(k+2.0)*(k+1.0)*vol;
          #endif
        }
      }
    } else if (dir == JDIR){
      if (n > 5) {
        print1 ("! Order of accuracy too large for jdir in PPM_FindWeights\n");
        QUIT_PLUTO(1);
      }
      for (j = jb; j <= je; j++){       /* -- stencil loop -- */
        #if GEOMETRY == SPHERICAL
         double zp, zm;
         double zp2, zm2;
         
         rp  = grid[dir].xr[j];
         rm  = grid[dir].xl[j];
         vol = cos(rm) - cos(rp);
   
         zp = rp - rc; zp2 = zp*zp;
         zm = rm - rc; zm2 = zm*zm;
         
       /* Table of integral, page 216, equation 2.635 */
       
         beta[0][j-jb] = vol;
         beta[1][j-jb] =  (-zp*cos(rp) + sin(rp)) 
                         -(-zm*cos(rm) + sin(rm));
         beta[2][j-jb] =  ((2.0-zp2)*cos(rp) + 2.0*zp*sin(rp))
                         -((2.0-zm2)*cos(rm) + 2.0*zm*sin(rm));
         beta[3][j-jb] =  (zp*(6.0-zp2)*cos(rp) + 3.0*(zp2-2.0)*sin(rp))
                         -(zm*(6.0-zm2)*cos(rm) + 3.0*(zm2-2.0)*sin(rm));
         beta[4][j-jb] =  ((-24.0 + zp2*(12.0-zp2))*cos(rp)
                                  + 4.0*zp*(zp2-6.0)*sin(rp))
                         -((-24.0 + zm2*(12.0 - zm2))*cos(rm)
                                  + 4.0*zm*(zm2-6.0)*sin(rm));

gx0 = rc;
for (gk = 0; gk < n; gk++){
double scrh;
scrh = GaussQuadrature(&BetaTheta, rm, rp, 1, 5);         
/*
if (fabs(scrh/beta[gk][j-jb]-1.0) > 1.e-4){
  printf ("gk = %d, I = %12.6e, beta = %12.6e, err = %12.6e\n",
           gk, scrh, beta[gk][j-jb], fabs(scrh-beta[gk][j-jb]));
  QUIT_PLUTO(1);
}
*/
beta[gk][j-jb] = scrh;         

}

         beta[0][j-jb] /= vol;
         beta[1][j-jb] /= vol;
         beta[2][j-jb] /= vol;
         beta[3][j-jb] /= vol;
         beta[4][j-jb] /= vol;

        #else
        #endif
      }
    } else {
      print1 ("! PPM_FindWeights: not defined for x3 direction\n");
      QUIT_PLUTO(1);
    }  
    
    LUDecompose(beta, n, indx, &d);

    rp = grid[dir].xr[i];
    rm = grid[dir].xl[i];

    /* ---------------------------------------------------------
        Solve B.w = xi^k
       --------------------------------------------------------- */
       
    a[0] = 1.0;
    for (k = 1; k < n; k++) a[k] = a[k-1]*(rp - rc);
    LUBackSubst (beta, n, indx, a);
    for (j = 0; j < n; j++) {
/*
      if (fabs(a[j]/wp[i][-iL+j]-1.0) > 1.e-9){
        printf ("! Err, i = %d, j = %d, Wp = %18.10e, a = %18.10e\n",
                i, j, wp[i][-iL+j], a[j]);
        QUIT_PLUTO(1);
      }
*/
      wp[i][-iL+j] = a[j];
    }

    #if PPM_VERSION != PPM4
     a[0] = 1.0;
     for (k = 1; k < n; k++) a[k] = a[k-1]*(rm - rc);
     LUBackSubst (beta, n, indx, a);
     for (j = 0; j < n; j++) {
/*
       if (fabs(a[j]/wm[i][-iL+j]-1.0) > 1.e-9){
         printf ("! Err, i = %d, j = %d, Wm = %12.6e, a = %12.6e\n",
                  i, j, wm[i][-iL+j], a[j]);
         QUIT_PLUTO(1);
       }
*/
       wm[i][-iL+j] = a[j];
     }
    #endif
  } /* -- end loop on zones -- */
}

/* ********************************************************************* */
void PPM_CartCoeff(double **wp, double **wm, int beg, int end)
/*!
 *  Compute the standard PPM weigth coefficients for a uniformly 
 *  spaced Cartesian grid. 
 *
 *********************************************************************** */
{
  int i;
  
  for (i = beg; i <= end; i++){
    #if PPM_VERSION == PPM3 
     wp[i][-2] =  0.0;
     wp[i][-1] = -1.0/6.0;
     wp[i][ 0] =  5.0/6.0;
     wp[i][ 1] =  1.0/3.0;
     wp[i][ 2] =  0.0;

     wm[i][-2] =  0.0;
     wm[i][-1] =  1.0/3.0;
     wm[i][ 0] =  5.0/6.0;
     wm[i][ 1] = -1.0/6.0;
     wm[i][ 2] =  0.0;
    #elif PPM_VERSION == PPM4
     wp[i][-2] =  0.0;
     wp[i][-1] = -1.0/12.0;
     wp[i][ 0] =  7.0/12.0;
     wp[i][ 1] =  7.0/12.0;
     wp[i][ 2] = -1.0/12.0;
    #elif PPM_VERSION == PPM5
     wp[i][-2] =   1.0/30.0;
     wp[i][-1] = -13.0/60.0;
     wp[i][ 0] =  47.0/60.0;
     wp[i][ 1] =   9.0/20.0;
     wp[i][ 2] =  -1.0/20.0;

     wm[i][-2] =  -1.0/20.0;
     wm[i][-1] =   9.0/20.0;
     wm[i][ 0] =  47.0/60.0;
     wm[i][ 1] = -13.0/60.0;
     wm[i][ 2] =   1.0/30.0;
    #endif
  }
}

double BetaTheta(double x)
{
  return pow(x - gx0, gk)*sin(x);
}

/* ********************************************************************* */
void PPM_Q6_Coeffs(double *hp, double *hm, int dir, Grid *grid)
/*
 *
 *
 *********************************************************************** */
{
  int i, beg, end;
   
  beg = 0;
  end = grid[dir].np_tot-1;
   
  if (dir == IDIR){
    double den, *r, *dr;
     
    r  = grid[IDIR].x;
    dr = grid[IDIR].dx;
    for (i = beg; i <= end; i++){
      #if GEOMETRY == CARTESIAN
       hp[i] = 3.0;
       hm[i] = 3.0;
      #elif GEOMETRY == CYLINDRICAL
       hp[i] = 3.0 + 0.5*dr[i]/r[i];
       hm[i] = 3.0 - 0.5*dr[i]/r[i];
      #elif GEOMETRY == SPHERICAL
       den   = 20.0*r[i]*r[i] + dr[i]*dr[i];
       hp[i] = 3.0 + 2.0*dr[i]*(10.0*r[i] + dr[i])/den;
       hm[i] = 3.0 - 2.0*dr[i]*(10.0*r[i] - dr[i])/den;
      #endif
    }
  }else if (dir == JDIR){
    double *th, *dth, *thp, cp, cm, sp, sm;
    double dmu, dmu_t;
    
    th  = grid[JDIR].x;
    thp = grid[JDIR].xr;
    dth = grid[JDIR].dx;
    for (i = beg; i <= end; i++){
      #if GEOMETRY != SPHERICAL
       hp[i] = 3.0;
       hm[i] = 3.0;
      #else
       cp = cos(thp[i]);   sp = sin(thp[i]);
       cm = cos(thp[i-1]); sm = sin(thp[i-1]);
       dmu   = cm - cp; 
       dmu_t = sm - sp;
       hp[i] =  dth[i]*(dmu_t + dth[i]*cp)/(dth[i]*(sp + sm) - 2.0*dmu);
       hm[i] = -dth[i]*(dmu_t + dth[i]*cm)/(dth[i]*(sp + sm) - 2.0*dmu);
      #endif
    }
  }else if (dir == KDIR){
    for (i = beg; i <= end; i++){
      hp[i] = 3.0;
      hm[i] = 3.0;
    }
  }
}
 
/* ********************************************************************* */
void PPM_CoefficientsGet(PPM_Coeffs *ppm_coeffs, int dir)
/*!
 *  Retrieve interpolation coefficients for parabolic reconstruction.
 *  This function can be called only if the previous one has been
 *  completed already.
 *
 * \param[in] grid      pointer to array of grid structure
 *
 * \return  a pointer to a static structure containing the coefficients
 *          (actually pointers to array of coefficients) in the
 *          direction given by ::g_dir.
 *********************************************************************** */
{
  if (Wp3D == NULL) {
    print1 ("! PPM_Coefficients: coefficients not set.\n");
    QUIT_PLUTO(1);
  }

  ppm_coeffs->wp = Wp3D[dir];
  ppm_coeffs->wm = Wm3D[dir];

  ppm_coeffs->hp = hp3D[dir];
  ppm_coeffs->hm = hm3D[dir];

}
