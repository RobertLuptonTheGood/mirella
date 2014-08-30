/*VERSION 88/10/24: Mirella 5.10                                          */

/* most of the verbiage is JUNK. these routines should be rewritten
   from scratch, incorporating masks, etc */

/************************  MIRHIST.C **************************************/
/* general histogram-twiddling routines for Mirella */    
/* basic ingredients are a floating array of values to histogram,
a char array (yesno) mask, a float array of abscissae for the
CENTERS of the histogram cells (the indices thought of
as belonging to the centers as well; cell 0 is the number in -0.5-> +0.5.)
and an int histogram array. No memory is allocated by these routines. */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

/************************* INITMASK() *************************************/

mirella void 
initmask(mask,npt)
char *mask;
int npt;
{
    int i;
    for(i=0;i<npt;i++) mask[i] = 1;
}

/*************************** SETMASK() **************************************/

/*
 * set mask on basis of window in acceptance array specarr, which must
 * have same indexing as array to histogram returns number of accepted pts
 */
mirella int 
setmask(specarr,mask,llim,ulim,npt)
float *specarr;
char *mask;
double llim,ulim;
int npt;
{
    register int i;
    float mem;
    int nacc = 0;    
    
    for(i=0;i<npt;i++){
        if((mem = specarr[i])>ulim || mem < llim)   mask[i] = 0;  /* reject */
        else nacc++ ;    /* accept */
    }
    return nacc;
} 

/******************* PUTHIST(), M_PUTHIST() *********************************/
/* these routines ignore overflow past histarr boundaries */

void 
puthist(q,histarr,cell,origin,ncell)
double q;             /* quantity to drop in histogram */
int  *histarr;        /* histogram */
double cell,origin;   /* cell size, lower boundary of 0th cell */
int ncell;            /* number of cells */
{
    int nc = (q-origin)/cell + 0.5; 
    if(nc >= 0 && nc < ncell) histarr[nc]++;
}

mirella void m_puthist()   /* Mirella procedure (WHY?)  */
{
    int ncell = pop;
    float origin = fpop;
    float cell = fpop;
    normal *histarr = (normal *)pop;
    float q = fpop;
    int nc = (q-origin)/cell + 0.5;
    if(nc >= 0 && nc < ncell) histarr[nc]++;
}

/******************* PUTHISTEX(), M_PUTHISEX() ******************************/
/* these routines populate cell 0 and ncell-1 with all data outside bdys */

void 
puthistex(q,histarr,cell,origin,ncell)
double q;             /* quantity to drop in histogram */
int  *histarr;        /* histogram */
double cell,origin;   /* cell size, lower boundary of 0th cell */
int ncell;            /* number of cells */
{
    int nc = (q-origin)/cell + 0.5; 
    if(nc < 0) nc = 0;
    if(nc >= ncell) nc=ncell-1;
    histarr[nc]++;
}

mirella void m_puthistex()   /* Mirella procedure. WHY?? */
{
    int ncell = pop;
    float origin = fpop;
    float cell = fpop;
    normal *histarr = (normal *)pop;
    float q = fpop;
    int nc = (q-origin)/cell + 0.5; 
        
    if(nc < 0) nc = 0;
    if(nc >= ncell) nc=ncell-1;
    histarr[nc]++;
}



/************************** CLRHIST() ************************************/

mirella void 
clrhist(histarr,ncell)
int *histarr;
int ncell;
{
    register int i;
    
    for(i=0;i<ncell;i++) histarr[i] = 0;
}

/***********************  ANHIST() *************************************/
/* NB!!!! qarr must have 4!!! cells. First is populated with the mean */

