/*version 89/06/02: Mirella 5.20 */
/********************** MIRSPLIN.C *************************************/
/*  spline package for interpolation  */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

int _circerr;

/*recent history:
 * 89/06/02     modified circe() to handle monotonically decreasing arrays
 */
 
/************************ CIRCE() ***************************************/

mirella int circechatter = 1;

/*
 * returns index i of first element of t >= x for monotonically
 * increasing arrays, <= x for decr, so interp is always between i, i-1
 */
mirella int 
circe(x,t,n)
double x ;
float *t ;
int n ;
{
   register int lo, hi, mid ;
   float tm ;
   
   _circerr = 0;
   lo = 0 ;
   hi = n-1 ;
   if(t[lo] < t[hi]){      /* monotonically increasing */
      if ( x >= *t && x <= *(t+n-1))  {
	 while ( hi - lo > 1 ) {
	    mid = ( lo + hi )/2. ;
	    tm = *(t + mid) ;
	    if ( x < tm ) hi = mid ;
	    else  lo = mid ;
	 }
	 return hi ;
      }
      else{
         if(circechatter != 0)
             scrprintf("\ncirce lookup value outside array bounds,%f\n",x);
      }
      _circerr = 1;
      if(x < *t){
	 _circerr = -1;
	 return 0 ;
      }
      if(x > *(t+n-1)) return n-1 ;
   }else{      /* monotonically decreasing */
      if ( x <= *t && x >= *(t+n-1))  {
	 while ( hi - lo > 1 ) {
	    mid = ( lo + hi )/2. ;
	    tm = *(t + mid) ;
	    if ( x > tm ) hi = mid ;
	    else  lo = mid ;
	 }
	 return hi ;
      }
      else scrprintf("\ncirce lookup value outside array bounds,%f\n",x);
      _circerr = 1;
      if(x > *t){
	 _circerr = -1;
	 return 0 ;
      }
      if(x < *(t+n-1)) return n-1 ;
   }
   scrprintf("\nfalling off bottom of circe: %f\n",x);
   return 0;				/* added RHL -- is this correct? */
}

/************************* LINTERP() ***************************************/

/*
 * lin interpolator, arg z, argtable x, ftable y (float), table size n
 * if fn is comapact at either end, returns 0 and no complaint if off
 * that end.
 */
mirella double 
linterp(z,x,y,n)
double z;
float *x, *y;
int n;
{
    register int i;
    float alpha;

    if ( z < x[0]   && y[0]   == 0. )     return 0.;
    if ( z > x[n-1] && y[n-1] == 0. )     return 0.;
    i = circe(z,x,n);
    if(_circerr) return( y[i]);
    alpha = (x[i] -z)/(x[i]-x[i-1]);
    return ( alpha*y[i-1] + (1.-alpha)*y[i] ) ;
}

#if 0
/* this function is now in mirmatrx, and has been trimmed a bit 
 * It is called only by gsplinit, below, to solve for the spline coefficients
 */

/*********************** TRIDI()  (static) *******************************/

static void
tridi(a,b,c,f,x,n)  /* solves tridiagonal linear system .   
    diag elts mii=ai, subdiag mii-1=bi, superdiag mii+1=ci.
    it is understood that b0 and cn-1 are zero, but are not referenced.
    f is rhs, x soln, may be same array; all arrays are trashed */

float *a, *b, *c, *f, *x;
int n ;
{
    register int i;
    for ( i=1; i < n; i++) {
        b[i] /= a[i-1];
        a[i] -= b[i] * c[i-1];
    }
    for( i=1 ; i < n ; i++){ 
        f[i] -= b[i] * f[i-1];
    }
    x[n-1] = f[n-1]/a[n-1] ;
    for( i=n-2; i > -1; i-- ){
        x[i] = (f[i] - (c[i] * x[i+1])) / a[i];
    }
}

#endif

/********************** GSPLINIT(), SPLINIT() ****************************/

/* this function used to be called splinit(); careful */
/*
 * sets up spline derivative array k for a given x and y array of
 * length n POINTS, n-1 intervals, for given estimates for the second
 * derivatives at the endpoints, q2b and q2e; "natural" boundary conditions
 * for q2b=q2e=0. We need to write one which uses the extrapolated second
 * differences for these estimates, or just the second diffs at the next
 * point.!!!!!
 */
