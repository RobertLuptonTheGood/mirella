/*VERSION 91/03/16:Mirella 5.50                                         */

/********************** MIRARRAY.C **************************************/
/* Mirella array manipulation code */

/* recent history:
 * 88/08/31: added code to deal with machines which shift logically
 * instead of arithmetically (LOGCSHIFT)
 * added escape routes from faprt, etc.
 */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

/****************** N(S)(F)AVERAGE(), N(S)(F)SUM() ***********************/
/* sum arrays */

mirella normal 
narsum(buf,n)          /* YOU have to check for overflow */
register normal *buf;
register normal n;
{
    register normal sum = 0;
    
    while(n--) sum += *buf++;
    return sum;
}

mirella normal 
sarsum(buf,n)
register short int *buf;
register normal n;
{
    register normal sum = 0;
    
    while(n--) sum += *buf++;
    return sum;
}

mirella double 
farsum(buf,n)
register float *buf;
register normal n;
{
    register float sum = 0.;
    
    while(n--) sum += *buf++;
    return sum;
}

mirella double
naverage(buf,n)
normal *buf;
normal n;
{
   return ( (float)(narsum(buf,n))/(float)n );
}

mirella double
saverage(buf,n)
short int *buf;
normal n;
{
   return ( (float)(sarsum(buf,n))/(float)n );
}

mirella double
faverage(buf,n)
float *buf;
normal n;
{
   return (farsum(buf,n)/(float)n);
}

/********************* N(S)(F)ARADD() ******************************/
/* sum two arrays */

mirella void 
naradd(src,dest,n)   /* adds src to dest */
register normal *src;
register normal *dest;
register normal n;
{
    while(n--) *dest++ += *src++;
}


mirella void 
saradd(src,dest,n)			/* adds src to dest */
register short int *src;
register short int *dest;
register normal n;
{
    while(n--) *dest++ += *src++;
}

mirella void 
faradd(src,dest,n)   /* adds src to dest */
register float *src;
register float *dest;
register normal n;
{
    while(n--) *dest++ += *src++;
}

/***********************N(S)(F)ARFMUL(), N(S)(F)ARFACCUM() ******************/
/* multiply array by a float constant */
mirella void 
sarfmul(buf,n,fac)  /* mpys buf by fac (to 12 bits)*/
register short int *buf;
register normal n;
double fac;
{
    register normal ifac = (normal)(fac*4096.);
    while(n--){
#ifndef LOGCSHIFT
        *buf = (*buf * ifac)>>12;
#else
        *buf = (*buf * ifac)/4096;
#endif
        buf++;
    }
}

mirella void 
narfmul(buf,n,fac)  /* mpys buf by fac (float conversion; slow) */
normal *buf;
normal n;
double fac;
{
    buf--;
    while(n--){
        *buf = (normal)((float)(*buf) * fac);
        buf++;
    }
}

mirella void 
farfmul(buf,n,fac)  /* mpys buf by fac */
register float *buf;
register normal n;
double fac;
{
    while(n--) (*buf++) *= fac;
}

/* accumulate linear combination : dest += fac*src */

mirella void 
sarfaccum(src, dest, n, fac)  /* mpys src by fac(12 bits), adds to dest */
register short int *src;
register short int *dest;
register normal n;
double fac;
{
    register normal ifac = (normal)(fac*4096.);
#ifndef LOGCSHIFT
    while(n--) *dest++ += (*src++ * ifac)>>12;
#else
    while(n--) *dest++ += (*src++ * ifac)/4096;
#endif
}

mirella void 
narfaccum(src, dest, n, fac) 
register normal *src;
register normal *dest;
register normal n;
register double fac;
{
    while(n--) *dest++ += (normal)((float)(*src++) * fac);
}

mirella void 
farfaccum(src, dest, n, fac) 
register float *src;
register float *dest;
register normal n;
register double fac;
{
    while(n--) *dest++ += (*src++ * fac);
}

/************************ N(S)(F)ARCON() ********************************/
/* add a constant to an array */

mirella void sarcon(buf,n,c)
register short int *buf;
register normal n;
register int c;
{
    while(n--) (*buf++) += c;
}

mirella void
narcon(buf,n,c)
register normal *buf;
register normal n;
register normal c;
{
    while(n--) (*buf++) += c;
}

mirella void
farcon(buf,n,c)
register float *buf;
register normal n;
register double c;
{
    while(n--) (*buf++) += c;
}

/**************************** N(S)(F)AMAXMIN() ********************************/
/* find minima and maxima of arrays and indices thereof. The mirella
procedures push all that stuff */

/*
 * finds maximum and minimum and indices in the short int array buf
 */
void
samaxmin(buf,n,pmin,pmax,pmindx,pmaxdx)
short int *buf;            /* short int buffer */
normal n;                  /* length */
normal *pmin,*pmax;        /* pointers to the min and max */
normal *pmindx,*pmaxdx;    /* pointers to the indices of the min and max */
{
    register short int *bptr = buf - 1;
    register short int *bend = buf + n;
    register int min = buf[0];
    register int max = buf[0];
    *pmaxdx = *pmindx = 0;
    
    while(++bptr < bend){
        if( *bptr > max){
            max = *bptr;
            *pmaxdx = bptr - buf;
        }else if( *bptr < min){
            min = *bptr;
            *pmindx = bptr - buf;
        }
    }
    *pmin = min;
    *pmax = max;
}

/*
 * finds maximum and minimum and indices in the normal array buf
 */
void
namaxmin(buf,n,pmin,pmax,pmindx,pmaxdx)
normal *buf;               /* normal buffer */
normal n;                  /* length */
normal *pmin,*pmax;        /* pointers to the min and max */
normal *pmindx,*pmaxdx;    /* pointers to the indices of the min and max */
{
    register normal *bptr = buf - 1;
    register normal *bend = buf + n;
    register normal min = buf[0];
    register normal max = buf[0];
    *pmaxdx = *pmindx = 0;
    
    while(++bptr < bend){
        if( *bptr > max){
            max = *bptr;
            *pmaxdx = bptr - buf;
        }else if( *bptr < min){
            min = *bptr;
            *pmindx = bptr - buf;
        }
    }
    *pmin = min;
    *pmax = max;
}

/*
 * finds maximum and minimum and indices in the float array buf
 */
void
famaxmin(buf,n,pmin,pmax,pmindx,pmaxdx)
float *buf;             /* float buffer */
normal n;               /* length */
float *pmin,*pmax;      /* pointers to the min and max */
normal *pmindx,*pmaxdx; /* pointers to the indices of the min and max */
{
    register float *bptr = buf - 1;
    register float *bend = buf + n;
    float  min = buf[0];
    float  max = buf[0];
    *pmaxdx = *pmindx = 0;
    
    while(++bptr < bend){
        if( *bptr > max){
            max = *bptr;
            *pmaxdx = bptr - buf;
        }else if( *bptr < min){
            min = *bptr;
            *pmindx = bptr - buf;
        }
    }
    *pmin = min;
    *pmax = max;
}

mirella void  /* maxmin proc for n arrays: ( buf n -- min max mindx maxdx )*/
_namaxmin()
{
    normal n = pop;
    normal *buf = (normal *)pop;
    normal min,max;
    normal mindx,maxdx;
    namaxmin(buf,n,&min,&max,&mindx,&maxdx);
    push(min);
    push(max);
    push(mindx);
    push(maxdx);
}

mirella void  /* maxmin proc for sarrays: ( buf n -- min max mindx maxdx )*/
_samaxmin()
{
    normal n = pop;
    short int *buf = (short int *)pop;
    normal min,max;
    normal mindx,maxdx;
    samaxmin(buf,n,&min,&max,&mindx,&maxdx);
    push(min);
    push(max);
    push(mindx);
    push(maxdx);
}