mirella double 
anhist(histarr,ncell,qarr)   /* returns equivalent sigma */
int * histarr;       /* histogram */
float *qarr;         /* array for storing quartiles */
int ncell;           /* no of cells */
{
    int nt,a,b;
    register int i;
    int nhist;    /* total count */
    float incr;
    float r;
    float alim;
    double sigma;
    double mean;
    
    /* count histogram */
    b=0;
    nhist=0;
    for(i=0;i<ncell;i++){ 
        nhist += histarr[i];
        b += (float)(i * histarr[i]);
    }        
    mean = b/(float)nhist;
    
    /* zero ntile array */
    for(i=0;i<4;i++) qarr[i] = 0;    
    /* find ntiles */
    incr = ((float)nhist)/4.;   /* number in each quartile */
    nt = 0;  /* first ntile */
    a = 0;
    /* mprintf(" ntiles="); */
    for(i=0;i<ncell ;i++){
        a += histarr[i];
        if((double)a > (alim = (float)((nt+1)*incr))){   /* next ntile */
            r = ((double)a - alim)/((double)histarr[i]);
            qarr[++nt] = i - r + 0.5 ;
            /* mprintf(" %4.3f ",qarr[nt]); */
        }
        if( nt == 3) break;
    }
    qarr[0] = mean;
    /* find equivalent sigma */
    sigma = (qarr[3] - qarr[1])/(2.*.675);
    /* mprintf(" mean, eq. sigma=%5.3f %5.3f", mean, sigma); */
    return sigma;
}
    
/************************** MAKEDER() **************************************/

/*
 * makes derivative of gaussian array, disp sigma (cells) cut off at 3 sigma
 */
mirella void 
makeder(sigma,derarr,nder)
double sigma;   /* dispersion in cells */
float *derarr;  /* array for storing derivative */
int nder;       /* dimension of derarr; must be at least 3.*sigma */
{
    register int i;
    float x;
    
    for(i=0;i<nder;i++){
        x = (i)/sigma;  /* values at 1., 2., 3., ....*/
        derarr[i] = (x < 3. ? x*exp(-0.5*x*x)*(1.-.1111*x*x) : 0.) ;
    }
}

/************************** FOLDDER() **************************************/

/*
 * convolves the histog array harray with the deriv array deriv, offset j0
 */
mirella double 
foldder(j0,histarr,ncell,derarr,nder)
int j0;             /* offset in cells */
int *histarr;       /* histogram array */
int nder;           /* size of deriv array */
float *derarr;      /* derivative array */
int ncell;          /* size of histogram */
{
    int iupper = (j0 + nder >= ncell ? ncell-1 : j0 + ncell);
    int ilower = (j0 -nder < 0 ? 0 : j0-nder);
    float ret = 0.;
    int i;
    
    for(i=j0;i<iupper;i++){
        ret += derarr[i-j0] * (float)histarr[i];
    }
    for( i=ilower; i<j0; i++){
        ret -= derarr[j0 - i] * (float)histarr[i];
    }
    return ret;
}

/***********************  FINDMODE() *****************************************/

/*
 * finds the mode of the histogram array harray by convolution. Mode must
 * be farther than nder cells of the end, or inaccurate results
 * will occur, though we do the best we can
 */
mirella double 
findmode(histarr,ncell,derarr,nder)  
int *histarr;       /* histogram array */
int ncell;          /* size of hisogram array */
float *derarr;      /* derivative array */
int nder;           /* size of derivative array */
{    
    int i,j;
    float res, resold;
    int direc;
    float mode;
    int guess, sg, ig;
    int j0;
    int nsm = (nder<6 ? 3 : nder/2) ;  /* smoothing length for first cut;
                                        about 1.5 sigma */
    int ng = ncell/nsm;
    
    /* find maximum of smoothed histogram for first cut */
    guess = 0;
    for( i=0;i<ng;i++){
        sg = 0;
        for(j=0;j<nsm;j++) sg += histarr[i*nsm + j];
        if(sg>guess){
            guess = sg;
            ig = i*nsm + nsm/2;
        }
    }     /* ig is guessed location of mode */
            
    resold = foldder(ig,histarr,ncell,derarr,nder);
    j0 = ng;
    direc = (resold > 0. ? 1 : -1 );  
                /* if conv is positive, mode is likewise */
    do{
        j0 += direc;
        res = foldder(j0,histarr,ncell,derarr,nder);
        if(j0 >= ncell-1  || j0 < 1){
            scrprintf("\n No solution: j0 = %d",j0);
            erret((char *)NULL);
        }
        if( res*resold < 0.) break;  /* changed sign; gotit ! */
    }while(1);
    mode =  ( j0 - ((float)direc)*(res/(res-resold)))  ;
    return mode ;
}