mirella void 
gsplinit(x,y,k,n,q2b,q2e)
float *x, *y, *k ;
double q2b, q2e ;
int n ;
{
    float hio, hip, dio, dip ;
    float *a, *b, *c, *f;
    int i, ip ;
    a = (float *)malloc(4*n*sizeof(float)) ; 
    b = a + n ;
    c = b + n ;
    f = c + n ;
    hio = 0. ;
    dio = 0. ;
    for( i=0 ; i < n ; i++ ) {
        hip = ((ip = i+1) < n ? *(x + ip) - *(x + i) : 0. ) ;
        dip = ( ip < n ? (*(y + ip) - *(y + i))/hip : 0. ) ;
        *(b+i) = ( ip < n ? hip : hio ) ;
        *(a+i) = 2.*( hip + hio ) ;
        *(c+i) = ( i > 0 ? hio : hip ) ;
        *(f+i) = 3.*(hip*dio + hio*dip ) ;
        if( i == 0 ) *f = 3.* hip * dip  - hip * hip * q2b * 0.5 ;
        else if ( i == n-1 )
            *(f+n-1) = 3.* hio* dio + hio* hio* q2e* 0.5  ;
        dio = dip ;
        hio = hip ;
        }
    tridi(a,b,c,f,k,n) ;  /* which function???*/
    free(a) ;
}

mirella void
splinit(x,y,k,n)
float *x, *y, *k;
int n;
{
    gsplinit(x,y,k,n,0.,0.);
}

/************************ SPLINE() ************************************/
static double 
spf(dx,x,y,k,m,pd,key) /* general spline evaluator; xyk are the x,y, and 
    derivative arrays, dx the argument = (x-xi-1), m the index of
    the next GREATER abscissa. If key>0, routine evaluates derivative,
    pointer pd; if key>1, ONLY derivative.  */

double dx ;
float *x, *y, *k, *pd ;
int m, key ;
{
    float h, t, d, tm, a, b ;
    double val ;
    h = *(x+m) - *(x+m-1) ;
    d = (*(y+m) - *(y+m-1))/h ;
    t = dx/h ;
    tm = 1.-t ;
    a = (*(k+m-1) - d) * tm ;
    b = (*(k+m) -d) *t ;
    if( key > 0) *pd = d + a*(1.-3.*t) - b*(2.-3.*t) ;
    if( key < 2) val = t * *(y+m) + tm * *(y+m-1) + h*t*tm*(a-b) ;
    return  val ;
}

/* routine spft for consistency with old code */
/*
 * spline evaluator for general monotonic abscissa tables
 */
double
spft(z,x,y,k,n,pd,key)
double z;
float *x, *y, *k, *pd ;
int key, n;
{
    float dx ;
    int m ;
    m = circe( z, x, n) ;
    dx = z - *(x+m-1) ;
    if(!_circerr) return spf(dx,x,y,k,m,pd,key) ;
    else return y[m];
}

mirella float splder;

/*
 * spline evaluator for general monotonic (increasing) abscissa tables;
 * deriv in splder. Modified 11/01 to return 0 and not give error
 * when function table is compact and arg is off end.
 */
double 
spline(z,x,y,k,n)
double z;
float *x, *y, *k;
int n;
{
    float dx ;
    int m ;
    if ( z < x[0]   && y[0]   == 0. )     return 0.;
    if ( z > x[n-1] && y[n-1] == 0. )     return 0.;
    m = circe( z, x, n) ;
    if(!_circerr){
            dx = z - *(x+m-1) ;
        return spf(dx,x,y,k,m,&splder,1) ;
    }else{
        splder = 0.;
        return y[m];
    }
}

mirella void    /* mirella spline procedure */
m_spline()
{
    int n = pop;
    float *k = (float *)pop;
    float *y = (float *)pop;
    float *x = (float *)pop;
    float z = fpop;
    float dx ;
    int m ;

    m = circe( z, x, n) ;
    if(_circerr) { fpush(y[m]); return; }
    dx = z - *(x+m-1) ;
    fpush( spf(dx,x,y,k,m,&splder,1)) ;
}

/************************ DEFINT() (static )********************************/

static float d12 = .0833333333 ;

/*
 * definite integral using splines from knot i to knot j
 * fixed up 2006/02 jeg 
 */
