#include "mirella.h"
#include "images.h"

#define NDPIC 8
static DISPLAY *disstack[NDPIC] = { NULL }; /* stack of displayed buffers */
static int dis_index;			/* index into disstack */
static int ndis;			/* number of active displays */
static int xmax_max = 0;		/* largest character position used */
int conv_dlt_to_dsp = 1;		/* should we convert the DLT to
					   device-specific form? */
/* 
 *
 * The windowing facility is very simple.  A given picture is displayed
 * within boundaries on the screen described by d_xlow, d_ylow, d_xhigh,
 * d_yhigh in the current display structure.  When it is displayed, the
 * current display structure, which contains the offsets into the picture
 * space for the lower left corner, the above boundaries, the zoom and
 * crunch factors, the stretch, the wrapflag, and the data access mode, is
 * pushed onto a circular stack of depth NDPIC, which is a power of 2. 
 * NDPIC - 1 pictures can be kept active at a time.  The cursor
 * code uses findwind() at each cursor keystroke to find the index of the
 * active window, which it then copies temporarily into the current window
 * structure. 
 */
/*********************** INIT_DISP_STACK() **********************************/

void
init_disp_stack()
{
   int i;
   
   if(disstack[0] == NULL) {
      disstack[0] = (DISPLAY *)envalloc(NDPIC*sizeof(DISPLAY));
      for(i = 0;i < NDPIC;i++) {
         disstack[i] = disstack[0] + i;
      }
   }
   reset_disp_stack();
}

/*********************** RESET_DISP_STACK() *******************************/
/*
 * reset the display stack
 */
void
reset_disp_stack()
{
   dis_index = -1;
   c_disp = NULL;
   ndis = 0;
}

/********************** FINDWIND() *****************************************/
/*
 * finds the first window containing the point (x,y)
 */
DISPLAY *
findwind(x,y)
normal x,y;
{
    register int i,j;

    for(i = dis_index;i >= 0 && i > dis_index - ndis;i--) {
       j = i % NDPIC;
       if(x >= disstack[j]->_dxlow && x <= disstack[j]->_dxhigh &&
	  y >= disstack[j]->_dylow && y <= disstack[j]->_dyhigh) {
	   return(disstack[j]);
	}
    }
    return(NULL);
}

/************************* PUSHWIND() ****************************/
/*
 * makes a window from the current display structure and sets dis_index to
 * point to it
 */
void
pushwind()
{
   if(d_xlow == 0 && d_ylow == 0 &&
      ((d_xhigh == wxsize && d_yhigh == wysize) ||
				    	     (d_xhigh == 0 && d_yhigh == 0))) {
      /* writing to whole screen; reset display stack */
      dis_index = -1;
      ndis = 0;
   }
   dis_index++;
   ndis++;
   strcpy(d_pfname,basename(h_pfname));
   bufcpy((char *)disstack[dis_index % NDPIC],(char *)display,dispsize);
   c_disp = disstack[dis_index % NDPIC];
}

/* Confusing as hell--the BUFFER is the memory/disk data; the image
 * is a display thing, but exactly what is it??
 */

/******************** BUF_TO_IMAGE() ****************************************/
/*
 * Return the image coordinates of a pixel, given its buffer coordinates
 */
int
buf_to_image(x,y)
int *x,*y;
{
   if((c_disp = findwind(*x,*y)) == NULL) {
      return(-1);
   } else {
      *x = c_disp->_xoff + 
                     (float)c_disp->_cden*(*x - c_disp->_dxlow)/c_disp->_cnum;
      *y = c_disp->_yoff +
                     (float)c_disp->_cden*(*y - c_disp->_dylow)/c_disp->_cnum;
      return(0);
   }
}

/************************** GET_VALUE() ************************************/
/*
 * Convert (*x,*y) from buffer to image cordinates and find the
 * value at (*x,*y); return 0 if OK, -1 if (*x,*y) is invalid. If
 * the point is valid, but the value may be out of date, return a
 * guess as to the value and a flag of 1
 */
int
get_value(x,y,val)
int *x,*y;				/* position in buffer coords */
short *val;
{
   int buf;
   
   if(buf_to_image(x,y) != 0) {
      *val = 0;
      return(-1);
   }
   buf = c_disp->_bufno;
   if(ISBUF(buf)) {
      *val = pbufs[buf]->_pline[*y][*x];
      return(c_disp->_bufsno == pbufs[buf]->_header._serno ? 0 : 1);
   } else {
      *val = 0;
      return(-1);
   }
}

