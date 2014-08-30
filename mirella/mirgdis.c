#ifdef VMS
#include mirella
#include edit
#include images
#include graphs
#else
#include "mirella.h"
#include "edit.h"
#include "images.h"
#include "graphs.h"
#endif

normal grafcolor = 0x7;       /* IBM-style colour number for graphics */
normal g_revv = 1;            /* flag for reverse video for graphics */

static normal donot();          /* do nothing function. returns 0 */
normal (*m_grefresh)()=donot;   /* function for refreshing screen when going
                                    to grafmode = 1 (graphics mode) in those
                                    systems in which a refresh is necessary */
normal (*m_irefresh)()=donot;   /* function for refreshing screen when going
                                    to grafmode = 2 (image mode) in those
                                    systems for which a refresh is necessary */
                                /* these functions are only executed at
                                   MODE CHANGES, which are only executed
                                   by gmode(). OK???? do NOT violate this
                                   rule. they are NEVER executed in any
                                   code which does not require screen
                                   refresh, which includes all multidisplay
                                   systems and systems with a resident 
                                   window manager. This mechanism was
                                   installed to accomodate the VGA display
                                   on DOS systems. The refresh functions
                                   do a refresh for zero arg, and any
                                   initialization necessary but DO NOT
                                   do a refresh for positive arg, and
                                   do any preparation for a new picture
                                   (vlt, etc) for negative arg */


/* recent history
 * 06/03 Added forth hook in gend() for NOGDISPLAY so can call an
 * external display program for graphics
 */

/******* This Code is for Graphics, as distinct from Images **************/

/********************** OGMODE() *****************************************/

static int oldgmode = 0;   /* last non-text mode */

void ogmode()  /* return to it */
{
    gmode(oldgmode);
}

/*********************** DONOT() *****************************************/

static normal donot()   /* doesnothing function */
{
    return 0;
}

/**************************************************************************
 * following is code for gmode(), gd_line(), gd_todis(), gend(), gbegin(),*
 * and gd_init()  for various bitmapped and vector display hardware       *  
 **************************************************************************/

/*********************************************************************/
/************************** HERCULES *********************************/
/*********************************************************************/

#ifdef HERCULES

/* toggles between Hercules graphics page 2 and text page 1
according to the contents of location 0 of its data register */
/* grafmode is 0 for text, 1 for graphics */

static u_char *hdbase = (u_char *)0xb8000;

normal gd_xsize = 720;
normal gd_ysize = 348;
normal gd_xbsize = 90;

static int pr =  0x03b4 ;     /* 6845 pointer register */
static int dr =  0x03b5 ;     /* 6845 data register */
static int msr = 0x03b8 ;     /* 6845 mode select register */
static int sr =  0x03ba ;     /* 6845 status register */

static int t6845[12] = {0x61,0x50,0x52,0x0f, 0x19,0x06,0x19,0x19, 
                        0x02,0x0d,0x0b,0x0c};  /* text table */
                        
static int g6845[12] = {0x35,0x2d,0x2e,0x07, 0x5b,0x02,0x57,0x57,
                        0x02,0x03,0x00,0x00} ;  /* graphics table */

static int ton =  0x08;   /* text, active */
static int toff = 0x00;   /* text, blank */
static int gon  = 0x8a;   /* graphics, active */
static int goff = 0x82;   /* graphics, blank */

#define WFRM 250  
/* wait before turning on beam between mode changes */

void
gmode(mode)   /* switches to text with arg 0, graphics
                        otherwise */
int mode;
{
    int tflg=0;
    int i;
    int timer;
    
    tflg = (!mode);  /* sw to text mode */
    if(tflg){
        if(grafmode){
            /* set text mode */
            OUTPUT(msr,toff);
            for(i=11;i>=0;i--){
                OUTPUT(pr,i);
                OUTPUT(dr,t6845[i]);
            }
        }
        grafmode=0;
    }else{
        /* set graphics mode */
        OUTPUT(msr,goff);
        for(i=11;i>=0;i--){
            OUTPUT(pr,i);
            OUTPUT(dr,g6845[i]);
        }
        grafmode=1;
    }
    waitfor(WFRM);    /* delay 250 ms */
    if(tflg) OUTPUT(msr,ton);
    else     OUTPUT(msr,gon);
}

Void *
gd_line(yd)  /* returns address of line origin */
int yd;
{ 
    return (0x2000 * ((yd) & 3) + 90*((yd) >> 2) + hdbase);  
}
        
