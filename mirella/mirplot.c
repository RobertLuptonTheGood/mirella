
/************************ MIRPLOT.C ***************************************/
/*
 * this module supports the in-memory bit-mapped/vector graphics code
 * in mirgraph.c to produce line graphs, make boxes, etc.
 */
#ifdef VMS
#include mirella
#include graphs
#else
#include "mirella.h"
#include "graphs.h"
#endif

/* exported functions ********************************************************

gbox(x1,y1,x2,y2)           draws box, corners x1,y1,x2,y2 (raw coordinates)

plotinall()                 sets plotting limits to whole page
plotinframe()               resets plotting area to frame boundaries

void frame(x1,x2,nx,y1,y2,ny)       draws frame on screen ,tick marks, numbers 
                            axes, and sets origin and scale for floating x and
                            y. x1,x2,y1,y2 are lower and upper lims on x,y 
                            respectively, floats, and ny, ny are number of 
                            divisions. If nx or ny is zero, routine uses
                            nicelims() to pick scales and divs; if negative,
                            flag for logs; in that case the limits MUST BE
                            INTEGERS FOR NOW.
                            
void sdraw(sx,sy,pen)       like ldraw() but takes floating args; routine 
                            scales by the parameters from frame and draw()s;
                            obeys ltype and lweight
int xtod(xs)                float xs to int pixel coord.
int ytod(ys)                ditto for y.
void graph(xa,ya,n)         draws a graph of the FLOAT array ya vs the FLOAT
                            array ya, n pts.
spoint(sx,sy,size,symb)     places a point in the graph at float coords xs,ys,
                            float size in % of ysize; see desc of symb variable
                            in the Mirella writeup below.
                            
and a number of alpha functions:

ylabel(sly),
xlabel(slx)                 labels the axes with the strings slx, sly.
htick(y,logflg),vtick(x,logflg)  ticks the axes, pixel coordinates.

Mirella functions, procedures, and variables:
*********************************
fxl fxh nxdiv xscale        sets scales to span first two values and
fyl fyn nydiv yscale        sets number of divs. ndiv=0 invokes a nicelimits
                            routine, ndiv<0 sets logarithmic axes flag
gframe                      Draws a frame and numbers axes. Must set
                            xscale and yscale first
plotinall                   sets plotting area to whole page.
plotinframe                 sets plotting area to frame only.
fxs fys pen sdraw           Draws a line to scaled fxs,fys from prev point if
                            pen=1; moves to point if pen=0.
xar yar n ffgraph           Plots n-pt arrays yar vs xar, of the 
          ssgraph           indicated types (f=float,s=short,n=integer)
          nsgraph  
yar n ngraph                Plots n-pt yar versus index, various types
      sgraph
      fgraph          
str xlabel
    ylabel                  Labels the axes
fxs fys str label           Places label str at fxs, fys (scaled coordinates)
fx fy str flabel            Places label str at fx, fy (fractions of plot box
                            size)
fxs fys size symb spoint    Places point  at scaled fxs,fys. Size is in 
                            % of ysize, integer. If size=0 places extended 
                            (Mongo) ascii char symb at sx,sy. If size != 0
                            draws star/polygon at fxs,fys. number of vertices 
                            is symb&15. if symb&16, rotates half the symmetry 
                            angle: the vertex is normally up; if symb&32, 
                            star, else ext polygon. If symb &64, filled. 
xf xtod -> xi               pushes pixel x,y value corresponding to scaled
yf ytod -> yi               xf, yf
lineconnect                 sets line mode
histogram                   sets histogram mode
g_angle                     Float Mirella variable for position angle of strings
g_magn                       "     "       " for magnification. 1.0 is "normal"
g_stdflg                    Int Mirella flag for 'normal' plots, with 'nice'
                            boundaries; if want something else, set to zero 
                            and set the following:
    g_xls                   Lower and upper boundaries in x and y of plotting
    g_xhs                   area, expressed as fractions of the x and y screen
    g_yls                   size; all floating variables between 0. and 1.
    g_yhs
    g_numflg                Int flag: do you want numerical labels ? If so,
    g_syxnum                At what value of y (fraction of scr size) do you
                            want the x labels ?  (font size g_magn).
    g_sxynum                And the x value (frac. of scr size) for the y labels
    g_yvflg                 Int flag: Do you want the y-numbers written 
                            vertically? (1) or horizontally (0) ?

g_xdec                      Number of decimal digits after dp for axis labels;
g_ydec                      overrides algorithm in numlabel() if nonnegative
                            
************************************************************************/
/* exported variables */
float g_magn = 1.;     /* Mongo-type magn; applies to text strings only */
float g_angle = 0.;    /* ditto angle */
int g_symb = 0;        /* symbol (see spoint()); used in pointdraw() */
int g_ssize = 0;       /* symbol size, in centiysize */
int g_stdflg = 1;      /* flag for 'standard' graphs; if turn off, the
                        * following variables are operative: */