/************************ IMAGE_TO_BUF() ********************************/
/*
 * Given coordinates in the image convert to buffer coordinates; if
 * we aren't in the window that the last findwind() returned, try to
 * find the right one.
 */
int
image_to_buf(xx,yy)
int *xx,*yy;
{
   int x = *xx,
       y = *yy;
   
   if(c_disp == NULL) {
      return(-1);
   }
/*
 * First try the naive conversion, valid if the cursor is still within
 * the window that the last findwind() returned
 */
   x = c_disp->_dxlow + c_disp->_cnum*(x - c_disp->_xoff)/c_disp->_cden;
   y = c_disp->_dylow + c_disp->_cnum*(y - c_disp->_yoff)/c_disp->_cden;
/*
 * So see if it's valid, and try again if it isn't
 */
   if(x >= c_disp->_dxlow && x <= c_disp->_dxhigh &&
      y >= c_disp->_dylow && y <= c_disp->_dyhigh) {
      *xx = x; *yy = y;
      return(0);
   } else {
      if((buf_to_image(&x,&y)) == -1) {
	 return(-1);
      } else {
	 *xx = x; *yy = y;
	 return(image_to_buf(xx,yy));
      }
   }
}


/******************* FIND_EXTREME() ***********************************/
/*
 * Find the extreme value of an image. This isn't really a very efficient
 * way to do it, but it uses a function that exists anyway, get_value()
 *
 * Note that the initial position (*x,*y) is in BUFFER coordinates and
 * is returned in IMAGE coordinates
 */
#define SIZE 10
static void _find_extreme();

void
find_extreme()
{
   normal y = pop;
   normal x = pop;
   int c = pop;

   image_to_buf(&x,&y);
   _find_extreme(&x,&y,c);
   push(x);
   push(y);
}

static void
_find_extreme(x,y,c)
int *x,*y;
int c;					/* m, M, z, or Z */
{
   short extreme;
   int i,j;
   int size;				/* size of region to search */
   short val;				/* value of a pixel */
   int xx,yy;				/* position in image coords. */
   int xmin,xmax;			/* edges of region */
   int ymin,ymax;

   size = islower(c) ? SIZE : 2*SIZE;
     
   xmin = *x - size/2;
   xmax = *x + size/2;
   ymin = *y - size/2;
   ymax = *y + size/2;
   
   if(c == 'm' || c == 'M') {
      extreme = -30000;
      for(i = ymin;i < ymax;i++) {
	 for(j = xmin;j < xmax;j++) {
	    xx = j; yy = i;
	    if(get_value(&xx,&yy,&val) == 0 && val > extreme) {
	       extreme = val;
	       *x = xx;
	       *y = yy;
	    }
	 }
      }
   } else {
      extreme = 30000;
      for(i = ymin;i < ymax;i++) {
	 for(j = xmin;j < xmax;j++) {
	    xx = j; yy = i;
	    if(get_value(&xx,&yy,&val) == 0 && val < extreme) {
	       extreme = val;
	       *x = xx;
	       *y = yy;
	    }
	 }
      }
   }

   return;
}

/*************************** CCUR() ***************************************/
/*
 * centres cursor in the the current (top) window
 */
void
ccur()
{
    c_xs = d_xoff + ((d_xhigh - d_xlow)/2)*d_cden/d_cnum;
    c_ys = d_yoff + ((d_yhigh - d_ylow)/2)*d_cden/d_cnum;
}

/************************* ZZ(), XYC(),XYCORN(),ZQUESTION() *****************/

/* sets the stretch between z1 and z2 */
void
zz(z1,z2)
int z1, z2;
{
   float fac;
   register int i;
   register u_char *tp = dltable;

   d_zorg = z1;
   d_zrng = z2 - z1;
   fac = (NCOLOR - 1)/(float)abs(z2 - z1);

   /*mprintf("\n(ME): NCOLOR=%d fac=%f dltable=%u\n",NCOLOR,fac,dltable);*/
   
   if(d_wrapflg) {    
      int offset = (int)((32768 + z1)*fac/NCOLOR + 1)*NCOLOR;
					      /* offset makes % well-behaved */
      for(i = -32768;i < 32768;i++) {
	 tp[i] = ((int)((i - z1)*fac) + offset)%NCOLOR;
      }
   } else {				/* no wrap */
      if(z2 > z1) {
	 for(i = -32768; i < z1; i++) { /* positive */
	    tp[i] = 0;
	 }
	 for(;i <= z2;i++) {
	    tp[i] = (int)((i - z1)*fac);
	 }
	 for(;i < 32768;i++) tp[i] = NCOLOR - 1;
      } else {
	 for(i = -32768;i < z2;i++) {
	    tp[i] = NCOLOR - 1;
	 }
	 for(;i <= z1;i++) {
	    tp[i] = NCOLOR - (int)((i - z2)*fac) - 1;
	 }
	 for(;i < 32768;i++) tp[i] = 0;
      }
   }
   /* mprintf("\n Calling dsp_setdlt?, conv_dlt_to_dsp=%d\n",conv_dlt_to_dsp);*/
   if(conv_dlt_to_dsp && qdsp_setup() ) {
      dsp_setdlt(); 
   }
}

