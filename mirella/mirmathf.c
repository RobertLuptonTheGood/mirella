/*version 2007/05/23 */
/********************** MIRMATHF.C *************************************/
/* Miscellaneous mathematical functions */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

/* modified Bessel function of the first kind, order zero */
/******************** i0(x) **********************************************/

mirella double
i0(double x)
{
    double f;
    double t = x/3.75;
    double t2;
    double ti;

    x = fabs(x);
    if (fabs(t) <= 1.){
        t2 = t*t;
        f = 1. + t2*(3.5156229 + t2*(3.0899424 + t2*(1.2067492 + t2*(
                0.2659732 + t2*(0.0360768 + t2*0.0045813)))));
    }else{
        ti = fabs(1./t);        
        f = 0.39894228 + ti*(0.01328592 + ti*(0.00225319 + ti*(
            -0.00157565 + ti*(0.00916281 + ti*(-0.02057706 + ti*(
            0.02635537 + ti*(-.01647633 + ti*0.00392377)))))));
        f = f*exp(x)/sqrt(x);
    }
    return f;
}

/* this fn is i0(x)exp(-x), which is quite  well behaved */

mirella double
i0emx(double x)
{
    double f;
    double t = x/3.75;
    double t2;
    double ti;
    
    x = fabs(x);
    if (fabs(t) <= 1.){
        t2 = t*t;
        f = 1. + t2*(3.5156229 + t2*(3.0899424 + t2*(1.2067492 + t2*(
                0.2659732 + t2*(0.0360768 + t2*0.0045813)))));
        f *= exp(-x);
    }else{
        ti = fabs(1./t);        
        f = 0.39894228 + ti*(0.01328592 + ti*(0.00225319 + ti*(
            -0.00157565 + ti*(0.00916281 + ti*(-0.02057706 + ti*(
            0.02635537 + ti*(-.01647633 + ti*0.00392377)))))));
        f = f/sqrt(x);
    }
    return f;
}


/* Modied Bessel function i1(x) for any real x */

mirella double 
i1(double x)
{
    double ax,ans;
    double y; 
    if ((ax=fabs(x)) < 3.75){ /* use polynomial */
        y=x/3.75;
        y*=y;
        ans=ax*(0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
            +y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3))))));
    } else {
        y=3.75/ax;
        ans=0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
            -y*0.420059e-2));
        ans=0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
            +y*(0.163801e-2+y*(-0.1031555e-1+y*ans))));
        ans *= (exp(ax)/sqrt(ax));
    }
    return x < 0.0 ? -ans : ans;
}

/* i1(x)exp(-x); well behaved */
mirella double 
i1emx(double x)
{
    double ax,ans;
    double y; 
    if ((ax=fabs(x)) < 3.75){ /* use polynomial */
        y=x/3.75;
        y*=y;
        ans=ax*(0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
            +y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3))))));
        ans *= exp(-ax);
    } else {
        y=3.75/ax;
        ans=0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
            -y*0.420059e-2));
        ans=0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
            +y*(0.163801e-2+y*(-0.1031555e-1+y*ans))));
        ans = ans/sqrt(ax);
    }
    return x < 0.0 ? -ans : ans;
}

/* modified bessel function i2(x) from recursion with i0 and i1 */

mirella double 
i2(double x)
{
    double ans;
    
    ans = (-2./x)*i1(x) + i0(x);
    return ans;
}

/* i2(x)exp(-x); well behaved for all x */
   
mirella double 
i2emx(double x)
{
    double ans;
    
    ans = (-2./x)*i1emx(x) + i0emx(x);
    return ans;
}
   

/* this routine returns i_n(x); developed from backward recursion and is
 * slow.
 */
 
/*Make larger to increase accuracy; ACC generates the starting order for recur*/
#define ACC 40.0 
#define BIGNO 1.0e10
#define BIGNI 1.0e-10

/* Returns the modied Bessel function i_ n(x) for any real x and n>= 2 */

mirella double
i_n(int n, double x)
{
    double i0();
    int j;
    double bi,bim,bip,tox,ans;
    if (n < 2) erret("Index n less than 2 in bessi");
    
    if (x == 0.0) { 
        return 0.0;
    } else {
        tox=2.0/fabs(x);
        bip=ans=0.0;
        bi=1.0;    
        for (j=2*(n+(int)sqrt(ACC*n));j>0;j--) { /*Down recur from even m*/
            bim=bip+j*tox*bi;
            bip=bi;
            bi=bim;
            if (fabs(bi) > BIGNO) {  /*Renormalize to prevent overoows. */
                ans *= BIGNI;
                bi *= BIGNI;
                bip *= BIGNI;
            }
            if (j == n) ans=bip;
        }
        ans *= i0(x)/bi; /* Normalize with bessi0 */
        return x < 0.0 && (n & 1) ? -ans : ans;
    }
}

/**************SIMPLE FUNCTIONS *********************************************/

mirella double
log2(double x)
{
    return log(x)/LN2;
}