#define EBUFL  4095   
void 
gers() /* erases Hercules graphics screen; memory operation, no registers*/
{
    u_char *ebuf;
    int i;
    register u_char *cp, *cpend;
    
    if(!(ebuf = (u_char *)malloc(EBUFL))) erret("No memory");
    cp = ebuf;
    cpend = ebuf + EBUFL;
    while(cp < cpend) *cp++ = 0;
    for(i=0;i<8;i++) 
        MOVE_TO(ebuf, hdbase + i*EBUFL, EBUFL );
    free(ebuf);
}

void
gd_todis(from,to,nby)
Void *from, *to;
normal nby;
{ 
    MOVE_TO(from,to,nby); 
}

void 
gend()
{
    ctoscreen();
}

void gbegin()
{
    gmode(1);
}

void gd_init()
{
    gers();
}  /* this should enable Herc graphics; but running hgc at autoexec
       does it, so why bother ? */

#endif



/************************************************************************/
/************************** G4010 ***************************************/
/************************************************************************/


#if defined(G4010)
/* graphics terminal */
/* grafmode is 0 for alpha terminal, 1 for TEK graphics, >=3 for any
    other display, if any */
normal gd_xsize = 1024;
normal gd_ysize =  768;
normal gd_xbsize = 128;
static FILE *grph_file;

static void g4010mode(),alphaterm();

void gmode(mode)
normal mode;
{
    if(mode) g4010mode();
    else if(grafmode && grafmode < 3 ) alphaterm();
}

void gers()
{
    void e4010();
    e4010();
}

Void *
gd_line(line)
int line;
{
   return(NULL);
}

void
gd_todis(from, to, n)
Void *from,*to;
int n;
{}

void 
gend()
{
    void wesc();
    wesc();
}

void 
gbegin()
{
    g4010mode();
}

void gd_init()
{
    grph_file=stdout;
}    

/*************************************************************************/
/********************** TEKTRONIX DRAWING ROUTINES ***********************/
/*************************************************************************/

/* routine to support 4010-type graphics on terminals with retrographics
    VT640 protocol */

static int hiy, loy, hix, lox ;

#ifndef vms
#define chout(c) putc((c),grph_file)
#else
#define chout(c)   { if(grph_file == stdout) raw_out(c);  \
                     else putc((c),grph_file);}
#endif


/******************* TEKDRAW() *******************************************/

static void tekdraw(x,y,pen) /* draws from prev pos to x,y if pen=1; 
    moves to x,y if pen=0;  screen coordinates: 0<x<1024  0<y<768   */
int x, y, pen ;

{
    if(!pen){
        chout(GS) ; 
        grafmode=1;  /* just in case */
    }
    if( ( x >= 0 ) && ( x < 1024 ) && ( y >= 0 ) && ( y < 768) ){
        hiy = y/32 ;
        loy = y - hiy*32 ;
        hix = x/32 ;
        lox = x - hix*32 ;
        chout(32 + hiy) ;
        chout(96 + loy) ;
        chout(32 + hix) ;
        chout(64 + lox) ;
    }
    g_xold = x; 
    g_yold = y;
    return ;
}

/*********************** WESC() *******************************/
void wesc()   /* wait for ESC key to be pressed to get out of graphics
                    mode if in screen mode; just close file if in file mode*/

{
    if(grph_file == stdout){
        flushit();
        fflush(stderr);
#if 0
        while(get1char(0) != ESC);
        chout(ESC);			/* erase the graphics screen */
        chout(FF);
#endif
        chout(ESC);			/* return to vt100 mode */
        chout(CTRL_C);
	fflush(grph_file);
#if 0
        flushit();
        sleep(1);
#endif
        emit('\n');
        flushit();
        grafmode = 0;
    }else{
        fclose(grph_file);
    }
    g_rdraw = nulldraw;
}

/*
 * return to standard terminal text mode from graphics mode
 */
static void
alphaterm()
{
#ifdef XTERM
    chout(ESC); chout(CTRL_C);
#endif
    chout(ESC);
    chout(FF);
    chout(CAN);
    grafmode = 0;
#ifdef XTERM
    flushit();
#else
    sleep(1);
#endif
}

/******************* G4010, G4010FILE *******************************/

static void
g4010mode()    /* enables 4010 graphics mode */
{
#ifdef XTERM
   chout(ESC); chout('['); chout('?'); chout('3'); chout('8'); chout('h'); 
#else
   clear_phy_scr();
   chout(ESC); chout(FF);    /* enter graphics alpha mode, clear screen */
#endif
   chout(US); chout(GS);    /* set vector graphics mode */
   g_rdraw = tekdraw;
   grafmode =1;
}