int g_tickleni = 20;   /* inverse length of long ticks; short ones are half
                        * as long: length = ypts/g_tickleni (default 20) */

float g_xls = 0.;
float g_xhs = 1.;
float g_yls = 0.;
float g_yhs = 1.;      /* scaled limits in units of g_xsize,g_ysize for the
                      plotting box; normally .26,.96,.15,.95 for 4:3 screens */
                      
float g_fxorg;         /* float x val at left edge of plot box */
float g_fyorg;         /* float y val at lower edge of plot box */
float g_fxhi;          /* float x value along right edge of plot box */
float g_fyhi;          /* float y value along top edge of plot box */
float g_fxscl;         /* float scale value along x; ie i = (x - xorg)*xscl */
float g_fyscl;         /* float scale value along y; ie j = (y - yorg)*yscl */
                       /* NB!!! scales are cells per delta value, not inv */

int g_numflg = 1;      /* numerically label axes ? */
float g_syxnum = 0.05; /* y level to write x numbers */
float g_sxynum = 0.15; /* x frac to begin y numbers */
int g_yvflg = 0;       /* flg to write y labels vertically */

int g_xdec = -1;
int g_ydec = -1;       /* integer vars to override default precision algorithm
                            in numlabel(); if set, specifies # of decimals
                            in x and y labels   */

void vtick(),htick(),g_dox(),g_doy(),vgrid(),hgrid();
static void shtick(),svtick();

static float xls,xus,yls,yus;   /* scaling limits for plotting area */
static normal nxdiv,nydiv;      /* number of divisions alog x, y axes */
static normal xpts,ypts;        /* number of pixels in x and y of plot area */
static normal xmpts,ympts;      /* number of pixels beween end ticks */
static normal lenynum;          /* max len of y axis numbers in pixels */
static int xflg=1;              /* flag for manipulating x-axis quantities */
static int yflg=0;              /* "                     y-       "        */
                                /* set by g_dox() and g_doy(), respectively */
                                
#define MAXABS(x,y) ( fabs(x) > fabs(y) ? fabs(x) : fabs(y) )
/* note float args only */

/********************** G_DOX(), G_DOY() *********************************/
void g_dox()
{ xflg = 1; yflg = 0; }

void g_doy()
{ yflg = 1; xflg = 0; }

/********************** NICELIMS() ***************************************/
/* takes lower and upper limits (pointers) and modifies them for "nice"
plots; returns the number of tick divisions */
/* clean this up -- I think it is basically OK, but fix the if chain to
 * make more sense, and make the intervals 1, .5, or .2
 */

/*#define NDEBUG*/

int nicelims(pllim,pulim)  /* returns ncell; modifies upper and lower lims */
double *pllim,*pulim;
{
    double atof();
    float llim = *pllim;
    float ulim = *pulim;
    int ncell;
    float ancell;
    float exrange;
    float interval;
    float llog;
    float range = ulim - llim;
    float srange = (range < 0. ? -1. : 1.); 
   
    if(range == 0.){
        erret("\nzero range");
    }
    llog = log10(fabs(range));
    ancell = pow(10.,llog - floor(llog));  /* mantissa */
    exrange = pow(10.,floor(llog));        /* pwr of 10 */
#ifdef NDEBUG
    scrprintf("\nllog,ancell,exrange= %f %f %f",llog,ancell,exrange);
#endif
    interval = exrange;
    if(ancell > 6.){
        ncell = ceil(ancell/2.);
        interval *= 2.;
    }else if(ancell < 1.8){
        ncell = ceil(ancell/0.3);
        interval = 0.3*exrange;
    }else if(ancell < 2.4){
        ncell = ceil(ancell/0.4);
        interval = 0.4*exrange;
    }else if(ancell < 3.0){
        ncell = ceil(ancell/0.5);
        interval = 0.5*exrange;
    }
    interval *= srange;
    llim = interval * floor(llim/interval);
    ncell = ceil((ulim-llim)/interval);
    ulim = llim + ncell*interval;
    *pllim = llim;
    *pulim = ulim;
#ifdef NDEBUG
    scrprintf("\nllim,ulim,ncell = %f %f %d",*pllim,*pulim,ncell);
#endif
    return ncell;
}     

/* #define FLDEBUG */

/****************************** FLIMS() **********************************
 * this routine takes an upper (x2) and lower (x1) float limit and two
 * pointers to float for an offset (xtf) to the first main tick of the
 * graph, and (sxd) the interval between ticks. It returns the number
 * of divisions ndiv. Note than ndiv*sxd <= x2-x1. Ndiv is always 10
 * or less. This routine is much like nicelims(), but always uses exactly
 * the proferred lower and upper limits and in general has fractional
 * divisions at the ends. There is some problem with roundoff; ceil and
 * floor do not always give integers properly for non-binary-representable
 * decimal divisors. Fudged here, but be careful. Could possibly fix
 * this by scaling up by 10 and working in ancell space.
 */
 