void xyc(x,y)    /* not xyCorner, but xyCenter */
int x,y;
{
    void xycorn();
#ifdef GRAPH32
    d_wxcen = x;
    d_wycen = y;
    dspwindow();
#else
    /* center the display in the current window */
    int dxl = d_xhigh-d_xlow;
    int dyl = d_yhigh-d_ylow;
    int hdx = (dxl*d_cden)/(2*d_cnum); 
        /* half of dim of window in image space */
    int hdy = (dyl*d_cden)/(2*d_cnum);
    int xo = x-hdx;
    int yo = y-hdy;
    xycorn(xo,yo);
#endif
}


void xycorn(x,y)
int x,y;
{
    d_xoff = x;
    if(d_xoff < 0) d_xoff = 0;
    if(d_xoff >= xsize) d_xoff = xsize-1;
    d_yoff = y;
    if(d_yoff < 0) d_yoff = 0;
    if(d_yoff >= ysize) d_yoff = ysize-1;
}


void zquestion()
{
    mprintf("Stretch %d %d\n",d_zorg, d_zorg + d_zrng);
}

/************************ MTV() ********************************************/

void
mtv()
{
   int yup,xup;			/* highest index in picture array */
   int ydh,xdh;			/* highest index in display array */
   int xo,yo;				/* lleft corner in picture space */
   int dxhold = d_xhigh;
   int dyhold = d_yhigh;

   xo = (d_xoff >= 0) ? d_xoff : 0;
   yo = (d_yoff >= 0) ? d_yoff : 0;

   if(d_xlow < 0) d_xlow = 0;
   if(d_xhigh == 0 || d_xhigh >= wxsize) d_xhigh = wxsize - 1;
   if(d_ylow < 0) d_ylow = 0;
   if(d_yhigh == 0 || d_yhigh >= wysize) d_yhigh = wysize - 1;
    
   xdh = d_xlow + ((xsize-xo)*d_cnum)/d_cden - 1;
   ydh = d_ylow + ((ysize-yo)*d_cnum)/d_cden - 1; /* naive upper limits */
   if(d_xhigh > xdh) d_xhigh = xdh;	/* if pic is smaller than avail space,
					   cut avail space */
   if(d_yhigh > ydh) d_yhigh = ydh;
    
   xup = xo + (( d_xhigh - d_xlow )*d_cden)/d_cnum;
   yup = yo + (( d_yhigh - d_ylow )*d_cden)/d_cnum;
			

   dsup();				/* update disp rev no */
   pushwind();				/* push displ params on window stack */

   dsp_image(pic,xo,yo,xup,yup);
   xmax_max = 0;			/* we've overwritten the print window*/
   
   d_xhigh = dxhold;
   d_yhigh = dyhold;
   return;
}

/************************ DSUP() *******************************************/
/*
 * makes display rev no. agree with current buffer's
 */
mirella void 
dsup()
{
    d_bufno = pbuffer;
    d_bufsno = pbuf->_header._serno;
}

#if 0 /* old rhl-jeg functions */
/*********************** DSPPRINT FUNCTIONS ***************************/

#define HEADSIZE (12 + 1)

static char *prline = NULL;
static char *headline[HEADSIZE];
static int prsize;
static int pxorig = 0;
static int pyorig = 0;
static int pxcur = 0;
static int pycur = 0;       /* display coordinates for next write */
static int pwipe = 1;       /* printing after a '\n' wipes prev text */
static int dp_twwid = 80;
static int dp_twhgt = 1;     /* window size */

int printfont = 0;			/* default is 8x12 font */

#define PBKG 63
#define PFG  0

/********************** SETDPORIG() ***************************************/
/*
 * sets the origin for DSPPRINT
 */