void e4010()   /* erase 4010 screen */
{
    chout(ESC);
    chout(FF);     /* erase screen */
    chout(GS);     /* go back to graphics mode */
    grafmode = 1;
}

#if 0   /* not used ?? */
void g4010set()   /* sets parameters for 4010, does not enable graphics */
{
    grph_file = stdout;
    g_xsize = 1024;
    g_ysize = 768;
    g_xold = g_yold = 0;
    plotinall();
    g_rdraw = nulldraw;
    gchan = -1;
}
#endif
  
void g4010file(filename)   /* opens file for output of 4010 stream; no screen
                        output */
char * filename;                        
{
    FILE *fp;
    
    if( !(fp = fopen(filename,"w"))){       /* NOT fopena */
        scrprintf("\nCannot open %s",filename);
        erret((char *)NULL);
    }
    grph_file = fp;
    g_xsize = 1024;
    g_ysize = 768;
    g_xold = g_yold = 0;
    plotinall();
    chout(GS);
    g_rdraw = tekdraw;
    gchan = -1;
    grafmode = 1;
}

#endif

/*************************************************************************/
/*************************  NOGDISPLAY   *********************************/
/*************************************************************************/
/* 
 * dummy routines for systems with no native graphics display, or for
 * those for which the user wishes to hand off the display to another
 * system. In this case, control is handed off in gend() to a forth
 * word, `display', which can do anything/nothing with the current 
 * graph.
 */


#ifdef NOGDISPLAY

/* grafmode is always zero */

normal gd_xsize;
normal gd_ysize;
normal gd_xbsize;       /* display size */

void gmode(mode)
int mode;
{}

Void *
gd_line(line)
int line;
{ return NULL; }

void gers() {}

void gd_todis(from,to,nby)
Void *from, *to;
normal nby;
{ }

void 
gend()
{
    int error;
    error = cinterp("m_display");
    if(error != 1) mprintf("\nForth word `m_display' is not defined");
}

void
gbegin()
{}

void 
gd_init()
{}

void 
g4010file(fname) /* this is a leftover in mirgraph.msc */
char *fname;
{}

#endif

/*************************************************************************/
/************************** VAXSTATION ***********************************/
/*************************************************************************/

#ifdef VAXSTATION
/* VMS UIS graphics */
/* grafmode is always zero */


void 
gmode(mode)
int mode;
{}

void 
gd_todis() 
{}

Void *
gd_line(line)
int line;
{}

void gers()
{
    void egraph();
    egraph();
}

void 
gend()
{}

void 
gbegin()
{
    void gwindow();
    gwindow();    
}

void 
gd_init()
{}


/********************* VMS VAXSTATION UISDC PLOTTING ************************/


#include <uisusrdef.h>
#include ssdef
#include descrip
#include iodef

#define MAXX 1024
#define MAXY 768
normal gd_xsize = MAXX;
normal gd_ysize = MAXY;

extern void vs_draw();

/* we make a formal correspondence between windows and Mirella graphs--
 * the only problem is that Mirella graphs can be arbitrarily big, but
 * windows cannot be bigger than 1024 x 768 (approx) */

#define XPPCM 30.3
#define YPPCM 30.3   
/* this works to about 1% */
/* workstation screen is about 28 x 36 cm */


void gwindow()   /* associate current graph with a window */
{
    float wxs,wys;
    struct dsc$descriptor_s tername;
    struct dsc$descriptor_s winname;
    char winstr[64];
    float f0x = 0.;
    float f1x ; 
    float f0y = 0.;
    float f1y ;
    float wid ;
    float ht ;
    int xsz = g_xsize;
    int ysz = g_ysize;
    int i;
    
    if(xsz > MAXX || ysz > MAXY){
        scrprintf("\nGraph too big; %dx%d max; will display subwindow");
        if(xsz > MAXX) xsz = MAXX;
        if(ysz > MAXY) ysz = MAXY;
    }
    wid = xsz/XPPCM;
    ht = ysz/YPPCM;
    f1x = xsz;
    f1y = ysz;
    
    if(gchan == -1) erret(" No graph");    
    if(g_env[gchan].vd_id != -1) 
        erret("\nWindow already associated with this graph");

    sprintf(winstr,"Mirella Graphics Window %d",gchan);
    _strdesc(winstr,&winname);
    _strdesc("SYS$WORKSTATION",&tername);
            
    g_env[gchan].vd_id = uis$create_display(&f0x,&f0y,&f1x,&f1y,&wid,&ht);
    g_env[gchan].wd_id = uis$create_window(&(g_env[gchan].vd_id),
            &tername,&winname,&f0x,&f0y,&f1x,&f1y,&wid,&ht,&0);

    g_rdraw = vs_draw;
}