int flims(x1,x2,pxtf,psxd)
double x1,x2;
float *pxtf, *psxd;
{
    float span = x2-x1; /* total range */
    float aspan = fabs(span);
    float lspan;
    float ancell;
    float exrange;
    float sxd;
    float xtf;
    float x1t;
    int ndiv;
    float sgn=1.0;
    
    if(span == 0.) erret("\nzero range");
    lspan = log10(aspan);
    if (span < 0.) sgn = -1.0;
    ancell = exp10(lspan - floor(lspan));  /* mantissa; <= 10 */
    exrange = exp10(floor(lspan)) * sgn;   /* signed power of 10 */
    if(lspan == floor(lspan)){
        ancell = 10.;
        exrange /= 10.;
    }
    /* note than ancell * exrange is aspan */
    if(ancell > 5.){ 
        sxd = exrange;
    }else if (ancell <= 5. && ancell >2.){
        sxd = 0.5*exrange;
    }else if (ancell <= 2. ){
        sxd = 0.2*exrange;
    }
    x1t = ceil(x1/sxd - .0001)*sxd;  /* ROUNDOFF !!! */
#ifdef FLDEBUG
    printf("  ancell=%f x1t=%f\n",ancell,x1t); /* debug*/    
#endif
    xtf = x1t - x1;
    ndiv = floor((x2 - x1t)/sxd + .0001) ; /* ROUNDOFF !!! */
    *pxtf = xtf;
    *psxd = sxd;
    return ndiv;
}
    
/****************************** GBOX() ***********************************/

void gbox(x1,y1,lx,ly)   /* guess!; uses g_angle */
int x1,y1,lx,ly;
{
    float sa;
    float ca;
    int dxx, dxy, dyx, dyy;

    sa = sin(g_angle);
    ca = cos(g_angle);
    dxx = lx*ca; dyx = lx*sa;
    dyy = ly*ca; dxy = -ly*sa;

    ldraw(x1,y1,0) ;  /* draw frame  */ 
    ldraw(x1 + dxx , y1 + dyx,1);
    ldraw(x1 + dxy + dxx, y1 + dyy + dyx,1);
    ldraw(x1 + dxy, y1 + dyy,1);
    ldraw(x1,y1,1) ;
}

/**************************** FRAME() ************************************/
/* This routine draws the graphics frame and writes scales on axes. The
 * arguments are the lower and upper bounds and number of divisions on
 * the x and then y axes. If the number of divisions is 0, the routine
 * invokes nicelims(); if -1, does logarithmic subdivisions, and
 * labels with the actual values, but the sx and sy limits are LOGS,
 * and of course you have to plot the logs when you make the graph.
 * if -2, invokes flims, which uses the actual values of sx? and sy?
 * as the graph limits, but grids sensibly and leaves partial grids
 * at the ends.
 * _frame is either frame or gridframe depending on value of gridflg. 
 */
 