mirella void  /* maxmin proc for farrays: ( buf n -- fmin fmax mindx maxdx )*/
_famaxmin()
{
    normal n = pop;
    float *buf = (float *)pop;
    float fmin,fmax;
    normal mindx,maxdx;
    famaxmin(buf,n,&fmin,&fmax,&mindx,&maxdx);
    fpush(fmin);
    fpush(fmax);
    push(mindx);
    push(maxdx);
}

/************************* SIMPLE MAX AND MIN FUNCTIONS *********************/
/* these are versions of the above with simpler calling sequences */

mirella normal
samax(buf, n)
short *buf;
normal n;
{
    normal scr;
    normal max;
    normal min;
    samaxmin(buf, n, &min, &max, &scr, &scr);
    return max;
}

mirella normal 
namax(buf, n)
normal *buf;
normal n;
{
    normal scr;
    normal min;
    normal max;
    namaxmin(buf, n, &min, &max, &scr, &scr );
    return max;
}

mirella double
famax(buf, n)
float *buf;
normal n;
{
    normal scr;
    float max;
    float min;
    famaxmin(buf, n, &min, &max, &scr, &scr);
    return (double)max;
}

mirella double
famin(buf, n)
float *buf;
normal n;
{
    normal scr;
    float max;
    float min;
    famaxmin(buf, n, &min, &max, &scr, &scr);
    return (double)min;
}
    
/************************ N(S)(F)ARIMAX() ***********************************/
/* finds and returns the (float) interpolated index of maximum in the 
   array buf of the approproate type; returns maximum in *pmax if pmax
   is nonzero. returns -1. if maximum is outside array */

/*jeg9909
   routines do local quadratic interpolation; we could probably do better
   for largish arrays. should spline or fall back to quadratic if n=3 */
    
mirella double 
narimax(buf,n,pmax)    
    normal *buf;
    int n;
    float *pmax;
{
    normal amax,amin;
    normal imax,imin;
    float a,b,c;
    float xm,am;
    
    namaxmin(buf,n,&amin,&amax,&imin,&imax);  /* find the maxima and minima*/
    if(imax == 0) imax = 1;
    if(imax == n-1) imax = n-2;    /* move away from endpoints */
    b = (buf[imax+1] - buf[imax-1])*0.5;
    a = (buf[imax+1] - 2.*buf[imax] + buf[imax-1])*0.5;
    c = buf[imax];
    xm = -b/(2.*a);
    am = xm*(b + xm*a) + c;
    xm += (float)imax;
    if(pmax) *pmax = am;
    if(xm < 0. || xm > (float)(n-1)) xm = -1.;
    return xm;
}

mirella double 
sarimax(buf,n,pmax)    
    short int *buf;
    int n;
    float *pmax;
{
    normal amax,amin;
    normal imax,imin;
    float a,b,c;
    float xm,am;
    
    samaxmin(buf,n,&amin,&amax,&imin,&imax);  /* find the maxima and minima*/
    if(imax == 0) imax = 1;
    if(imax == n-1) imax = n-2;    /* move away from endpoints */
    b = (buf[imax+1] - buf[imax-1])*0.5;
    a = (buf[imax+1] - 2.*buf[imax] + buf[imax-1])*0.5;
    c = buf[imax];
    xm = -b/(2.*a);
    am = xm*(b + xm*a) + c;
    xm += (float)imax;
    if(pmax) *pmax = am;
    if(xm < 0. || xm > (float)(n-1)) xm = -1.;
    return xm;
}

mirella double 
farimax(buf,n,pmax)    
    float *buf;
    int n;
    float *pmax;
{
    float amax,amin;
    normal imax,imin;
    float a,b,c;
    float xm,am;
    
    famaxmin(buf,n,&amin,&amax,&imin,&imax);  /* find the maxima and minima*/
    if(imax == 0) imax = 1;
    if(imax == n-1) imax = n-2;    /* move away from endpoints */
    b = (buf[imax+1] - buf[imax-1])*0.5;
    a = (buf[imax+1] - 2.*buf[imax] + buf[imax-1])*0.5;
    c = buf[imax];
    xm = -b/(2.*a);
    am = xm*(b + xm*a) + c;
    xm += (float)imax;
    if(pmax) *pmax = am;
    if(xm < 0. || xm > (float)(n-1)) xm = -1.;
    return xm;
}

/* added jeg9909; add others if necessary, but I think not */
mirella double
farimin(buf,n,pmin)
    float *buf;
    int n;
    float *pmin;
{
    float amax,amin;
    normal imax,imin;
    float a,b,c;
    float xm,am;
                                
    famaxmin(buf,n,&amin,&amax,&imin,&imax);  /* find the maxima and minima*/
    if(imin == 0) imin = 1;
    if(imin == n-1) imin = n-2;    /* move away from endpoints */
    b = (buf[imin+1] - buf[imin-1])*0.5;
    a = (buf[imin+1] - 2.*buf[imin] + buf[imin-1])*0.5;
    c = buf[imin];
    xm = -b/(2.*a);
    am = xm*(b + xm*a) + c;
    xm += (float)imin;
    if(pmin) *pmin = am;
    if(xm < 0. || xm > (float)(n-1)) xm = -1.;
    return xm;
}

                                                                            
/************************ MIRELLA PROCEDURES FOR ABOVE **********************/

mirella void _narimax()
{
    normal n = pop;
    normal *buf = (normal *)pop;
    float amax,imax;
    
    imax = narimax(buf,n,&amax);
    fpush(imax);
    fpush(amax);
}

mirella void _sarimax()
{
    normal n = pop;
    short int *buf = (short int *)pop;
    float amax,imax;
    
    imax = sarimax(buf,n,&amax);
    fpush(imax);
    fpush(amax);
}

mirella void _farimax()
{
    normal n = pop;
    float *buf = (float *)pop;
    float amax,imax;
    
    imax = farimax(buf,n,&amax);
    fpush(imax);
    fpush(amax);
}

mirella void _farimin()
{
    normal n = pop;
    float *buf = (float *)pop;
    float amax,imax;
                
    imax = farimin(buf,n,&amax);
    fpush(imax);
    fpush(amax);
}
                        
/************************N(S)(F)ASOLVE() *******************************/
/* finds the linearly interpolated index at which an array crosses some value;
   does direct search, not clever. All return 0.0 if no solution. CAREFUL */
   
/*
 * returns float index at which interpolated normal array first crosses
 * value, 0.0 if no solution. CAREFUL
 */
mirella double 
nasolve(buf,n,value)
normal *buf;
normal n;
register normal value;
{
    normal dir = value - buf[0];
    register normal *bptr = buf-1;
    register normal *bend = buf + n;

    if(dir == 0) return ( 0.0 ) ;    
    if(dir > 0){
        while(++bptr < bend && *bptr < value) continue;
    }else{
        while(++bptr < bend && *bptr > value) continue;
    }
    if(bptr == bend) return ( 0. );
    return (  (float)(value - *(bptr-1)) / (float)(*bptr - *(bptr-1)) 
            + (float)(bptr - buf -1) );
}

/*
 * returns float index at which interpolated short array first crosses
 * value, 0.0 if no solution. CAREFUL
 */
mirella double 
sasolve(buf,n,value)
short int *buf;
normal n;
register int value;
{
    int dir = value - buf[0];
    register short int *bptr = buf-1;
    register short int *bend = buf + n;

    if(dir == 0) return ( 0.0 ) ;    
    if(dir > 0){
        while(++bptr < bend && *bptr < value) continue;
    }else{
        while(++bptr < bend && *bptr > value) continue;
    }
    if(bptr == bend) return ( 0. );
    return (  (double)(value - *(bptr-1)) / (double)(*bptr - *(bptr-1)) 
            + (double)(bptr - buf -1) );
}

