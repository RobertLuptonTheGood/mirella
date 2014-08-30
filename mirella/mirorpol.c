/* VERSION 88/10/24: Mirella 5.10                                          */

/********************** MIRORPOL() *****************************************/
/* package for orthogonal polynomial fitting  */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

#define MAXPTS 100
#define MAXORD 10
#define MAXORP 11

static double aorp[ 121 ] ;
static double *ap[ MAXORP ] ;
static double dp();
static double q[ MAXORP ] ;   /* normalizations */
static float p[ 1100 ] ;      /* MAXORP*MAXPTS */
static float *pv[ MAXORP ] ;
static int npt ;
static float *wts ;
int ver_orpol = 0;     /* "version" number to allow checking whether
                       polynomials need regenerating; incremented by
                       mkorpol each time it is called */

/************************ MKORPOL() *************************************/
/*
 * mkorpol makes a set of polynomials which are orthogonal with respect to
 * the weights w; ie SUM w[i]*Pj(x[i])*Pk(x[i]) = q[j]delta(j,k).
 * If you wish to construct a set of orthogonal functions which are
 * f(x)Pj(x), just use a set of weights which are w' = w*f(x)^2; the
 * routine will construct the poly part
 */
mirella void 
mkorpol(w,x,n,m)
float *w ;        /* weights */  
float *x ;        /* abscissa array */
int n;            /* number of points */
int m ;           /* maximum order */
{
	register int i,j ;
	double alpha, beta ;
	int maxo2 = MAXORP*MAXORP ;
	int maxop = MAXORP*MAXPTS ;
	
        ver_orpol++;
	wts = w ;
	npt = n ;
	for ( i=0 ; i< maxo2 ; i++ )
		aorp[i] = 0. ;
	for ( i=0 ; i < MAXORP ; i++ )  /* pointers to polys */
		ap[i] = aorp + i*MAXORP ;
	for ( i=0 ; i < maxop ; i++ )    /* poly value matrix */
		p[i] = 0. ;
	for ( i=0 ; i < n ; i++ )        
		*(p+i) = 1. ;     /* p0 is identically 1 */
	for ( i=0 ; i < MAXORP ; i++ ) 
		pv[i] = p + i * n ;
	q[0] = dp( p,p,p ) ;
	aorp[0] = 1. ;
	for ( j=1 ; j <= m ; j++ ) {
		if ( j > 1 ){
			beta = -q[j-1]/q[j-2] ;
		}else{
			beta = 0. ;
		}
		alpha = -dp(x, pv[j-1] , pv[j-1] )/ q[j-1] ;
		for (i=0 ; i<n ; i++ )
			*(*(pv+j)+i) = (x[i] + alpha) * (*(*(pv+j-1)+i))
				+ beta * ( j > 1 ? (*(*(pv+j-2)+i)) : 0. )  ;
		for (i=1 ; i <= j ; i++ )
			*(*(ap+j)+i) = *(*(ap+j-1) + i-1 );
		for (i=0 ; i<j ; i++ )
			*(*(ap+j)+i) += alpha * (*(*(ap+j-1)+i))
				+ beta * ( j>1 ? (*(*(ap+j-2)+i)) : 0. ) ;
		q[j] = dp( pv[j], pv[j], p ) ;
	}
#ifdef DEBUG          /* this crashes for some reason--NAN error */
 	scrprintf("\n") ;
	for ( i=0 ; i < npt ; i++)  {
		scrprintf ("\n %5.1f",x[i]) ;
		for (j=0 ; j < 7 ; j++ ){
			scrprintf (" %8.5f", *(*(pv+j)+i) ) ;
		}
		scrprintf("\n      ") ;
		for ( j=0 ; j < 7 ; j++ ){
			scrprintf (" %8.5f", dpoly(ap[j],x[i],j)) ;
		}
	}
	for (i=0 ; i <= m ; i++ )  {
		scrprintf ("\n order %d  ", i) ;
		for ( j=0 ; j <= i ; j++ ){
			scrprintf ( " %8.5f", *(*(ap+i)+j)) ;
		}
	}
#endif
}

/************************** ORPFIT() *****************************************/

mirella void
orpfit(y,ay,m)   /* y is the ordinate array, m the max order, ay the polynomial
		    coefficients. orpfit expects FLOAT coef, note. If you are
		    fitting to a set f(x)Pj(x) as described in mkorpoly(), the
		    ordinates presented to this routine need to be the ratio 
		    of the real ordinates to f(x)   */
float y[], ay[] ;
int m ;

{
	double bc ;
	int i ;
	int j ;

	for (i=0 ; i <= m ; i++ )
		ay[i] = 0. ;
	for ( j=0 ; j <= m ; j++ ) {
		bc = dp(y,pv[j],p)/q[j] ;
		for (i=0 ; i <= j ; i++)
			ay[i] += bc * (*(*(ap+j)+i)) ;
		}
}

static double 
dp(a1, a2, a3)
float a1[], a2[], a3[] ;

{
	double dot ;
	register int i ;
	dot = 0. ;
	for ( i=0 ; i < npt ; i++ )
		dot += a1[i] * a2[i] * a3[i] * (*(wts+i)) ;
	return dot ;
}

/****************************** POLY() **************************************/

/*
 * poly expects float coef array. Note that the arg order is different
 * from the routine in ctools
 */
double 
fpoly(z,a,m)
double z ;
float a[] ;
int m ;

{
	register int i ;
	float p ;
	p = 0. ;
	for ( i=m ; i >0 ; i-- )
		p = z * (p + a[i]) ;
	p += a[0] ;
	return p ;
}

mirella void m_poly()
{
    int m = pop;
    register float *a = (float *)pop;
    float z = fpop;    
    register int i ;
    float p ;
    
    p = 0. ;
    for ( i=m ; i >0 ; i-- )
        p = z * (p + a[i]) ;
    p += a[0] ;
    fpush(p) ;
}

/************************** END, MIRORPOL.C ********************************/