/************************** STATHIST() ************************************/
/* performs a full analysis on the histogram hist; evaluates mean, sigma,
mode, and quartiles. returns result in structure hist_stat */

struct hstat_t{
    float h_mean;
    float h_sigma;
    float h_mode;
    float h_qt[3];
};

mirella struct hstat_t hist_stat;

mirella void
stathist(hist,ncell)
int *hist;
int ncell;
{
    float trysig;
    float *derptr;
    float mean=0.;
    float sigma=0.;
    int nder;
    register int i;
    float fscr;
    int scr;
    
    trysig = anhist(hist,ncell,hist_stat.h_qt);
    nder = 3.*trysig + 2;
    if((derptr = (float *)malloc(nder*sizeof(float))) == NULL) {
        erret("\nSTATHIST:Cannot allocate memory for deriv. array");
    }
    makeder(trysig,derptr,nder);       
    hist_stat.h_mode = findmode(hist,ncell,derptr,nder);
    scr = 0;
    for(i=0;i<ncell;i++) scr += i*hist[i];
    mean = (float)scr/(float)ncell;
    for(i=0;i<ncell;i++){
        fscr = (float)i-mean;
        sigma += fscr*fscr*hist[i];
    }
    sigma = sqrt(sigma*sigma/(float)ncell);
    hist_stat.h_mean = mean;
    hist_stat.h_sigma = sigma;
}

    
/************************** REGRESS() **************************************/
/* performs the regresssion of the array y on the array x; returns the
filled-out struct regress_t regstat */

struct regress_t{
    float regr_m;       /* slope */
    float regr_b;       /* intercept */
    float regr_corr;    /* correlation coefficient */
    float regr_sig;     /* sigma of linear fit*/
    int   regr_n;       /* number of points in array */ 
};

mirella struct regress_t regstat;

/* this is the general routine. If mask is the null pointer, no mask is
 * used; we should generalize this, maybe.
 */

mirella void
gregress(x,y,mask,n,pstat)
float *x;       /* abscissa array */
float *y;       /* ordinate array */
char *mask;     /* acceptance mask */
int n;          /* # elements */
struct regress_t *pstat;    /* return structure */
{
    register int i;
    double xbar=0.,ybar=0.,x2bar=0.,y2bar=0.,xybar=0.;
    int npass=0;
    double scrx,scry;
    double m,b,corr;
    double var=0.;
    double dev;
    int nomask = (mask == (char *)0);
    
    if(nomask) mask = (char *)x;  /* this SHOULD not be necessary */
    
    /* find means */
    for(i=0;i<n;i++){
        if(nomask || mask[i] != 0){
            xbar += x[i];
            ybar += y[i];
            npass++;
        }
    }
    if(npass < 2) erret("\nGREGRESS:No or 1 points accepted");
    xbar /= (float)npass;
    ybar /= (float)npass;
    for(i=0;i<n;i++){
        if(nomask || mask[i] != 0){
            scrx = (x[i]-xbar);
            scry = (y[i]-ybar);
            x2bar += scrx*scrx;
            y2bar += scry*scry;
            xybar += scrx*scry;
        }
    }
    m = xybar/x2bar;
    b = ybar - xbar*m;
    corr = xybar/sqrt(x2bar*y2bar);
    pstat->regr_m = m;
    pstat->regr_b = b;
    pstat->regr_corr = corr;
    pstat->regr_n = n;
    for(i=0;i<n;i++){
        if(nomask || mask[i] != 0){
            dev = y[i] - m*x[i] - b;
            var += dev*dev;
        }
    }
    var /= npass - 1;
    pstat->regr_sig = sqrt(var);
}