mirella void 
setdporig(x,y)
int x,y;
{
   pxorig = x;
   pyorig = y;   
}


/********************** SETDPWINDOW() ***********************************/
/* sets the window size for dspprint */
mirella void
setdpwindow(wid,hgt)
int wid,hgt;
{
   dp_twwid = wid;
   dp_twhgt = hgt;
   if(dp_twwid*m_cwid > d_xsize - pxorig){
      dp_twwid = (d_xsize - pxorig)/m_cwid;
      mprintf("\nWindow will not fit; reducing width to %d",dp_twwid);
   }
   if(dp_twhgt*(m_chgt + 1) > d_ysize -pyorig){
      dp_twhgt = (d_ysize - pyorig)/(m_chgt+1);
      mprintf("\nWindow will not fit; reducing height to %d",dp_twhgt);
   }
}

/**************** DSPPRINT() ***********************************************/
/*
 * prints the string on the display at pxorig,pyorig
 */


mirella int
dspprint(str)
char *str;
{
   int i, j, k, nchar, c;
   int oldfont;
   register char cline;
   register char *hptr;
   int xn = 0;          /* how many pixels have we added ? */
   int xmax = pxcur;    /* maximum x, this call */
   int xmin = pxcur;    /* minimum x, this call */
   static int xmaxe= 0;
   static int pxoo = -1;
   static int pyoo = -1;
   static int woo = -1;
   static int hoo = -1;
   int scr;
   int nwrt = 0;
   int prs1;
   int newflg =0;
   
   if(m_fontindex != printfont){
      oldfont = m_fontindex;
      setfont(printfont);
      newflg = 1;
   }
   
   setdpwindow(dp_twwid,dp_twhgt);   /* just checking */
    
   if(pxoo != pxorig || pyoo != pyorig || woo != dp_twwid || hoo != dp_twhgt) {
      newflg = 1;
   }
   pxoo = pxorig;
   pyoo = pyorig;
   woo = dp_twwid;
   hoo = dp_twhgt;

   prs1 = dp_twwid*m_cwid*(m_chgt+1);    
   if(prline == NULL || prsize < prs1 || newflg) {
      prsize = prs1;
      if((prline = malloc(prsize + 1024)) == NULL) {
         erret("\nDSPPRINT: Cannot allocate memory");
      }
      filln(prsize,prline,PBKG);
      for(i = 0;i <= m_chgt;i++) {
         headline[i] = prline + i*dp_twwid*m_cwid;
      }
      pxcur=pxorig;
      pycur=pyorig;
      xmaxe = pxorig;
      newflg = 0;
        
   }
   prsize = prs1;

   nchar = strlen(str);
    
   for(k=0; k<nchar; k++) {
      c = (str[k]&0x7f) - ' ';
      if(c<0){        /* control char; we recognize a few */
         c += ' ';  /* recover ascii code */
         switch(c){
          case '\r':
            pxcur = pxorig;
            xn = 0;
            break;
          case '\n':
            pxcur = pxorig;
            xn = 0;
            pwipe = 1;  /* wipe before you print any FOLL. text */
            break;
          default:
            break;
         }                
         continue;
      }
      if(pwipe) {      /* wipe the line to the max # of chars prev. printed */
         for(j=0;j<m_chgt;j++){
            if(xmaxe-pxorig)filln(xmaxe-pxorig,headline[j],PBKG);
         }
         pwipe = 0;
      }

      if(xn > dp_twwid*m_cwid){  /* run over, like a \r */
         pxcur = pxorig;
         xn = 0;      /* start over at the origin */
      }
      if((scr = xn+pxcur) < xmin) xmin = scr;

      for(j = 0;j < m_chgt;j++){       /* loop over char lines */
         hptr = (char *)headline[j] + pxcur + xn ;
         cline = m_ctab[c][j];
         for(i = 0;i < m_cwid;i++){
            if(cline & m_cmask[i]){
               *hptr++ = PFG;
            } else {
               *hptr++ = PBKG;
            }
         }
      }
      xn += m_cwid;
      if((scr = pxcur + xn) > xmax) xmax = scr;
      if(scr > xmaxe) xmaxe = scr;
      nwrt++;
   }
   /* now have string; write it */
   for(j=0;j<(m_chgt+1);j++){
      if(xmax-xmin > 0) dbytewrt(headline[j]+xmin,xmax-xmin,xmin,pycur+j);
   }

   pxcur += xn;
                                    
   return(nwrt);
}
#endif

/*************** JEGDOSCODE *******************************************/