/*
 * returns float index at which interpolated float array first crosses value,
 * 0.0 if no solution. CAREFUL. Added error message if runs off end
 * THE DIRECTION LOGIC IS CRAP. FIX IT, AND OTHER WORDS
 */
mirella double 
fasolve(buf,n,value)
float *buf;
normal n;
register double value;
{
    float dir = value - buf[0];
    register float *bptr = buf-1;
    register float *bend = buf + n;

    if(dir == 0.) return ( 0.0 ) ;    
    if(dir > 0.){
        while(++bptr < bend && *bptr < value) continue;
    }else{
        while(++bptr < bend && *bptr > value) continue;
    }
    if(bptr == bend){
        mprintf("\n%s","FASOLVE: reached end of array: no solution");
        return ( *(bend - 1) );
    }
    return ( (value - *(bptr-1)) / (*bptr - *(bptr-1)) 
            + (double)(bptr - buf -1) );
}

/******************** FQUADSOLVE() ***********************************/
/* jeg9812: This routine takes monotonic abscissa and ordinate arrays,
 * and, given an ordinate, solves for & rets the float abscissa. Uses
 * circe and does a quadratic solution for ordinate as a function of
 * abscissa. Could be done more simply the other way around, but MAY
 * be less well behaved. Depends. If you just want to interpolate, use
 * fquadinterp, below; uses quadratic interpolation but is otherwise
 * identical.
 */
 
mirella double 
fquadsolve(y,xar,yar,n)
float *xar;
float *yar;
double y;
int n;
{
    int idx;
    int i0, ip, im;
    float y0, yp, ym;
    float x0, xp, xm;
    float det, a,b;
    float dy;
    float dx;
    
    idx = circe(y,yar,n);  /* soln is between idx, idx-1    */
    if(idx == 1){
        im=0; i0=1; ip=2;
    }else if(idx == n-1){
        im=n-3; i0=n-2; ip=n-1;
    }else{
        if(fabs(y-yar[idx]) < fabs(y-yar[idx-1])){  /* closest pt is idx */
            im=idx-1; i0=idx; ip=idx+1;
        }else{                                      /* closest pt is idx-1 */
            im=idx-2; i0=idx-1; ip=idx;
        }
    }
    yp = yar[ip]; y0=yar[i0]; ym=yar[im];
    xp = xar[ip]; x0=xar[i0]; xm=xar[im];

    /*debug 
    printf("\nid=%d i=%d %d %d  x=%8.4f %8.4f %8.4f  y=%8.4f %8.4f %8.4f",
        idx,im,i0,ip,xm,x0,xp,ym,y0,yp); */
        
    det = (xm-x0) * (xp-x0) * (xp-xm);
    a = ((xp-x0)*(xp-x0)*(ym-y0) - (xm-x0)*(xm-x0)*(yp-y0))/det;
    b = ((xm-x0)*(yp-y0) - (xp-x0)*(ym-y0))/det;
    
    /*debug printf("\na,b,det=%8.4f %8.4f %8.4f",a,b,det); */

    /* a,b are coef of dy = y-y0 = a*dx + b*(dx)^2 ; dx = x-x0 */
    dy = y-y0;
    if(a == 0.) erret("\nFMONSOLVE:nonchanging ordinate: cannot cope");
    dx = (2. * dy)/(a*(1. + sqrt(1. + 4.*b*dy/(a*a))));
    return x0 + dx;
}


/******************** FQUADINTERP() ***********************************/
/* jeg9812--quadratic interpolator for monotonic abscissa */

mirella double 
fquadinterp(x,xar,yar,n)
float *xar;
float *yar;
double x;
int n;
{
    int idx;
    int i0, ip, im;
    float y0, yp, ym;
    float x0, xp, xm;
    float det, a,b;
    float dy;
    float dx;
    
    idx = circe(x,xar,n);  /* point is between idx, idx-1    */
    if(idx == 1){
        im=0; i0=1; ip=2;
    }else if(idx == n-1){
        im=n-3; i0=n-2; ip=n-1;
    }else{
        if(fabs(x-xar[idx]) < fabs(x-xar[idx-1])){  /* closest pt is idx */
            im=idx-1; i0=idx; ip=idx+1;
        }else{                                      /* closest pt is idx-1 */
            im=idx-2; i0=idx-1; ip=idx;
        }
    }
    yp = yar[ip]; y0=yar[i0]; ym=yar[im];
    xp = xar[ip]; x0=xar[i0]; xm=xar[im];
    det = (xm-x0) * (xp-x0) * (xp-xm);
    a = ((xp-x0)*(xp-x0)*(ym-y0) - (xm-x0)*(xm-x0)*(yp-y0))/det;
    b = ((xm-x0)*(yp-y0) - (xp-x0)*(ym-y0))/det;
    /* a,b are coef of dy = y-y0 = a*dx + b*(dx)^2 ; dx = x-x0 */
    dx = x-x0;
    dy = dx*(a + b*dx);
    return y0 + dy;
}


/************************ N(S)(F)INTERP() ******************************/
/* linearly interpolate into array */

mirella normal
nainterp(buf,findx)
normal *buf;
double findx;
{
    normal i = findx;
    float alf = findx - i;
    return (alf * (float)(buf[i+1]) + (1.-alf) * (float)(buf[i]) );
}

mirella normal
sainterp(buf,findx)
short int *buf;
double findx;
{
    normal i = findx;
    float alf = findx - i;
    return (alf * (float)(buf[i+1]) + (1.-alf) * (float)(buf[i]) );
}

mirella double
fainterp(buf,findx)
float *buf;
double findx;
{
    normal i = findx;
    float alf = findx - i;
    return (alf * buf[i+1] + (1.-alf) * buf[i] );
}

/********* BYTE_SWAPPING: SARBSWP(), NARBSWP() ***************************/

mirella void 
sarbswp(buf,n)  /*swaps the bytes in buf, length n shorts */
short int *buf;
register int n;
{
    register char *p1 = (char *)buf;
    register char temp;
    while(n--){
        temp = *p1++;
        *(p1-1) = *(p1);
        *p1++ = temp;
    }
}


/*
 * swaps the bytes, then the words, in buf, length n normals
 * this should produce a Motorola/IBM long int stream from
 * a Dec/Intel one and vice versa
 */
mirella void 
narbswp(buf,n)
normal *buf;
register int n;
{
    register short int *p1 = (short int *)buf;
    register short int temp;

    sarbswp((short *)buf,2*n);		/* swap the bytes */
    while(n--){				/* now swap the words */
        temp = *p1++;
        *(p1-1) = *(p1);
        *p1++ = temp;
    }
}

/*********************(N)(S)(C)ARMASK***************************************/
/* ands the array with mask */
mirella void 
sarmask(buf,n,mask)
register short int *buf;
register normal n;
register int mask;
{
    while(n--){
        *buf++ |= mask;
    }
}

mirella void 
narmask(buf,n,mask)
register normal *buf;
register normal n;
register int mask;
{
    while(n--){
        *buf++ |= mask;
    }
}

mirella void 
carmask(buf,n,mask)
register u_char *buf;
register normal n;
register int mask;
{
    while(n--){
        *buf++ |= mask;
    }
}

/******** GAUSSIAN SMOOTHING ROUTINES (SHORT,FLOAT) ***********************/

/* 
 * gaussian-smooths an equispaced float array bufin into float array bufout 
 * sigma in in CELLS; no abscissa array is used. The code is different from
 * the short int version in the memory allocation--there is no `external'
 * static storage for this one.
 */

#define GAUSMAX 1000 
#define GAUSDEBUG 0