/* this routine returns the result in the static struct regstat */
        
mirella void 
regress(x,y,mask,n)
float *x;
float *y;
char *mask;
int n;
{
    gregress(x,y,mask,n,&regstat);
}

/* this one does not use a mask */
mirella void
sregress(x, y, n)
float *x;
float *y;
int n;
{
    gregress(x,y,(char *)0,n,&regstat);
}

/******************** SIGMA-REJECTION ROUTINES ****************************/

/********************* MFCLIP *********************************************/

/** Code added jeg0106 from ccdtest stuff/phist ***/

/* This is a general-purpose statistical routine which produces a
 * mean and standard deviation for a float array with deviations
 * bigger than sclip*sigma rejected. It makes up to three passes.
 * This is an end run around the routines in this module, but we need it.
 */

/*#define CLIPDEBUG*/

mirella void 
mfclip(buf,npt,sclip,pmean,psig,png,badarr)
float *buf;
int npt;
double sclip;
float *pmean;   /* ptr to mean */
float *psig;    /* ptr to sd */
int *png;       /* ptr to # of good points */
short int *badarr; /* ptr to array of rej points; if 0, no storage; your
                        responsibility to make array big enough */
{
    
    float sum=0;
    double mean;
    float fmean;
    float sumsq = 0.;
    int s;
    register float *ptr;
    register int n;
    register float dum;
    int ngood=npt;
    int nbad = 0;
    double thrm,thrp;
    float fsig;
    int ngoodold;

    /* first pass; makes initial guess at mean and sigma */
    sum = sumsq = 0.;
    n = npt;
    ptr = buf;
    while(n--) sum += *ptr++;
    mean = sum/(float)npt;
    fmean = mean;
    n = npt;
    ptr = buf;
    while(n--){
        dum = *ptr++ - fmean;
        sumsq += dum*dum;
    }
    fsig = sqrt(sumsq/(npt-1));

#ifdef CLIPDEBUG
    fmean = sum/(double)npt;
    mprintf("\nbuf,mean,sig,npt=%d %f %f %d",buf,fmean,fsig,npt);
#endif

    for(s=0;s<2;s++){
        thrp = sclip*fsig;
        thrm = -thrp;
        n = npt;
        ptr = buf;
        sum = 0.;
        sumsq = 0.;
        ngoodold = ngood;
        ngood = 0;
        nbad = 0;
        while(n--){
            dum = *ptr++ - fmean;
            if(dum<thrp && dum>thrm){ 
                ngood++;
                sum += dum;
                sumsq += dum*dum;
            }else{
                if(badarr) badarr[nbad++] = ptr - buf - 1 ;
            }
        }
        sum /= (float)ngood;
        mean = sum + mean;   /* new mean */
        fsig = sqrt(sumsq/ngood - sum*sum);
        fmean = mean;
#ifdef CLIPDEBUG
        mprintf("\nbuf,mean,sig,ngood=%d %f %f %d",buf,fmean,fsig,ngood);
#endif
            /* if no bad pts or
             * # same as in last pass,
             * quit
             */
        if(ngood == npt || ngoodold == ngood) break;  
    }                
    if(pmean) *pmean = fmean;
    if(psig ) *psig = fsig;
    if(png)   *png = ngood;
}

/*************************** MFCLIP2() **************************************/

/** Code added jeg0208, mod from ccdtest stuff/phist ***/

/* This is a general-purpose statistical routine which produces a
 * mean and standard deviation for a float array with deviations
 * bigger than sclip*sigma rejected. It makes up to three passes.
 * The only difference between this routine and mfclip() is that it
 * uses a standard mask array instead of outputting an array of bad points
 */

/*#define CLIPDEBUG*/