static void
_frame(sx1,sx2,nx,sy1,sy2,ny,gridflg) 
double sx1, sx2;
int nx;
double sy1, sy2 ;
int ny ;
int gridflg; /* if 0, ticks axes; if nonzero, makes grid on plotting area */
{
    int i, ix, iy ;
    float lx, ly ;
    char nmark[40];
    int len, em ;
    int xoffs, yoffs;
    float maxx, maxy;
    float sxspan, syspan; /* total plotting span */
    float sxd, syd;       /* span of one division--NB!! nx*xdspan != xspan */
    int xl,yl,xh,yh;      /* pixel plotting limits, exported as g_xlp, etc*/
    float p_magn = 0.9 ;  /* string magnification */
    int numflg = 1;       /* flag for numbering axes */
    int p_yxnum;
    int p_xynum;
    float yangle = 0.;     /* angle for y-numbers */
    float xtf=0., ytf=0.;  /* float vbl offset to first tick */   
    int logxflg = 0;       /* flags for log ticking of axes and numbering
                            * appropriately */
    int logyflg = 0;
#if 0
    int nicexflg = 0;      /* flags for invoking nicelims */
    int niceyflg = 0;
#endif    
    int fxflg = 0;         /* flags for invoking flims */
    int fyflg = 0;
    
    g_angle = 0.;
    plotinall();
    /* set divisions */
    if(nx > 10) nx = 10 ;
    if(ny > 10) ny = 10 ;
    /*check logs and nicety*/
    if(nx == -1)logxflg = 1;
    if(ny == -1)logyflg = 1;
    /* 
     * we have to do this early because nicelims affects the limits, which
     * are used almost everywhere. We need to figure out how to decide
     * whether to use nicelims or flims, if flims in fact works.
     */
    if(nx == 0){
        nx = nicelims(&sx1,&sx2);
        /* nicexflg = 1; */
    }
    if(ny == 0){
        ny = nicelims(&sy1,&sy2);
        /* niceyflg = 1; */
    }
    if(nx == -2) fxflg = 1;
    if(ny == -2) fyflg = 1;

    /* at this point we know the plotting limits and the architecture
     * of the screen. We do NOT know yet what the real number of divisions
     * is (we do for the `normal' usage, either through specifying nx and
     * ny or by nicelims(), but do not for logs or if we are to 
     * invoke flims.
     */        
   
    maxx = MAXABS(sx1,sx2);
    maxy = MAXABS(sy1,sy2);  /* maximum label and coordinate values */
    /* set box size */
    yoffs = g_ysize/20;
    xoffs = g_ysize/10;
    /* calculate plotting box limits in pixels */
    yl = g_ysize/5 - yoffs ; /* .15 g_ysize; letters are .032 g_ysize tall */
    xl = (2*g_ysize/5) - xoffs ;  /* .35 g_ysize, or .26 g_xsize for 4:3 scr*/
                                  /* the x offsets are in units of the y size
                                     because the fonts are determined by the
                                     heights and hence ysize */
    xh = g_xsize - xoffs ;    /* .90 g_xsize */
    yh = g_ysize - yoffs ;    /* .95 g_ysize */
    p_yxnum = g_ysize/10;     /* y coord of x numbers */
    if(!g_stdflg){         /* nonstandard graph */
        xl = g_xls*g_xsize;
        xh = g_xhs*g_xsize;
        yl = g_yls*g_ysize;
        yh = g_yhs*g_ysize;
        p_magn = g_magn;
        numflg = g_numflg;
        p_yxnum = g_syxnum*g_ysize;
        p_xynum = g_sxynum*g_xsize;
        if(g_yvflg) yangle = 90.;
    }
    xpts = xh - xl ;
    ypts = yh - yl ;
    sxspan = sx2 - sx1;
    syspan = sy2 - sy1;

    sxd = sxspan/(float)nx ;
    xtf = 0.;
    syd = syspan/(float)ny ;    
    ytf = 0.;
    /* these are wrong for nonaligned grids and are
     * recalculated below (only for logs for now)
     */
     
    /* calculate tick origin offset & set interval if nonaligned logs */   
    if(logxflg == 1){
        xtf = ceil(sx1) - sx1 ;  /* offset to first tick in vbl space */
        nx = floor(sx2) - ceil(sx1);
        sxd = 1.;
        /* this needs to be modified for general non-aligned grids,
         * where sxd is not unity 
         *
         * in general, the pix coord of a main tick is 
         * xl + round( (i*sxd + xtf)*xpts/(sx2-sx1) );
         * and this is general (I think), but need to scale sx1,2 for general
         * non-log case to calculate nx, and calculate sxd properly.
         * The axis *value* at the tick is
         * sx1 + xtf + i*sxd
         */
    }
    if(logyflg == 1){
        ytf = ceil(sy1) - sy1;
        ny = floor(sy2) - ceil(sy1);
        syd = 1.;
    }

    /* calculate tick origin offsets and interval if nonaligned grid */
    if(fxflg == 1){
        nx = flims(sx1, sx2, &xtf, &sxd);
    }
    if(fyflg == 1){
        ny = flims(sy1, sy2, &ytf, &syd);
    }
       
    /* set external & static plotting limits */
    nxdiv = nx;
    nydiv = ny;
    g_xlp = xl; g_ylp = yl; g_xhp = xh; g_yhp = yh;
    xmpts = round(xpts*(float)nx*sxd/sxspan);
    ympts = round(ypts*(float)ny*syd/syspan);  /*# of pixels between end ticks*/
    
    /* draw the box and calculate plotting scales */
    gbox(xl,yl,xpts,ypts);

    g_fxorg = sx1 ;
    g_fyorg = sy1 ;
    g_fxhi = sx2 ;
    g_fyhi = sy2 ;
    g_fxscl = (float)xpts / (sx2-sx1) ;
    g_fyscl = (float)ypts / (sy2-sy1) ;
    /* NB!!! scales are cells/value, not inverse. Do NOT forget to ROUND when
     * calculating cell values
     */

    /* now label the axes */
    em = gstrlen("0",0.,g_magn);
    g_dox();
    /* this should be OK if nx, ny are OK, but may need to go more and
     * just not plot if off the box limits. */
    for( i=0 ; i <= nx ; i++ ){
        /* ix is the location of the label, lx the label value */
        ix = xl + round( (i*sxd + xtf)*(float)xpts/(sx2-sx1) );
        lx = sx1 + xtf + (float)i*sxd;
        len = numlabel(lx,nmark,p_magn,logxflg,maxx);
        ldraw(ix-(len/2)-1,p_yxnum,0);  /*numbers at .1 g_ysize, normally*/
        if(numflg) gstring(nmark,0.,p_magn);
    }
    lenynum = 0.;  /* this is a global, used by ylabel and to set
                      origins of y numbers below */
    g_doy();
    for( i = 0 ; i <= ny ; i++ ) {    /* this loop just gets length */
        iy = yl + round( (i*syd + ytf)*(float)ypts/(sy2-sy1) );
        ly = sy1 + ytf + (float)i*syd;
        len = numlabel(ly,nmark,p_magn,logyflg,maxy);
        if(len > lenynum) lenynum = len;
    }
    for(i=0 ; i <= ny ; i++ ) {
        iy = yl + round( (i*syd + ytf)*(float)ypts/(sy2-sy1) );
        ly = sy1 + ytf + (float)i*syd;
        len = numlabel(ly,nmark,p_magn,logyflg,maxy);
        /* numlabel makes the string. How does it know how long? */
        /* is this the problem? needs to pay attention to lenynum */
        if(g_stdflg) ldraw(xl - lenynum - em,iy,0); /*leave a '0' sized space*/
        else if(yangle == 0.) ldraw(p_xynum,iy,0);
        else ldraw(p_xynum, iy-len/2,0);  /* yangle = 90 */
        if(numflg) gstring(nmark,yangle,p_magn);
    }
    lenynum = lenynum + 2*em;

    plotinframe();  /* set plotting boundaries */
    
    /* now tick the axes or make grid */
    for( i=-1 ; i <= nx ; i++ ){
        ix = xl + round( (i*sxd + xtf)*(float)xpts/(sx2-sx1) );
        if(gridflg == 0 ) {
            vtick(ix,logxflg) ;
        } else {
            vgrid(ix);
        }
    }
    for(i=-1 ; i <= ny ; i++ ) {
        iy = yl + round( (i*syd + ytf)*(float)ypts/(sy2-sy1) );
        if (gridflg == 0){
            htick(iy,logyflg) ;
        } else {
            hgrid(iy);
        }
    }
}