void setgwin(nw)
int nw;
{
    if(nw < 0 || nw > NGRAPH-1)erret("\nNo such window");
    if(g_env[nw].vd_id == -1) erret("\nWindow not currently active");
    /* this is normally called by setgraph, so error checking is not really
    necessary, but leave it */
    g_rdraw = vs_draw;
    uis$pop_viewport(&(g_env[nw].wd_id));        
}   
             

void popgwin(nw)
int nw;
{
    if(nw < 0 || nw > NGRAPH-1)erret("\nNo such window");
    if(g_env[nw].vd_id == -1) erret("\nWindow not currently active");
    uis$pop_viewport(&(g_env[nw].wd_id));    
}

void pushgwin()
{
    if(gchan < 0 )erret("\nNo graph");
    if(g_env[gchan].vd_id == -1) erret("\nWindow not currently active");
    uis$push_viewport(&(g_env[gchan].wd_id));    
}

void delgwin(nw) /* deletes window; normally exec by freegraph(), but
                    available separately to Mirella to disconnect window
                    from graph */
int nw;
{
    if(nw < 0 || nw > NGRAPH-1)erret("\nNo such window");
    if(g_env[nw].vd_id == -1) erret("\nWindow not currently active");
    uis$delete_display(&(g_env[nw].vd_id));
    g_env[nw].vd_id = g_env[nw].wd_id = -1;
    if(nw == gchan && g_env[nw].gmcp !=0){  /*current channel, not just freed*/
       g_rdraw = rm_draw;
       mprintf("\nsetting drawing function back to rm_draw");
    }
}

void erswin()  /* erases window; normally executed by egraph() */
{
    if(gchan<0) erret("No graph");    
    if(g_env[gchan].vd_id == -1) erret("\nNo currently active window");
    uisdc$erase(&(g_env[gchan].wd_id),&0,&0,&g_xsize,&g_ysize);
}

void vs_draw(x,y,pen)
int x,y,pen;
{
    if(g_env[gchan].vd_id == -1) erret("No window");
    if(pen) uisdc$plot(&(g_env[gchan].wd_id),&0,&g_xold,&g_yold,&x,&y);
    g_xold = x;
    g_yold = y;
}

void bmtowin()
{
    if(g_env[gchan].vd_id==-1) erret("No window");    
    uisdc$image(&(g_env[gchan].wd_id),&0,&0,&0,&g_xsize,&g_ysize,
            &g_xsize,&g_ysize,&1,gpointer);
}
            
void wintobm()           
{
    int rasterlen = g_ysize*((g_xsize-1)/8 + 1);
    
    if(g_env[gchan].vd_id==-1) erret("No window");
    uisdc$read_image(&(g_env[gchan].wd_id),&0,&0,&g_xsize,&g_ysize,
            &g_xsize,&g_ysize,&1,gpointer,&rasterlen);
}
          
#endif  /* VAXSTATION */

/*****************************************************************************/
/*
 * VGA
 */
#if defined(VGA)

/************************************************************************/
/*************************** VGA ****************************************/
/************************************************************************/
#if defined(FAKE_VGA)
#define INT86(I,J) ;
#define MOVE_TO(S,T,N) ;
typedef struct {
   int intno, ax, ebx, ecx, es, edx;
} R86;
void setmapmask();
int grafcolor;
int g_revv;
int atipage();
void vgaoff();
void vgaon();
void g4010file(file) char *file; {}	/* only need this as I said that
					   I had a G4010 on grendel */
#else
#ifndef DSI
#include <dos.h>
#else
#include <doscalls.h>
#endif
#endif

/* These routines handle most of the VGA functions, including many
    used by Mirage in image display applicatons; Notice is taken of
    whether the board is an ATI board, and the extended modes are
    implemented if so */


int dosdisplay = 0x07;
normal gd_xsize =  640;
normal gd_ysize =  480;
normal gd_xbsize =  80;
int vga_gr_mode = 0x12;       /* dos graphics vga mode, 480x640, 4 planes */

/* most of this stuff is used for imaging */
extern short int ATI_REG;
int ati_plane = -1;           /* current 64K page. set by ati_line() */
int vga_im_mode=-1;           /* DOS vga image mode; def is unknown */

u_char vpagel[800];           /* memory page for the line (index); high
                                 nibble is set if page switch in line */
short int vpagep[800];        /* index in line where page switches; 0 if
                                 no page switch */
u_char *valptr[800];          /* pointer in page a for origin of line */