/*********************** DSPPRINT FUNCTIONS ***************************/

extern u_char **m_ctab;			/* defined in hardchar.c */

/* This will eventually be a full-up window manager for the display */

static u_char *prline= 0;     /*jeg9207*/
static u_char *headline[20];  /*jeg9207*/
static int prsize=0;          /*jeg9207*/
static int pxcur = 0;
static int pycur = 0;       /* display coordinates for next write */
static int pxorig = 0;
static int pyorig = 0;
static int pwipe = 0;       /* printing after a '\n' wipes prev text */
static int dp_twwid = 80;
static int dp_twhgt = 1;     /* window size */

int printfont = 0;      /* default is 8x12 font */

#define PBKG 127 /* WH2 */
#define PFG  0


/*********************** SETDPORIG() **********************************/
/* sets the origin for DSPPRINT */
mirella void 
setdporig(x,y)
int x,y;
{
    pxorig = x;
    pyorig = y;    
}

/***********************  GETDPORIG() **********************************/
/* pushes the origin */   /*jeg9808*/
mirella void
getdporig()
{
    push(pxorig);
    push(pyorig);
}

/********************** SETDPWINDOW() ***********************************/
/* sets the window size for dspprint */
mirella void
setdpwindow(wid,hgt)
int wid,hgt;
{
   dp_twwid = wid;
   dp_twhgt = hgt;
   if(dp_twwid*m_cwid > d_xsize - pxorig){
      dp_twwid = (d_xsize - pxorig)/m_cwid;
      mprintf("\nWindow will not fit; reducing width to %d",dp_twwid);
   }
   if(dp_twhgt*(m_chgt + 1) > d_ysize -pyorig){
      dp_twhgt = (d_ysize - pyorig)/(m_chgt+1);
      mprintf("\nWindow will not fit; reducing height to %d",dp_twhgt);
   }
}

/*********************** DSPPRINT() *************************************/
/* a (*scrnprt)() function for writing text to the screen in a window */

mirella int
dspprint(str)       /* prints the string on the display at pxcur,pycur */
char *str;
{
    int i, j, k, nchar, c;
    /* int oldfont; */
    register char cline;
    register char *hptr;
    int xn = 0;          /* how many pixels have we added ? */
    int xmax = pxcur;    /* maximum x, this call */
    int xmin = pxcur;    /* minimum x, this call */
    static int xmaxe= 0;
    static int pxoo = -1;
    static int pyoo = -1;
    static int woo = -1;
    static int hoo = -1;
    int scr;
    int nwrt = 0;
    int prs1;
    int newflg =0;

    if(m_fontindex != printfont){
        /* oldfont = m_fontindex; */
        setfont(printfont);
        newflg = 1;
    }
    
    setdpwindow(dp_twwid,dp_twhgt);   /* just checking */
    
    if(pxoo != pxorig || pyoo != pyorig || woo != dp_twwid || 
            hoo != dp_twhgt) newflg = 1;
    pxoo = pxorig;
    pyoo = pyorig;
    woo = dp_twwid;
    hoo = dp_twhgt;

    prs1 = dp_twwid*m_cwid*(m_chgt+1);    
    if(!prline || prsize < prs1 || newflg){
        prsize = prs1;
        prline = (u_char *)malloc(prsize + 1024);  /* dp_twwid-char line */
        if(!prline) erret("\nDSPPRINT: Cannot allocate memory");
        filln(prsize,(char *)prline,PBKG);
        for(i=0;i<m_chgt+1;i++){
            headline[i] = prline + i*dp_twwid*m_cwid;
        }
        pxcur=pxorig;
        pycur=pyorig;
        xmaxe = pxorig;
        newflg = 0;
        
    }
    prsize = prs1;

    nchar = strlen(str);
    
    for(k=0; k<nchar; k++) {
        c = (str[k]&0x7f) - ' ';
        if(c<0){        /* control char; we recognize a few */
            c += ' ';  /* recover ascii code */
            switch(c){
            case '\r':
                pxcur = pxorig;
                xn = 0;
                break;
            case '\n':
                pxcur = pxorig;
                xn = 0;
                pwipe = 1;  /* wipe before you print any FOLL. text */
                break;
            case BS:
            case DEL:       /* backspace or del; just reposition cursor */
                if(xn + pxcur > pxorig){
                    xn -= m_cwid;
                }
                break;               
            default:
                break;
            }                
            continue;
        }
        if(pwipe){
            /* wipe the line to the max # of chars prev. printed */
            for(j=0;j<m_chgt;j++){
                if(xmaxe-pxorig)filln(xmaxe-pxorig,(char *)headline[j],PBKG);
            }
            pwipe = 0;
        }

        if(xn > dp_twwid*m_cwid){  /* run over, like a \r */
            pxcur = pxorig;
            xn = 0;      /* start over at the origin */
        }
        if((scr = xn+pxcur) < xmin) xmin = scr;

        for(j=0;j<m_chgt;j++){       /* loop over char lines */
            hptr = (char *)headline[j] + pxcur + xn ;
            cline = m_ctab[c][j];
            for(i=0;i<m_cwid;i++){
                if(cline & m_cmask[i]){
                    *hptr++ = PFG;
                }else{
                    *hptr++ = PBKG;
                }
            }
        }
        xn += m_cwid;
        if((scr = pxcur + xn) > xmax) xmax = scr;
        if(scr > xmaxe) xmaxe = scr;
        nwrt++;
    }
    /* now have string; write it; this only works for systems supporting
       dbytewrite; all?? */
    for(j=0;j<(m_chgt+1);j++){
        if(xmax-xmin > 0) dbytewrt((char *)headline[j]+xmin,xmax-xmin,xmin,pycur+j);
    }

    pxcur += xn;
                                    
    return(nwrt);
}