mirella double 
defint(i,j,x,y,k)
int i, j;
float *x, *y, *k;
{
    float integ, h ;
    float sgn = 1. ;
    int l ;
    if ( j < i ){  /* going backwards */
        l=j; j=i; i=l ;
        sgn = -1.;
    }
    integ = 0. ;
    for ( l = i+1 ; l <= j ; l++ ) {
        h = *(x+l) - *(x+l-1) ;
        integ += 0.5*h*( *(y+l) + *(y+l-1))
            + h*h*d12*( *(k+l-1) - *(k+l) ) ;
        }
    return sgn * integ ;
}  

/*********************  INTEGRAL() ****************************************/
/*
 * does general integral from a to b
 */
mirella double 
integral(a,b,x,y,k,n)
double a,b ;
float *x, *y, *k ;
int n ;
{
    float de,he,ye ;
    float integ ;
    int i,j ;
    i = circe( a, x, n);
    j = circe( b, x, n);
    j = j-1; /* cell boundary *below* b */

    integ = defint(i,j,x,y,k) ;
    he = *(x+i) - a ;
    ye = spf(a - *(x+i-1),x,y,k,i,&de,1) ;
    integ += 0.5*(ye + *(y+i))*he + d12*he*he*( de - *(k+i) ) ;
    he = b - *(x+j) ;
    ye = spf(he,x,y,k,j+1,&de,1) ;
    integ += 0.5*(ye + *(y+j))*he + d12*he*he*( *(k+j) - de )  ;
    return integ ;
}

/**************** ROUTINES TO MAKE FUNCTIONS FROM TABLES IN FILES **********/

#define MAXENTRY 300000  /* back this off when you do the space allocation
                            correctly (ie NOT in the dictionary) */
static int ftser = 0;

static void rfbufcpy(dest,src,n)  /* copies the float array src into the
                                    float array dest, reversing as it goes */
register float *dest;
register float *src;
register int n;
{   
    src = src + n ;
    while(n--) *dest++ = *(--src);
}

/***************** FILEFNARRAY() **************************************/
/* from the filename 'file', extracts the column xcol for x data, ycol
for y data. Data columns are blank-delimited. 0 is the first column (!).
Rows beginning with '\' are ignored; a blank row or eof terminates the read.
if ord is >0, sets up k array for spline. The x values must be monotonic,
but may be increasing or decreasing. */

