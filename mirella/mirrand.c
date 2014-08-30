/********************* MIRRAND.C ******************************************/
/* Numerical recipes ran2 and ran3 */
/* 94/02/15 */

#include "mirella.h"

/********************** RAN2(),IRAN2() ***********************************/
/* 
 * The routine has been modified to take no argument and to return an int
 * in the range [0,65535] ; it can be seeded with the routine seed2(seed) 
 * at any time. ran2() returns a double as in the
 * original (actually, the original returns a float, and the routine
 * has only float-accuracy resolution, so beware).
 * NB!!! the seed must be negative
 */

#define IM1 2147483563
#define IM2 2147483399
#define AM (1.0/IM1)
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

static long idum2 = -65535;
static long iy2=0;

void seed2(int id)
{
    idum2 = id;
}

int iran2(void)
{
    int j;
    long k;
    static long idumu=123456789;
    static long iv[NTAB];
    static long iy = 0;
    long *idum = &idum2;

    if (*idum <= 0) {
        if (-(*idum) < 1) *idum=1;
        else *idum = -(*idum);
        idumu=(*idum);
        for (j=NTAB+7;j>=0;j--) {
            k=(*idum)/IQ1;
            *idum=IA1*(*idum-k*IQ1)-k*IR1;
            if (*idum < 0) *idum += IM1;
            if (j < NTAB) iv[j] = *idum;
        }
        iy=iv[0];
    }
    k=(*idum)/IQ1;
    *idum=IA1*(*idum-k*IQ1)-k*IR1;
    if (*idum < 0) *idum += IM1;
    k=idumu/IQ2;
    idumu=IA2*(idumu-k*IQ2)-k*IR2;
    if (idumu < 0) idumu += IM2;
    j=iy/NDIV;
    iy=iv[j]-idumu;
    iv[j] = *idum;
    if (iy < 1) iy += IMM1;
    iy2 = iy;
    return (iy >> 15);
}

double 
ran2(void)
{
    return (iran2(),iy2*AM);
}

#undef IM1
#undef IM2
#undef AM
#undef IMM1
#undef IA1
#undef IA2
#undef IQ1
#undef IQ2
#undef IR1
#undef IR2
#undef NTAB
#undef NDIV
#undef EPS
#undef RNMX


/********************** IRAN3() *******************************************/
/* 
 * The routine has been modified to take no argument and to return an int
 * in the range [0,65535] ; it can be seeded with the routine seed3(seed) 
 * at any time. ran3() returns a float as in the
 * original (actually, a double, but with only float resolution--beware).
 * I think this is broken--the redefinition of MBIG screwed things up?
 * jeg071106 it seems indeed to be broken--setting seed3 does not result
 * in deterministic output. FIXME
 * jeg071108: maybe..the seed must be NEGATIVE
 */

/*#define MBIG 1000000000*/
#define MBIG 1073741824
   /* 2^30 */
#define MSEED 161803398
#define MZ 0
#define FAC (1.0/MBIG)

static long idum3 = -32767;
static int iff3=0;
static long mj3;

void seed3(int id)
{
    idum3 = id;
}

int 
iran3(void)
{
    static int inext,inextp;
    static long ma[56];

    long *idum = &idum3;
    long mk;
    int i,ii,k;

    if (*idum < 0 || iff3 == 0) {
        iff3=1;
        mj3=MSEED-(*idum < 0 ? -*idum : *idum);
        mj3 %= MBIG;
        ma[55]=mj3;
        mk=1;
        for (i=1;i<=54;i++) {
            ii=(21*i) % 55;
            ma[ii]=mk;
            mk=mj3-mk;
            if (mk < MZ) mk += MBIG;
            mj3=ma[ii];
        }
        for (k=1;k<=4;k++)
            for (i=1;i<=55;i++) {
                ma[i] -= ma[1+(i+30) % 55];
                if (ma[i] < MZ) ma[i] += MBIG;
            }
        inext=0;
        inextp=31;
        *idum=1;
    }
    if (++inext == 56) inext=1;
    if (++inextp == 56) inextp=1;
    mj3=ma[inext]-ma[inextp];
    if (mj3 < MZ) mj3 += MBIG;
    ma[inext]=mj3;
    return (mj3 >> 14);
}

double
ran3(void)
{
    return(iran3(),mj3*FAC);
}

#undef MBIG
#undef MSEED
#undef MZ
#undef FAC

/************************ NDF() *******************************************/
/* normal distribution function */

double 
ndf(double x)
{
    double erf();        
    return (0.5*erf(x * 0.70710678) + 0.5);
}

/************************ NPD() *******************************************/
/* normal probability density */

double
npd(double x)
{
    return exp(-0.5*x*x)*0.39894228;
}

/************************ NDFI() ******************************************/
/* normal distribution function inverse; this is slow; uses newton-raphson */

#define EPS 1.e-6

double
ndfi(double y)
{
    double x,xo;
    int i=0;
    
    x = (y-0.5)*2.50662828;
    do{
        xo = x;
        x = x + (y - ndf(x))/npd(x);
        i++;
        /*debug*/ if(i > 1000) break;
    }while(fabs(x - xo) > EPS);
/*debug mprintf("\nx=%7.6f, %d iterations",x,i); */
    return x;
}
    
/********************* GAUSITAB ***************************************/
/* constructs table of abscissae for the generation of pseudogaussian
 * random numbers
 */
  
#define NGTAB 1023
#define NGTABP NGTAB+1
#define LNGTAB 8  
    /* log2 of NGTAB+1, which must be a power of 2 */

static float gitable[NGTAB];

static void
gausitab()
{
    int i;
    float y;
    
    for(i=0;i<NGTAB;i++){
        y = ((float)(2*i+1))/(4.*((float)(NGTAB))) + 0.5;
        gitable[i] = ndfi(y);
    }
    /*debug mprintf("\ngitable=%d ",gitable); */
}

/******************* NRAND() ******************************************/
/* returns pseudonormal random number; interpolates linearly among 2*NGTAB
 * distinct values; all the float arithmetic makes it SLOW; try redoing it
 * with scaled ints--65k distinct values should be enough, and if not,
 * iran2() can be fixed up to yield more.
 */

double
nrand()
{
    static int firstime = 1;
    float rnd = ran2();
    float ret;
    float dx = 2. * (rnd - 0.5) * NGTAB;
    int idx;
    int pos = 1;

    if(firstime){
        gausitab();
        firstime = 0;
    }
    if(rnd < 0.5) pos = 0;
    dx = fabs(dx) - 0.5;     /* range -0.5 to NGTAB-0.5 */
    idx = round(dx);
    if(idx < 0 ) idx = 0;
    if(idx > NGTAB-1) idx = NGTAB-1;   /* float vagaries may do this */
    dx = dx - idx;           /* fractional part */
    if(dx > 0.){
        if(idx != NGTABP-1){
            ret = (1. - dx)*gitable[idx]  + (dx * gitable[idx+1]);
        }else{  /* extrapolate */
            ret = (1. + dx)*gitable[idx] - (dx * gitable[idx-1]);
        }
    }else{    /* dx <= 0 */
        if(idx != 0){
            ret = (1. + dx)*gitable[idx] - (dx * gitable[idx-1]);
        }else{  /* idx = 0; the -0.5 entry is 0, so */
            ret = (1. + dx)*gitable[0];
        }
    }
    /*debug mprintf("\nidx=%d ",idx); */
    return  ( pos ? ret : -ret );
}
        

/********************* END MODULE MIRRAND.C ********************************/