/********************* DSPPUT(),DSPGETC(),DSPGETS() **********************/
/* added jeg9405 */

/* puts one char on screen in 'next' position */

mirella void 
dspput(c)
int c;
{
    static char dspstr[2]={0,0};
    dspstr[0] = c & 0x7f;
    (void)dspprint(dspstr);
}

/* accepts a char from the kbd and dspput()s it on screen */

mirella int 
dspgetc()
{
    int c;
    c = get1char(0) & 0x7f;
    dspput(c);
    return c;
}

/* 
 * Accepts a \n term. string from the keybd and writes it on screen as you go;
 * simple editing (backspace) is allowed. It dprint()s the final newline but
 * does NOT place it in string.
 */
     
mirella void
dspgets(str) 
char *str;
{
    int c;
    char * str0 = str;
    
    while((c=dspgetc()) != '\r' && c != '\n'){
        if(c == BS || c == DEL){
            if(str > str0) str--;   /* back up the string pointer */ 
            dspput(' ');   /* erase the char on the display */
            dspput(BS);   /* and set the screen cursor back */
        }else{
            *str++ = c;    /* just add the char to the string */
        }
    }
    *str = '\0';           /* terminate it if get a \n */
    return;
}


/***************** RHLPRINTF ***************************************/

/*********************** DPRINTF() *********************************/


#ifdef STDARGS
/*
 * this is the ansi standard way of doing things
 */
#include <stdarg.h>

int 
mdprintf(char *fmt, ...)
{
   va_list ap;

   va_start(ap,fmt);
   vsprintf(out_string,fmt,ap);
   va_end(ap);
   return dspprint(out_string);
}

#endif
#ifdef VARARGS
/*
 * This is the old way of doing things `properly' -- please use stdargs
 * if you can
 */
#include <varargs.h>

int 
mdprintf(va_alist)
va_dcl
{
    va_list ap;
    char *fmt;
   
    va_start(ap);
    fmt = va_arg(ap,char*);
    vsprintf(out_string,fmt,ap);
    va_end(ap);
    return dspprint(out_string);
}
#endif

#if !defined(STDARGS) && !defined(VARARGS)
#define SDIR +

int
mdprintf(str,args)
char *str;   
normal args;
{
   normal *aptr = (&args);
   
   sprintf(out_string,str,
	   *(aptr       ),
	   *(aptr SDIR 1),
	   *(aptr SDIR 2),
	   *(aptr SDIR 3),
	   *(aptr SDIR 4),
	   *(aptr SDIR 5),
	   *(aptr SDIR 6),
	   *(aptr SDIR 7),
	   *(aptr SDIR 8),
	   *(aptr SDIR 9),
	   *(aptr SDIR 10),
	   *(aptr SDIR 11),
	   *(aptr SDIR 12),
	   *(aptr SDIR 13),
	   *(aptr SDIR 14),
	   *(aptr SDIR 15),
	   *(aptr SDIR 16));
   /* hokey,hokey, hokey --at most 16 normal-sized arguments, but it
      should not be impossible to figure out how to add more. */
   return dspprint(out_string); /* goes only to screen */
}
#endif

/********************** END MODULE DISPLAYS.C ******************************/