struct ftab_t *
filefnarray(file,xcol,ycol,ord)
char *file;
int xcol;
int ycol;
int ord;
{
    FILE *fp;
    char lbuf[200];
    char *wptr[100];
    int i;
    int err;
    int ncol;
    int ntab = 2;
    float *scrxtab,*scrytab,*keepxtab,*keepytab,*keepktab;
    struct ftab_t *sptr;
    int sstr = sizeof(struct ftab_t)/sizeof(float);  /* size in floats */
    int dir;
    char stser[10];
    int nentry;
    double atof();
    float scr,scr1;
    int xnmon = 0;   /* flag for nonmonotonic x table */
    char c;
    
    if((fp = fopen(file,"r")) == NULL) {        /* open input file */
        scrprintf("\nFILEFNARRAY:Cannot open %s",file);
        erret((char *)NULL);
    }
    /* allocate scratch space */
    if((scrxtab=(float *)malloc(MAXENTRY*2*sizeof(float))) == NULL) {  
        fclose(fp);
        erret("\nFILEFNARRAY:Cannot allocate scratch memory");
    }
    scrytab = scrxtab + MAXENTRY;
    nentry = 0;    
    mprintf("\nreading xcol=%d, ycol=%d from file %s",xcol,ycol,file);
    do{
        err = (int)fgets(lbuf,199,fp);
        c = lbuf[0];
        if(!err || c == '\n' || c == '\r') break;
        if(c == '\\' || c == ';' || c == '#' || c == '!') continue;  
            /* '\' or ';' or '#' or '!'  in 1st col is comment */
        ncol = parse(lbuf,wptr);
        if(ncol == 0) break;     /* quit if only whitespace */
        if(xcol >= ncol || ycol >= ncol){
            fclose(fp);
            free(scrxtab);
            scrprintf(
                "\nFILEFNARRAY: xcol=%d ycol=%d, but there are only %d columns"
                    ,xcol,ycol,ncol);            
            erret((char *)NULL);                    
        }
        /* check if column should be ignored */
        if( *(wptr[xcol]) == '*' || *(wptr[ycol]) == '*') continue;
        scrxtab[nentry] = atof(wptr[xcol]);
        scrytab[nentry] = atof(wptr[ycol]);
        nentry ++;
        if(nentry >= MAXENTRY){
            scrprintf(
                "\nFILEFNARRAY:Too many entries; I will stop reading here");
            break;
        }
    }while(1);
    fclose(fp);   /* done with input file */
    mprintf(" %d entries.",nentry);        
    /* have table; now allocate DICTIONARY space and move to correct-sized 
    table */

    if(ord) ntab = 3;   /* for splines, x,y, and k */
    sprintf(stser,"ft%04d",ftser++);        
    if((sptr =
          (struct ftab_t *)dicalloc(stser,(nentry*ntab+sstr)*sizeof(float)))
								     == NULL) {
        free(scrxtab);
        erret("\nFILEFNARRAY:Cannot allocate table storage");
    }        
    
    mprintf("\nCreated %s, pointer %u",stser,sptr);
    
    keepxtab = (float *)sptr + sstr;
    keepytab = keepxtab + nentry;
    if(ord) keepktab = keepytab + nentry;
    dir = (scrxtab[1] >= scrxtab[0] ? 1 : -1 );
    if(dir == 1){    /* if x array increasing, just copy */
        bufcpy((char *)keepxtab,(char *)scrxtab,nentry*sizeof(float));
        bufcpy((char *)keepytab,(char *)scrytab,nentry*sizeof(float));
    }else{          /* else reverse */
        rfbufcpy(keepxtab,scrxtab,nentry);
        rfbufcpy(keepytab,scrytab,nentry);
    }               
    free(scrxtab);   /* we are done with the scratch space */

    /* check for monotonicity of abscissa; if not, go ahead and finish,
    but warn the joe. */
    scr = keepxtab[0] /* *dir */;
    for(i=1;i<nentry;i++){
        scr1 = keepxtab[i] /* *dir */; /* I think this is already fixed above*/
        if(scr1 <= scr){
            xnmon = 1;
            scrprintf(
            "\nFILEFNARRAY:Nonmonotonic abscissa array at n=%d,old,new=%f,%f"
                ,i,scr,scr1);
            break;
        }
        scr = scr1;
    }
    /* if ord>0, want splines, so set up */
    if(ord && !xnmon) splinit(keepxtab,keepytab,keepktab,nentry);
    
    /* fill out function structure */
    sptr->ft_xptr = keepxtab;
    sptr->ft_yptr = keepytab;
    sptr->ft_kptr = ( ord ? keepktab : 0 );
    sptr->ft_n = nentry;
    sptr->ft_ord = ord;
    sptr->ft_maxx = keepxtab[nentry-1];
    sptr->ft_minx = keepxtab[0];
    sptr->ft_ser = ftser;    
    /* and return the pointer thereto */
    return sptr;
}

/************************ DOTABFUN() *********************************/
/*
 * takes a float argument and a pointer to a struct ftab_t, which contains
 * the particulars for the arrays needed to define the interpolating
 * quantities
 */
double
dotabfun(z,ftptr)    
double z;
struct ftab_t *ftptr;
{
    int datoff = ((char *)(ftptr->ft_xptr) - (char *)ftptr);
    int ord = ftptr->ft_ord;
/*    float minx = ftptr->ft_minx;
    float maxx = ftptr->ft_maxx;
    int n = ftptr->ft_n; */
         
    
    if(datoff != sizeof(struct ftab_t)) {
       printf("\nDOTABFUN: Invalid ftab structure; dataoff=%d, size=%d",
	      datoff, sizeof(struct ftab_t));
       erret((char *)NULL);
    }
    /* if data are off end but endpoints are zero, just return 0 and do
     * not complain */
    /* this code is now in spline() */ 
/*    if(z<minx && ftptr->ft_yptr[0] == 0.) return 0.;
    if(z>maxx && ftptr->ft_yptr[n-1] == 0.) return 0.; */

    return(ord ? 
        spline(z,ftptr->ft_xptr,ftptr->ft_yptr,ftptr->ft_kptr,ftptr->ft_n)
        :
        linterp(z,ftptr->ft_xptr,ftptr->ft_yptr,ftptr->ft_n)  );
}
        
/* there is code in dosmirella for double versions of much of the above--
 * let us leave it there for now.
 */                    

/******************** BICUBIC INTERPOLATOR *********************************/
/* 
 * IEEE Transactions on Acoustics, Speech, and Signal Processing ASSP-29, 6,
 * 1981, Robert G. Keys; it is O(h^3) and involves the sixteen points
 * surrounding the cell which the interpolated point occupies.
 */
 