int BITPERPIX = 8;            /* bits per pixel */
int PIXPERBYTE = 1;           /* pixels per byte in mode. 1 if < 1(ie 16 bit)*/
int PIXPAGES = 5;             /* how many pages in PIX modes ? */
int PIXMODE = 1;              /* is it a PIX (either 1 byte or 1 nibble per
                                pixel) mode ? if not, assumed to be
                                a bitplane mode. 2 for 16-bit packed pixel
                                modes. The number of pixels per running byte
                                is PIXPERBYTE/PIXMODE interpreted as a 
                                fraction */
int VGAPLANES = 1;            /* how many planes (for bitplane modes) ? */
int VGACOLORS=256;            /* 1 << BITPERPIX */
int ATICARD = 0;
int VGACARD = 0;

static int oldvgagmode= -1;   /* vga mode last used in gr or im */

unsigned int dorig = 0xa0000;

extern int atiline();   /* these functions select the line; for the
                        special ati modes, 1 is returned if the page
                        changes in the line, 0 otherwise. Stdline just
                        returns 0, and is used for the IBM std modes */
static void rgers();
extern int stdline();
int (*seline)() = atiline; 

    
/*************************** VGAIMINIT() *******************************/
/* this function sets up the variables appropriate to a vga IMAGE
mode but does NOT perform a mode switch */

void
vgaiminit(mode)
int mode;
{
    int i;
    int scr,scrr;
    int dxsz;
    
    switch(mode){
    case 0x12:
        d_xsize = 640;   /* 640x480 4-bit bitplane */
        d_ysize = 480;
        PIXPERBYTE = 8;
        BITPERPIX = 4;
        PIXPAGES = 1;
        PIXMODE = 0;
        VGAPLANES = 4;
        VGACOLORS=16;
        seline = stdline;
        break;
    case 0x13:          /* 320x200x8 */
        d_xsize = 320;
        d_ysize = 200;
        BITPERPIX = 8;
        PIXPERBYTE = 1;
        PIXPAGES = 1;
        PIXMODE = 1;
        VGAPLANES = 1;
        seline = stdline;
        VGACOLORS=256;
        break;
    case 0x61:          /* 640x400x8 */
        if(!(dosdisplay & 8)) erret("\nVGAIMINIT: NO ATI CARD. ILLEGAL MODE");
        d_xsize = 640;
        d_ysize = 400;
        BITPERPIX = 8;
        PIXPERBYTE = 1;
        PIXPAGES = 4;
        PIXMODE = 1;
        VGAPLANES =1;
        seline = atiline;
        VGACOLORS=256;
        break;
    case 0x62:          /* 640x480x8 */
        if(!(dosdisplay & 8)) erret("\nVGAIMINIT: NO ATI CARD. ILLEGAL MODE");
        d_xsize = 640;
        d_ysize = 480;
        BITPERPIX = 8;
        PIXPERBYTE = 1;
        PIXPAGES = 5;
        PIXMODE = 1;
        VGAPLANES = 1;
        seline = atiline;
        VGACOLORS=256;
        break;
    case 0x63:          /* 800x600x8 */
        if(!(dosdisplay & 8)) erret("\nVGAIMINIT: NO ATI CARD. ILLEGAL MODE"); 
        d_xsize = 800;
        d_ysize = 600;
        PIXPERBYTE = 1;
        BITPERPIX = 8;
        PIXPAGES = 8;
        PIXMODE = 1;
        VGAPLANES = 1;
        seline = atiline;
        VGACOLORS=256;
        break;
    case 0x65:          /* 1024x768x4 */
        if(!(dosdisplay & 8)) erret("\nVGAIMINIT: NO ATI CARD. ILLEGAL MODE");
        d_xsize = 1024;
        d_ysize = 768;
        BITPERPIX = 4;
        PIXPERBYTE = 2;
        PIXPAGES = 8;
        PIXMODE = 1;
        VGAPLANES = 1;
        seline = atiline;
        VGACOLORS=16;
        break;
    case 0x64:          /* 1024x768x8 */ 
        if(!(dosdisplay & 8)) erret("\nVGAIMINIT: NO ATI CARD. ILLEGAL MODE"); 
        d_xsize = 1024;
        d_ysize = 768;
        PIXPERBYTE = 1;
        BITPERPIX = 8;
        PIXPAGES = 12;
        PIXMODE = 1;
        VGAPLANES = 1;
        seline = atiline;
        VGACOLORS=256;
        break;
    case 0x72:          /* 480x640x16 */ 
        if(!(dosdisplay & 8)) erret("\nVGAIMINIT: NO ATI CARD. ILLEGAL MODE"); 
        d_xsize = 640;
        d_ysize = 480;
        PIXPERBYTE = 1;
        BITPERPIX = 16;
        PIXPAGES = 10;
        PIXMODE = 2;
        VGAPLANES = 1;
        seline = atiline;
        VGACOLORS=32768;
        break;
    default:
        scrprintf("\nVGAIMINIT: CANNOT INIT: 0x%x is a mode I do not know",mode);
        erret((char *)NULL);
    }
    /* set global mode */
    vga_im_mode = mode;
    /* set address arrays */    
    if(PIXMODE){            
        for(i=0;i<d_ysize;i++){
            vpagel[i] = (i*((PIXMODE*d_xsize)/PIXPERBYTE))/65536;
            scr      = (i*((PIXMODE*d_xsize)/PIXPERBYTE))%65536;
            scrr = 65536 - scr;
            vpagep[i] = 0;
            if(scrr < (PIXMODE*d_xsize)/PIXPERBYTE){
                vpagep[i] = scrr;
                vpagel[i] |= 0x10;
                /* switches at this pixel in line i-1; marked by
                    16 bit on in page array */
            }
        }
    }
    dxsz = (PIXMODE ? d_xsize*PIXMODE : d_xsize);
    for(i=0;i<d_ysize;i++){
        valptr[i] = (i*(dxsz/PIXPERBYTE))%65536 + (u_char *)dorig;
    }
    wxsize = d_xsize;
    wysize = d_ysize;
    (*m_irefresh)(1);
    if(verbose) mprintf("\nVGA image mode set to %x",vga_im_mode);
}


