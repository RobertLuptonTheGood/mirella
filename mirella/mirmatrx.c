/*VERSION 88/10/24: Mirella 5.10                                      */

/***************** MIRMATRX.C *****************************************/

#ifdef VMS
#include mirella
#include dotokens
#else
#include "mirella.h"
#include "dotokens.h"
#endif

/* It is assumed that all matrices are allocated with matralloc() -- see
mirelmem.c -- , which sets up storage as an array of pointers to lines,
followed by the matrix data itself.  It is not assumed in what follows
that the storage is contiguous, though in practice it is if that rule is
followed.  In Mirella, the row and column numbers are available in the
dictionary entry, and are used in this code for addition and
multiplication and inversion.  */

/*
 * returns absolute max of float array a, pointer to index of max element
 * in pj
 */
double
afamax(a, n, pj)
float *a ;
register int n ;
int *pj ;
{
    int j= 0 ;
    register int i ;
    float b,c ;


    b = 0. ;
    for (i=0 ; i < n ; i++ ) {
        if((c = fabs(*(a+i))) > b) {
            b=c ;
            j=i ;
        }
    }
    *pj = j ;
    return b ;
}

/*************************** MADD() *************************************/

void
madd( pa, pb, pc, n, m )   /* adds matrices a,b, sum in c */
int n, m ;
float *pa[],*pb[],*pc[] ;
{
    register int i,j ;
    for( i=0 ; i<n ; i++ ){
        for ( j=0; j<m; j++){ 
            *(*(pc+i)+j) = *(*(pa+i)+j) + *(*(pb+i)+j) ;
        }
    }
    return ;
}

/*********************** MCMUL() ****************************************/

/*
 * multiplies matrix a by constant c, product in b
 */
void
mcmul(pa,c,pb,n,m)
float *pa[], *pb[] ;
double c ;
int n, m ;
{
    register int i, j ;
    for ( i=0 ; i < n ; i++ ){
        for( j=0; j<m; j++){
            *(*(pb+i)+j) = c * *(*(pa+i)+j) ;
        }
    }
    return ;
}

/************************* MMUL() ****************************************/ 

void
mmul( pa, pb, pc, n, m, l)  /* multiplies a(nxm) by b(mxl), res in c(nxl) */
int n,m,l ;
float *pa[],*pb[],*pc[] ;
{
    register int j ;
    register int i,k ;
    for (i = 0 ; i < n ; i++){
        for (k = 0 ; k < l ; k++){
            *(*(pc+i)+k) = 0. ;
            for (j = 0 ; j < m ; j ++ ){
                *(*(pc+i)+k) += (*(*(pa+i)+j)) * (*(*(pb+j)+k));
            }
        }
    }
    return ;
}

/******************** MVMUL() ******************************************/

/*
 * multiplies matrix (n x m) by vector (dim m), result in pvec(dim n)
 */
void
mvmul(pa,vec,pvec,n,m)
float *pa[],*vec,*pvec;
int n,m;
{
    register int i,j ;
    register float sum;
    
    for(i=0;i<n;i++){
        sum = 0.;
        for(j=0;j<m;j++) sum += pa[i][j]*vec[j];
        pvec[i] = sum;
    }
}

/******************** VMMUL() ******************************************/

/*
 * multiplies vector (dim n) by matrix (n x m) result in vector pvec
 */
void 
vmmul(vec,pa,pvec,n,m)
float *vec,*pa[],*pvec;
int n,m;
{
    register int i,j;
    register float sum;
    for(j=0;j<m;j++){
        sum = 0.;
        for(i=0;i<n;i++) sum += vec[i]*pa[i][j];
        pvec[j] = sum;
    }
}


/******************* MATINV() *******************************************/
/* matrix inverter, pivoted gauss elimination, inverts pa[i][j] and puts
result in pai[i][j]. Returns determinant */
/* NB!!! it appears that this destroys pa. Check if it is doing the
   right thing!! If it IS, it should allocate a scratch matrix to bugger */

#define BUFFER 1.e-5
extern double afamax() ;