mirella void
fargausmth(bufin, bufout, n, sigma)
float *bufin, *bufout;
int n;
double sigma;
{
    static double sigsv = 0.;
    static float fgausfil[GAUSMAX];
    static float wt[GAUSMAX];
    static double wsum;
    static int nw; 
    int i, j;
    double arg;
    double sum = 0.;
    double flr;
    
    /* make filter if not a recall with same value */
    if ( sigma != sigsv ){
        nw = round(4. * sigma) + 1;
        if(nw > GAUSMAX){
            mprintf("\nFARGAUSMTH: size of wt array %d exceeds allocated size %d; aborting", nw, GAUSMAX);
            erret(0);
        }
        flr = exp(-8.);  /* exp(-4^2/2), floor*/
        for(i=0; i< nw; i++){
            arg = (double)i/sigma;
            fgausfil[i] = exp(-0.5*arg*arg) - flr;
            sum += fgausfil[i];
            if(i == 0){
                sum *= 0.5;
                fgausfil[0] = 0.5;
            }
            wt[i] = 2. * sum;
        }
        wsum = 2.* sum;
        sigsv = sigma;
    }

#if GAUSDEBUG
    mprintf("\nsigma, sigsv, nw, wsum = %f %f %d %f", sigma, sigsv, nw, wsum);
    faprt(fgausfil, nw, "fgausfil");
    faprt(wt, nw, "wt");
#endif
    
    for(i=nw; i < n-nw; i++){
        sum = 0.;
        for ( j=0; j< nw; j++){
            sum += (bufin[i+j] + bufin[i-j])*fgausfil[j];
        }
        bufout[i] = sum/wsum;
    }
    for(i=0; i< nw; i++){
        sum = 0.;
        for(j=0; j<= i; j++){
            sum += (bufin[i+j] + bufin[i-j])*fgausfil[j];
        }
        bufout[i] = sum/wt[i];
    }
    for(i=(n-nw); i< n ; i++){
        sum = 0;
        for(j=0; j<= (n - i -1); j++){
            sum += (bufin[i+j] + bufin[i-j])*fgausfil[j];
        }
        bufout[i] = sum/wt[n - i -1];
    }
}

/* 
 ***** SOME SHORT INT STUFF FOR SMOOTHING AND PEAK FINDING ************
 */

/* statics for access by later routines */ 
#define GAUSFLEN 200
/* NB!!!! these are 'mirella'---change to static */
mirella short sgausfil[GAUSFLEN];  /* gaussian filter, half */
mirella short sdgausfil[GAUSFLEN]; /* derivative filter, half */
mirella short s2gausfil[GAUSFLEN]; /* second moment filter */
mirella float sgauswt[GAUSFLEN];   /* weights for truncated filter (edges) */
mirella double sgaussigsv = 0.;    /* sigma for current filter */
mirella int sgausnw;               /* rnd(3.5sigma+1)--half length of filter */
mirella int sgauswsum;             /* total weight for gaussian filter */
/* NB!!! we need total weight for derivative filter, so derivs are properly
 * scaled
 */
 
/********** MAKESGAUSFIL() *******************************************
 * makes the Gaussian filter and derivative arrays for subsequent work,
 * and stuffs the static variables above.
 */
/*static*/ mirella void
makesgausfil(double sigma)
{
    int i;
    double arg;
    double earg;
    double sum = 0.;
    
    /* make filter if not a recall with same value */
    if ( fabs(sigma - sgaussigsv) > 1.e-4 ){    /* kluge */
        sgausnw = round(3.5 * sigma) + 2;
        for(i=0; i< sgausnw; i++){
            arg = (double)i/sigma;
            earg = exp(-0.5*arg*arg);
            sgausfil[i]  = round( 4096. * earg);
            sdgausfil[i] = round( 4096* ((double)i * earg)/sigma) ;
            s2gausfil[i] = round( 4096* ((double)(i*i) * earg)/(sigma*sigma)) ;
            sum += sgausfil[i];
            if(i == 0){
                sum *= 0.5;
                sgausfil[0] = 4096/2;
            }
            sgauswt[i] = 2. * sum;
        }
        sgauswsum = 2.* sum;
        sgaussigsv = sigma;
#if 0
        mprintf("\nsigma, sgaussigsv, sgausnw, sgauswsum = %f %f %d %f", 
            sigma, sgaussigsv, sgausnw, sgauswsum);
        saprt(sgausfil, sgausnw, "sgausfil");
        saprt(sdgausfil, sgausnw, "sdgausfil");
        saprt(s2gausfil, sgausnw, "s2gausfil");
        faprt(sgauswt, sgausnw, "sgauswt");
#endif
    }
}    

/************* SARGAUSMTH ******************************************/
/* Smooths a short 1-d array, input bufin, output bufout */

mirella void
sargausmth(bufin, bufout, n, sigma)
short *bufin, *bufout;
int n;
double sigma;
{
    int i, j;
    int sum ;
    int nw ;
    int wsum;
        
    makesgausfil(sigma);
    nw = sgausnw;
    wsum = sgauswsum;
   
    for(i=nw; i < n-nw; i++){
        sum = 0;
        for ( j=0; j< nw; j++){
            sum += (bufin[i+j] + bufin[i-j])*sgausfil[j];
        }
        bufout[i] = sum/wsum;
    }
    for(i=0; i< nw; i++){
        sum = 0;
        for(j=0; j<= i; j++){
            sum += (bufin[i+j] + bufin[i-j])*sgausfil[j];
        }
        bufout[i] = sum/sgauswt[i];
    }
    for(i=(n-nw); i< n ; i++){
        sum = 0;
        for(j=0; j<= (n - i -1); j++){
            sum += (bufin[i+j] + bufin[i-j])*sgausfil[j];
        }
        bufout[i] = sum/sgauswt[n - i -1];
    }
}

/*********** GAUSSIAN SMOOTHING FOR 2-D ARRAYS (FLOAT, SHORT) ***********/
/* MAYBE these should be in image.c ?? */
/* 
 * gaussian-smooths a float matrix (image) in place. Do something about
 * allocation of intermediate memory (????)...fixed for short version.
 */
 
mirella void
fmatgausmth(mat, xs, ys, sigma)
float **mat;
int xs;
int ys;
double sigma;
{
    float *bufout;
    float *bufin;
    int n; 
    int i, j;
       
    n = (xs > ys ? xs : ys);      
    bufin = (float *)malloc(2*n*sizeof(float));
    bufout = bufin + n;
    if(bufin == (float *)NULL){
        erret("\nFMATSMTH: cannot allocate intermediate memory");
    }
    
    /* do rows */
    for(i=0; i< ys; i++){
        fargausmth(mat[i],bufout,xs,sigma);
        memcpy(mat[i],bufout,xs*sizeof(float));
    }
    /* do columns */
    for(j=0;j<xs;j++){
        for(i=0; i<ys; i++){
            bufin[i] = mat[i][j];
        }
        fargausmth(bufin,bufout,ys,sigma);
        for(i=0; i<ys; i++){
            mat[i][j] = bufout[i];
        }
    }
    free(bufin);
}

/* 
 * gaussian-smooths a short matrix (image) *IN PLACE*. The general routine
 * takes a pointer to intermediate storage, 2*max(xs,ys)*sizeof(short)
 * bytes long. If the pointer is zero, the routine mallocs and frees the
 * buffers. There is a 'simple' version in which the buffer is set to void.
 */

mirella void
smatgausmthb(mat, xs, ys, sigma, buffer)
short **mat;
int xs;
int ys;
double sigma;
short *buffer;
{
    short *bufin;
    short *bufout;
    int n; 
    int i, j;
       
    n = (xs > ys ? xs : ys) ;  
    /* allocate buffer if it is not provided */
    if(buffer == (short *)0){ 
        bufin = (short *)malloc(2*n*sizeof(short));
        if(bufin == (short *)0){
            erret("\nSMATSMTH: cannot allocate intermediate memory");
        }
    }else{
        bufin = buffer;
    }
    bufout = bufin + n;

    /* do rows */
    for(i=0; i< ys; i++){
        /* smooth into bufout */
        sargausmth(mat[i],bufout,xs,sigma);
        /* copy it back */
        memcpy(mat[i],bufout,xs*sizeof(short));
    }
    /* do columns */
    for(j=0;j<xs;j++){
        /* copy x-smoothed image col into bufin */
        for(i=0; i<ys; i++){
            bufin[i] = mat[i][j];
        }
        /* smooth it into bufout */
        sargausmth(bufin,bufout,ys,sigma);
        /* and copy it back into the image; we are done */
        for(i=0; i<ys; i++){
            mat[i][j] = bufout[i];
        }
    }
    if(buffer != (short *)0) free(bufin);
}