/*************************** VGAGRINIT() *******************************/
/* this function sets up the variables appropriate to a vga GRAPHICS
mode but does NOT perform a mode switch */

void
vgagrinit(mode)
int mode;
{
    switch(mode){
    case 0x12:
        gd_xsize = 640;   /* 640x480 4-bit bitplane */
        gd_ysize = 480;
        gd_xbsize = 80;
        break;
    case 0x54:          
        if(!(dosdisplay & 8)) erret("\nVGAGRINIT: NO ATI CARD. ILLEGAL MODE");
        gd_xsize = 800;   /* 800x600 4-bit bitplane */
        gd_ysize = 600;
        gd_xbsize = 100;
        break;
    default:
        scrprintf("\nVGAGRINIT: CANNOT INIT: 0x%x is a mode I do not know",mode);
        erret((char *)NULL);
        break;
    }
    /* set global mode */
    vga_gr_mode = mode;
}


/************************ STATIC GMODE() FNS *******************************/

#if 0
static int oldmode;
static int oldln;
static int oldpg;
#endif

#ifdef DSI
#define vgaon()   donot()
#define vgaoff()  donot()
#endif

    
#define SETTLE()  vgaoff(); waitfor(300); vgaon();


static int
setvgaim()   /* sets vga image mode with INT 10H */
{
    R86 regs;
    char erbuf[80];
    int newmode;

    vgaoff();
    /* set the new one */
    regs.ax = (vga_im_mode&0xff)|0x80;
    INT86(0x10,&regs);
    SETTLE();
    
    /* get the new mode */
    regs.ax = 0x0f00;
    INT86(0x10,&regs);
    newmode = (regs.ax)&0x7f;
    /* check */
    if(newmode != vga_im_mode){ 
       sprintf(erbuf,"\nSETVGAIM: error setting mode,new=%x, req=%x",
            newmode,vga_im_mode);
        erret(erbuf);
    }
 
    ati_plane = -1;
    grafmode = 2;
    (*m_irefresh)(-1);    /* init the screen */
    return 0;
}


static int 
setvgagr(erase)   /* sets vga graphics mode with INT 10H */
int erase;
{
    R86 regs;
    char erbuf[80];
    int newmode;

    vgaoff();       /* turn off the display */
    /* set the new one */
    if(erase){
        regs.ax = (vga_gr_mode&0xff);
    }else{
        regs.ax = (vga_gr_mode&0xff)|0x80;   /* set, saving gr memory */
    }
    INT86(0x10,&regs);
    SETTLE();

    /* get the new mode */
    regs.ax = 0x0f00;
    INT86(0x10,&regs);
    newmode = (regs.ax)&0x7f;
    /* check */
    if(newmode != vga_gr_mode){ 
       sprintf(erbuf,"\nSETVGAGR: error setting mode,new=%x, req=%x",
            newmode,vga_gr_mode);
        erret(erbuf);
    }
 
    ati_plane = -1;
    grafmode = 1;
#ifndef DSI
    setmapmask(grafcolor);
#endif
    return(0);
}


