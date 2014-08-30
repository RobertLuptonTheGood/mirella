#include "mirella.h"
#include "images.h"

#if defined(NOIMDISPLAY)
/* these routines are all called in setup or in existing Mirella code */

dspinit()  /* set INITIAL software size parameters */
{
   printf("\nINITING IMAGINARY DISPLAY");
   
   dxsize = 816;
   dysize = 800;
   
   d_xoff = 0;				/* default x,y offsets; 816x800 */
   d_yoff = 0;
}
void draw_a_line_to(x1,y1) {}		/* dummy for now */
putdsppts() {}
void dbytewrt(b,n,x,y) int b,n,x,y; {}
ggg()       {}
void cursor()    {}
markit()    {}
makemark()  {}
qq()        {}
panmode()   {}
popmark()   {}
greyscale() {}
void dspwindow() {}
void setdlt()    {}
getpix()    {}
setpix()    {}
dps_erase() {}



/* these routines are called neither in setup or in existing Mirella code,
but have links to Mirella; they can be dummied */

vltsel()    {}
void edgraph(x0, y0, nx, ny) int x0, y0, nx, ny; {}
colvlt()    {}
truecolor() {}
gryvlt()    {}
to_pal()    {}
rdvlt()     {}
setbm()     {}
hramp()     {}
dspclose()  {}

/* these are global variables which are used internally in the
existing cursor and display code; they can be dummied */

u_char vlta[768];
u_char vltb[768];
#else
int mirella_video_c;			/* Make ld/ar/ranlib happy even if it's picky */
#endif