void
frame(sx1,sx2,nx,sy1,sy2,ny)
double sx1, sx2;
int nx;
double sy1, sy2 ;
int ny ;
{
    _frame(sx1,sx2,nx,sy1,sy2,ny,0);
}


/*************************** VTICK(),HTICK ********************************/
/* 
 * draws ticks on the axes at (screen) coordinate x (vtick) or y (htick); 
 * takes a flag for logarithmic ticking, which if true completes the log
 * pattern to higher values (right,up). 2010: new varible list for these
 * functions; need plotting limits to begin and end ticking for non-integral
 * divisions 
 */

static normal logdiv[8] = {308,489,616,716,797,865,925,977}; /*fracs of 1024*/
    
void vtick(x, lxflg)
normal x;
normal lxflg;
{
    int i,xs;
    
    if(x < g_xhp && x >= g_xlp ) svtick(x,ypts/g_tickleni);
    if(!lxflg){
        for(i=1;i<5;i++){
            xs = x + (i*xmpts)/(5*nxdiv);
            if(xs < g_xhp && xs >= g_xlp ) svtick(xs,ypts/(2*g_tickleni));
        }
    }else{
        for(i=0;i<8;i++){
            xs = x + (logdiv[i]*xmpts)/(1024*nxdiv);
            if(xs < g_xhp && xs >= g_xlp ) svtick(xs,ypts/(2*g_tickleni));
        }
    }
}    

void vgrid(x)
normal x;
{
    if(x < g_xhp && x >= g_xlp ) ldraw(x,g_ylp,0);
    if(x < g_xhp && x >= g_xlp ) ldraw(x,g_yhp,1);
}

static void
svtick(x,len)  /* single vtick */
normal x;
normal len;
{
    ldraw(x,g_ylp,0); ldraw(x,g_ylp+len,1);
    ldraw(x,g_yhp,0); ldraw(x,g_yhp-len,1);
}


void htick(y, lyflg)
normal y;
normal lyflg;
{
    int i,yt;
    
    if(y < g_yhp && y >= g_ylp ) shtick(y,ypts/g_tickleni);
    if(!lyflg){
        for(i=1;i<5;i++){
            yt = y + (i*ympts)/(5*nydiv);
            if(yt < g_yhp && yt >= g_ylp) shtick(yt,ypts/(2*g_tickleni));
        }
    }else{
        for(i=0;i<8;i++){
            yt = y + (logdiv[i]*ympts)/(1024*nydiv);
            if(yt < g_yhp && yt >= g_ylp) shtick(yt,ypts/(2*g_tickleni));
        }
    }
}    