static void
setvgatxt()   /* sets vga text mode with INT 10H */
{
    R86 regs;
    char erbuf[80];
    int newmode;
    
#if 0
    /* get the current mode */
    regs.ax = 0x0f00;
    INT86(0x10,&regs);
    oldmode = (regs.ax)&0xff;
    oldln = (regs.ax)>>8;
    oldpg = (regs.bx)>>8;
    /*debug scrprintf("\nOLDMODE = 0x%x  OLD#COL = %d OLDPAGE=%d",
            oldmode,oldln,oldpg); */
#endif       

    vgaoff();     
    /* check for manyline screen and reopen in that mode if so */
    if(m_fterm.t_nrows > 25) vgaopen();
    else{                   /* else just reopen in text mode */
        /* set the new one */
        regs.ax = 0x83;   /* set mode 3 saving graphics memory */
        INT86(0x10,&regs);
    }
    SETTLE();
        

    /* get the new mode */
    regs.ax = 0x0f00;
    INT86(0x10,&regs);
    newmode = (regs.ax)&0x7f;
    /* check */
    if(newmode != 3){ 
       sprintf(erbuf,"\nSETVGATXT: error setting mode,new=%x, req=%x",
            newmode,3);
        erret(erbuf);
    }

    ati_plane = -1;
    grafmode = 0;
}

/********************** STDLINE(), ATILINE() *******************************/
/* For ATI high-res modes: returns 0 if no pagebreak in line, 1 if so */


int
stdline(line)
int line;
{
    return 0;
}

int atiline(line)
int line;
{
    int atip;
    
#ifdef DSI
    return 0;
#else    
    
    atip = vpagel[line]&0x0f;
    if(ati_plane != atip){
        ati_plane = atip;
        atipage(ati_plane);
    }
    return (vpagel[line]&0x10 ? 1 : 0);
#endif
}


/***** GENERAL GRAPHICS FUNCTIONS: GMODE(),GERS(), *************************/


/***************** GMODE() *****************/

extern void vgaiers();
void (*iers)() = vgaiers;

int m_ngset = 0;

void
gmode(mode)
int mode;
{
    /*debug*/
    if(m_ngset){
        scrprintf("\nGMODE CALLED, ARG=%d",mode); 
        fflush(stdout);  
        if(mode > 10) m_ngset = 0;
        return;
    }
    
    switch(mode){
    case 0:
        if(grafmode == 0) break;    /* already in text mode */
    case (-1):                      /* fall through */
        setvgatxt();
        res_scr();     /* restore screen */
        break;
    case 1:
        if(grafmode == 1) break;
        if(vga_gr_mode != oldvgagmode || oldgmode != 1) setvgagr(1);
        else{
            setvgagr(1);    
            (*m_grefresh)(0);          /* returning to graphics. refresh scr*/
        }
        oldvgagmode = vga_gr_mode;
        oldgmode = 1;
        break;
#ifndef DSI
    case 2:
        if(grafmode == 2) break;
        setvgaim();
        if(vga_im_mode != oldvgagmode || oldgmode != 2){
            (*iers)();
        }else (*m_irefresh)(0);          /* returning to image. refresh scr */
        oldvgagmode = vga_im_mode;
        oldgmode = 2;
        break;
#endif        
    default:
        setvgatxt();
        scrprintf("\nno such mode as %d",mode);
        erret((char *)NULL);
        break;
    }
}

/**************** GD_LINE() ************************/

Void *
gd_line(yd)  /* returns address of line origin--AS IF bitplane mode;
                in other modes, gd_todis() translates. Hokey. */
int yd;
{ 
    return (Void *)(yd*g_xbsize + dorig);  
}



/**************** RGERS(), GERS() ******************/        

#ifdef DSI /* DSI system--use BIOS */

void rgers()
{ 
    setvgagr(1);
#if 0
    int i,j;
    int x0;
    int gbyte;
    R86 regs;
    int nby = g_xbsize*g_ysize;
    int line;
    
    for(line=0;line<g_ysize;line++){
        for(i=0;i<g_xbsize;i++){    /* for each byte */
            x0 = i*8;
            for(j=0;j<8;j++){  /* for each bit */
                regs.ax = 0xc00;
                regs.bx = 0;      /* data */
                regs.cx = x0+j;
                regs.dx = line;
                INT86(0x10,&regs);
            }
        }
    }        
#endif    
}

void gers()
{
    setvgagr(1);
}


#else