/* this one allocates and frees memory for the intermediate storage */
mirella void
smatgausmth(mat, xs, ys, sigma)
short **mat;
int xs;
int ys;
double sigma;
{
    smatgausmthb(mat, xs, ys, sigma, (short *)0);
}

/*************** SARGAUPEAKFIND, SARDGAUSMTH ***************************/

/* 
 * Convolves a short array with the derivative of a gaussian and finds maximum
 * in convolved array; does not do anything on the boundary outside of 
 * +/- 3.5 sigma from the border; sets these values to zero in the output buffer.
 */

mirella void
sardgausmth(bufin, bufout, n, sigma)
short *bufin, *bufout;
int n;
double sigma;
{
    int i, j;
    normal sum = 0.;

    makesgausfil(sigma);
     
    for(i=sgausnw; i < n - sgausnw; i++){
        sum = 0;
        for ( j=0; j< sgausnw; j++){
            sum += (bufin[i+j] - bufin[i-j])*sdgausfil[j];
        }
        bufout[i] = sum/sgauswsum; /* is this reasonable?? */
    }
    /* at the boundaries, just punt; this is too complicated to try to deal
     * with, and I do not believe you can do the locations with any accuracy
     * anyhow
     */
    for(i=0; i< sgausnw; i++){
        bufout[i] = 0.;
    }
    for(i=(n - sgausnw); i< n ; i++){
        bufout[i] = 0.;
    }
}

/* this routine finds a simple peak above a threshold in a 1-d array; only
 * the first one is found
 */
mirella double
sargaupkfind(bufin, n, sigma, thresh)
short *bufin;
int n;
double sigma;
int thresh;
{
    int i;
    int ii;
    int ip;
    double res;
    short int *bufout = malloc(n*sizeof(short));
    /* fix this: malloc once, free if change, so need static bufout, nsav--
     * think about how this should be done--maybe leave it--this routine
     * is really only for testing 
     */
    
    if(bufout == (short *)0){
        erret("\nSARGAUPKFIND: Cannot allocate memory");
    }
    sardgausmth(bufin, bufout, n, sigma);
    for( i=0; i<n; i++){
        if(bufin[i] >= thresh){  /* begin looking for a zero crossing*/
            ii = i;
            break;
        }
    }
    for( i=ii; i<n; i++){
mprintf("\n i,in,out = %d %d %d",i,bufin[i],bufout[i]);        
        if(bufout[i] < 0){  /* find first negative derivative point */
            ip=i;
            break;
        }
    }
    if(i == n){
        mprintf("SARGAUPKFIND:Could not find a peak");
        free(bufout);
        return (-1.);
    }
    res = (double)bufout[ip-1]/(double)(bufout[ip-1] - bufout[ip]) 
            + (double)(ip-1);
    free(bufout);
    return res;
}


/**** FAST IMAGE RESOLUTION REDUCTION ROUTINES ****************************/

/* these routines makes a large float(short) picture into a small one, combining
 * (averaging in these routines) a rat x rat square into a single pixel
 * in the target. IT IS YOUR RESPONSIBILITY TO BE SURE THE TARGET IS
 * BIG ENOUGH, and you have to be careful at the bottom and right edges if
 * the sizes of the input are not divisible by rat. 
 * xsh and ysh are fractional shifts of a pixel in the output image; if
 * their product with rat is an integer, they are exact. 
 * For some purposes you want to just add in the short case; write one.
 */
 
mirella void
fimredres( 
    float **in, 
    int xsz, 
    int ysz, 
    float **out, 
    int rat, 
    double xsh, 
    double ysh )
{
    int i, j;
    float sum;
    int ii, jj;
    int xszt = xsz/rat;
    int yszt = ysz/rat;
    int x, y, ys;
    int dx = round(rat*xsh);
    int dy = round(rat*ysh);
    int xl,xu;
    int yl,yu;
    float rat2 = rat*rat;

    if(dx > 0){
        xl = (dx + rat)/rat;
        xu = xszt;
    }else if(dx < 0){
        xl = 0;
        xu = xszt - (rat - dx)/rat;    
    }else{
        xl = 0;
        xu = xszt ;
    }

    if(dy > 0){
        yl = (dy + rat)/rat;
        yu = yszt;
    }else if(dy < 0){
        yl = 0;
        yu = yszt - (rat - dy)/rat;    
    }else{
        yl = 0;
        yu = yszt ;
    }

    for(i = yl ; i < yu; i++){
        y = rat * i - dy; /* ll y corner of superpixel in big image */
        for(j = xl; j < xu; j++){
            x = rat * j - dx; /* ll x corner, ditto */
            sum = 0;
            for (ii = 0 ; ii < rat ; ii++ ){
                ys = y + ii ;
                for(jj = 0; jj < rat; jj++ ){
                    sum += in[ys][x + jj];
                }
            }
            out[i][j] = sum/rat2 ;
        }
    }
}

mirella void
gsimredres( 
    short **in, 
    int xsz, 
    int ysz, 
    short **out, 
    int rat, 
    double xsh, 
    double ysh,
    int addflg )  /* 0 for average, !=0 for add */
{
    int i, j;
    int sum;
    int ii, jj;
    int xszt = xsz/rat;
    int yszt = ysz/rat;
    int x, y, ys;
    int dx = round(rat*xsh);
    int dy = round(rat*ysh);
    int xl,xu;
    int yl,yu;
    int rat2 = rat*rat;

    if(dx > 0){
        xl = (dx + rat)/rat;
        xu = xszt;
    }else if(dx < 0){
        xl = 0;
        xu = xszt - (rat - dx)/rat;    
    }else{
        xl = 0;
        xu = xszt ;
    }

    if(dy > 0){
        yl = (dy + rat)/rat;
        yu = yszt;
    }else if(dy < 0){
        yl = 0;
        yu = yszt - (rat - dy)/rat;    
    }else{
        yl = 0;
        yu = yszt ;
    }

    for(i = yl ; i < yu; i++){
        y = rat * i - dy; /* ll y corner of superpixel in big image */
        for(j = xl; j < xu; j++){
            x = rat * j - dx; /* ll x corner, ditto */
            sum = 0;
            for (ii = 0 ; ii < rat ; ii++ ){
                ys = y + ii ;
                for(jj = 0; jj < rat; jj++ ){
                    sum += in[ys][x + jj];
                }
            }
            if(addflg == 0){
                out[i][j] = sum/rat2;
            }else{
                out[i][j] = sum;
            }
        }
    }
}

mirella void
simredres( 
    short **in, 
    int xsz, 
    int ysz, 
    short **out, 
    int rat, 
    double xsh, 
    double ysh )
{
    gsimredres( in, xsz, ysz, out, rat, xsh, ysh, 0 );
}

mirella void
simredresadd( 
    short **in, 
    int xsz, 
    int ysz, 
    short **out, 
    int rat, 
    double xsh, 
    double ysh )
{
    gsimredres( in, xsz, ysz, out, rat, xsh, ysh, 1 );
}
                                    

/***************** SHORT IMAGE PEAK FINDER AND CENTROIDER *****************/

