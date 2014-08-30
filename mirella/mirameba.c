/* version 88/06/02:Mirella 5.0                                          */

/********************* MIRAMEBA.C ****************************************/

/* This module is essentially the numerical recipes amoeba program, modified
for Mirella/tools support */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

static float ALPHA=1.0;
static float BETA=0.5;
static float GAMMA=2.0;

mirella normal am_iter = 0;      /* final iteration in samoeba */
mirella float  am_tol = 2.e-6;   /* tolerance in samoeba */
mirella int    am_pflg = 0;      /* flag to print iteration and error */
mirella int    am_debug = 0;     /* flag to pring intermediate results */
mirella int    am_itmax = 500;   /* maximum number of iterations */
extern int interrupt;

/*
 * p is a matrix[ndim+1][ndim] which is initialized to the n+1 vectors of
 * the starting simplex; at the end, any row is a solution.
 * ndim is the dimension of the space (number of parameters)
 * ftol is the fractional tolerance (try 2.e-6 for singles)
 * funk is the pointer to the function which you are trying to minimize; it
 * takes a float vector argument of length ndim.
 * piter is a pointer to the iteration number.
 * the function returns funk averaged over the final simplex.
 */
mirella double 
amoeba(p,ndim,ftol,funk,piter)
float **p;
int ndim;
double ftol;
double (*funk)();
normal *piter;
{
    int mpts,j,inhi,ilo,ihi,i;
    float yprr,ypr,rtol,*pr,*prr,*pbar,ybar;
    float *y;
    float step;
    
    mpts=ndim+1;    
    y=(float *)malloc((mpts)*sizeof(float));
    pr=(float *)malloc((ndim)*sizeof(float));
    prr=(float *)malloc((ndim)*sizeof(float));
    pbar=(float *)malloc((ndim)*sizeof(float));
    /* init y to values of funk at vertices */
    for(i=0;i<mpts;i++) y[i] = (*funk)(p[i]);

    *piter=0;
    ilo = 0;
    if(am_pflg) scrprintf("\n");
begin:    
    for(;;) {
        ilo=0;      
        if (y[0] > y[1]) {
            ihi=0;
            inhi=1;
        } else {
            ihi=1;
            inhi=0;
        }
        for(i=0;i<mpts;i++) {
            if (y[i] < y[ilo]) ilo=i;
            if (y[i] > y[ihi]) {
                inhi=ihi;
                ihi=i;
            } else if (y[i] > y[inhi])
                if (i != ihi) inhi=i;
        }
        if(am_debug){
            scrprintf("\ni,yl,yh=%3d %12.3f %12.3f\nx[ilo]:",*piter,y[ilo],y[ihi]);
            for(j=0;j<ndim;j++) scrprintf(" %7.5f",p[ilo][j]);
        }
        if ((*piter)++ >= am_itmax) {
            scrprintf("\nToo many iterations in AMOEBA");
            goto cleanup;
        }
        rtol=2.0*fabs(y[ihi]-y[ilo])/(fabs(y[ihi])+fabs(y[ilo]));
        if(am_pflg && !((*piter)%10)){
            ybar = 0;
            for(i=0;i<mpts;i++) ybar += y[i];
            ybar /= mpts;
            scrprintf("\ri=%3d f=%f tol=%f",*piter,ybar,rtol);
        }
        if (rtol < ftol) break;
        if(dopause()){
            interrupt = 0; 
            scrprintf("\nDo you want to rescale and start again?");
            if(_yesno()){
                scrprintf("\nStepsize? ");
                scanf("%f",&step);
                j=0;
                for(i=0;i<mpts;i++) if(i != ilo) p[i][j++] += step;
                goto begin;
            }else goto cleanup;
        }
        for(j=0;j<ndim;j++) pbar[j]=0.0;
        for(i=0;i<mpts;i++)
            if (i != ihi)
                for(j=0;j<ndim;j++)
                    pbar[j] += p[i][j];
        for(j=0;j<ndim;j++){
            pbar[j] /= ndim;
            pr[j]=(1.0+ALPHA)*pbar[j]-ALPHA*p[ihi][j];
        }
        ypr=(*funk)(pr);
        if (ypr <= y[ilo]) {
            for(j=0;j<ndim;j++)
                prr[j]=GAMMA*pr[j]+(1.0-GAMMA)*pbar[j];
            yprr=(*funk)(prr);
            if (yprr < y[ilo]) {
                for(j=0;j<ndim;j++) p[ihi][j]=prr[j];
                y[ihi]=yprr;
            } else {
                for(j=0;j<ndim;j++) p[ihi][j]=pr[j];
                y[ihi]=ypr;
            }
        } else if (ypr >= y[inhi]) {
            if (ypr < y[ihi]) {
                for(j=0;j<ndim;j++) p[ihi][j]=pr[j];
                y[ihi]=ypr;
            }
            for(j=0;j<ndim;j++)
                prr[j]=BETA*p[ihi][j]+(1.0-BETA)*pbar[j];
            yprr=(*funk)(prr);
            if (yprr < y[ihi]) {
                 for(j=0;j<ndim;j++) p[ihi][j]=prr[j];
                 y[ihi]=yprr;
            } else {
                for(i=0;i<mpts;i++) {
                    if (i != ilo) {
                        for(j=0;j<ndim;j++) {
                            pr[j]=0.5*(p[i][j]+p[ilo][j]);
                            p[i][j]=pr[j];
                        }
                        y[i]=(*funk)(pr);
                    }
                }
            }
        } else {
            for(j=0;j<ndim;j++) p[ihi][j]=pr[j];
            y[ihi]=ypr;
        }
    }
cleanup: 
    free(y);
    free(pbar);
    free(prr);
    free(pr);    
    ypr = 0.;
    for(i=0;i<mpts;i++) ypr += (*funk)(p[i]);
    ypr /= (float)mpts;
    if(am_pflg) scrprintf("\n");
    am_iter = *piter;			/* export to mirella */
    return ypr;
}

/*
 * simple amoeba interface
 */
mirella double 
samoeba(x,dx,funk,nd)
float *x, *dx;          /* initial guess, initial deltas */
double (*funk)();       /* function */   
int nd;                 /* dimension */
{
    float ***mp;
    float **p;
    int i,j;
    float ret;
    
    mp = (float ***)matralloc("amoebamat",nd+1,nd,sizeof(float));
    p = *mp;
    for(i=0;i<nd+1;i++){
        for(j=0;j<nd;j++){
            p[i][j] = x[j];
            if(j == i-1) p[i][j] += dx[j];
        }
    }
    ret = amoeba(p,nd,am_tol,funk,&am_iter);
    bufcpy((char *)x,(char *)p[0],nd*sizeof(float));
    chfree((char *)mp);
    return ret;
}