double 
matinv(pa,pai,n)
float *pa[],*pai[];
int n ;
{
    float am, c, m, akk, r, *sc, *xt ;
    int im, *rp, i, j, k, l, rpi, rpk, it ;
    float det;
    
    sc = (float *)malloc(n*sizeof(float));
    xt = (float *)malloc(n*sizeof(float));
    rp = (int *)malloc(n*sizeof(float));

    if(!sc || !xt || !rp) erret(
        "\nMATINV: Cannot allocate intermediate vectors");
    for( i = 0 ; i < n ; i++) {
        rp[i] = i ;  /* initialize row ptrs */
        sc[i] = afamax( pa[i], n, &im ) ; /* scale facs=max row elts */
    }
    for( k = 0 ; k < n ; k++) {
        /* first find max abs scaled element in col k, row >= k */
        am = 0. ;
        for ( i = k ; i < n ; i++ ) {
            rpi = rp[i] ;
            c = fabs(*(*(pa+rpi)+k))/sc[rpi] ;
            if(c > am ){
                am = c ;
                im = i ;
            }
        }
        it = rp[im] ;
        rp[im] = rp[k] ;
        rp[k] = it ;
        akk = *(*(pa + it) + k ) ;
        for ( i = k+1 ; i < n ; i++ ) {
            rpi = rp[i] ;
            m = (*(*(pa + rpi) + k ) =  *(*(pa + rpi)+k)/akk) ;
            for( j = k + 1 ; j < n ; j++ )
                *(*(pa + rpi)+j) -= m * (*(*(pa + it) + j )) ;
        }
    }
    
    /* matrix now decomposed. compute determinant */
    det = 1. ;
    for ( i=0 ; i < n ; i++) det *= *(*(pa + rp[i])+i) ;
    
    /* now compute inverse. first set RHS to identity */
    for( i=0 ; i < n ; i++ ) clearn( n*sizeof(float), (char *)pai[i] ) ;
    for( i=0 ; i < n ; i++){
        rpi = rp[i] ;
        for ( l=0 ; l < n ; l++ ){
            *(*(pai + rpi)+l) = (rpi == l ? 1. : 0.);
        }
    }
    /* now run multipliers through rhs  */
    for( i=0 ; i<n ; i++ ){
        rpi = rp[i] ;
        for ( l=0 ; l < n ; l++ ){
            for ( k= i+1; k < n; k++) {
                rpk = rp[k] ;
                *(*(pai+rpk)+l) -= (*(*(pai+rpi)+l)) * (*(*(pa + rpk)+i)) ;
            }
        }
    }
    /* now do back substitution */
    for ( l= 0 ; l < n ; l++ ) {
        for ( i = n-1 ; i >= 0 ; i-- ){
            rpi = rp[i] ;
            r = *(*(pai+rpi)+l) ;
            for (j = i+1 ; j <n ; j++){
                r  -= (*(*(pa+rpi)+j) * (*(xt+j))) ;
            }
            xt[i] = r/(*(*(pa+rpi)+i)) ;
        }
        for ( i=0 ; i < n ; i++){
            *(*(pai+i)+l) = *(xt+i) ;
        }
    }
    free(xt);
    free(rp);
    free(sc);
    return (det);
}

/************************* LINSOLV() ***********************************/
/*  Linear equation solver. Solves pa[i][j]*x[j] = b[i] */

#define BUFFER 1.e-5
extern double afamax() ;

void 
linsolv(pa,b,x,n)
float *pa[],b[],x[] ;
int n ;
{
    float am, c, m, akk, r, *sc ;
    int im, *rp, i, j, k, rpi, it ;
    
    sc = (float *)malloc(n*sizeof(float));
    rp = (int *)malloc(n*sizeof(int));
    if(!sc || !rp) erret("\nLINSOLV: Cannot allocate intermediate matrix");

    for( k = 0 ; k < n ; k++) {
        rp[k] = k ;  /* initialize row ptrs */
        sc[k] = afamax( pa[k], n, &im ) ; /* scale facs=max row elts */
    }
    for( k = 0 ; k < n ; k++) {
        /* first find max abs scaled element in col k, row >= k */
        am = 0. ;
        for ( i = k ; i < n ; i++ ) {
            rpi = rp[i] ;
            c = fabs(*(*(pa+rpi)+k))/sc[rpi] ;
            if(c > am ){
                am = c ;
                im = i ;
            }
        }
        it = rp[im] ;
        rp[im] = rp[k] ;
        rp[k] = it ;
        akk = *(*(pa + it) + k ) ;
        for ( i = k+1 ; i < n ; i++ ) {
            rpi = rp[i] ;
            m = (*(*(pa + rpi) + k ) =  *(*(pa + rpi)+k)/akk) ;
            for( j = k + 1 ; j < n ; j++ ){
                *(*(pa + rpi)+j) -= m * (*(*(pa + it) + j )) ;
            }
            b[rpi] -= m * b[it] ;
        }
    }
    for( i = n-1 ; i >= 0 ; i--) {
        rpi = rp[i] ;
        r = b[rpi] ;
        for ( j = i + 1 ; j <n ; j++ ) r -= (*(*(pa + rpi)+j))*x[j] ;
        if ( fabs((m=(*(*(pa+rpi)+i)))) > BUFFER*sc[rpi] ){
            x[i] = r/m ;
        }else if( r < BUFFER*sc[rpi] ){
            mprintf("\nmatrix singular, system consistent\n");
            x[i] = 0. ;
        }else{
            free(rp);
            free(sc);
            erret("\nmatrix singular");
        }
    }
    free(sc);
    free(rp);
}