struct peak_t{
    float x_pk;     /* final centroided x */
    float y_pk;     /* final centroided y */
    int   ix_pk;    /* integer guessed x value--peak in unsmoothed image  */
    int   iy_pk;    /* integer guessed y value--peak in unsmoothed image  */
    float val_pk;   /* value at peak in unsmoothed image */
    float flux_pk;  /* total flux above background */
    float fwhm_pk;  /* equivalent fwhm of unsmoothed image */
    float axrat_pk; /* maximum axial ratio of unsmoothed image */
    int   error_pk; /* error code */
};

#define SIZE_PK sizeof( struct peak_t )
int size_pk = sizeof( struct peak_t ); /* for mirella */

struct peak_t picpeak;
    
#define BOXBDY 1   
#define IMGBDY 2
#define NORAWPKINRANGE 4
#define NOSMPKINRANGE  8



/* these routines evaluate the value, derivative, and second moments 
 * of a locally gaussian-smoothed image at some pixel location xp, yp. 
 * YOU MUST HAVE BUILT THE GAUSSIAN FILTER before calling any of these
 * routines (makesgausfil) 
 */
 
void 
sgausimgval(short **p, int xp, int yp, float *out)   
{
    int i,j;
    int sum=0;

    for(i = 0; i<sgausnw; i++){
        for( j = 0; j<sgausnw; j++){
            sum += ((sgausfil[i]*sgausfil[j])/sgauswsum) *
              (p[yp+i][xp+j] + p[yp-i][xp+j] + p[yp+i][xp-j] + p[yp-i][xp-j]);
        }
    }
    *out = (float)sum;
}
 
void 
sgausimgderiv(short **p, int xp, int yp, float *out)   
{
    int i,j;
    int sumx=0, sumy=0;

    for(i = 0; i<sgausnw; i++){
        for( j = 0; j<sgausnw; j++){
            sumx += ((sgausfil[i]*sdgausfil[j])/sgauswsum) *
              (p[yp+i][xp+j] + p[yp-i][xp+j] - p[yp+i][xp-j] - p[yp-i][xp-j]);
            sumy += ((sdgausfil[i]*sgausfil[j])/sgauswsum) *   
              (p[yp+i][xp+j] - p[yp-i][xp+j] + p[yp+i][xp-j] - p[yp-i][xp-j]);
        }
    }
    out[0] = (float)sumx;
    out[1] = (float)sumy;
}

void 
sgausimg2mom(short **p, int xp, int yp, float *out)   
{
    int i,j;
    int sumx=0, sumy=0;

    for(i = 0; i<sgausnw; i++){
        for( j = 0; j<sgausnw; j++){
            sumx += ((sgausfil[i]*s2gausfil[j])/sgauswsum) *
              (p[yp+i][xp+j] + p[yp-i][xp+j] + p[yp+i][xp-j] + p[yp-i][xp-j]);
            sumy += ((s2gausfil[i]*sgausfil[j])/sgauswsum) *   
              (p[yp+i][xp+j] + p[yp-i][xp+j] + p[yp+i][xp-j] + p[yp-i][xp-j]);
        }
    }
    out[0] = (float)sumx;
    out[1] = (float)sumy;
}


/* 
 * this routine finds the centroid of a peak in a 2d short image. 
 * It is assumed that the peak is reasonably represented by a gaussian 
 * with parameter sig, is at least thresh high, and is within +/- an integer 
 * rng in both coords of a supplied integer guess for its position
 * (typically the peak of the unsmoothed image). The result is returned in a 
 * struct peak_t supplied by the caller. The peak is found by a combination
 * of convolving with the x and y partial derivatives of a gaussian of width 
 * sigma and finding zeros, and  by quadratic interpolation to find the 
 * peak in the locally gaussian-smoothed image.
 *
 * NB!!! this routine allocates a small amount of memory for local 
 * convolution which it reuses at each invocation and never frees. 
 * The pointer is published as a global gau_gmat so it can be freed externally
 * if desired. It is freed and reallocated if
 * the routine is called with a different width parameter or a different
 * value of rng from the previous call; otherwise it reuses the space allocated
 * on its first call at each subsequent invocation.
 */


#define GPKDEBUG0 0
#define GPKDEBUG 0

short **gau_gmat = (short **) 0 ;
float pk_alphadp = 0.5; /* relative weight for deriv peak value to
                         * quadratic peak value in peak finder 
                         */

 