void hgrid(y)
normal y;
{
    if(y < g_yhp && y >= g_ylp ) ldraw(g_xlp,y,0);
    if(y < g_yhp && y >= g_ylp ) ldraw(g_xhp,y,1);
}

static void
shtick(y,len)  /* single h tick */
normal y;
normal len;
{
    ldraw(g_xlp,y,0); ldraw(g_xlp+len,y,1);
    ldraw(g_xhp,y,0); ldraw(g_xhp-len,y,1);
}

/***************** NUMLABEL() ******************************************/
   
int numlabel(f,s,mag,logf,maxq)  /*converts the float f into a graph 
                                label string;
                                returns the length in pixels*/
double f;       /* number to convert */
char *s;        /* output string*/
double mag;     /* magnification */
int logf;       /* log axis flag */
double maxq;    /* maximum abs. coord; controls format */
{
  char nstr[40], *cp,*cdp,*ce;
#if 0
  char *sdp ;
#endif
  int i,l2,c;
  int neflg=0;
  int ndec = (xflg ? g_xdec : g_ydec );  /* normally ndec = -1, and this
                                            routine chooses format; setting
                                            g_xdec or g_ydec allows special
                                            formats */
  static char efmt[] = "%10.3e";
  static char ffmt[] = "%6.4f";
    
  if(!logf){   
    cp = nstr;
    if(maxq >= 100000. || maxq < 0.01){
        if(ndec < 0) sprintf(nstr,"%10.3e",f*1.000002);
        else{
            if(ndec > 5) ndec = 5;    /* single precision accuracy limit */    
            efmt[4] = '0' + ndec;
            sprintf(nstr,efmt,f*1.000002);
        }
    }
    else if(ndec >=0){   
        if(ndec > 6) ndec = 6;
        ffmt[3] = '0' + ndec;
        if(ndec > 0) sprintf(nstr,ffmt,f);
        else sprintf(nstr,"%6d",(int)(f*1.000002));        
    }
    else if(maxq >= 1000.)          sprintf(nstr,"%6d"  ,(int)(f*1.000002));
    else if(maxq >= 100.)           sprintf(nstr,"%6.1f",f);
    else if(maxq >=10.)             sprintf(nstr,"%6.2f",f);
    else if(maxq >=0.1)             sprintf(nstr,"%6.3f",f);
    else if(maxq >=0.01)            sprintf(nstr,"%6.4f",f);
    
    while(isspace(*cp++)) continue;    /* skip leading whitespace */
    cp-- ;
    cdp = strchr(cp,'.');
    if((ce = strchr(cp,'e')) == NULL) ce = strchr(cp,'E');
    if(cdp) *cdp = '\0';
    if(ce) *ce = '\0';
    strcpy(s,cp);    /* integer part */
    if(cdp || maxq < 1000.){ /*is the entry small or does it have a decimal?*/
      strcat(s,".");     /* add decimal point */
      if(!cdp) strcat(s,"0");  /* if no dp originally, add 0 after dp */
      else{
        l2 = strlen(cdp+1);  /* how many figures after decimal pt ? */
#if 0        
        sdp = strchr(s,'.');
#endif        
        strcat(s,cdp+1);
#if 0
        for(i=l2;i>1;i--){
            if(*(sdp+i) == '0') *(sdp+i) = '\0';
        }   /* kill trailing zeros after the first */
#endif
        if(l2 == 0) strcat(s,"0"); /* but put a zero if none there in orig */
        /* problem is HERE??? */
      }
      if(ce){   /* there is an exponent */
        l2 = strlen(ce+1);
        for(i=l2;i>0;i--){
            if(isspace(c= *(ce+i)) ) *(ce+i) = '\0';
        }  /* truncate trailing whitespace */
        if(*(++ce) ==  '-'){
            neflg = 1;
            ce++;
        }else if(*ce == '+') ce++;
        ce--;
        while(*++ce == '0') continue;  /* skip leading zeros */
        if(*ce){     /* is anything left ? Use Feynman notn if so */
            strcat(s,"\\\\d");     /* \\d for Mongo subscript */
            if(neflg) strcat(s,"-");
            else strcat(s,"+");
            strcat(s,ce);
        }
      }
    }
    return (gstrlen(s,0.,g_magn));    
  }else{     /* logflag */
    i = f;
    if( f != (float)i ) erret("I cannot yet cope with nonintegral log divs");
    strcpy(s,"10\\\\u");
    sprintf(nstr,"%f",f);
    if((cdp = strchr(nstr,'.')) != NULL) *cdp = '\0';
    strcat(s,nstr);
    return (gstrlen(s,0.,g_magn));
  }
}

/*********************** XLABEL(), YLABEL() *******************************/
void xlabel(slx)
char *slx;
{
    int len;

    plotinall();    
    len = gstrlen(slx,0.,g_magn);
    ldraw(g_xlp + (xpts - len)/2, g_ysize/20, 0);   /* xlabel at .05 g_ysize */
    gstring(slx,0.,g_magn);
    plotinframe();
}