/* this is the bicubic kernel, defined for all s, but only nonzero for
 * |s| < 2
 */
double 
bckern(double s)
{
    double s2;
    double s3;
    double ret;
    if ((s= fabs(s)) >= 2) return 0. ;
    s2 = s*s;
    s3 = s2*s;
    if (s < 1){
        ret = 3.*s3 - 5.*s2 + 2.;

    }else{
        ret = -s3 + 5.*s2 - 8.*s + 4.;
    }       
    return 0.5*ret;
}


/* the hocus pocus is so imat[0][0] is one in from the corner of the
 * matrix; so imat[-1][-1] is legal; the indices run from -1 to 2
 */
static float ipt[16];
static float *imatr[4] = {ipt+1,ipt+5,ipt+9,ipt+13} ;
/*static*/float **bc_imat;   /* = imatr+1; in bicube */

/* 
 * Interpolates in a (nx x ny) float image img using bicubic convolution.
 * x and y are float *indices* ranging from (0.,0.) to 
 * ((double)(nx-1),(double)(ny-1))
 */

double
bicube(double x, double y, float **img, int nx, int ny)
{
    int jx = x;
    int iy = y;
    /* flags for points at edge */
    int le = (x < 1);
    int re = (x >= nx - 2);
    int be = (y < 1);
    int te = (y >= ny - 2);
    int edge = (le || re || be || te);
    int jxl = le ? 0 : jx-1;
    int iyl = be ? 0 : iy-1;
    int jxu = re ? nx-1: jx+2;
    int iyu = te ? ny-1: iy+2;
    int i, j;
    double ret = 0.;
    
    bc_imat = imatr + 1;
    
    for( i=0; i<16; i++) ipt[i] = 0.;  /* debug */
   
    for ( i = iyl ; i <= iyu ; i++){
        for ( j = jxl ; j <= jxu ; j++){
            bc_imat[i-iy][j-jx] = img[i][j];
        }
    }
    /* if an edge point, must extrapolate */
    if(edge){
        if(le){
            for( i = iyl; i <= iyu ; i++ ){
                bc_imat[i-iy][-1] = 3.*(img[i][0] - img[i][1]) + img[i][2];
            }
            if(be) bc_imat[-1][-1] = 
                3.*(bc_imat[0][-1] - bc_imat[1][-1]) + bc_imat[2][-1];
            if(te) bc_imat[2][-1] =  
                3.*(bc_imat[1][-1] - bc_imat[0][-1]) + bc_imat[-1][-1];
        }
        if(re){
            for( i = iyl; i <= iyu ; i++ ){
                bc_imat[i-iy][2] = 
                    3.*(img[i][nx-1] - img[i][nx-2]) + img[i][nx-3];
            }
            if(be) bc_imat[-1][2] = 
                3.*(bc_imat[-1][1] - bc_imat[-1][0]) + bc_imat[-1][2];
            if(te) bc_imat[2][2] =  
                3.*(bc_imat[2][1]  - bc_imat[2][0])  + bc_imat[2][-1];
        }
        if(be){
            for( j = jxl; j <= jxu; j++){
                bc_imat[-1][j - jx] = 3.*(img[0][j] - img[1][j]) + img[2][j];
            }
            if(le) bc_imat[-1][-1] = 
                3.*(bc_imat[0][-1] - bc_imat[1][-1]) + bc_imat[2][-1];
            if(re) bc_imat[-1][2]  = 
                3.*(bc_imat[0][2]  - bc_imat[1][2])  + bc_imat[2][2];
        }
        if(te){
            for( j= jxl; j <= jxu; j++){
                bc_imat[2][j-jx] = 
                    3.*(img[ny-1][j] - img[ny-2][j]) + img[ny-3][j];
            }
            if(le) bc_imat[2][-1] = 
                3.*(bc_imat[1][-1] - bc_imat[0][-1]) + bc_imat[-1][-1];
            if(re) bc_imat[2][2]  = 
                3.*(bc_imat[1][2]  - bc_imat[0][2] ) + bc_imat[-1][2];
        }
        /* 
         * Now bc_imat is populated with either real or extrapolated values;
         * do the interpolation
         */
    } 
    for( i = -1; i <= 2; i++){
        for(j = -1; j <= 2; j++){
            ret += bc_imat[i][j]*bckern(y - (iy + i))*bckern(x - (jx + j));
        }
    }
    return ret;
}



        
/******************** END, MIRSPLIN.C **************************************/