int
sgausimgpkcent( short **p, int xs, int ys, int x, int y, int rng, 
            int thresh, double sig, struct peak_t *sp)
{
    int span;
    int sp2 ; 
    /* these limits are for matrix manipulations--note that the SEARCH
     * area is smaller, only x+/-rng, y+/-rng
     */
    int xl ;
    int xu ; /* upper x limit for for convolution loops, INCLUDE */
    int yl ;
    int yu ; /* ditto for y */
    int i, ii, j ;
    int pk ;
    int val;
    int xp, yp; /* intermediate integer peak locations */

    int find=0;
    int ret=1;
    static int ospan = 0;
    static short **gmat = (short **)0;

    float der00[2], der01[2], der10[2], der11[2];
    double xtlt, ytlt, xcrv, ycrv;
    double dx0, dx1, dy0, dy1, delx, dely;
    double xid, etad;  /* frac pixels offset from integer peak, deriv */
    double xip, etap ; /* frac pixels offset from integer peak, quad peak */
    double xi, eta ;   /* adopted frac pixels offset */
#if GPKDEBUG
    void smprt();
#endif
    float smpeak;
    float rawpeak;

    makesgausfil(sig);  /* sets sgausnw = round(3.5*sig + 1). NB!!
                         * the filter starts at 0, so the number of
                         * filter elements is 2*sgausnw - 1) 
                         */

    span = (2*sgausnw - 1) +  2 * rng;  /* range over which we convolve. ODD */
    sp2 = span/2;
    xl = x - sp2;
    xu = x + sp2; /* upper limit for for convolution loops, INCLUDE */
    yl = y - sp2;
    yu = y + sp2; /* ditto */
    
    /* check range */
    if( xl<0 || yl < 0 || xu > xs-1 || yu > ys-1){
        puts("\nSARGAUSIMGPKFIND: point too near boundary");
        return IMGBDY;
    }
    
    /* find the peak in the original image */
    pk = p[y-rng][x-rng] - 1;  /* 1 less than initial value */
    for (i=y-rng; i<=y+rng; i++){
        for (j=x-rng; j<=x+rng; j++){
            val=p[i][j];
             /* mprintf("\nx,y,val,thresh,rng,sp2: %d %d %5d %d %d %d",
                        i,j,val,thresh,rng,sp2); */
            if(val > thresh && val > pk){
                pk = val;
                xp = j;
                yp = i;
                find=1;
            } 
        }
    }
    if (find == 0){
        printf(
        "\nSARGAUSIMGPKFIND: failed to find a raw peak above thresh near %d %d",
             x,y);
        return NORAWPKINRANGE;
    }
    rawpeak = (float)pk;
    ret = 0;  /* OK for now */
#if GPKDEBUG
    mprintf("\n Peak in raw image at %d %d, val=%d", xp, yp, pk   );
#endif        

    /* Now make a convolved sub matrix; first allocate memory if needed: */
    if( span != ospan ){
        if ( gmat != (short **)0 ) free(gmat);
        gmat = (short **)matrix(span, span, sizeof(short));
    }
    gau_gmat = gmat; /* publish */
    
    /* extract the matrix--note we use limits xl,xu,yl,yu */
    for( i=0; i < span; i++){
        ii = i + yl;
        for( j=0; j< span; j++){
            gmat[i][j] = p[ii][j+xl];
        }
    }
#if GPKDEBUG0
    smprt(gmat,span,span,"extracted raw image");
#endif

    /* smooth it and find the peak in the smoothed image */
    smatgausmth(gmat,span,span,sig);

#if GPKDEBUG0
    smprt(gmat,span,span,"extracted conv image");
#endif
    
    find = 0;
    pk = gmat[sp2-rng][sp2-rng] -1;  /* value smaller than 1st corner value */

    /* OK. now find peak in convolved subimage. throw away  xp, yp */
    for (i=sp2-rng; i<=sp2 + rng; i++){
        for (j=sp2-rng; j<=sp2+rng; j++){
            if((val=gmat[i][j]) > pk){
                pk = val;
                xp = j;
                yp = i;
                find=1;
            } 
        }
    }
    if (find == 0){
        printf(
        "\nSARGAUSIMGPKFIND: failed to find a smoothed peak near %d %d",
             x,y);
        return NOSMPKINRANGE;
    }
    smpeak = (float)pk;
#if GPKDEBUG
    printf(
      "\n Peak pixel in smoothed image at %d %d, val=%d, in orig image %d %d", 
        xp, yp, pk, xp+xl, yp+yl);
#endif        
    /* now have peak in smoothed image. real peak is within half a pixel
     * of this location unless image is pathological. We next find
     * where the maximum is by quadratic interpolation */
    xtlt = (gmat[yp-1][xp+1] + gmat[yp][xp+1] + gmat[yp+1][xp+1]) -
           (gmat[yp-1][xp-1] + gmat[yp][xp-1] + gmat[yp+1][xp-1]) ;

    ytlt = (gmat[yp+1][xp-1] + gmat[yp+1][xp] + gmat[yp+1][xp-1]) -
           (gmat[yp-1][xp-1] + gmat[yp-1][xp] + gmat[yp-1][xp-1]) ;

    xcrv = (gmat[yp-1][xp+1] + gmat[yp][xp+1] + gmat[yp+1][xp+1]) -
         2*(gmat[yp-1][xp]   + gmat[yp][xp]   + gmat[yp+1][xp]  ) +
           (gmat[yp-1][xp-1] + gmat[yp][xp-1] + gmat[yp+1][xp-1]) ;       

    ycrv = (gmat[yp+1][xp-1] + gmat[yp+1][xp] + gmat[yp+1][xp-1]) -
         2*(gmat[yp][xp-1]   + gmat[yp][xp]   + gmat[yp][xp-1]  ) +
           (gmat[yp-1][xp-1] + gmat[yp-1][xp] + gmat[yp-1][xp-1]) ;

    xip = -xtlt/(2.*xcrv);
    etap = -ytlt/(2.*ycrv);

#if GPKDEBUG
    printf(
     "\nxip, etap, xtlt, ytlt, xcrv, ycrv = %6.3f %6.3f %4.1f %4.1f %5.3f %5.3f",
        xip,etap,xtlt,ytlt,xcrv,ycrv);
#endif

    if(xip < 0.) { xp-- ; xip += 1.0; }
    if(etap < 0.){ yp-- ; etap += 1.0; }       
    /* now have maximum in box if all is well */
    /* Now evaluate the derivatives at xp,yp; they SHOULD be
     * positive*/ 
     
    /* translate xp and yp to real image coordinates */ 
    xp += xl;
    yp += yl;
    /* these quadratically interpolated centers are as good as the derivative 
     * ones, and have errors of the opposite sign............
     */

#if GPKDEBUG
    printf(
        "\nReady for final evaluation: xp, yp, xip, etap = %d %d %5.3f %5.3f",
        xp, yp, xip, etap);
#endif

    /* Evaluate the derivatives at the corners: */
    sgausimgderiv(p,xp,yp,der00);   
    sgausimgderiv(p,xp+1,yp,der10);
    sgausimgderiv(p,xp,yp+1,der01);   
    sgausimgderiv(p,xp+1,yp+1,der11);

#if GPKDEBUG
    printf("\nx derivatives (00,10,01,11)= %4.2f %4.2f %4.2f",
        der00[0], der10[0], der01[0], der11[0]);
    printf("\ny derivatives (00,10,01,11)= %4.2f %4.2f %4.2f",
        der00[1], der10[1], der01[1], der11[1]);
#endif

    /* linearly interpolate to find point at which both derivatives vanish */
    dx0 = der00[0]/(der00[0] - der10[0]);
    dx1 = der01[0]/(der01[0] - der11[0]);
    dy0 = der00[1]/(der00[1] - der01[1]);
    dy1 = der10[1]/(der10[1] - der11[1]);
    delx = dx1 - dx0;
    dely = dy1 - dy0;
    xid  = (dx0 + delx*dy0)/(1. - delx*dely);
    etad = (dy0 + dely*dx0)/(1. - delx*dely);

    /* we now combine the interpolated value with the derivative value with
     * weights; default weights = 0.5, 0.5 
     */    
    xi  = pk_alphadp * xid  + (1. - pk_alphadp) * xip ;
    eta = pk_alphadp * etad + (1. - pk_alphadp) * etap ; 
  
    sp->x_pk = xi + xp ;
    sp->y_pk = eta + yp ;
    sp->val_pk = rawpeak;
    sp->flux_pk = 12.57*smpeak*sig*sig;

#if GPKDEBUG
    printf("\ndx0,dx1,delx=%5.3f,%5.3f,%5.3f, dy0,dy1,dely=%5.3f,%5.3f,%5.3f",
        dx0,dx1,delx,dy0,dy1,dely);
    printf("\nxi,eta,x,y,val:%5.3f, %5.3f, %5.3f, %5.3f, %8.1f", 
        xi,eta,sp->x_pk,sp->y_pk,sp->val_pk);
#endif    

    return ret;
}


/************************** SIMPKFINDER  ********************************/

/* this routine finds all the peaks above thresh in an image, and returns
 * the number found. It populates an array of peak_t structures with
 * everything except the float centroid values, which must be populated by
 * a separate run through the array calling sgausimgpkcent()
 *
 * the idea is to go line by line; as soon as thresh is reached, we have
 * a peak nearby, by definition. The maximum is found, looking ahead in  y 
 * and back and forth in x, at xp,yp. Then we go to the peak and 
 * evaluate its properties, including flux, area at half-peak, etc. All
 * the pixels above the threshold are marked in a byte-wide mask frame,
 * to tell us NOT to visit them again.
 */

#define PKFINDDEBUG  0
 
int 
simpkfinder(
    short **img,    /* short int image */
    u_char **mask,  /* byte mask same size as image--we may work on this later*/
    int xsz,     
    int ysz,        /* x and y size of image/mask  */
    int maxpk,      /* max # of peaks -- dim of arrays */
    int thresh,     /* threshold */
    int bkg,        /* background level */
    int rs,         /* `radius' of search box */
    double sig,     /* sigma of fitting gaussian */
    struct peak_t * sp )   /* pointer to peak structure array */
{
    int i,j,js,jj;
    int k,l;
    int vp;
    int vp2;
    int val;
    int xp;
    int yp;
    int sum;
    int npk = 0;
    int siz;
    int xpext;
    int xmext;
    int ypext;
    int ymext;
    int xpypext;
    int xpymext;
    int xmypext;
    int xmymext;
    int lpk;
    int lmk;
    int xext;
    int yext;
    int xpyext;
    int xmyext;
    float axratxy;
    float axrat45;
    float axrat;
    float fwhm;
    
    /* clear the peak arrays and the mask*/
    for(i=0; i < ysz; i++ ){
        memset(mask[i],0,xsz);
    }
    memset(sp, 0, maxpk*sizeof(struct peak_t));