mirella void 
mfclip2(buf,mask,npt,sclip,pmean,psig,png)
float *buf;     /* ptr to data */
char * mask;    /* ptr to standard mask array */
int npt;        /* how many points ? */
double sclip;   /* how many sigma to clip beyond */
float *pmean;   /* ptr to mean */
float *psig;    /* ptr to sd */
int *png;       /* ptr to # of good points */
{
    
    float sum=0;
    double mean;
    float fmean;
    float sumsq = 0.;
    int s;
    register float *ptr;
    register char *mptr;
    register int n;
    register float dum;
    int ngood=npt;
    double thrm,thrp;
    float fsig;
    int ngoodold;

    /* first pass; makes initial guess at mean and sigma */
    sum = sumsq = 0.;
    n = npt;
    ptr = buf;
    while(n--) sum += *ptr++;
    mean = sum/(float)npt;
    fmean = mean;
    n = npt;
    ptr = buf;
    while(n--){
        dum = *ptr++ - fmean;
        sumsq += dum*dum;
    }
    fsig = sqrt(sumsq/(npt-1));

#ifdef CLIPDEBUG
    fmean = sum/(double)npt;
    mprintf("\nbuf,mean,sig,npt=%d %f %f %d",buf,fmean,fsig,npt);
#endif

    for(s=0;s<2;s++){
        thrp = sclip*fsig;
        thrm = -thrp;
        n = npt;
        ptr = buf;
        mptr = mask;
        sum = 0.;
        sumsq = 0.;
        ngoodold = ngood;
        ngood = 0;
        while(n--){
            dum = *ptr++ - fmean;
            if(dum<thrp && dum>thrm){ 
                ngood++;
                sum += dum;
                sumsq += dum*dum;
                *mptr++ = 1;
            }else{
                *mptr++ = 0;
            }
        }
        sum /= (float)ngood;
        mean = sum + mean;   /* new mean */
        fsig = sqrt(sumsq/ngood - sum*sum);
        fmean = mean;
#ifdef CLIPDEBUG
        mprintf("\nbuf,mean,sig,ngood=%d %f %f %d",buf,fmean,fsig,ngood);
#endif
            /* if no bad pts or
             * # same as in last pass,
             * quit
             */
        if(ngood == npt || ngoodold == ngood) break;  
    }                
    if(pmean) *pmean = fmean;
    if(psig ) *psig = fsig;
    if(png)   *png = ngood;
}

/******************** MMFCLIP2() ******************************************/
/* Mirella procedure */

mirella void
mmfclip2()
{
    int npt = pop;
    char *mask = (char *)pop;
    float *buf = (float *)pop;
    double sclip = fpop;
    float mean;
    float sig;
    int ngood;
    
    mfclip2(buf, mask, npt, sclip, &mean, &sig, &ngood);
    
    push(ngood);
    fpush(sig);
    fpush(mean);
}

/*************************** MFCLIP3() **************************************/

/** Code added jeg0208, mod from ccdtest stuff/phist ***/

/* This is a general-purpose statistical routine for data which is
 * assumed to have the same mean but different population variances;
 * the inverse population variance for each variable is input in the
 * array wt. We do not assume that these values are well known, and
 * in particular the scale may be wrong. So rejection is done by
 * scaling the population variance to agree with the sample variance 
 * and then rejecting points which lie more than sclip sd away from
 * the sample mean. Two iterations are made.
 * This sometimes does not behave. we need to institute a weight ceiling
 * (actually a variance floor) if there is a large range in variance
 * values, and raise it if the rejection rate is too high. for the nonce,
 * we just fall back on no rejection if the cut rate is larger than 30 percent.
 * (dont ask me)
 */

/*#define CLIPDEBUG*/