void ylabel(sly)
char *sly;    
{
    int len;
    
    plotinall();
    len = gstrlen(sly,90.,g_magn);
    ldraw(g_xlp - lenynum - ypts/40,g_ylp + (ypts - len)/2,0);
    gstring(sly,90.,g_magn);
    plotinframe();
}


/************************  SPOINT *****************************************/
/*
 * places point  at scaled sx,sy. size is in centiysize, integer. If
 * size is zero, spoint places extended ascii char symb at sx,sy. If not,
 * draws star/polygon at sx,sy. number of vertices is symb&15. if symb&16,
 * rotates half the symmetry angle: the vertex in normally up; 
 * if symb&32, star, else ext polygon. If symb &64, filled.
 *
 * NB!!! on high-res screens, you need to use lweight > 1. This routine
 * also needs, similarly. So use ldraw, but make sure ltype = 0.
 * lytpe needs work; it never worked properly. likewise symbol filling
 * does not work well, but it is a rather complex problem. Maybe
 * change to just do stars, squares, diamonds, and circles ????
 */
void
spoint(sx,sy,size,symb)   
double sx, sy ;
int size ;
int symb ;
{
    register int i;
    int ix,iy, len, nvert, prad, sr ;
    int pstar;
    char chars[2];
    short int vrtx[16], vrty[16];
    float thet0, delthet;
    int oltype = ltype;
    
    ltype = 0;
    
    ix = (sx-g_fxorg)*g_fxscl + g_xlp;
    iy = (sy-g_fyorg)*g_fyscl + g_ylp;
    /* check plot boundaries */
    if(ix < g_xl || ix >= g_xh || iy < g_yl || iy >= g_yh) return ;
    if(symb == 0){
        gpoint(ix,iy);
        return;
    }
    if(size == 0){    /* ascii char */
        chars[0] = symb;
        chars[1] = '\0';
        len = gstrlen(chars,0.,g_magn);
        draw(ix-len/2,iy,0);    
        gstring(chars,0.,g_magn);
    }else{          /* polygon */
        /* calculate vertices */
        nvert = symb&15;
        prad = (size*ypts)/200;  /* radius of symbol */
        delthet = 6.283185/(float)nvert;
        thet0 = ((symb&16) ? 0.5*delthet : 0. );
        for(i=0;i<nvert;i++){
            vrtx[i] = prad*sin(thet0);
            vrty[i] = prad*cos(thet0);
            thet0 += delthet;
        }
        vrtx[nvert] = vrtx[0];
        vrty[nvert] = vrty[0];
        pstar = symb&32;
        for(i=0;i<nvert;i++){
            ldraw(ix+vrtx[i],iy+vrty[i],0);            
            if(pstar) ldraw(ix,iy,1);
            else      ldraw(ix+vrtx[i+1],iy+vrty[i+1],1);
        }
        if((symb&64) && (sr = prad-1) >= 0){    /* fill */
            ldraw(ix + (sr*vrtx[0])/prad, iy + (sr*vrty[0])/prad, 0);
            do{
                for(i=0;i<=nvert;i++){
                    ldraw(ix + (sr*vrtx[i])/prad, iy + (sr*vrty[i])/prad, 1);
                }
            }while(--sr >= 0);
        }
    }
    ltype = oltype;
    
}        


/************************* XTOD(), YTOD() *******************************/
/* converts scaled floating coords  to screen coordinates */

int xtod(x)
double x;
{
    return (int)( (x-g_fxorg)*g_fxscl + g_xlp );
}

int ytod(y)
double y;
{
    return (int)( (y-g_fyorg)*g_fyscl + g_ylp );
}

/******************* LSDRAW(),HSDRAW() ************************************/


void lsdraw(xf,yf,pen) /* draws from prev pos to x,y if pen=1; moves to x,y
    if pen=0;  scaled floating coordinates   */
double xf, yf;
int pen ;
{
    int x = (xf-g_fxorg)*g_fxscl + g_xlp;
    int y = (yf-g_fyorg)*g_fyscl + g_ylp;
    ldraw(x,y,pen);
}

void hsdraw(xf,yf,pen)   /* histogram connect; vertical stroke halfway between
                            old and new x values ; float
                            scaled coordinates */
double xf,yf;
int pen;                            
{
    int x = (xf-g_fxorg)*g_fxscl + g_xlp;
    int y = (yf-g_fyorg)*g_fyscl + g_ylp;
    int xhlf = (g_xold + x)/2;
    ldraw(xhlf,g_yold,pen);
    ldraw(xhlf,y,pen);
    ldraw(x,y,pen);
}

void pointdraw(xf,yf,pen) /* draws a symbol at xf,yf, symbol g_symb, size
                                g_ssize */