    /* find the peaks */
    for( i=0; i < ysz; i++ ){
        /* make a list of x's for peaks within rs */
        for ( j = 0; j< xsz; j++ ){
            if( img[i][j] > thresh ){
                /* Above threshold; First see if we have been here already*/
                if(mask[i][j] != 0){
                    /* been here before -- skip overthresh pixels */
                    while( mask[i][j++] != 0 && j < xsz ){;} ;
                }else{ /* firsttime; find the the real nearby peak */
                    js = j;  /* start of over-thresh region on this line */
                    while(img[i][j++] > thresh && j < xsz) {;} ;
                    jj = (js + j - 1)/2 ; /*middle of overthresh on this line*/
                    sum = 0;
                    vp = img[i][jj] -1; /* safely less than peak */
                    for ( k = 0; k <= rs +1; k++ ){
                        for ( l = -rs; l <= rs; l++ ){
                            val = img[i+k][jj+l];
                            if( val > vp ) {
                                vp = val;
                                xp = jj+l;
                                yp = i+k;
                            }
                        }
                    }
                    /* check if on boundary */
                    if(xp == jj-rs || xp == jj+rs || yp == i + rs +1){
                        printf("\n Max at (x,y): %d %d is on box boundary",xp,yp);
                        sp->error_pk = BOXBDY;
                    }
                    vp2 = (vp - bkg)/2 + bkg;  /* half peak value */
                    /* set all extents to zero */
                    siz = xpext = xmext = ypext = ymext = 0;
                    xpypext = xpymext = xmypext = xmymext = 0;
                    /* found the peak; now mark and evaluate flux 
                     * and extents in x, y, and +/- 45 degrees
                     */
                    for( k= -rs ; k <= rs; k++ ){
                        for( l= -rs; l <= rs; l++ ){
                            val = img[yp + k][xp+l];
                            sum += val - bkg;
                            if( val > thresh ) mask[yp+k][xp+l] = 1 ;
                            if( val > vp2 ){  /* above half-peak point */
                                siz++ ;       /* add up area */
                                /* do max extents along axes */
                                if( l > xpext ) xpext = l;
                                if( l < xmext ) xmext = l;
                                if( k > ypext ) ypext = k;
                                if( k < ymext ) ymext = k;
                                /* do max extents perp to axes */
                                lmk = k - l;
                                lpk = k + l;
                                if( lpk > xpypext ) xpypext = lpk;
                                if( lpk < xpymext ) xpymext = lpk;
                                if( lmk > xmypext ) xmypext = lmk;
                                if( lmk < xmymext ) xmymext = lmk;
                            }
                        }
                    }
                    
                    /* calculate axis ratio */
                    xext = xpext - xmext + 1;
                    yext = ypext - ymext + 1;
                    xpyext = xpypext - xpymext + 1 ;
                    xmyext = xmypext - xmymext + 1 ;
                    axratxy = (float)xext / (float) yext;
                    if (axratxy < 1.) axratxy = 1./axratxy;
                    axrat45 = (float)xpyext / (float)xmyext ;
                    if (axrat45 < 1.) axrat45 = 1./axrat45;
                    axrat = axrat45 > axratxy ? axrat45 : axratxy ;
                    fwhm = 2. * sqrt((double)siz/3.14159);
                    
                    /* find the centroid ??? */
                    sgausimgpkcent( img, xsz, ysz, xp, yp, rs,
                        thresh, sig, sp);
                    
                    /* stuff structure */
                    sp->iy_pk   = yp;
                    sp->ix_pk   = xp;
                    sp->val_pk  = (float)vp; 
                    sp->flux_pk = (float)sum;
                    sp->fwhm_pk = fwhm;
                    sp->axrat_pk = axrat ;
                    
#if PKFINDDEBUG
                    printf(
             "\n%4d   %4d %4d   %7.2f %7.2f   %4d %6d   %4.1f %4.1f",
                    npk, xp, yp, sp->x_pk, sp->y_pk, vp, sum, fwhm, axrat);
#endif                        
                    sp++ ;
                    npk++;
                    /* 
                     * move on; we have already set j to first under-thresh 
                     * point 
                     */
                }
            }           /* if over thresh */
        }               /* loop over pixels */
    }                   /* loop over lines */
    printf("\nFound %d peaks\n", npk);
    return npk;
}

/*** SHORT INT IMAGE HISTOGRAMMING ROUTINE *********************************/

void
simhist(u_short **img, int xsz, int ysz, int bits, int pkno, 
        int *histo, int *pth, int *med)
{
    int i, j;
    int np = xsz*ysz;
    int np2 = np/2;
    int val;
    int top = (1<<bits);
    int medflg = 0;
    int hispeak = np - pkno;
    
    /* clear histogram */
    memset(histo,0,top*sizeof(int));
    
    /* populate histogram */
    for( i=0; i< ysz; i++ ){
        for( j=0; j< xsz; j++ ){
            histo[img[i][j]]++;
        }
    }
    /* create cumulative histogram */
    for(i=1;i<top;i++){
        histo[i] += histo[i-1];
        val = histo[i];
        if(medflg == 0 && val > np2){
            medflg = 1;
            *med = i;
        }
        if(val > hispeak){
            *pth = i;
            break;
        }            
    }
}
                         
/** ARRAY DISPLAY ROUTINES: FMPRT(),SMPRT()FAPRT(),SAPRT(),NAPRT(),CAPRT() ***/

mirella void
fmprt(px, n, m, titl)   /* print floating point matrix with title */
float *px[] ;
int n, m ;
char titl[] ;
{
    int i,j ;
    
    mprintf("\n\n%d by %d matrix %s \n",n,m,titl);
    for ( i=0 ; i < n ; i++ ) {
        for ( j=0 ; j < m ; j++ )
            mprintf ("%10.5f", px[i][j]) ;
        mprintf("\n") ;
        pauseerret();
    }
}
mirella void
smprt(px, n, m, titl)   /* print short matrix with title */
short *px[] ;
int n, m ;
char titl[] ;
{
    int i,j ;
    
    mprintf("\n\n%d by %d matrix %s \n",n,m,titl);
    for ( i=0 ; i < n ; i++ ) {
        for ( j=0 ; j < m ; j++ )
            mprintf ("%6d", px[i][j]) ;
        mprintf("\n") ;
        pauseerret();
    }
}



/*
 * prints float array with title & numbers elements
 */
mirella void
faprt(x,n,titl)
float *x ;
int n ;
char *titl ;
{
int i ;

    mprintf("\narray %s of length %d\n",titl,n) ;
    for ( i=0 ; i < n ; i++ ) {
        if( i % 5 == 0) mprintf("\n %5d",i) ;
        mprintf("%13.5g",x[i]) ;
    }
    mprintf("\n") ;
    pauseerret() ;
}

/*
 * prints integer array with title and numbers elements
 */
mirella void
naprt(ia,n,titl)
int *ia ;
int n ;
char *titl ;
{
    int i ;
    
    mprintf("\ninteger array %s of length %d\n",titl,n) ;
    for ( i=0 ; i < n ; i++ ) {
        if( i % 8 == 0) mprintf("\n %5d", i ) ;
        mprintf( "%8d" , ia[i]) ;
    }
    mprintf ( "\n" ) ;
    pauseerret() ;
}

/*
 * prints short array with title and numbers elements
 */
mirella void
saprt(ia,n,titl)
short int *ia ;
int n ;
char *titl ;
{
    int i ;
    
    mprintf("\nshort integer array %s of length %d\n",titl,n) ;
    for ( i=0 ; i < n ; i++ ) {
        if( i % 8 == 0) mprintf("\n %5d", i ) ;
        printf( "%8d" , ia[i]) ;
    }
    mprintf ( "\n" ) ;
    pauseerret() ;
}

/*
 * prints u_char array with title and numbers elements
 */
mirella void
caprt(ia,n,titl)
char *ia;
int n;
char *titl ;
{
    int i ;
    
    mprintf("\nu_char array %s of length %d\n",titl,n) ;
    for ( i=0 ; i < n ; i++ ) {
        if( i % 16 == 0) mprintf("\n%4d ", i ) ;
        printf( "%4u" , ia[i]) ;
    }
    mprintf ( "\n" ) ;
    pauseerret() ;
}

/******************** END, MIRARRAY.C *************************************/