mirella void 
mfclip3(buf,wt,mask,npt,sclip,pmean,pchi,psdm,png)
float *buf;     /* ptr to data */
float *wt;      /* inverse variance array */
char *mask;     /* ptr to standard mask array */
int npt;        /* how many points ? */
double sclip;   /* how many sigma to clip beyond */
float *pmean;   /* ptr to mean */
float *pchi;    /* ptr to chisq */
float *psdm;    /* sigma of mean */
int *png;       /* ptr to # of good points */
{
    
    float sum;
    float mean, mean1;
    float sdm, sdm1;
    float chisq, chisq1;

    float sumsq;
    float sumwt;

    float w;
    float x;
    float dum;    
    register int i;
    float sc2 = sclip*sclip;

    int s;
    
    int ngood=npt;
    /* double thrm,thrp; */
    int ngoodold;


    /* first pass; makes initial guess at mean and sigma */
    sum = sumsq = sumwt = 0.;
    
    for(i=0;i<npt;i++){
        w = wt[i];
        sum += w*buf[i];
        sumwt += w;
    }
    mean = sum/sumwt;

    sum = sumsq = sumwt = 0.;
    for(i=0;i<npt;i++){
        w = wt[i];
        sum += (x = w*(buf[i] - mean));
        sumsq += x*x ;  /* SUM v^2/sig^4 ~ SUM 1/sig^2 = SUM wt */
        sumwt += w;
    }
    chisq = (sumsq / sumwt);   /* this should be unity if the 
                                * sigmas are good 
                                */
    sdm = sqrt(chisq/sumwt);

    mean1 = mean;
    chisq1 = chisq;
    sdm1 = sdm;

    if (npt < 5){    /* don't iterate on very small samples */
        ngood = npt;
        goto out;
    }


#ifdef CLIPDEBUG
    mprintf("\nbuf,wt,mask,npt,mean,chisq=%d %d %d %d %f %f",
        buf,wt,mask,npt,mean,chisq);
#endif

    ngood = npt;
    for(s=0;s<2;s++){
        sum = sumsq = sumwt = 0.;
        ngoodold = ngood;
        ngood = 0;
        for(i=0;i<npt;i++){
            w = wt[i];
            x = (buf[i] - mean);
            dum = x*x*w;  
            x *= w;
            if(dum<sc2){ 
                ngood++;
                sum += x;
                sumsq += x*x;
                sumwt += w;
                mask[i] = 1;
            }else{
                mask[i] = 0;
            }
        }
        sum /= sumwt;
        chisq = (sumsq/sumwt);
        mean = sum + mean;   /* new mean */
        sdm = sqrt(chisq/sumwt);

#ifdef CLIPDEBUG
        mprintf("\nbuf,wt,mask,npt,mean,chisq,sdm=%d %d %d %d %f %f %f",
            buf,wt,mask,npt,mean,chisq,sdm);
#endif
            /* if no bad pts or
             * # same as in last pass, or sample has gotten very small
             * quit
             */
        if(ngood < 7*npt/10){ /* something is broken */
            mprintf("\nWARNING: npt,ngood= %d %d; falling back",npt,ngood);
            mean = mean1;
            chisq = chisq1;
            sdm = sdm1;
            ngood = npt;
            break;
        }
        if(ngood == npt || ngoodold == ngood || ngood < 5 ) break;  
    }                
out:    
    if(pmean) *pmean = mean;
    if(pchi ) *pchi = chisq;
    if(psdm ) *psdm = sdm;
    if(png)   *png = ngood;
}

/******************** MMFCLIP3() ******************************************/
/* Mirella procedure */

mirella void
mmfclip3()
{
    double sclip = fpop;
    int npt = pop;
    char *mask = (char *)pop;
    float *wt  = (float *)pop;
    float *buf = (float *)pop;
    float mean;
    float chisq;
    float sdm;
    int ngood;
    
    mfclip3(buf, wt, mask, npt, sclip, &mean, &chisq, &sdm, &ngood);
    
    push(ngood);
    fpush(sdm);
    fpush(chisq);
    fpush(mean);
}


/************************** END, MIRHIST.C **********************************/




