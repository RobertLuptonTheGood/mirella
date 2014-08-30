/*
 * This is a set of C functions to allow Mirella to call SM
 */
#include "mirella.h"
#if defined(HAVE_SM)

void sm_angle P_ARGS(( float ));
int sm_axis P_ARGS(( float, float, float, float, int, int, int, int, int ));
int sm_conn P_ARGS(( float [], float [], int ));
void sm_curs P_ARGS(( float *, float *, int *));
void sm_defvar P_ARGS(( char *, char * ));
int sm_device P_ARGS(( char * ));
void sm_draw P_ARGS(( float, float ));
void sm_draw_point P_ARGS(( float, float, int, int ));
int sm_errorbar P_ARGS(( float [], float [], float [], int, int ));
void sm_expand P_ARGS(( float ));
void sm_gflush P_ARGS (( void ));
int sm_histogram P_ARGS(( float [], float [], int ));
int sm_limits P_ARGS(( float, float, float, float ));
void sm_line P_ARGS(( float, float, float, float ));
void sm_notation P_ARGS(( float, float, float, float ));
void sm_plotsym P_ARGS(( float [], float [], int, int [], int ));
void sm_points P_ARGS(( float [], float [], int ));
void sm_redraw P_ARGS(( void ));
void sm_relocate P_ARGS(( float, float ));
void sm_shade P_ARGS(( int, float *, float *, int ));
void sm_ticksize P_ARGS(( float, float, float, float ));
int shade_hist P_ARGS(( int, float *, float *, int ));

static int
msm_redraw()
{
   sm_redraw();
   return(0);
}

void
mir_device(s)
char *s;
{
   static int first = 1;

   if(first) {
      first = 0;
      sm_defvar("TeX_strings","1");	/* use TeX-style labels */
   }
   
   (void)sm_device(s);
   push_callback(msm_redraw);		/* add to callback list */
   push_callback(sm_gflush);		/* if not already there */
}

/*******************************************************/
/*
 * Put up a cursor.
 * Return the character struck on the int stack and y and x on the
 * floating stack. Y is pushed first, so f. f. will print (x,y)
 */
void
mir_curs()
{
   int k;
   float x,y;

   sm_curs(&x,&y,&k);
   if(k == 'e') sm_relocate(x,y);
   push(k);
   fpush(y);
   fpush(x);
}

void
mir_shade(delta,x,nx,y,ny,type)
int delta;
float *x;
int nx;
float *y;
int ny;
int type;
{
   int n;
   
   if(nx == ny) {
      n = nx;
   } else {
      error("\nVectors have different dimensions in shade");
      n = (nx < ny) ? nx : ny;
   }

   if(type == 1) {
      sm_shade(delta,x,y,n);
   } else if(type == 2) {
      shade_hist(delta,x,y,n);
   } else {
      printf("Unknown type of shading: %d\n",type);
   }
   sm_gflush();
}

/*
 * These functions are needed to ensure that we pass floats, not doubles
 */
void
mir_angle(a)
double a;
{
   sm_angle(a);
}
   
void
mir_axis(a1,a2,as,ab,ax,ay,alen,ilabel,iclock)
double a1,a2,			/* labels from a1 to a2  */
       as,ab; 			/* if as < 0, logarithmic */
int ax,ay,alen,
    ilabel,iclock;
{
   sm_axis(a1,a2,as,ab,ax,ay,alen,ilabel,iclock);
}

void
mir_connect(x,nx,y,ny)
float *x;
int nx;
float *y;
int ny;
{
   int n;
   
   if(nx == ny) {
      n = nx;
   } else {
      error("\nVectors have different dimensions in connect");
      n = (nx < ny) ? nx : ny;
   }

   sm_conn(x,y,n);
}

void
mir_draw(x,y)
double x,y;
{
   sm_draw(x,y);
}
   
void
mir_draw_point(ux,uy,n,istyle)
double ux,uy;				/* user coordinates to draw point */
int n, istyle;
{
   sm_draw_point(ux,uy,n,istyle);
}

void
mir_errorbar(x,nx,y,ny,e,ne,dir)
float *x;
int nx;
float *y;
int ny;
float *e;
int ne;
int dir;
{
   int n;
   
   if(nx == ny) {
      n = nx;
   } else {
      error("\nVectors have different dimensions in errorbar");
      n = (nx < ny) ? nx : ny;
   }
   if(n != ne) {
      error("Dimension of errors differs from that of vectors in errorbar");
      n = n > ne ? ne : n;
   }

   sm_errorbar(x,y,e,dir,n);
}
   
void
mir_expand(e)
double e;
{
   sm_expand(e);
}

void
mir_histogram(x,nx,y,ny)
float *x;
int nx;
float *y;
int ny;
{
   int n;
   
   if(nx == ny) {
      n = nx;
   } else {
      error("\nVectors have different dimensions in histogram");
      n = (nx < ny) ? nx : ny;
   }

   sm_histogram(x,y,n);
}
   
void
mir_limits(ax1,ax2,ay1,ay2)
double ax1,ax2,ay1,ay2;
{
   sm_limits(ax1,ax2,ay1,ay2);
}
   
void
mir_relocate(x,y)
double x,y;
{
   sm_relocate(x,y);
}
   
void
mir_line(xa,ya,xb,yb)
double xa,ya,xb,yb;
{
   sm_line(xa,ya,xb,yb);
}
   
void
mir_notation(xlo, xhi, ylo, yhi)
double xlo, xhi, ylo, yhi;
{
   sm_notation(xlo, xhi, ylo, yhi);
}

void
mir_plotsym(x,nx,y,ny,sym,nsym)
float *x;
int nx;
float *y;
int ny;
int *sym;
int nsym;
{
   int n;
   
   if(nx == ny) {
      n = nx;
   } else {
      error("\nVectors have different dimensions in plotsym");
      n = (nx < ny) ? nx : ny;
   }

   sm_plotsym(x,y,n,sym,nsym);
}

void
mir_points(x,nx,y,ny)
float *x;
int nx;
float *y;
int ny;
{
   int n;
   
   if(nx == ny) {
      n = nx;
   } else {
      error("\nVectors have different dimensions in points");
      n = (nx < ny) ? nx : ny;
   }

   sm_points(x,y,n);
}

void
mir_ticksize(asx,abx,asy,aby)
double asx,abx,asy,aby;
{
   sm_ticksize(asx,abx,asy,aby);
}
#else
int mirella_sm_c;			/* Make ld/ar/ranlib happy even if it's picky */
#endif