/***************** TRIDIAGONAL SOLVER ***************************************/
/* This has the same interface as the mirsplin one, but should be faster--if
 * it works, removde that one. Note that the matrix is represented as 3
 * linear arrays, not a matrix. 
 */
 
/*********************** TRIDI() *******************************/

/* solves tridiagonal linear system .   
 * diag elts mii=ai, subdiag mii-1=bi, superdiag mii+1=ci.
 * it is understood that b0 and cn-1 are zero, but are not referenced.
 * f is rhs, x soln, may be same array; all arrays are trashed
 * This version should be much faster--I do not see why it won't work??? 
 * Well, it wouldn't have--stupid typo.
 */

mirella void
tridi(a,b,c,f,x,n)  
float *a, *b, *c, *f, *x;
int n ;
{
    register int i;
    register float beta;
    for ( i=1; i < n; i++) {
        beta = b[i]/a[i-1];
        a[i] -= beta * c[i-1];
        f[i] -= beta * f[i-1];
    }
    x[n-1] = f[n-1]/a[n-1] ;
    for( i=n-2; i > -1; i-- ){
        x[i] = (f[i] - (c[i] * x[i+1])) / a[i];
    }
}

/* this one conditions  first, but should be the same */

mirella void tridi2(a,b,c,f,x,n)
float *a, *b, *c, *f, *x;
int n;
{
    register int i;
    register float beta;
    for (i=0; i< n; i++){
        beta = 1/a[i];
        b[i] *= beta ;
        c[i] *= beta ;
        f[i] *= beta ;
        a[i] = 1. ;     /* remove later */
    }
    /* matrix now has a[i] = 1 */
    for( i=1; i < n; i++ ){
        beta = 1./(1. - b[i]*c[i-1]);
        c[i] *= beta;
        f[i] = beta*(f[i] - b[i]*f[i-1]);
        b[i] = 0.;    /* remove later */
    }
    /* matrix is now upper diagonal and still has unit diagonal */
    x[n-1] = f[n-1];
    for( i = n-2; i > -1; i--){
        x[i] = f[i] - c[i]*x[i+1];
    }
}   

/* this one deals with very diagonally dominant matrices to help
 * roundoff. Since tridi trashes the rhs, you need to supply a
 * dummy array (we could malloc and free, but this seems simpler)
 * to save the rhs.
 */
mirella void trididd(a,b,c,f, fs, x,n)
float *a, *b, *c, *f, *fs, *x;
int n;
{
    register int i;
    register float beta;
    for (i=0; i< n; i++){
        beta = 1/a[i];
        b[i] *= beta ;
        c[i] *= beta ;
        f[i] *= beta ;
        fs[i] = f[i];
    }
    /* matrix now has a[i] = 1; we use this matrix throughout rest of code 
     * The idea is to solve A(x-f) = f - Af, so we deal only with small
     * f and x values. It may not help at all, but may.
     * We begin by evaluating f - Af and substuting for f
     */ 
    f[0] = 0.;
    for( i=1; i< n; i++){
        f[i] = -b[i]*fs[i-1];
    }
    for( i=0; i<n-1; i++){
        f[i] += -c[i]*fs[i+1];
    }
    /* now make upper diagonal */
    for( i=1; i < n; i++ ){
        beta = 1./(1. - b[i]*c[i-1]);
        c[i] *= beta;
        f[i] = beta*(f[i] - b[i]*f[i-1]);
        b[i] = 0.;    /* remove later */
    }
    /* matrix is now upper diagonal and still has unit diagonal */
    /* now march through and solve it */
    x[n-1] = f[n-1];
    for( i = n-2; i > -1; i--){
        x[i] = f[i] - c[i]*x[i+1];
    }
    /* and now add back in f */
    for(i=0; i<n; i++) x[i] += fs[i]; 
}   