double xf, yf;
int pen;                                
{
/*    int x = (xf-g_fxorg)*g_fxscl + g_xlp;
    int y = (yf-g_fyorg)*g_fyscl + g_ylp; */
    spoint(xf,yf,g_ssize,g_symb);
}

/******************** LINECONNECT(), HISTOGRAM() ***************************/

static void (*do_sdraw)() = lsdraw ;

void lineconnect() { do_sdraw = lsdraw; }
/* do_sdraw draws lines */

void histogram() { do_sdraw = hsdraw; }
/* do_sdraw draws histograms */

void points()  { do_sdraw = pointdraw; }
/* do_sdraw draws symbols */

/************************* SDRAW()***********************************/

void sdraw(xf,yf,pen)  /* function definition in case anyone wants it */
double xf,yf;
int pen;
{
   (*do_sdraw)(xf,yf,pen);
}

/************************* GRAPH() ***********************************/

void graph(xa,ya,n)    /* float xa vs float ya */
float xa[], ya[] ;
int n;
{
    register int i ;
    for ( i= 0; i < n ; i++ ){
        (*do_sdraw)(xa[i],ya[i],i);
    }
}

/******************** MIRELLA PROCEDURES ******************************/

void msdraw()   /* sdraw's from old loc to x,y, flag pen */
{
    normal pen = pop;
    double y = fpop;
    double x = fpop;
    (*do_sdraw)(x,y,pen);
}

void ffgraph()   /* Mirella procedure :float xa vs float ya */
{
    normal n = pop;
    float * ya = (float *)pop;
    float * xa = (float *)pop;
    graph(xa,ya,n); 
}

void
ssgraph()    /* Mirella procedure: short xa vs short ya */
{
    normal n = pop;
    short int *ya = (short int *)pop;
    short int *xa = (short int *)pop;
    register int i ;
    
    for ( i= 0; i < n ; i++ ){
        (*do_sdraw)((double)xa[i],(double)ya[i],i);
    }
}

void
lsgraph()  /* Mirella procedure: long xa[i] vs short  ya[i] */
{
    normal n = pop;
    short int *ya = (short int *)pop;
    normal *xa = (normal *)pop;
    register int i ;
    
    for ( i= 0; i < n ; i++ ){
        (*do_sdraw)((double)xa[i],(double)ya[i],i);        
    }
}

void
lgraph()   /* Mirella procedure: long ya[i] vs i */
{
    normal n = pop;
    normal *ya = (normal *)pop;
    register int i ;
    
    for ( i= 0; i < n ; i++ ){
        (*do_sdraw)((double)i,(double)ya[i],i);
    }
}

void
sgraph()    /* short ya[i] vs i */
{
    normal n = pop;
    short int *ya = (short int *)pop;
    register int i ;
    
    for ( i= 0; i < n ; i++ ){
        (*do_sdraw)((double)i,(double)ya[i],i);        
    }
}

void
fgraph()    /* float ya[i] vs i */
{
    normal n = pop;
    float *ya = (float *)pop;
    register int i ;
    
    for ( i= 0; i < n ; i++ ){
        (*do_sdraw)((double)i,(double)ya[i],i);        
    }
}
    
/******************* MIRELLA STUFF ****************************************/

/* NB!!! the values xus, etc are temporary and can be changed by, 
 * e.g. nicelims--
 * use the values g_fxorg, g_fxhi, etc, AFTER frame has been executed.
 */
void xscale()
{
    nxdiv = pop;
    xus = fpop;
    xls = fpop;
}

void yscale()
{
    nydiv = pop;
    yus = fpop;
    yls = fpop;
}

void gframe()
{
    _frame(xls,xus,nxdiv,yls,yus,nydiv,0);
}

void gridframe()
{
    _frame(xls,xus,nxdiv,yls,yus,nydiv,1);
}

void label()   /* writes label at x,y (scaled coordinates) */
{
    char *lab = cspop;
    float y0 = fpop;
    float x0 = fpop;
    normal x,y;
    
    plotinall();
    x = xtod(x0);
    y = ytod(y0);
    draw(x,y,0);
    gstring(lab,g_angle,g_magn);
    plotinframe();
}

void flabel()   /* writes label at xf,yf (fractions of plot box size) */
{
    char *lab = cspop;
    float y0 = fpop;
    float x0 = fpop;
    normal x,y;
    
    plotinall();
    x = g_xlp + (g_xhp - g_xlp)*x0 ;
    y = g_ylp + (g_yhp - g_ylp)*y0 ;
    draw(x,y,0);
    gstring(lab,g_angle,g_magn);
    plotinframe();
}

void
mspoint()
{
    float y = fpop;
    float x = fpop;
    spoint(x,y,g_ssize,g_symb);
}


/********************** end of module ************************************/