static void 
rgers() /* erases vga graphics screen; memory operation, no registers*/
{
    char *ebuf;
    int EBUFL = (g_xbsize*g_ysize);

    if((ebuf = (char *)malloc(EBUFL)) == NULL) erret("No memory");
    clearn(EBUFL,ebuf);
    vgaoff();
    setmapmask(0xf);                /* all planes */
    MOVE_TO(ebuf,(char *)dorig, EBUFL );
    vgaon();
    free(ebuf);
    setmapmask(grafcolor);
}



void gers()
{
    gmode(1);
    rgers();
}

#endif

/******************  ERS() ************************/

#ifndef DSI

void 
vgaiers()  /* clear screen, image mode  */
{

    int nby; 
    int npage;
    char *erbuf;
    int i;
    int EBUFL;


    if(PIXMODE){
        erbuf = (char *)malloc(0x10000);
        if(!erbuf) erret("\nERS:cannot allocate erase memory");
        clearn(0x10000,erbuf);
        
        nby = (d_xsize*d_ysize)/PIXPERBYTE;
        npage = (nby-1) / 0x10000  + 1;
        vgaoff();
        for(i=0;i<npage;i++){
            if(npage>1){
                atipage(i);
            }
            MOVE_TO(erbuf,(char *)dorig,0x10000);
        }
        vgaon();
        free(erbuf);
        ati_plane = -1;    
    }else{
        EBUFL = (d_xsize*d_ysize)/8;
        if((erbuf = (char *)malloc(EBUFL)) == NULL) erret("No memory");
        clearn(EBUFL,erbuf);
        setmapmask(0xf);                /* all planes */
        vgaoff();
        MOVE_TO(erbuf,(char *)dorig, EBUFL );
        vgaon();
        free(erbuf);
        ati_plane = -1;
    }
}
#endif

/************* GD_TODIS() ********************/


static char g_bbuf[800];


#ifdef DSI

/* BIOS routine-- MOLASSES !!! */
void 
gd_todis(from,to,nby)
Void *from, *to;
normal nby;
{ 
    int line = (to - (u_char *)dorig)/g_xbsize ;
    int xb0 = (to - (u_char *)dorig)%g_xbsize ;   /* starting byte in line */
    int i,j;
    int x0;
    int gbyte;
    R86 regs;

    for(i=0;i<nby;i++){    /* for each byte */
        gbyte = *from++;
        x0 = (xb0+i)*8;
        for(j=0;j<8;j++){  /* for each bit */
            if(gbyte & g_mask[j]){ 
                regs.ax = 0xc00 + (grafcolor & 0xff);
                regs.bx = 0;
                regs.cx = x0+j;
                regs.dx = line;
                INT86(0x10,&regs);
            }
        }
    }
}

#else    /* 386 */


void
gd_todis(from,to,nby)
Void *from, *to;
normal nby;
{ 
    register int n= nby;
    register char *des = g_bbuf;
    register char *src = from;
    
    if(g_revv){
        while(n--) *des++ = ~(*src++);
        MOVE_TO(g_bbuf,to,nby);
    }else{
        MOVE_TO(from,to,nby);
    }
}
#endif



#if 0  /* stuff for one-byte-per pixel graphics--may be useful someday */
    unsigned realaddr;
    unsigned page;
    int nwrite;
    unsigned end;
    register int gbyte,j;
    int i,x0;
    int npt = nby*8;

    if(!GRPIXMODE){
        /* stuff */
    }else{   /* one-byte-per-pix mode */
        if(nby>100){
            putchar(7);
            erret("\nGD_TODIS:line too long");
        }
        realaddr = (to - (u_char *)dorig)<<3;
        page = realaddr/0x10000;
        if(GRPAGES > 1)atipage(page);
        realaddr = realaddr%0x10000;  /* offset in page */
        end = realaddr + npt;
        nwrite = end > 0x10000 ? 0x10000 - realaddr : npt;
        /* prepare the buffer */
        clearn(npt,g_bbuf);
        for(i=0;i<nby;i++){
            gbyte = *from++ ;
            x0 = i<<3;
            for(j=0;j<8;j++){
                if(g_mask[j] & gbyte) g_bbuf[x0+j] = grafcolor;
            }
        }
        MOVE_TO(g_bbuf,realaddr+dorig,nwrite);
        if(nwrite < npt){    /* page break */
            atipage(page+1);
            MOVE_TO(g_bbuf+nwrite,dorig,npt-nwrite);
        }
    }
}

#endif



/************* GEND(), GBEGIN(), GD_INIT() *************/

void 
gend()
{
    ctoscreen();
    while(!key_avail()) continue;
    while(key_avail()) get1char(0);
    gmode(0);
    
}

void gbegin()
{}

void gd_init()
{} 

#endif