/*********************** MIRELLA INTERFACE **********************************/
/* These procedures all take as arguments for the matrices the cfa's of the */
/* relevant matrices; the dimensions are retrieved from the dictionary      */
/* entries, and are checked. The pointer to the pointer array is also       */
/* calculated, and is done correctly for any kind of matrix storage         */
/*                                                                          */
/* thus to add two matrices a and b, sum in c:                              */
/*   ' a ' b ' c madd                                                       */
/*                                                                          */
/* vectors, however, are simply addresses, as usual. This is a little       */
/* inconsistent, but without doing much violence to existing code cannot    */
/* be easily undone.                                                        */
/****************************************************************************/

/*
 * returns the address of the pointer array for the matrix whose cfa
 * is cfa-- same functionality as (m' in mirella.m, but checks for
 * normal-sized matrices, since the arithmetic works only for
 float matrices
 */
float **
mtick(cfa)
normal *cfa;
{
    switch(*cfa){
    case DONMAT:
        return (float **)(cfa + 3);
    case DONCMAT:
    case DONMMAT:
        return (float **)(*(float ***)(*(float ****)(cfa+3)));
    default:
        mprintf("\n%d is not a valid float matrix token",*cfa);
        flushit();
        erret((char *)NULL);
        break;
    }
    return(NULL);			/* NOTREACHED - erret doesn't return */
}

void
m_madd()
{
    normal *pc = (normal *)pop;
    normal *pb = (normal *)pop;
    normal *pa = (normal *)pop;
    normal nr = *(pa + 2);
    normal nc = *(pa + 1);     /* nrow, ncol */
    if(nr != *(pb+2) || nr != *(pc+2) || nc != *(pb+1) || nc != *(pc+1))
        erret("\nsorry:matrices not same size");
    madd(mtick(pa),mtick(pb),mtick(pc),nr,nc);
}

void 
m_mcmul()
{
    normal *pb = (normal *)pop;
    float c    = fpop;
    normal *pa = (normal *)pop;
    normal nr = *(pa+2);
    normal nc = *(pa+1);
    
    if(nr != *(pb+2) || nc != *(pb+1))
        erret("\nsorry:matrices not same size");
    mcmul(mtick(pa),c,mtick(pb),nr,nc);
}

void 
m_mmul()    
{
    normal *pc = (normal *)pop;
    normal *pb = (normal *)pop;
    normal *pa = (normal *)pop;
    normal l = *(pb+1);
    normal m = *(pb+2);
    normal n = *(pa+2);
    
    if(m != *(pa +1) || n != *(pc+2) || l != *(pc+1))
        erret("\nsorry:matrices not correct size");
    mmul(mtick(pa),mtick(pb),mtick(pc),n,m,l);
}

void 
m_mvmul()
{
    float *pvec = (float *)pop;
    float *vec  = (float *)pop;
    normal *pa = (normal *)pop;
    normal nr = *(pa+2);
    normal nc = *(pa+1);
    /* the vectors are simply assumed to be big enough */
    mvmul(mtick(pa),vec,pvec,nr,nc);
}

void 
m_vmmul()
{
    float *pvec = (float *)pop;
    normal *pa = (normal *)pop;
    float *vec = (float *)pop;
    normal nr = *(pa+2);
    normal nc = *(pa+1);
    
    vmmul(vec,mtick(pa),pvec,nr,nc);
}

void 
m_matinv()
{
    normal *pai = (normal *)pop;
    normal *pa = (normal *)pop;
    normal nr = *(pa+2);
    
    if(nr != *(pa+1) || nr != *(pai+1) || nr != *(pai+2))
        erret("\nsorry: matrices not square or not same size");
    fpush( matinv(mtick(pa),mtick(pai),nr));
}

void 
m_linsolv()
{
    float *x = (float *)pop;
    float *b = (float *)pop;
    normal *pa = (normal *)pop;
    normal nr = *(pa+2);
    
    if(nr != *(pa+1)) erret("\nsorry: matrix not square");
    linsolv(mtick(pa),b,x,nr);
}

/* need a simple mprint routine to display matrices. */
    
/********************* END MIRMATRX.C **************************************/


