#define X11_C				/* this is x11.c; needed for
					   bug-avoidance in declare.h */
#include "mirella.h"
#include "images.h"

#if defined(X11)
#define OPENWINDOWS_BUG 1

void x11close P_ARGS(( void ));
int x11cursor P_ARGS(( normal *, normal *, int ));
void x11erase P_ARGS(( void ));
void x11erase_graph P_ARGS(( void ));
void x11flush P_ARGS(( void ));
void x11line P_ARGS(( int x1, int y1, int x2, int y2 ));
int x11redraw P_ARGS(( void ));
int x11set_cursor P_ARGS(( int n ));
int x11set_lookup P_ARGS(( u_char red[], u_char green[], u_char blue[], int n_rgb ));
int x11setup P_ARGS(( char *s ));
void x11stretch_lookup P_ARGS(( int bottom, int top ));
int x11zoom P_ARGS(( int x, int y, int zoom ));
void x11_cursor_window P_ARGS(( int flg ));
void x11_toggle_cursor_window P_ARGS(( void ));
void x11_name_window P_ARGS(( int flg ));
void x11_toggle_name_window P_ARGS(( void ));

#if defined(SYS_V)
#  define SYSV				/* needed by X */
#endif
#if defined(SUN)			/* work arounds for X including its 
					   own unistd.h but there being no
					   system one. Grrrr. */
#  undef getcwd
#  undef pause
#endif
#if !defined(_POSIX_SOURCE)
#  undef u_short
#  undef u_char
#endif

#if defined(SOLARIS)
					/* define timeval in sys/time.h */
#  undef _POSIX_SOURCE			/* solaris 2.[12] */
#  undef _POSIX_C_SOURCE		/* solaris 2.3 */
#endif
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>

#if OPENWINDOWS_BUG
#  define XQueryPointer x_query_pointer
static void x_query_pointer();
mirella int fix_openwindows_bug = 0;	/* workaround a bug in XQueryPointer */
#endif 

#define NAME_SIZE 30			/* max length of name of image */
#define NCOL 200			/* max number of colours desired. Note
                                         * that images.h defines a global value
                                         * NCOLOR, which is the max number 
                                         * Mirella EVER needs in any appl. It
                                         * is not clear these should be 
                                         * different */
#define NCURSCHAR 17			/* cursor window width in characters */
#define NVEC 200			/* initial size of xpoint and xvec */
#define PALHEIGHT 15			/* height of palette */

mirella int xpalheight = PALHEIGHT;

#define PROGNAME "Mirella"

static char buff[80];			/* buffer for messages to error() */
static char *next_word P_ARGS ((char * ));
/*
 * These are used to describe our way of dealing with X
 */
static XColor back_color;		/* colour of cursor background */
static int backpix;                     /* pixel value for background */
static int border_width = 2;		/* as it says */
static Colormap cmap;
static XColor colors[NCOL];		/* The colours that we want */
static unsigned int depth;		/* depth of display */
static Display *disp = NULL;
static void DrawPoints();		/* actually draw some points */
static void DrawSegments();		/* actually draw some segments */
static GC erase_gc;			/* Graphics Context for erasing */
static void fill_name();		/* fill in the name window */
static void fill_palette();		/* fill the palette with paint */
static Font font;
static XFontStruct *font_info = NULL;
static XColor fore_color;		/* colour of cursor foreground */
static int forepix;			/* pixel value for foreground */
static GC gc;                           /* GC for cursor position etc. */
static GC graph_gc;			/* GC for graphics */
static GC graph_xor_gc;			/* GC for XOR graphics */
static Pixmap icon_pixmap;		/* pixmap for icon */
static int hand_XError();
static int hand_XIOError();
static int h_font = 0,w_font;		/* height and (mean) width of chars */
static unsigned int ncol;		/* number of colours we got */
static unsigned long *pixels;		/* pixel values */
static Window root;			/* root window */
static void size_x11();
static void to_device_coords();		/* convert from file coordinates */
static void to_buf_coords();		/* convert to file coordinates */
/*
 * These describe the particular image display
 */
static Window curswind;			/* window for cursor output */
static int curswind_is_mapped = 0;	/* well, is it? */
static long event_mask;			/* which events do I want to see? */
static int file_xoff,file_yoff;		/* origin of displayed data in file */
static Window framewind = 0;		/* main Wolf window */
static Cursor graphics_cursor = 0;	/* cursor for cursor */
static Cursor bw_graphics_cursor[3];	/* direct/inverted/off video cursor */
static int hardzoom;			/* `hardware' zoom for display */
static int hard_xorig,hard_yorig;       /* origin of zoomed region */
static unsigned int height = 0;		/* height of image Window wind */
static XImage *image = NULL;		/* the displayed image */
static int left_offset;			/* offset of image origin from left */
static char imagname[NAME_SIZE];	/* name of current image */
static Window namewind = 0;		/* window for name of window */
static int namewind_is_mapped = 0;	/* well, is it? */
static int Ncol = 64;                   /* max number of colors desired in
                                         * a given call to open */
static int npoint;			/* number of points drawn */
static int nvec;                        /* number of vectors drawn */
static Window palwind = 0;		/* window for palette */
static int top_offset;			/* offset of image origin from bottom*/
static XImage *unzoomed_image = NULL;	/* an unzoomed image */
static unsigned int width;		/* width of window */
static Visual *visual;			/* the visual in use */
static Window wind;			/* window for image display */
static int xdispsize,ydispsize;		/* size of the displayed image */
static XPoint *xpoint;			/* points to be drawn */
static XPoint *xpoint_ptr;		/* pointer to next segment */
static XPoint *xpoint_bottom;		/* pointer to first unflushed segment*/
static XPoint *xpoint_top;		/* pointer to last allocated segment */
static XSegment *xvec;			/* vectors to be drawn */
static XSegment *xvec_ptr;		/* pointer to next segment */
static XSegment *xvec_bottom;		/* pointer to first unflushed segment*/
static XSegment *xvec_top;		/* pointer to last allocated segment */

/****************************************************************/
/*
 * Setup (or re-setup) an image display
 */
int
x11setup(s)
char *s;
{
   char *backcolor = "black";		/* name of background colour */
   char *disp_name = NULL;		/* name of display */
   char *cursor_backcolor = NULL;	/* background colour of cursor */
   char *cursor_forecolor = NULL;	/* foreground colour of cursor */
   XEvent event;
   XFontStruct *font_info;
   char *forecolor = "green";		/* name of foreground colour */
                  /* "white" */
   XGCValues gc_values;
   static char gray_bits[32] = {  /* A 16*16 array of bits, in the pattern: */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,   /* 1010101010101010 */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,   /* 0101010101010101 */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,   /* 1010101010101010 */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55};  /* 0101010101010101 */
   XSizeHints hints;                    /* hints for window manager */
   int i;
   static char icon_bitmap[] = {
#     include "icon.h"
   };
   int iconic = 0;			/* start iconised? */
   int icon_position = 0;		/* is icon position specified? */
   int icon_x,icon_y;			/* offset of icon */
   unsigned long mask;                  /* mask for attributes */
   char *name = PROGNAME;		/* name of application */
   int ncol_in_visual;			/* number of colours in the visual */
   static unsigned long pixels_s[256];	/* storage for pixel values */
   unsigned long plane_mask;		/* unused */
   char *ptr;				/* pointer to str */
   int root_height;			/* height of display */
   int root_width;			/* width of display */
   int screen;				/* screen in use */
   int sync = 0;			/* Synchronise Errors */
   char *title = PROGNAME;		/* title of application */
   XSetWindowAttributes win_atts;       /* struct to g/set window attributes */
   XVisualInfo template;		/* what I desire in a visual */
   XVisualInfo *vinfos;			/* list of the desired visual info */
   XWMHints wmhints;			/* More hints for WM */
   int xoff,yoff;			/* position of window */
   int x_position = 0;			/* is window position specified? */
   unsigned int xs = 512,ys = 512;	/* size of Window wind */

   if(framewind == 0) {
      XSetErrorHandler(hand_XError);
      XSetIOErrorHandler(hand_XIOError);

      if(s != NULL) {
	 for(ptr = next_word(s);*ptr != '\0';ptr = next_word((char *)NULL)) {
	    if(*ptr == '-') {
	       switch (*++ptr) {
		case 'b':
		  if(!strcmp(ptr,"bd")) { /* -bd # */
		     ptr = next_word((char *)NULL);
		     if(*ptr == '\0') {
			error("\nYou must specify a width with \"-bd\"");
			break;
		     }
		     border_width = atoi(ptr);
		  } else if(!strcmp(ptr,"bg")) { /* -bg colour */
		     backcolor = next_word((char *)NULL);
		     if(*backcolor == '\0') {
			error("\nYou must specify a colour with \"-bg\"");
			backcolor = "black";
			break;
		     }
		  } else {
		     sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		     error(buff);
		  }
		  break;
		case 'c':
		  if(!strcmp(ptr,"cb")) { /* -cb colour # */
		     ptr = next_word((char *)NULL);
		     if(*ptr == '\0') {
			error("\nYou must specify a colour with \"-cb\"");
			break;
		     }
		     cursor_backcolor = ptr;
		  } else if(!strcmp(ptr,"cf")) { /* -cf colour */
		     cursor_forecolor = next_word((char *)NULL);
		     if(*cursor_forecolor == '\0') {
			error("\nYou must specify a colour with \"-cf\"");
			cursor_forecolor = NULL;
			break;
		     }
		  } else {
		     sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		     error(buff);
		  }
		  break;
		case 'd':		/* -display */
		  if(*(disp_name = next_word((char *)NULL)) == '\0') {
		     error("\nYou must specify a display with \"-d[isplay]\"");
		     break;
		  }
		  break;
		case 'f':
		  if(!strcmp(ptr,"fg")) { /* -fg colour */
		     forecolor = next_word((char *)NULL);
		     if(*forecolor == '\0') {
			error("\nYou must specify a colour with \"-fg\"");
			forecolor = "white";
			break;
		     }
		  } else {
		     sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		     error(buff);
		  }
		  break;
		case 'g':			/* -geometry */
		  if(*(ptr = next_word((char *)NULL)) == '\0') {
		     sprintf(buff,"\n%s",
			   "You need a geometry string with \"-g[eometry]\"");
		     error(buff);
		     break;
		  }
		  
		  mask = XParseGeometry(ptr,&xoff,&yoff,&xs,&ys);
		  if((mask & XValue) && (mask & YValue)) {
		     x_position = 1;
		  }
		  if(xs == 0) xs = 1;
		  if(ys == 0) ys = 1;
		  break;
		case 'h':			/* -help */
		  error(
"\ndevice x11 [#geom] [-bd n] [-bg colour] [-cb colour] [-cf colour]");
		  error(
"\n           [-display name] [-fg colour] [-geometry geom] [-help]");
		  error(
"\n           [-iconic] [-name name] [-synchronise] [-title title]");
		  break;
		case 'i':			/* -iconic */
		  iconic = 1;
		  break;
		case 'n':			/* -name */
		  if(*(name = next_word((char *)NULL)) == '\0') {
		     error("\nYou need a name string with \"-n[ame]\"");
		     break;
		  }
		  break;
		case 's':			/* -synchronise */
		  sync = 1;
		  break;
		case 't':			/* -title */
		  if(*(title = next_word((char *)NULL)) == '\0') {
		     error("\nYou need a title string with \"-t[itle]\"");
		     break;
		  }
		  break;
		default:
		  sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		  error(buff);
		  break;
	       }
	    } else if(*ptr == '#') {
	       unsigned int h,w;	/* not used */
	       
	       mask = XParseGeometry(ptr + 1,&icon_x,&icon_y,&w,&h);
	       if((mask & XValue) && (mask & YValue)) {
		  icon_position = 1;
	       }
	    } else {
	       sprintf(buff,"\nUnknown option \"%s\"",ptr);
	       error(buff);
	    }
	 }
      }
/*
 * Finished parsing string
 */
      if(disp == NULL) {
         if((disp = XOpenDisplay(disp_name)) == NULL) {
	    sprintf(buff,"\nCould not open display \"%s\"",
		    XDisplayName(disp_name));
	    error(buff);
            return(-1);
         }
      }
      screen = DefaultScreen(disp);
      if(sync) {
	 XSynchronize(disp,1);
      }
      depth = DisplayPlanes(disp,screen);
      root = RootWindow(disp,screen);
      root_height = DisplayHeight(disp,screen);
      root_width = DisplayWidth(disp,screen);

      visual = DefaultVisual(disp,screen);
      cmap = DefaultColormap(disp,screen);
/*
 * Look for a pseudo-colour visual (we might be able to use the default)
 */
      template.class = PseudoColor;
      mask = VisualClassMask;

      if((vinfos = XGetVisualInfo(disp,mask,&template,&i)) == NULL) {
	 error("\nCan't find a PseudoColor visual");
	 ncol_in_visual = NCOL;		/* let's guess */
      } else {
	 while(--i >= 0) {
	    if(vinfos[i].colormap_size >= Ncol) {
	       break;
	    }
	 }
	 if(i < 0) {			/* couldn't find one large enough */
	    error("\nCan't find a large enough PseudoColor visual");
	    i = 0;
	 }
	 visual = vinfos[i].visual;
	 ncol_in_visual = vinfos[i].colormap_size;
	 XFree((char *)vinfos);
      }
/*
 * Allocate Colourmap (it'll be filled by a call to x11set_lookup)
 *
 * Also try to get the desired back/foreground, but don't grab r/w cells
 * for them * if we are using the default colour map. While we are about
 * it, try to get colours for cursors if so requested
 */
      ncol = Ncol;			/* try the default cmap first */
      pixels = pixels_s;
      if(XAllocColorCells(disp,cmap,False,&plane_mask,0,pixels,ncol)) {
	 if(XParseColor(disp,cmap,backcolor,&back_color) &&
	    XAllocColor(disp,cmap,&back_color)) {
	    backpix = back_color.pixel;
	 } else {
	    backpix = BlackPixel(disp,screen);
	 }
	 if(XParseColor(disp,cmap,forecolor,&fore_color) &&
	    XAllocColor(disp,cmap,&fore_color)) {
	    forepix = fore_color.pixel;
	 } else {
	    forepix = WhitePixel(disp,screen);
	 }
	 
	 if(cursor_backcolor != NULL) {
	    if(!XParseColor(disp,cmap,cursor_backcolor,&back_color) ||
	       !XAllocColor(disp,cmap,&back_color)) {
	       fprintf(stderr,"Can't get cursor background: %s\n",
		       cursor_backcolor);
	    }
	 }
	 if(cursor_forecolor != NULL) {
	    if(!XParseColor(disp,cmap,cursor_forecolor,&fore_color) ||
	       !XAllocColor(disp,cmap,&fore_color)) {
	       fprintf(stderr,"Can't get cursor foreground: %s\n",
		       cursor_forecolor);
	    }
	 }
      } else {
	 fprintf(stderr,
		"Can't allocate %d colours: creating a new colour map\n",ncol);
	 if((cmap = XCreateColormap(disp,root,visual,AllocNone)) == 0) {
	    fprintf(stderr,"Can't allocate PsuedoColor colour map\n");
	    cmap = DefaultColormap(disp,screen);
	 }

	 if(ncol_in_visual > sizeof(pixels_s)/sizeof(pixels_s[0])) {
	    ncol_in_visual = sizeof(pixels_s)/sizeof(pixels_s[0]);
	 }
	 ncol = ncol_in_visual;		/* now we want them all so as to
					   try to preserve the lower ones */
	 for(;ncol > 0;ncol--) {
	    if(XAllocColorCells(disp,cmap,False,&plane_mask,0,pixels,ncol)
									!= 0) {
	       break;			/* success */
	    }
	 }
	 if(ncol == 0) {
	    fprintf(stderr,"Can't allocate colourmap\n");
	    return(-1);
	 } else if(ncol < Ncol + 4) {
	    fprintf(stderr,"I can only allocate %d colours; sorry\n",ncol);
	    XFreeColors(disp,cmap,pixels,ncol,plane_mask);
	    XFreeColormap(disp,cmap);	      
	    return(-1);
	 }
	 
	 for(i = 0;i < ncol - Ncol;i++) { /* copy some of default cmap */
	    colors[i].pixel = i;
	 }
	 i = ncol - Ncol;
	 if(i > Ncol) i = Ncol;
	 XQueryColors(disp,DefaultColormap(disp,screen),colors,i);
	 XStoreColors(disp,cmap,colors,i);
	 
	 pixels += ncol - Ncol;
	 ncol = Ncol;

	 if(XParseColor(disp,cmap,backcolor,&back_color)) {
	    backpix = back_color.pixel = pixels[-1];
	    XStoreColor(disp,cmap,&back_color);
	 } else {
	    backpix = BlackPixel(disp,screen);
	 }
	 if(XParseColor(disp,cmap,forecolor,&fore_color)) {
	    forepix = fore_color.pixel = pixels[-2];
	    XStoreColor(disp,cmap,&fore_color);
	 } else {
	    forepix = WhitePixel(disp,screen);
	 }
	 
	 if(cursor_backcolor != NULL) {
	    if(XParseColor(disp,cmap,cursor_backcolor,&back_color)) {
	       back_color.pixel = pixels[-3];
	       XStoreColor(disp,cmap,&back_color);
	    } else {
	       fprintf(stderr,"Can't get cursor background: %s\n",
		       cursor_backcolor);
	    }
	 }
	 if(cursor_forecolor != NULL) {
	    if(XParseColor(disp,cmap,cursor_forecolor,&fore_color)) {
	       fore_color.pixel = pixels[-4];
	       XStoreColor(disp,cmap,&fore_color);
	    } else {
	       fprintf(stderr,"Can't get cursor foreground: %s\n",
		       cursor_forecolor);
	    }
	 }
      }
/*
 * Create fonts and GC's and such
 */
      if((font_info = XLoadQueryFont(disp,"8x13")) != NULL) {
         h_font = font_info->max_bounds.ascent + font_info->max_bounds.descent;
         w_font = font_info->max_bounds.width;
         font = font_info->fid;
      } else {
	 fprintf(stderr,"Can't open font\n");
	 w_font = 8;			/* we don't want these to be zero */
	 h_font = 13;
      }

      mask = 0;
      gc_values.fill_style = FillSolid;
      mask |= GCFillStyle;
      gc_values.foreground = forepix;
      mask |= GCForeground;
      gc_values.background = backpix;
      mask |= GCBackground;
      gc_values.font = font;
      mask |= GCFont;

      gc = XCreateGC(disp,root,mask,&gc_values);
      gc_values.foreground = backpix;
      erase_gc = XCreateGC(disp,root,mask,&gc_values);
      gc_values.foreground = fore_color.pixel;
      gc_values.background = back_color.pixel;
      graph_gc = XCreateGC(disp,root,mask,&gc_values);
      gc_values.foreground = 255;
      gc_values.function = GXxor;
      mask |= GCFunction;
      graph_xor_gc = XCreateGC(disp,root,mask,&gc_values);
/*
 * Set colourmap to gray
 */
      for(i = 0;i < ncol;i++) {
	 colors[i].pixel = pixels[i];
	 colors[i].flags = DoRed | DoGreen | DoBlue;
	 colors[i].red = colors[i].green = colors[i].blue = i*65535/(ncol - 1);
      }
      XStoreColors(disp,cmap,colors,ncol);

      x11set_cursor(0);

      icon_pixmap = XCreateBitmapFromData(disp,root,icon_bitmap,
					  WOLF_ICON_XSIZE,WOLF_ICON_YSIZE);

      mask = 0;
      win_atts.background_pixel = backpix;
      mask |= CWBackPixel;
      win_atts.border_pixmap = XCreatePixmapFromBitmapData(disp,
                	root,gray_bits,16,16,BlackPixel(disp,screen),
					WhitePixel(disp,screen),depth);
      mask |= CWBorderPixmap;
      win_atts.backing_store = Always;
      mask |= CWBackingStore;
      win_atts.bit_gravity = CenterGravity;
      mask |= CWBitGravity;
      win_atts.colormap = cmap;
      mask |= CWColormap;
/*
 * Finally create the window!
 */
      hints.flags = 0;
      hints.width = xs;
      hints.height = ys + PALHEIGHT;
      hints.flags |= PSize;
      if(x_position) {
	 if(xoff < 0) {
	    xoff += root_width - 1 - hints.width - 2*border_width;
	 }
	 if(yoff < 0) {
	    yoff += root_height - 1 - hints.height - 2*border_width;
	 }
	 hints.x = xoff;
	 hints.y = yoff;
	 hints.flags |= USPosition;
      } else {
	 hints.x = hints.y = 100;
      }
      hints.min_width = 100;
      hints.min_height = hints.min_width + PALHEIGHT;
      hints.flags |= PMinSize;

      if(framewind == 0 &&
          (framewind = XCreateWindow(disp,root,hints.x,hints.y,
		     hints.width,hints.height,border_width,depth,
                        InputOutput,visual,mask,&win_atts)) == 0) {
	 sprintf(buff,"\nCan't open disp %s",XDisplayName(s));
	 error(buff);
         return(-1);
      }
/*
 * and proceed with setting it up
 */
      XSetStandardProperties(disp,framewind,name,name,None,
			                             (char **)NULL,0,&hints);

      wmhints.flags = 0;
      wmhints.initial_state = iconic ? IconicState : NormalState;
      wmhints.flags |= StateHint;
      wmhints.icon_pixmap = icon_pixmap;
      wmhints.flags |= IconPixmapHint;
      wmhints.input = True;
      wmhints.flags |= InputHint;
      if(icon_position) {
	 if(icon_x < 0) {
	    icon_x += root_width - 1 - WOLF_ICON_XSIZE - 2*2;
	    				/* guess that icon border width is 2 */
	 }
	 if(icon_y < 0) {
	    icon_y += root_height - 1 - WOLF_ICON_YSIZE - 2*2;
	 }
	 wmhints.icon_x = icon_x;
	 wmhints.icon_y = icon_y;
	 wmhints.flags |= IconPositionHint;
      }
      XSetWMHints(disp,framewind,&wmhints);

      XStoreName(disp,framewind,title);
      event_mask = ExposureMask | StructureNotifyMask;
      XSelectInput(disp,framewind,event_mask);
/*
 * Now we have the window put it up and do some book-keeping
 */
      XMapWindow(disp,framewind);
      size_x11();
/*
 * Now set up other windows, positioned relative to framewind
 *
 * wind is at the top of framewind
 * palwind is at the bottom.
 * curswind occupies the top left corner of wind,
 * namewind occupies the top right corner.
 */
      mask = CWBackPixel | CWBorderPixmap | CWBackingStore | CWBitGravity;

      if((wind = XCreateWindow(disp,framewind,-border_width,-border_width,
		       width,height,border_width,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	 fprintf(stderr,"Can't open image window\n");
      }
      if(PALHEIGHT && (palwind = XCreateWindow(disp,framewind,
				  0,border_width + height,
				  width,PALHEIGHT,0,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	 fprintf(stderr,"Can't open palette window\n");
      }

      if(h_font > 0) {			/* we got the font OK */
	 if((namewind = XCreateWindow(disp,wind,width - 1 - border_width,
			      -border_width,1,h_font,border_width,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	    fprintf(stderr,"Can't create name window\n");
	 }
	 if((curswind = XCreateWindow(disp,wind,-border_width,-border_width,
	      		NCURSCHAR*w_font,h_font,border_width,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	    fprintf(stderr,"Can't open cursor window\n");
	 }
      }
      XFreePixmap(disp,win_atts.border_pixmap);
/*
 * create display list for points and vectors
 */
      if((xpoint = (XPoint *)malloc(NVEC*sizeof(XPoint))) == NULL) {
         error("\nCan't allocate points in x11setup");
         return(-1);
      }
      npoint = NVEC;
      xpoint_bottom = xpoint_ptr = xpoint;
      xpoint_top = xpoint + npoint - 1;

      if((xvec = (XSegment *)malloc(NVEC*sizeof(XSegment))) == NULL) {
         error("\nCan't allocate vectors in x11setup");
         return(-1);
      }
      nvec = NVEC;
      xvec_bottom = xvec_ptr = xvec;
      xvec_top = xvec + nvec - 1;
/*
 * Put up window for the first time
 */
      XMapWindow(disp,wind);
      if(palwind) XMapWindow(disp,palwind);
      if(curswind && curswind_is_mapped) XMapWindow(disp,curswind);
      if(!iconic) {
	 XMaskEvent(disp,StructureNotifyMask,&event); /* watch creation */
	 XPutBackEvent(disp,&event);		       /* and reschedule it */
      }
      hardzoom = 1;
   }
   size_x11();
   height--;				/* force a redraw */
   x11redraw();
   XRaiseWindow(disp,framewind);

   return(0);
}

/****************************************************************/
/* play with this function for graphics window */

int
x11gsetup(s)
char *s;
{
   char *backcolor = "black";		/* name of background colour */
   char *disp_name = NULL;		/* name of display */
   char *cursor_backcolor = NULL;	/* background colour of cursor */
   char *cursor_forecolor = NULL;	/* foreground colour of cursor */
   XEvent event;
   XFontStruct *font_info;
   char *forecolor = "white";		/* name of foreground colour */
   XGCValues gc_values;
   static char gray_bits[32] = {  /* A 16*16 array of bits, in the pattern: */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,   /* 1010101010101010 */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,   /* 0101010101010101 */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,   /* 1010101010101010 */
      0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55};  /* 0101010101010101 */
   XSizeHints hints;                    /* hints for window manager */
   int i;
   static char icon_bitmap[] = {
#     include "icon.h"
   };
   int iconic = 0;			/* start iconised? */
   int icon_position = 0;		/* is icon position specified? */
   int icon_x,icon_y;			/* offset of icon */
   unsigned long mask;                  /* mask for attributes */
   char *name = PROGNAME;		/* name of application */
   int ncol_in_visual;			/* number of colours in the visual */
   static unsigned long pixels_s[256];	/* storage for pixel values */
   unsigned long plane_mask;		/* unused */
   char *ptr;				/* pointer to str */
   int root_height;			/* height of display */
   int root_width;			/* width of display */
   int screen;				/* screen in use */
   int sync = 0;			/* Synchronise Errors */
   char *title = PROGNAME;		/* title of application */
   XSetWindowAttributes win_atts;       /* struct to g/set window attributes */
   XVisualInfo template;		/* what I desire in a visual */
   XVisualInfo *vinfos;			/* list of the desired visual info */
   XWMHints wmhints;			/* More hints for WM */
   int xoff,yoff;			/* position of window */
   int x_position = 0;			/* is window position specified? */
   unsigned int xs = 512,ys = 512;	/* size of Window wind */

   if(framewind == 0) {
      XSetErrorHandler(hand_XError);
      XSetIOErrorHandler(hand_XIOError);

/*
 * parse X string
 */
 
      if(s != NULL) {
	 for(ptr = next_word(s);*ptr != '\0';ptr = next_word((char *)NULL)) {
	    if(*ptr == '-') {
	       switch (*++ptr) {
		case 'b':
		  if(!strcmp(ptr,"bd")) { /* -bd # */
		     ptr = next_word((char *)NULL);
		     if(*ptr == '\0') {
			error("\nYou must specify a width with \"-bd\"");
			break;
		     }
		     border_width = atoi(ptr);
		  } else if(!strcmp(ptr,"bg")) { /* -bg colour */
		     backcolor = next_word((char *)NULL);
		     if(*backcolor == '\0') {
			error("\nYou must specify a colour with \"-bg\"");
			backcolor = "black";
			break;
		     }
		  } else {
		     sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		     error(buff);
		  }
		  break;
		case 'c':
		  if(!strcmp(ptr,"cb")) { /* -cb colour # */
		     ptr = next_word((char *)NULL);
		     if(*ptr == '\0') {
			error("\nYou must specify a colour with \"-cb\"");
			break;
		     }
		     cursor_backcolor = ptr;
		  } else if(!strcmp(ptr,"cf")) { /* -cf colour */
		     cursor_forecolor = next_word((char *)NULL);
		     if(*cursor_forecolor == '\0') {
			error("\nYou must specify a colour with \"-cf\"");
			cursor_forecolor = NULL;
			break;
		     }
		  } else {
		     sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		     error(buff);
		  }
		  break;
		case 'd':		/* -display */
		  if(*(disp_name = next_word((char *)NULL)) == '\0') {
		     error("\nYou must specify a display with \"-d[isplay]\"");
		     break;
		  }
		  break;
		case 'f':
		  if(!strcmp(ptr,"fg")) { /* -fg colour */
		     forecolor = next_word((char *)NULL);
		     if(*forecolor == '\0') {
			error("\nYou must specify a colour with \"-fg\"");
			forecolor = "white";
			break;
		     }
		  } else {
		     sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		     error(buff);
		  }
		  break;
		case 'g':			/* -geometry */
		  if(*(ptr = next_word((char *)NULL)) == '\0') {
		     sprintf(buff,"\n%s",
			   "You need a geometry string with \"-g[eometry]\"");
		     error(buff);
		     break;
		  }
		  
		  mask = XParseGeometry(ptr,&xoff,&yoff,&xs,&ys);
		  if((mask & XValue) && (mask & YValue)) {
		     x_position = 1;
		  }
		  if(xs == 0) xs = 1;
		  if(ys == 0) ys = 1;
		  break;
		case 'h':			/* -help */
		  error(
"\ndevice x11 [#geom] [-bd n] [-bg colour] [-cb colour] [-cf colour]");
		  error(
"\n           [-display name] [-fg colour] [-geometry geom] [-help]");
		  error(
"\n           [-iconic] [-name name] [-synchronise] [-title title]");
		  break;
		case 'i':			/* -iconic */
		  iconic = 1;
		  break;
		case 'n':			/* -name */
		  if(*(name = next_word((char *)NULL)) == '\0') {
		     error("\nYou need a name string with \"-n[ame]\"");
		     break;
		  }
		  break;
		case 's':			/* -synchronise */
		  sync = 1;
		  break;
		case 't':			/* -title */
		  if(*(title = next_word((char *)NULL)) == '\0') {
		     error("\nYou need a title string with \"-t[itle]\"");
		     break;
		  }
		  break;
		default:
		  sprintf(buff,"\nUnknown option \"%s\"",ptr - 1);
		  error(buff);
		  break;
	       }
	    } else if(*ptr == '#') {
	       unsigned int h,w;	/* not used */
	       
	       mask = XParseGeometry(ptr + 1,&icon_x,&icon_y,&w,&h);
	       if((mask & XValue) && (mask & YValue)) {
		  icon_position = 1;
	       }
	    } else {
	       sprintf(buff,"\nUnknown option \"%s\"",ptr);
	       error(buff);
	    }
	 }
      }
/*
 * Finished parsing string
 */
      if(disp == NULL) {
         if((disp = XOpenDisplay(disp_name)) == NULL) {
	    sprintf(buff,"\nCould not open display \"%s\"",
		    XDisplayName(disp_name));
	    error(buff);
            return(-1);
         }
      }
      screen = DefaultScreen(disp);
      if(sync) {
	 XSynchronize(disp,1);
      }
      depth = DisplayPlanes(disp,screen);
      root = RootWindow(disp,screen);
      root_height = DisplayHeight(disp,screen);
      root_width = DisplayWidth(disp,screen);

      visual = DefaultVisual(disp,screen);
      cmap = DefaultColormap(disp,screen);
/*
 * Look for a pseudo-colour visual (we might be able to use the default)
 */
      template.class = PseudoColor;
      mask = VisualClassMask;

      if((vinfos = XGetVisualInfo(disp,mask,&template,&i)) == NULL) {
	 error("\nCan't find a PseudoColor visual");
	 ncol_in_visual = NCOL;		/* let's guess */
      } else {
         /* see if we have one big enough */
	 while(--i >= 0) {
	    if(vinfos[i].colormap_size >= Ncol) {
	       break;
	    }
	 }
	 if(i < 0) {			/* couldn't find one large enough */
	    error("\nCan't find a large enough PseudoColor visual");
	    i = 0;
	 }
	 visual = vinfos[i].visual;  /* got one */
	 ncol_in_visual = vinfos[i].colormap_size;
	 XFree((char *)vinfos);
      }
     
/*
 * Allocate Colourmap (it'll be filled by a call to x11set_lookup)
 *
 * Also try to get the desired back/foreground, but don't grab r/w cells
 * for them * if we are using the default colour map. While we are about
 * it, try to get colours for cursors if so requested
 */
      ncol = Ncol;			/* try the default cmap first */
      pixels = pixels_s;
      if(XAllocColorCells(disp,cmap,False,&plane_mask,0,pixels,ncol)) {
	 if(XParseColor(disp,cmap,backcolor,&back_color) &&
	    XAllocColor(disp,cmap,&back_color)) {
	    backpix = back_color.pixel;
	 } else {
	    backpix = BlackPixel(disp,screen);
	 }
	 if(XParseColor(disp,cmap,forecolor,&fore_color) &&
	    XAllocColor(disp,cmap,&fore_color)) {
	    forepix = fore_color.pixel;
	 } else {
	    forepix = WhitePixel(disp,screen);
	 }
	 
	 if(cursor_backcolor != NULL) {
	    if(!XParseColor(disp,cmap,cursor_backcolor,&back_color) ||
	       !XAllocColor(disp,cmap,&back_color)) {
	       fprintf(stderr,"Can't get cursor background: %s\n",
		       cursor_backcolor);
	    }
	 }
	 if(cursor_forecolor != NULL) {
	    if(!XParseColor(disp,cmap,cursor_forecolor,&fore_color) ||
	       !XAllocColor(disp,cmap,&fore_color)) {
	       fprintf(stderr,"Can't get cursor foreground: %s\n",
		       cursor_forecolor);
	    }
	 }
      } else {
	 fprintf(stderr,
		"Can't allocate %d colours: creating a new colour map\n",ncol);
	 if((cmap = XCreateColormap(disp,root,visual,AllocNone)) == 0) {
	    fprintf(stderr,"Can't allocate PsuedoColor colour map\n");
	    cmap = DefaultColormap(disp,screen);
	 }
         /* cmap is X pointer to ncw color map if successful */
         
         /* just defensive */
	 if(ncol_in_visual > sizeof(pixels_s)/sizeof(pixels_s[0])) {
	    ncol_in_visual = sizeof(pixels_s)/sizeof(pixels_s[0]);
	 }
	 
	 ncol = ncol_in_visual;		/* now we want them all so as to
					   try to preserve the lower ones */
	 for(;ncol > 0;ncol--) {
	    if(XAllocColorCells(disp,cmap,False,&plane_mask,0,pixels,ncol)
	       != 0) {
	       break;			/* success */
	    }
	 }

/*DB*/ mprintf("\nXAllocColorCells returns ncol = %d at first success\n",ncol);
	
	 /* what the hell does this accomplish??? Should it not
	  * always return ncol_in_visual???  
	  */
	 
	 if(ncol == 0) {
	    fprintf(stderr,"Can't allocate colourmap\n");
	    return(-1);
	 } else if(ncol < Ncol + 4) {
	    fprintf(stderr,"I can only allocate %d colours; sorry\n",ncol);
	    XFreeColors(disp,cmap,pixels,ncol,plane_mask);
	    XFreeColormap(disp,cmap);	      
	    return(-1);
	 }
	 
	 /* at this point, ncol is the number of colors we have; it may
	  * be between Ncol+4 and NCOL. If there are extra ones, we
	  * want to preserve the low original ones if possible.
	  */
	 
	 for(i = 0;i < ncol - Ncol;i++) { /* copy some of default cmap */
	    colors[i].pixel = i;  /* just the index of the pixel??  Why does
	                           * this work?? */
	 }
	 i = ncol - Ncol;   /* Ncol + 4? */
	 if(i > Ncol) i = Ncol;
	 XQueryColors(disp,DefaultColormap(disp,screen),colors,i);
	 XStoreColors(disp,cmap,colors,i);
	 
	 pixels += ncol - Ncol;  /* Ncol+4  ? */
	 ncol = Ncol;
	 
/*
 * set for/bkgnd for display and cursor--don't think I need 
 */
 
	 if(XParseColor(disp,cmap,backcolor,&back_color)) {
	    backpix = back_color.pixel = pixels[-1];
	    XStoreColor(disp,cmap,&back_color);
	 } else {
	    backpix = BlackPixel(disp,screen);
	 }
	 if(XParseColor(disp,cmap,forecolor,&fore_color)) {
	    forepix = fore_color.pixel = pixels[-2];
	    XStoreColor(disp,cmap,&fore_color);
	 } else {
	    forepix = WhitePixel(disp,screen);
	 }
	 
	 if(cursor_backcolor != NULL) {
	    if(XParseColor(disp,cmap,cursor_backcolor,&back_color)) {
	       back_color.pixel = pixels[-3];
	       XStoreColor(disp,cmap,&back_color);
	    } else {
	       fprintf(stderr,"Can't get cursor background: %s\n",
		       cursor_backcolor);
	    }
	 }
	 if(cursor_forecolor != NULL) {
	    if(XParseColor(disp,cmap,cursor_forecolor,&fore_color)) {
	       fore_color.pixel = pixels[-4];
	       XStoreColor(disp,cmap,&fore_color);
	    } else {
	       fprintf(stderr,"Can't get cursor foreground: %s\n",
		       cursor_forecolor);
	    }
	 }
      }

/*
 * Create fonts and GC's and such
 */
      if((font_info = XLoadQueryFont(disp,"8x13")) != NULL) {
         h_font = font_info->max_bounds.ascent + font_info->max_bounds.descent;
         w_font = font_info->max_bounds.width;
         font = font_info->fid;
      } else {
	 fprintf(stderr,"Can't open font\n");
	 w_font = 8;			/* we don't want these to be zero */
	 h_font = 13;
      }

      mask = 0;
      gc_values.fill_style = FillSolid;
      mask |= GCFillStyle;
      gc_values.foreground = forepix;
      mask |= GCForeground;
      gc_values.background = backpix;
      mask |= GCBackground;
      gc_values.font = font;
      mask |= GCFont;

      gc = XCreateGC(disp,root,mask,&gc_values);
      gc_values.foreground = backpix;
      erase_gc = XCreateGC(disp,root,mask,&gc_values);
      gc_values.foreground = fore_color.pixel;
      gc_values.background = back_color.pixel;
      graph_gc = XCreateGC(disp,root,mask,&gc_values);
      gc_values.foreground = 255;
      gc_values.function = GXxor;
      mask |= GCFunction;
      graph_xor_gc = XCreateGC(disp,root,mask,&gc_values);
#if 0
/*
 * Set colourmap to gray. This needs to be fixed.
 */
      /* ncol is Ncol at this point, probably. Now set the colors 
       * structure array with ncol grey values equally spaced between
       * 0 and 65535 (?) 
       */
      for(i = 0;i < ncol;i++) {
	 colors[i].pixel = pixels[i];
	 colors[i].flags = DoRed | DoGreen | DoBlue;
	 colors[i].red = colors[i].green = colors[i].blue = i*65535/(ncol - 1);
      }
      XStoreColors(disp,cmap,colors,ncol);

      x11set_cursor(0);
#endif




      icon_pixmap = XCreateBitmapFromData(disp,root,icon_bitmap,
					  WOLF_ICON_XSIZE,WOLF_ICON_YSIZE);

      mask = 0;
      win_atts.background_pixel = backpix;
      mask |= CWBackPixel;
      win_atts.border_pixmap = XCreatePixmapFromBitmapData(disp,
                	root,gray_bits,16,16,BlackPixel(disp,screen),
					WhitePixel(disp,screen),depth);
      mask |= CWBorderPixmap;
      win_atts.backing_store = Always;
      mask |= CWBackingStore;
      win_atts.bit_gravity = CenterGravity;
      mask |= CWBitGravity;
      win_atts.colormap = cmap;
      mask |= CWColormap;
/*
 * Finally create the window!
 */
      hints.flags = 0;
      hints.width = xs;
      hints.height = ys + PALHEIGHT;  /* fix */
      hints.flags |= PSize;
      if(x_position) {
	 if(xoff < 0) {
	    xoff += root_width - 1 - hints.width - 2*border_width;
	 }
	 if(yoff < 0) {
	    yoff += root_height - 1 - hints.height - 2*border_width;
	 }
	 hints.x = xoff;
	 hints.y = yoff;
	 hints.flags |= USPosition;
      } else {
	 hints.x = hints.y = 100;
      }
      hints.min_width = 100;
      hints.min_height = hints.min_width + PALHEIGHT; /* fix */
      hints.flags |= PMinSize;

      if(framewind == 0 &&
          (framewind = XCreateWindow(disp,root,hints.x,hints.y,
		     hints.width,hints.height,border_width,depth,
                        InputOutput,visual,mask,&win_atts)) == 0) {
	 sprintf(buff,"\nCan't open disp %s",XDisplayName(s));
	 error(buff);
         return(-1);
      }

/*
 * and proceed with setting it up
 */
      XSetStandardProperties(disp,framewind,name,name,None,
			                             (char **)NULL,0,&hints);

      wmhints.flags = 0;
      wmhints.initial_state = iconic ? IconicState : NormalState;
      wmhints.flags |= StateHint;
      wmhints.icon_pixmap = icon_pixmap;
      wmhints.flags |= IconPixmapHint;
      wmhints.input = True;
      wmhints.flags |= InputHint;
      if(icon_position) {
	 if(icon_x < 0) {
	    icon_x += root_width - 1 - WOLF_ICON_XSIZE - 2*2;
	    				/* guess that icon border width is 2 */
	 }
	 if(icon_y < 0) {
	    icon_y += root_height - 1 - WOLF_ICON_YSIZE - 2*2;
	 }
	 wmhints.icon_x = icon_x;
	 wmhints.icon_y = icon_y;
	 wmhints.flags |= IconPositionHint;
      }
      XSetWMHints(disp,framewind,&wmhints);

      XStoreName(disp,framewind,title);
      event_mask = ExposureMask | StructureNotifyMask;
      XSelectInput(disp,framewind,event_mask);

/*DB ok to here */

/*
 * Now we have the window put it up and do some book-keeping
 */
      XMapWindow(disp,framewind);
      size_x11();

/* XMapWindow  barfs if ncol is too small. Why??? */
/*DB mprintf("\n\nNcol,ncol=%d %d",Ncol,ncol); erret("\nleaving now\n"); */

/*
 * Now set up other windows, positioned relative to framewind
 *
 * wind is at the top of framewind
 * palwind is at the bottom.
 * curswind occupies the top left corner of wind,
 * namewind occupies the top right corner.
 */
      mask = CWBackPixel | CWBorderPixmap | CWBackingStore | CWBitGravity;

      if((wind = XCreateWindow(disp,framewind,-border_width,-border_width,
		       width,height,border_width,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	 fprintf(stderr,"Can't open image window\n");
      }

      if((palwind = XCreateWindow(disp,framewind,
				  0,border_width + height,
				  width,PALHEIGHT,0,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	 fprintf(stderr,"Can't open palette window\n");
      }

      if(h_font > 0) {			/* we got the font OK */
	 if((namewind = XCreateWindow(disp,wind,width - 1 - border_width,
			      -border_width,1,h_font,border_width,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	    fprintf(stderr,"Can't create name window\n");
	 }
	 if((curswind = XCreateWindow(disp,wind,-border_width,-border_width,
	      		NCURSCHAR*w_font,h_font,border_width,depth,
			    InputOutput,visual,mask,&win_atts)) == 0) {
	    fprintf(stderr,"Can't open cursor window\n");
	 }
      }
      XFreePixmap(disp,win_atts.border_pixmap);
/*
 * create display list for points and vectors
 */
      if((xpoint = (XPoint *)malloc(NVEC*sizeof(XPoint))) == NULL) {
         error("\nCan't allocate points in x11setup");
         return(-1);
      }
      npoint = NVEC;
      xpoint_bottom = xpoint_ptr = xpoint;
      xpoint_top = xpoint + npoint - 1;

      if((xvec = (XSegment *)malloc(NVEC*sizeof(XSegment))) == NULL) {
         error("\nCan't allocate vectors in x11setup");
         return(-1);
      }
      nvec = NVEC;
      xvec_bottom = xvec_ptr = xvec;
      xvec_top = xvec + nvec - 1;
/*
 * Put up window for the first time
 */
      XMapWindow(disp,wind);
      XMapWindow(disp,palwind);
      if(curswind && curswind_is_mapped) XMapWindow(disp,curswind);
      if(!iconic) {
	 XMaskEvent(disp,StructureNotifyMask,&event); /* watch creation */
	 XPutBackEvent(disp,&event);		       /* and reschedule it */
      }
      hardzoom = 1;
   }
   size_x11();
   height--;				/* force a redraw */
   x11redraw();
   XRaiseWindow(disp,framewind);

   return(0);
}

 
/****************************************************************/

void
x11close()
{
   if(framewind != 0) {
      x11flush();

      puts("\nXFreeColors(disp,cmap,(unsigned long *)&backpix,1,0);");      
      XFreeColors(disp,cmap,(unsigned long *)&backpix,1,0);
      fflush(stderr); fflush(stdout);
      puts("\nXFreeColors(disp,cmap,(unsigned long *)&forepix,1,0);");      
      XFreeColors(disp,cmap,(unsigned long *)&forepix,1,0);
      fflush(stderr); fflush(stdout);
      puts("\nAll rest is OK"); fflush(stdout);
      if(font_info != NULL) {
	 XFreeFont(disp,font_info);
	 font_info = NULL;
      }
      /* puts("XFreeGC(disp,ETC);"); */
      XFreeGC(disp,gc);
      XFreeGC(disp,erase_gc);
      XFreeGC(disp,graph_gc);
      /* puts("XFreeColors(disp,cmap,pixels,ncol,0);"); */
      XFreeColors(disp,cmap,pixels,ncol,0);
      /* puts("XFreePixmap(disp,icon_pixmap);"); */
      XFreePixmap(disp,icon_pixmap);
      free((char *)xpoint);
      npoint = 0;
      free((char *)xvec);
      nvec = 0;
      if(graphics_cursor > 0) {
      /* puts("XFreeCursor(disp,bw_graphics_cursor[0,1,2]);"); */
	 XFreeCursor(disp,bw_graphics_cursor[0]);
	 XFreeCursor(disp,bw_graphics_cursor[1]);
	 XFreeCursor(disp,bw_graphics_cursor[2]);
	 graphics_cursor = 0;
      }
      if(image != NULL) {
      /* puts("XDestroyImage(image);"); */
	 XDestroyImage(image);
	 image = NULL;
      }
      if(unzoomed_image != NULL) {
      /* puts("XDestroyImage(unzoomed_image);"); */
	 XDestroyImage(unzoomed_image);
	 unzoomed_image = NULL;
      }

      /* puts("XCloseDisplay(disp);"); */
      XCloseDisplay(disp);
      disp = NULL;
      framewind = 0;
      d_xsize = 0;
      d_ysize = 0;
   }
   return;
}

/****************************************************************/
/*
 * Deal with a cursor.
 */
static int blinkflg = 0;		/* flag to blink */
#if !defined(BLINKTIME)
#  define BLINKTIME 500000		/* steady time in usec for cursor
					   (No blinking if == 0) */
#endif

/* 
 * define blink characteristic; 1 blinks between fgcolor and black; 2 blinks
 * between fgcolor and none.
 */
 
mirella int curs_alt = 2;


int
x11cursor(x,y,one_key)
normal *x,*y;				/* cursor position */
int one_key;				/* return on first key seen? */
{
#if BLINKTIME
   fd_set mask;				/* mask of descriptors to check */
   fd_set readmask;			/* the desired mask */
   struct timeval start,timeout;	/* used in timing blinking */
   int xfd;				/* x11's file descriptor */
#endif
   char buff[5];
   unsigned int button_status;		/* not used */
   int c;
   Window child_wind;			/* not used */
   int c_xs_old,c_ys_old;		/* cx,cy before do_cursor_key() */
   int c_xi = 0, c_yi = 0;              /* cx,cy at beginning of loopy loop */
   static normal cx = -1,cy;		/* last position of cursor in wind */
   XEvent event;
   int value_flag;			/* cursor is in an image */
   int kx,ky;				/* cursor position in keyboard wind */
   static int level = 0;		/* depth of recursive calls */
   int loopy = 1;			/* should I keep looping? */
   Window root_wind;			/* not used */
   int rx,ry;				/* cursor position in root wind */
   int firstlb=1;                       /* lets joe click on window to raise */
   

   if(level >= 10) {
      fprintf(stderr,"\nCursor: Too many recursive calls (%d)",level);
      return(-1);
   }
   if(level++ == 0) {
      unsigned long mask = 0;		/* event mask */
      
      XQueryPointer(disp,root,&root_wind,&child_wind,&rx,&ry,
		    &kx,&ky,&button_status);
      XDefineCursor(disp,framewind,graphics_cursor);
      
      mask = event_mask | ButtonPressMask | ButtonReleaseMask |
	KeyPressMask | PointerMotionMask | PointerMotionHintMask;
      XSelectInput(disp,wind,mask);
      XSelectInput(disp,palwind,mask);
      
      if(image_to_buf(x,y) == 0) {
	 cx = *x; cy = *y;
	 to_device_coords(&cx,&cy);
      } else {
	 if(cx < 0) {
	    cx = width/2;
	    cy = height/2;
	 }
      }
      XWarpPointer(disp,None,framewind,0,0,0,0,cx,cy);
      XFlush(disp);			/* make the pointer move NOW */
      
      while(XCheckTypedEvent(disp,ButtonPress|KeyPress,&event)) ;
   }
   
#if BLINKTIME
   xfd = ConnectionNumber(disp);
   FD_ZERO(&readmask);
   FD_SET(xfd,&readmask);
   timeout.tv_sec = 0;
   timeout.tv_usec = BLINKTIME;
   gettimeofday(&start,(struct timezone *)NULL);
   if(curs_alt < 1 || curs_alt > 2) curs_alt = 2;
#endif

   while(loopy) {
#if BLINKTIME
      if(QLength(disp) == 0 && blinkflg) {
	 mask = readmask;
	 if((c = select(xfd + 1,&mask,
			(fd_set *)NULL,(fd_set *)NULL,&timeout)) == -1) {
	    perror("select in cursor loop");
	    continue;
	 } else if(c == 0) {		/* timed out, so blink */
	    graphics_cursor = (graphics_cursor == bw_graphics_cursor[0]) ?
	      bw_graphics_cursor[curs_alt] : bw_graphics_cursor[0];
	    XDefineCursor(disp,framewind,graphics_cursor);
	    timeout.tv_usec = BLINKTIME;
	    continue;
	 } else {			/* an X event; adjust wait for blink */
	    struct timeval now;
	    gettimeofday(&now,(struct timezone *)NULL);
	    if((timeout.tv_usec -= now.tv_usec - start.tv_usec) < 0) {
	       timeout.tv_usec += 1e6;
	    } else if(timeout.tv_usec >= BLINKTIME) {
	       timeout.tv_usec = BLINKTIME/2; /* BLINKTIME is too long */
	    }
	    start = now;
	 }
      }
      graphics_cursor = bw_graphics_cursor[0]; /* force cursor to be on */
      XDefineCursor(disp,framewind,graphics_cursor);
      XSynchronize(disp,1);
      timeout.tv_usec = BLINKTIME;
#endif
      
      XNextEvent(disp,&event);
      XQueryPointer(disp,wind,&root_wind,&child_wind,&rx,&ry,
		    &cx,&cy,&button_status);
      *x = cx;
      *y = cy;
      to_buf_coords(x,y);
      {
	 short val;
	 value_flag = get_value(x,y,&val);
	 c_val = val;
      }

      if (c_xs != c_xi || c_ys != c_yi){
        /* (void)qql(); */
        c_xi = c_xs;
        c_yi = c_ys;
      }


      switch(event.type) {
       case ButtonRelease:		/* ones that we don't care about */
       case MapNotify:
       case UnmapNotify:
	 break;
       case ConfigureNotify:
       case Expose:
	 XPutBackEvent(disp,&event);	/* reschedule it */
	 x11redraw();
	 break;
       case ButtonPress:		/* treat these together */
       case KeyPress:
	 switch(event.type) {
	  case ButtonPress:
	    switch (event.xbutton.button) {
	     case Button1:
	       if(firstlb != 0){
	         c = -1;
	         firstlb = 0;
	       } else {
	         c = BUTTON1;
	       }
	       break;
	     case Button2:
	       c = BUTTON2;
	       break;
	     case Button3:
	       c = BUTTON3;
	       break;
	     default:
	       fprintf(stderr,"Unknown button %d\n",event.xbutton.button);
	       continue;
	    }
	    if(event.xbutton.state & ControlMask) {
	       c |= CONTROL;
	    } else if(event.xbutton.state & Mod1Mask) {
	       c |= META;
	    }
	    break;
	  case KeyPress:
	    {
	       KeySym keysym;
	       int len;
	       
	       len = XLookupString((XKeyEvent *)&event,buff,sizeof(buff),
				   &keysym,(XComposeStatus *)NULL);
	       switch (keysym) {
		case XK_Left:
		case XK_KP_Left:
		  c = W_ARROW;
		  break;
		case XK_Right:
		case XK_KP_Right:
		  c = E_ARROW;
		  break;
		case XK_Up:
		case XK_KP_Up:
		  c = N_ARROW;
		  break;
		case XK_Down:
		case XK_KP_Down:
		  c = S_ARROW;
		  break;
		case XK_KP_Home:
		  c = NW_ARROW;
		  break;  
		case XK_KP_End:
		  c = SW_ARROW;
		  break;  
		case XK_KP_Prior:
		  c = NE_ARROW;
		  break;  
		case XK_KP_Next:
		  c = SE_ARROW;
		  break;  
		case XK_KP_Begin:
		  c = KEYPAD_CEN;
		  break;  
		default:
		  if(len == 0) {
		     continue;
		  }
		  c = buff[0];
		  break;
	       }
	    }
	 }

	 cursor_key = c;		/* key that Mirella thinks was hit */
	 c_xs_old = c_xs;
	 c_ys_old = c_ys;
	 if(do_cursor_key() >= 0) {  /* exit */
	 
	                             /* note that c_xs, c_ys, and cursor_key 
	                              * are globals (#def to cur_ members)
	                              */
	    loopy = 0;
	 }
	 if(one_key) loopy = 0;
	 if(dopause()) loopy = 0;  /* ^C */

#if 1
	 if(c_xs != c_xs_old || c_ys != c_ys_old) {
	    cx = c_xs; cy = c_ys;
	    if(image_to_buf(&cx,&cy) == 0) {
	       to_device_coords(&cx,&cy);
	       XWarpPointer(disp,None,framewind,0,0,0,0,cx,cy);
	       XFlush(disp);		/* make the pointer move NOW */
#if 0
	       XWarpPointer(disp,None,framewind,0,0,0,0,cx,cy);
	       XFlush(disp);		/* make the pointer move NOW */	       
#endif	       
	    }
	 }
#endif
	 break;
       case MotionNotify:
#if 0
         scrprintf("\rCursor: %-4d %-4d %-5d        ",*x,*y,c_val);
#endif       
	 if(curswind_is_mapped) {
	    char string[20];
	    
	    if(value_flag == 0) {
	       sprintf(string," %-4d %-4d %-5d  ", (int)*x, (int)*y, (int)c_val);
	    } else if(value_flag == 1) {
	       sprintf(string," %-4d %-4d %-5d* ", (int)*x, (int)*y, (int)c_val);
	    } else {
	       sprintf(string," %-4d %-4d %-5s  ", (int)*x, (int)*y, "");
	    }
	    XDrawImageString(disp,curswind,gc,0,(int)(0.8*h_font),string,
			     sizeof(string));
	 }
	 if(namewind_is_mapped) {
	    if(c_disp != NULL && strcmp(imagname,c_disp->_dpfname) != 0) {
	       (void)strncpy(imagname,c_disp->_dpfname,NAME_SIZE);
	       fill_name(imagname);
	    }
	 }
	 break;
       default:
	 fprintf(stderr,"Unknown event: %d\n",event.type);
	 break;
      }
   }
   mprintf("\nCursor: %d %d %d",c_xs, c_ys, c_val);

   if(--level == 0) {
      XSelectInput(disp,wind,event_mask); /* turn cursor off again */

      XUndefineCursor(disp,framewind);
      XWarpPointer(disp,None,root,0,0,0,0,kx,ky);
      XFlush(disp);			/* make the pointer move NOW */
   }

   return(cursor_key);
}

/*
 * Various functions to bind to buttons; exported to Mirella
 */

/* toggle the state of cursor blinking */
void
toggle_blinkflag()
{
   if(blinkflg) {
      graphics_cursor = bw_graphics_cursor[0];
      XDefineCursor(disp,framewind,graphics_cursor);
      blinkflg = 0;
   } else {
      blinkflg = 1;
   }
}

/* toggle the state of the cursor coordinate window */
void
x11_toggle_cursor_window()
{
   if(curswind_is_mapped) {
      XUnmapWindow(disp,curswind);
      curswind_is_mapped = 0;
   } else {
      XMapWindow(disp,curswind);
      curswind_is_mapped = 1;
   }
}

/*
 * Turn cursor window off (flg = 0) or on (flg= 1)
 */
void 
x11_cursor_window(int flg)
{
    if(curswind_is_mapped == 0 && flg != 0) {  /* off and want on. turn on */ 
        XMapWindow(disp,curswind);
        curswind_is_mapped = 1;
    }
    if(curswind_is_mapped != 0 && flg == 0) {  /* on and want off. turn off */
        XUnmapWindow(disp,curswind);
        curswind_is_mapped = 0;
    }
}   
                    
/* toggle the state of the name window */
void
x11_toggle_name_window()
{
   if(namewind_is_mapped) {
      XUnmapWindow(disp,namewind);
      namewind_is_mapped = 0;
   } else {
      XMapWindow(disp,namewind);
      namewind_is_mapped = 1;
   }
}

/*
 * Turn the name window on (flg=1) or off (flg=0)
 */
 
void
x11_name_window( int flg )
{
    
   if(namewind_is_mapped != 0 && flg == 0) {
      XUnmapWindow(disp,namewind);
      namewind_is_mapped = 0;
   }
   if(namewind_is_mapped == 0 && flg != 0){
      XMapWindow(disp,namewind);
      namewind_is_mapped = 1;
   }
}

extern float stretch_bottom,stretch_top;

void
x11_scroll_lookup()
{
   unsigned int button_status;		/* not used */
   Window child_wind;			/* not used */
   int cx,cy;				/* initial position of cursor */
   XEvent event;
   unsigned long mask;
   Window root_wind;			/* not used */
   int rx,ry;				/* not used */
   int x,y;
   
   XQueryPointer(disp,root,&root_wind,&child_wind,&rx,&ry,
		 &cx,&cy,&button_status);
   mask = ButtonPressMask | ButtonReleaseMask |
     PointerMotionMask | PointerMotionHintMask;
   XGrabPointer(disp,framewind,True,mask,GrabModeAsync,GrabModeAsync,False,
		None,CurrentTime);

   x = stretch_bottom*width;
   y = stretch_top*(height + PALHEIGHT);
   XWarpPointer(disp,None,framewind,0,0,0,0,x,y);
   
   for(;;) {
      XQueryPointer(disp,wind,&root_wind,&child_wind,&rx,&ry,
		    &x,&y,&button_status);
      stretch_bottom = (float)x/width;
      stretch_top = (float)y/(height + PALHEIGHT);
      x11stretch_lookup((int)(256*stretch_bottom),(int)(256*stretch_top));
      XNextEvent(disp,&event);
      if(event.type == KeyPress || event.type == ButtonPress) {
	 XUngrabPointer(disp,CurrentTime);
	 XWarpPointer(disp,None,framewind,0,0,0,0,cx,cy - PALHEIGHT);
	 break;
      }
   }
}

/****************************************************************/

void
x11erase()
{
   xpoint_bottom = xpoint_ptr = xpoint;
   xvec_bottom = xvec_ptr = xvec;
   XClearWindow(disp,wind);
   if(image != NULL) {
      image->data = NULL;		/* we don't own it */
      XDestroyImage(image);
      image = NULL;
   }
   if(unzoomed_image != NULL) {
      XDestroyImage(unzoomed_image);
      unzoomed_image = NULL;
   }
   fill_palette();
   XRaiseWindow(disp,framewind);

   imagname[0] = '\0';
}

/****************************************************************/

void
x11erase_graph()
{
   xpoint_bottom = xpoint_ptr = xpoint;
   xvec_bottom = xvec_ptr = xvec;
   x11zoom(0,0,0);			/* repeat last zoom */
}

/****************************************************************/

void
x11flush()
{
   if(disp != NULL) {
      DrawPoints(xpoint_bottom,xpoint_ptr - xpoint_bottom);
      xpoint_bottom = xpoint_ptr;
      
      DrawSegments(xvec_bottom,xvec_ptr - xvec_bottom);
      xvec_bottom = xvec_ptr;
      
      XFlush(disp);
   }
}

void gflush() { x11flush(); }

/********************************************************************/
/* moves cursor to x,y (buffer coordinates */

void
x11warpcursor(x,y)
int x,y;
{
    c_xs = x;
    c_ys = y;
    to_device_coords(&x,&y);
    XWarpPointer(disp,None,framewind,0,0,0,0,x,y);
    XFlush(disp);
}



/****************************************************************/
/*
 * Works in buffer coords (i.e. image coords have been converted
 * knowing about mirage's windows)
 */
void
x11line(x1,y1,x2,y2)
int x1,y1,				/* draw from here */
    x2,y2;				/* to here */
{
   if(xvec == NULL) {
      return;
   }
   if(xvec_ptr >= xvec_bottom + NVEC) {
      DrawSegments(xvec_bottom,NVEC);
      xvec_bottom += NVEC;
   }
   if(xvec_ptr > xvec_top) {
      if(xvec_ptr - xvec_bottom > 0) {
	 DrawSegments(xvec_bottom,xvec_ptr - xvec_bottom);
      }

      nvec *= 2;
      if((xvec = (XSegment *)realloc((char *)xvec,nvec*sizeof(XSegment)))
								== NULL) {
	 fprintf(stderr,"Can't reallocate storage for xvec\n");
	 return;
      }
      xvec_bottom = xvec_ptr = xvec + nvec/2;
      xvec_top = xvec + nvec - 1;
   }

   xvec_ptr->x1 = x1;
   xvec_ptr->y1 = y1;
   xvec_ptr->x2 = x2;
   xvec_ptr->y2 = y2;
   xvec_ptr++;
}

/*****************************************************************************/
/*
 * Draw a line that can be erased by redrawing the same line; doesn't
 * survive expose events, zooms, etc.
 */
void
x11_templine(x1,y1,x2,y2)
normal x1,y1,				/* draw from here */
       x2,y2;				/* to here */
{
   XSegment xvec;
   int xorig,yorig;			/* origin of current display */
   int zoom;				/* zoom in effect */
   
   image_to_buf(&x1,&y1);
   image_to_buf(&x2,&y2);

   xorig = hard_xorig;
   yorig = hard_yorig;
   zoom = hardzoom;

   if(xorig >= 0) {
      xorig *= zoom;			/* take this outside loop */
   }
   if(yorig >= 0) {
      yorig *= zoom;			/* and this too */
   }
   xorig -= zoom/2;			/* draw to centre of pixel */
   yorig -= zoom/2;			/* (minus because it's -zorig) */

   xvec.x1 = (x1 - file_xoff)*zoom - xorig;
   xvec.x2 = (x2 - file_xoff)*zoom - xorig;
   xvec.y1 = (y1 - file_yoff)*zoom - yorig;
   xvec.y2 = (y2 - file_yoff)*zoom - yorig;
   
   xvec.y1 = height - xvec.y1 - 1;
   xvec.y2 = height - xvec.y2 - 1;
   XDrawSegments(disp,wind,graph_xor_gc,&xvec,1);
}

/****************************************************************/

void
x11point(x,y)
int x,y;				/* draw this point */
{
   if(xpoint == NULL) {
      return;
   }
   if(xpoint_ptr >= xpoint_bottom + NVEC) {
      DrawPoints(xpoint_bottom,NVEC);
      xpoint_bottom += NVEC;
   }
   if(xpoint_ptr > xpoint_top) {
      if(xpoint_ptr - xpoint_bottom > 0) {
	 DrawPoints(xpoint_bottom,xpoint_ptr - xpoint_bottom);
      }

      npoint *= 2;
      if((xpoint = (XPoint *)realloc((char *)xpoint,npoint*sizeof(XPoint)))
								== NULL) {
	 fprintf(stderr,"Can't reallocate storage for xpoint\n");
	 return;
      }
      xpoint_bottom = xpoint_ptr = xpoint + npoint/2;
      xpoint_top = xpoint + npoint - 1;
   }

   xpoint_ptr->x = x;
   xpoint_ptr->y = y;
   xpoint_ptr++;
}

/****************************************************************/

int
x11redraw()
{
   XEvent event;
   unsigned int mask;
   XWindowChanges values;

   if(framewind == 0) {
      return(0);
   }
   
   while(XCheckMaskEvent(disp,event_mask,&event)) {
      switch (event.type) {
       case Expose:			/* backing store will deal with it */
	 break;      /* cannot just comment this out; goes crazy if NO bs */
       case ConfigureNotify:
	 if(event.xany.window == framewind) {
	    event.xconfigure.height -= PALHEIGHT;
	    if(height != event.xconfigure.height ||
	       				   width != event.xconfigure.width) {
	       height = event.xconfigure.height;
	       width = event.xconfigure.width;
	    
	       mask = 0;
	       values.height = height; mask |= CWHeight;
	       values.width = width; mask |= CWWidth;
	       XConfigureWindow(disp,wind,mask,&values);
	       
	       mask = 0;
	       values.y = height + border_width; mask |= CWY;
	       values.width = width; mask |= CWWidth;
	       XConfigureWindow(disp,palwind,mask,&values);
	       fill_palette();
	       
	       mask = 0;
	       if(namewind) {
		  unsigned int b_width,depth,nheight; /* unused */
		  unsigned int nwidth;
		  int x,y;			/* unused */
		  
		  XGetGeometry(disp,namewind,&root,&x,&y,&nwidth,&nheight,
			       &b_width,&depth);
		  values.x = width - nwidth - border_width; mask |= CWX;
		  XConfigureWindow(disp,namewind,mask,&values);
	       }
	       x11zoom(0,0,0);		/* repeat last zoom */
	    }
	 } else {
	    if(event.xany.window != palwind && event.xany.window != wind) {
	       fprintf(stderr,"0x%x\n",(int)event.xany.window);
	    }
	 }
	 break;
       default:
	 continue;
      }
   }
   XFlush(disp);
   return(0);
}

/****************************************************************/

/* this rountine does vaguely what I want, but I really want something
 * to give me just a few colors. need to export ncol.
 */
 
int
x11set_lookup(red,green,blue,n_rgb)
u_char red[],green[],blue[];
int n_rgb;				/* size of red, green, blue */
{
   float dcol;				/* convert cmap index to rgb index */
   float f;				/* f = [0,1] for interpolation */
   int i,j;

   if(disp == NULL) {
      return(-1);
   }

   for(i = 0;i < ncol;i++) {
      colors[i].pixel = pixels[i];
      colors[i].flags = DoRed | DoGreen | DoBlue;
      
      dcol = (float)(n_rgb - 1)/(ncol - 1);
      if((j = i*dcol) == n_rgb - 1) {
	 j--;
      }
      f = i*dcol - j;
      colors[i].red = ((1 - f)*red[j] + f*red[j + 1])*256;
      colors[i].green = ((1 - f)*green[j] + f*green[j + 1])*256;
      colors[i].blue = ((1 - f)*blue[j] + f*blue[j + 1])*256;
   }
   XStoreColors(disp,cmap,colors,ncol);
   return(0);
}

/*************** yes, this ***************/

/* 
 * sets the vlt for 1 bit per primary color, 8 colors 
 * lsb is r, next g, next b. see code below for color extraction
 * given a data value.
 * black   = 0
 * red     = 1
 * green   = 2
 * yellow  = 3
 * blue    = 4
 * magenta = 5
 * cyan    = 6
 * white   = 7
 */
 
void 

x11_gvlt1()
{
    int i,r,g,b;
    
    if(ncol < 8){
        mprintf("\nncol = %d is too small to set 8 RGB graphics colors");
        erret(0);
    }
    for(i=0; i<8; i++){
        r = i & 1;
        g = (i & 2) >> 1;
        b = (i & 4) >> 2;
        colors[i].pixel = pixels[i];
        colors[i].flags = DoRed | DoGreen | DoBlue;
        colors[i].red   = 65535*r;
        colors[i].green = 65535*g;
        colors[i].blue  = 65535*b;
    }
    XStoreColors(disp,cmap,colors,8);  /* ???? this was 64?? */
}


/*
 * sets the vlt for 2 bits per primary, so 4 values for each, 64 colors.
 * the lowest two bits are r, next g, next b; see code below for
 * color extraction
 * 
 * if one wishes to specify r,g,b as bytes, the data number is
 * v = ((r>>6) + ((g>>6)<<2) + ((b>>6)<<4) )
 */
 
void
x11_gvlt2()
{
    int i,r,g,b;
    
    if(ncol < 64){
        mprintf("\nncol = %d is too small to set 64 RGB graphics colors");
        erret(0);
    }
    for(i=0; i<64; i++){
        r = i & 3;
        g = (i & 12) >> 2;
        b = (i & 48) >> 4;
        colors[i].pixel = pixels[i];
        colors[i].flags = DoRed | DoGreen | DoBlue;        
        colors[i].red   = 21845*r;
        colors[i].green = 21845*g;
        colors[i].blue  = 21845*b;  /* 21845 is 65535/3 */
    }
    XStoreColors(disp,cmap,colors,64);   /* ????? this was 8?? */
}

/*
 * sets the vlt for 3 bits for rg 2 for b, 256 colors
 * the lowest three bits are r, next g, next 2 b; see code below for
 * color extraction
 * 
 * if one wishes to specify r,g,b as bytes, the data number is
 * v = ((r>>5) + ((g>>5)<<3) + ((b>>6)<<6) )
 */

#if 0  
/* 
 * there is something wrong with the addressing--the optimizer complains
 * for loop indices > 200 ????
 */
 
void
x11_gvlt3()
{
    int i,r,g,b;
    int maxrg = 65535/7;
    int maxb  = 65535/3;
    
    if(ncol < 256){
        mprintf("\nncol = %d is too small to set 64 RGB graphics colors");
        erret(0);
    }
    for(i=0; i<256; i++){
        r = (i & 7);
        g = (i & 54) >> 3;
        b = (i & 192) >> 6;
        colors[i].pixel = pixels[i];
        colors[i].flags = DoRed | DoGreen | DoBlue;        
        colors[i].red   = maxrg*r;
        colors[i].green = maxrg*g;
        colors[i].blue  = maxb*b;  
    }
    XStoreColors(disp,cmap,colors,64);   /* ????? this was 8?? */
}
#endif

/****************************************************************/

void
x11stretch_lookup(bottom,top)
int bottom;				/* value of lowest coloured pixel */
int top;				/* and highest */
{
   int i,j;
   XColor new_colors[NCOL];		/* the new colour table */

   if(bottom < 0) {
      bottom = 0;
   } else if(bottom >= 256) {
      bottom = 255;
   }
   if(top < 0) {
      top = 0;
   } else if(top >= 256) {
      top = 255;
   }
   bottom = (ncol - 1)/(float)255*bottom + 0.5;
   top = (ncol - 1)/(float)255*top + 0.5;
   if(top == bottom) {
      if(bottom == ncol - 1) {
	 bottom--;
      } else {
	 top++;
      }
   }
   
   for(i = 0;i < ncol;i++) {
      new_colors[i].pixel = colors[i].pixel;
      new_colors[i].flags = colors[i].flags;

      j = (int)ncol*(i - bottom)/(float)(top - bottom) + 0.5;
      if(j <= 0) {
	 j = 0;
      } else if(j >= ncol) {
	 j = ncol - 1;
      }
      new_colors[i].red = colors[j].red;
      new_colors[i].green = colors[j].green;
      new_colors[i].blue = colors[j].blue;
   }
   XStoreColors(disp,cmap,new_colors,ncol);
}

/****************************************************************/
/*
 * Zoom the current image.
 *
 * Do the book keeping to make this look like a hardware zoom
 */
int
x11zoom(x,y,zoom)
int x,y,				/* position to zoom about */
    zoom;				/* desired zoom */
{
   int dest_x,dest_y;			/* origin in window for display */
   int i,j;
   char *uptr,*uend;			/* pointers to unzoomed data */
   static int hard_xcentre = 0,		/* previous centre of zoom */
   	      hard_ycentre = 0;
   int xzsize,yzsize;			/* zoomed size of image */
   int src_x,src_y;			/* origin in image for display */
   char *zdata;				/* storage for the zoomed image */
   char *zptr;				/* pointer to zdata */

   if(zoom == 0) {			/* repeat previous zoom */
      fill_palette();			/* may be trashed */
      zoom = hardzoom;
      x = hard_xcentre;
      y = hard_ycentre;
   } if(zoom < 2) zoom = 1;
   else if(zoom < 4) zoom = 2;
   else if(zoom < 8) zoom = 4;
   else if(zoom < 16) zoom = 8;
   else zoom = 16;

   if(image == NULL) {
      if(zoom != 1) {
	 fprintf(stderr,"No image is currently displayed\n");
      }
      return(-1);
   }
/*
 * find the origin of the zoomed or panned region
 */
   left_offset = x - width/(2*zoom);	/* find origin of desired region */
   top_offset = y - height/(2*zoom);
   left_offset -= file_xoff;		/* and forget the displacement of */
   top_offset -= file_yoff;		/* the data in the disk file */
   
   if(left_offset < 0) {		/* check it's in region displayed */
      left_offset = 0;
   } else if(left_offset > xdispsize - (int)width/zoom) {
      left_offset = xdispsize - (int)width/zoom;
      if(left_offset < 0) {
	 left_offset = 0;
      }
   }
   if(top_offset < 0) {
      top_offset = 0;
   } else if(top_offset > ydispsize - (int)height/zoom) {
      top_offset = ydispsize - (int)height/zoom;
      if(top_offset < 0) {
	 top_offset = 0;
      }
   }
   
   if(zoom == 1) {			/* unzoom or pan */
      if(unzoomed_image != NULL) {	/* we must have zoomed */
	 if(image != NULL) {
	    XDestroyImage(image);	/* and this is the zoomed image */
	 }
	 image = unzoomed_image;
	 unzoomed_image = NULL;
      }
      yzsize = height;			/* we don't need xzsize */
   } else {				/* We are zooming, not just panning */
      xzsize = zoom*(xdispsize - left_offset); /* size of zoomed image */
      if(xzsize > width) {
	 xzsize = width;
      }
      yzsize = zoom*(ydispsize - top_offset);
      if(yzsize > height) {
	 yzsize = height;
      }
/*
 * get storage for the zoomed image, if we need one
 */
      if(unzoomed_image == NULL) {	/* i.e. there is no zoomed one */
	 if((zdata = malloc(xzsize*yzsize)) == NULL) {
	    fprintf(stderr,"Can't allocate storage for zdata\n");
	    return(-1);
	 }
      } else {
	 if((zdata = realloc(image->data,xzsize*yzsize)) == NULL) {
	    fprintf(stderr,"Can't reallocate storage for zdata\n");
	    return(-1);
	 }
      }
      
      if(hardzoom == 1) {		/* first save the unzoomed one */
	 if((unzoomed_image = XCreateImage(disp,visual,depth,
	      ZPixmap,0,image->data,image->width,image->height,8,0)) == NULL) {
	    error("\nFailed to allocate unzoomed XImage");
	    free(zdata);
	    return(-1);
	 }
      }
      image->height = yzsize;
      image->width = xzsize;
      image->bytes_per_line = image->width;
      image->data = zdata;		/* may have been moved by realloc */
      zptr = zdata + xzsize*(yzsize - 1);
   }
   
   if(zoom == 1) {
      ;
   } else if(zoom == 2) {
      for(i = 0;i < yzsize;i++) {	/* complete pixels */
	 if(i%zoom == 0) {
	    uptr = &unzoomed_image->data[
		(ydispsize - 1 - top_offset - i/zoom)*xdispsize + left_offset];
	    uend = uptr + xzsize/zoom;
	    while(uptr < uend) {
	       *zptr++ = *uptr;
	       *zptr++ = *uptr++;
	    }
	    for(j = xzsize%zoom;j > 0;j--) { /* fragments in x */
		  *zptr++ = *uptr;
	       }
	    zptr -= 2*xzsize;
	 } else {
	    memcpy(zptr,zptr + xzsize,xzsize);
	    zptr -= xzsize;
	 }
      }
   } else if(zoom == 4) {
      for(i = 0;i < yzsize;i++) {	/* complete pixels */
	 if(i%zoom == 0) {
	    uptr = &unzoomed_image->data[
		(ydispsize - 1 - top_offset - i/zoom)*xdispsize + left_offset];
	    uend = uptr + xzsize/zoom;
	    while(uptr < uend) {
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr++;
	    }
	    for(j = xzsize%zoom;j > 0;j--) { /* fragments in x */
	       *zptr++ = *uptr;
	    }
	    zptr -= 2*xzsize;
	 } else {
	    memcpy(zptr,zptr + xzsize,xzsize);
	    zptr -= xzsize;
	 }
      }
   } else if(zoom == 8) {
      for(i = 0;i < yzsize;i++) {	/* complete pixels */
	 if(i%zoom == 0) {
	    uptr = &unzoomed_image->data[
		(ydispsize - 1 - top_offset - i/zoom)*xdispsize + left_offset];
	    uend = uptr + xzsize/zoom;
	    while(uptr < uend) {
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr++;
	    }
	    for(j = xzsize%zoom;j > 0;j--) { /* fragments in x */
	       *zptr++ = *uptr;
	    }
	    zptr -= 2*xzsize;
	 } else {
	    memcpy(zptr,zptr + xzsize,xzsize);
	    zptr -= xzsize;
	 }
      }
   } else if(zoom == 16) {
      for(i = 0;i < yzsize;i++) {	/* complete pixels */
	 if(i%zoom == 0) {
	    uptr = &unzoomed_image->data[
		(ydispsize - 1 - top_offset - i/zoom)*xdispsize + left_offset];
	    uend = uptr + xzsize/zoom;
	    while(uptr < uend) {
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr;
	       *zptr++ = *uptr; *zptr++ = *uptr++;
	    }
	    for(j = xzsize%zoom;j > 0;j--) { /* fragments in x */
	       *zptr++ = *uptr;
	    }
	    zptr -= 2*xzsize;
	 } else {
	    memcpy(zptr,zptr + xzsize,xzsize);
	    zptr -= xzsize;
	 }
      }
   }
/*
 * Remember where the zoom is about
 */
   if(zoom*xdispsize < width) {		/* image doesn't fill window */
      left_offset = (width - zoom*xdispsize)/2;
      hard_xorig = -left_offset;
   } else {
      hard_xorig = left_offset;
      left_offset = 0;
   }
   if(zoom*ydispsize < height) {	/* image doesn't fill window */
      top_offset = (height - zoom*ydispsize)/2;
      hard_yorig = -top_offset;
      yzsize = height;			/* makes top_offset correct */
   } else {
      hard_yorig = top_offset;
      top_offset = 0;
   }
   hard_xcentre = x;			/* Save the centre in case we want */
   hard_ycentre = y;			/* to restore a zoomed image */
   hardzoom = zoom;
/*
 * and display the zoomed image
 */
   XClearWindow(disp,wind);
   if(hardzoom == 1) {
      wxoff = src_x = hard_xorig > 0 ? hard_xorig : 0;
      wyoff = src_y = hard_yorig > 0 ? hard_yorig : 0;
      dest_x = left_offset;
      dest_y = top_offset;
      
      src_y += ((height > ydispsize) ? ydispsize : height) - 1;
      dest_y += ((height > ydispsize) ? ydispsize : height) - 1;
      src_y = ydispsize - src_y - 1;
      dest_y = height - dest_y - 1;

      XPutImage(disp,wind,gc,image,src_x,src_y,dest_x,dest_y,width,height);
   } else {
      src_x = 0;
      src_y = 0;
      dest_x = left_offset;
      dest_y = top_offset;
      
      dest_y += ((height > yzsize) ? yzsize : height) - 1;
      dest_y = height - dest_y - 1;

      XPutImage(disp,wind,gc,image,src_x,src_y,dest_x,dest_y,xzsize,yzsize);
   }
   DrawPoints(xpoint,xpoint_ptr - xpoint);
   DrawSegments(xvec,xvec_ptr - xvec);
   return(0);
}

normal
dsp_zoom(x,y,zoom)
int x,y,				/* position to zoom about */
    zoom;				/* desired zoom */
{
   return(x11zoom(x,y,zoom));
}



/***************************************************************/
/*
 * change the cursor's shape
 */

int
x11set_cursor(n)
int n;					/* which cursor? */
{
   static struct {
      char source[32];
      char mask[32];
      unsigned int hot_x,hot_y;
   } cursors[] = {
#     include "cursors.h"
   };					/* bitmaps for cursors */
   char nomask[32];
   Pixmap source,mask,no_cursor;

   if(n >= sizeof(cursors)/sizeof(cursors[0])) {
      return(-1);
   }
   if((source = XCreatePixmapFromBitmapData(disp,
				   root,cursors[n].source,16,16,0,1,1)) == 0) {
      return(-1);
   }
   if((mask = XCreatePixmapFromBitmapData(disp,
				      root,cursors[n].mask,16,16,1,0,1)) == 0){
      XFreePixmap(disp,source);
      return(-1);
   }
   clearn(32,nomask);
   if((no_cursor = XCreatePixmapFromBitmapData(disp,
				      root,nomask,16,16,1,0,1)) == 0){
      XFreePixmap(disp,source);
      XFreePixmap(disp,mask);
      return(-1);
   }

   if(graphics_cursor > 0) {
      XFreeCursor(disp,bw_graphics_cursor[0]);
      XFreeCursor(disp,bw_graphics_cursor[1]);
      XFreeCursor(disp,bw_graphics_cursor[2]);      
   }
   /* cursor in forground color */
   bw_graphics_cursor[0] =  
     XCreatePixmapCursor(disp,source,mask,&back_color,&fore_color,
			 cursors[n].hot_x,cursors[n].hot_y);
   /* cursor in background color */ 			 
   bw_graphics_cursor[1] =
     XCreatePixmapCursor(disp,no_cursor,mask,&fore_color,&back_color,
			 cursors[n].hot_x,cursors[n].hot_y);
   /* no cursor--mask is zero */ 			 
   bw_graphics_cursor[2] =
     XCreatePixmapCursor(disp,source,no_cursor,&fore_color,&back_color,
			 cursors[n].hot_x,cursors[n].hot_y);
   graphics_cursor = bw_graphics_cursor[0];
   
   XFreePixmap(disp,source);
   XFreePixmap(disp,mask);
   XFreePixmap(disp,no_cursor);

   return(graphics_cursor == 0 ? -1 : 0);
}

/****************************************************************/
/*
 * Static utilities:
 */
static int
hand_XError(dispp,event)
Display *dispp;
XErrorEvent *event;
{
   char str[80];

   XGetErrorText(dispp,event->error_code,str,80);
   sprintf(buff,"\nX Error: %s",str);
   error(buff);
   return(-1);
}

/****************************************************************/

static int
hand_XIOError(dispp)
Display *dispp;
{
   framewind = 0;
   erret("Fatal X Error: closing device\n");
   /*NOTREACHED*/
   return(0);				/* shut up gcc -Wall */
}

/****************************************************************/

static void
size_x11()
{
   int x,y;
   unsigned int b_width;
   Window root;

   XGetGeometry(disp,framewind,&root,&x,&y,&width,&height,&b_width,&depth);
   height -= PALHEIGHT;
   d_xsize = width;
   d_ysize = height;
}

/****************************************************************/
/*
 * Fill namewind with proper value
 */
static void
fill_name(name)
char *name;				/* name to use, or NULL */
{
   int len;
   unsigned long mask;                  /* mask for attributes */
   XWindowChanges values;

   len = (strlen(imagname) + 2)*w_font;
   mask = 0;
   values.x = width - len - border_width; mask |= CWX;
   values.width = len; mask |= CWWidth;
   XConfigureWindow(disp,namewind,mask,&values);

   XClearWindow(disp,namewind);
   XDrawImageString(disp,namewind,gc,w_font,(int)(0.8*h_font),imagname,
	       strlen(imagname));
}

/****************************************************************/
/*
 * Fill palette with proper values
 */
static void
fill_palette()
{
   char c;
   char *data;
   int i,j;
   /* int screen = DefaultScreen(disp);; */
   XImage *pimage;



   if((data = malloc(width*PALHEIGHT)) == NULL) {
      fprintf(stderr,"Can't allocate storage for palette\n");
      return;
   }
   if((pimage = XCreateImage(disp,visual,depth,ZPixmap,0,data,
			    width,PALHEIGHT,8,0)) == NULL) {
      error("\nFailed to allocate XImage for palette");
      free(data);
      return;
   }

   for(i = 0;i < width;i++) {
      c = pixels[(int)(i*(ncol - 1)/(float)(width - 1) + 0.001)];
      data[i] = c;
   }
   for(j = 1;j < PALHEIGHT;j++) {
      memcpy(&data[j*width],&data[(j - 1)*width],width);
   }

   XPutImage(disp,palwind,gc,pimage,0,0,0,0,width,PALHEIGHT);
   XDestroyImage(pimage);
}

/****************************************************/
/*
 * Convert device coords (image pixels wrt top left of display)
 * to buffer coordinates (data pixels wrt bottom left of image buffer)
 */
static void
to_buf_coords(x,y)
int *x,*y;
{
   *y = height - 1 - *y;
   if(hard_xorig < 0) {
      *x = (hard_xorig + *x)/hardzoom;
   } else {
      *x = hard_xorig + *x/hardzoom;
   }
   if(hard_yorig < 0) {
      *y = (hard_yorig + *y)/hardzoom;
   } else {
      *y = hard_yorig + *y/hardzoom;
   }
}

/****************************************************/
/*
 * Convert buffer coordinates (data pixels wrt bottom left of buffer)
 * to device coords (image pixels wrt top left of display)
 */
static void
to_device_coords(x,y)
int *x,*y;
{
   if(hard_xorig < 0) {
      *x = hardzoom*(*x) - hard_xorig;
   } else {
      *x = hardzoom*(*x - hard_xorig);
   }
   if(hard_yorig < 0) {
      *y = hardzoom*(*y) - hard_yorig;
   } else {
      *y = hardzoom*(*y - hard_yorig);
   }
   *x += hardzoom/2;
   *y += hardzoom/2;
   *y = height - 1 - *y;
}

/*********************** STOD() ************************************/
/* Try this */
void
stod(sx,sy,pxd,pyd)
int sx,sy;
int *pxd, *pyd;
{
   int x; 
   int y; 
   x = sx; 
   y = sy;
  
   if(hard_xorig < 0) {
      x = hardzoom*(x) - hard_xorig;
   } else {
      x = hardzoom*(x - hard_xorig);
   }
   if(hard_yorig < 0) {
      y = hardzoom*(y) - hard_yorig;
   } else {
      y = hardzoom*(y - hard_yorig);
   }
   x += hardzoom/2;
   y += hardzoom/2;
   *pxd = x;
   *pyd = y;
}

/*********************************************************/
/*
 * Draw a set of segments
 */
static void
DrawSegments(vecs,n)
XSegment *vecs;
int n;
{
   int i,j;
   int xorig,yorig;			/* origin of current display */
   XSegment xvec2[NVEC];		/* vectors scaled to current zoom */
   int zoom;				/* zoom in effect */
   
   xorig = hard_xorig;
   yorig = hard_yorig;
   zoom = hardzoom;

   if(xorig >= 0) {
      xorig *= zoom;			/* take this outside loop */
   }
   if(yorig >= 0) {
      yorig *= zoom;			/* and this too */
   }
   xorig -= zoom/2;			/* draw to centre of pixel */
   yorig -= zoom/2;			/* (minus because it's -zorig) */

   for(i = j = 0;i < n;i++,j++) {
      xvec2[j].x1 = (vecs[i].x1 - file_xoff)*zoom - xorig;
      xvec2[j].x2 = (vecs[i].x2 - file_xoff)*zoom - xorig;
      xvec2[j].y1 = (vecs[i].y1 - file_yoff)*zoom - yorig;
      xvec2[j].y2 = (vecs[i].y2 - file_yoff)*zoom - yorig;
      
      xvec2[j].y1 = height - xvec2[j].y1 - 1;
      xvec2[j].y2 = height - xvec2[j].y2 - 1;
      if(j == NVEC - 1) {
	 XDrawSegments(disp,wind,graph_gc,xvec2,NVEC);
	 j = 0;
      }
   }
   if(j != 0) {
      XDrawSegments(disp,wind,graph_gc,xvec2,j);
   }
}

/*********************************************************/
/*
 * Draw a set of segments
 */
static void
DrawPoints(points,n)
XPoint *points;
int n;
{
   int i,j;
   int xorig,yorig;			/* origin of current display */
   XPoint xpoint[NVEC];			/* points scaled to current zoom */
   int zoom;				/* zoom in effect */
   
   xorig = hard_xorig;
   yorig = hard_yorig;
   zoom = hardzoom;

   if(xorig >= 0) {
      xorig *= zoom;			/* take this outside loop */
   }
   if(yorig >= 0) {
      yorig *= zoom;			/* and this too */
   }

   if(zoom == 1) {			/* draw dots */
      for(i = j = 0;i < n;i++,j++) {
	 xpoint[j].x = (points[i].x - file_xoff)*zoom - xorig;
	 xpoint[j].y = (points[i].y - file_yoff)*zoom - yorig;
	 
	 xpoint[j].y = height - xpoint[j].y - 1;
	 if(j == NVEC - 1) {
	    XDrawPoints(disp,wind,graph_gc,xpoint,NVEC,CoordModeOrigin);
	    j = 0;
	 }
      }
      if(j != 0) {
	 XDrawPoints(disp,wind,graph_gc,xpoint,j,CoordModeOrigin);
      }
   } else {				/* draw points as crosses */
      XSegment xvec2[2*NVEC];
      
      for(i = j = 0;i < n;i++,j += 2) {
	 xvec2[j].x1 = (points[i].x - file_xoff)*zoom - xorig;
	 xvec2[j].y1 = (points[i].y - file_yoff)*zoom - yorig;
	 xvec2[j].x2 = xvec2[j].x1 + zoom - 1;
	 xvec2[j].y2 = xvec2[j].y1 + zoom - 1;
	 xvec2[j + 1].x1 = xvec2[j].x1;
	 xvec2[j + 1].y1 = xvec2[j].y1 + zoom - 1;
	 xvec2[j + 1].x2 = xvec2[j].x1 + zoom - 1;
	 xvec2[j + 1].y2 = xvec2[j].y1;
	 
	 xvec2[j].y1 = height - xvec2[j].y1 - 1;
	 xvec2[j].y2 = height - xvec2[j].y2 - 1;
	 xvec2[j + 1].y1 = height - xvec2[j + 1].y1 - 1;
	 xvec2[j + 1].y2 = height - xvec2[j + 1].y2 - 1;
	 if(j == 2*(NVEC - 1)) {
	    XDrawSegments(disp,wind,graph_gc,xvec2,2*NVEC);
	    j = 0;
	 }
      }
      if(j != 0) {
	 XDrawSegments(disp,wind,graph_gc,xvec2,j);
      }
   }
}

/*
 * If called with a string argument, return the first non-blank character.
 * Future calls to next_char with a NULL string will return pointers
 * to succesive words, all nul terminated. No extra storage is allocated,
 * and the nuls are inserted into the original string.
 */
static char *
next_word(ptr)
char *ptr;
{
   char *word;
   static char *next = NULL;

   if(ptr != NULL) {
      while(isspace(*ptr)) ptr++;
      next = ptr;
   }
   if(next == NULL) {
      error("\nYou must set next_word with a string!");
      return("");
   }
   word = ptr = next;

   while(*ptr != '\0') {
      if(isspace(*ptr)) {
	 *ptr++ = '\0';
	 break;
      }
      ptr++;
   }
   
   while(isspace(*ptr)) ptr++;

   next = ptr;
   return(word);
}

#if defined(NO_MEMCPY)
Void *
memcpy(s1,s2,n)
Void *s1;
Void const *s2;
int n;
{
   return(bufcpy(s1,s2,n));
}
#endif

/************ MIRELLA INTERFACE ****************************************/
/*
 * Now the interface to Mirella:
 */
char x11_arg_string[100] = "-geom +1+1"; /* -disp grendel:0 etc */
u_char vlta[768];			/* two lookup tables */
u_char vltb[768];
static u_char *vlt = vlta;		/* current lookup table */

/*
 * Initialise the window system, but don't open any windows.
 */
normal
dspinit()
{
   push_callback(x11redraw);
   return(0);
}

int
dspopen()
{
   char str[100];

   strcpy(str,x11_arg_string);		/* it'll get trampled on */
   if(x11setup(str) < 0) {
      return(-1);
   }

   dsp_setdlt();
   /*debug mprintf("\nX assigned width=%d height=%d\n",d_xsize, d_ysize); */
   return(0);
}

int 
dspg1open()
{
   char str[100];

   strcpy(str,x11_arg_string);		/* it'll get trampled on */
   if(x11gsetup(str) < 0) {
      return(-1);
   }
   x11_gvlt1();
   return(0);
}

int 
dspg2open()
{
   char str[100];

   strcpy(str,x11_arg_string);		/* it'll get trampled on */
   if(x11gsetup(str) < 0) {
      return(-1);
   }
   x11_gvlt2();
   return(0);
}



normal
dsp_image(pict,xl,yl,xh,yh)
short **pict;				/* image to display */
int xl,yl;				/* lower left corner in picture */
int xh,yh;				/* upper right corner "   "  "  */
{
   char *data;				/* data to display (scaled) */
   char *dbase,*dptr;			/* pointers to data */
   int dest_x,dest_y;			/* origin in window for display */
   short *ptr;
   int src_x,src_y;			/* origin in image for display */
   int xsize,ysize;			/* x and y size of image */
   int y;

   if(d_cden != 1) {
      fprintf(stderr,"\nSorry - no scrunching in X11 (yet)");
      return(-1);
   }
   if(dltable == NULL) {
      fprintf(stderr,"There is no lookup table allocated");
      return(-1);
   }

   if(unzoomed_image != NULL) {		/* we were zoomed */
      XClearWindow(disp,wind);
      XDestroyImage(image);		/* the zoomed one */
      image = unzoomed_image;
      unzoomed_image = NULL;
   }
   if(image != NULL) {
      if(image->width != wxsize || image->height != wysize) {
	 XDestroyImage(image);
	 image = NULL;
      }
   }
   if(image == NULL) {
      if((data = malloc(wxsize*wysize)) == NULL) {
	 fprintf(stderr,"Can't allocate storage for data\n");
	 return(-1);
      }
      filln(wxsize*wysize,data,backpix);
      
      if((image = XCreateImage(disp,visual,depth,ZPixmap,0,data,
					       wxsize,wysize,8,0)) == NULL) {
	 error("\nFailed to allocate XImage");
	 free((char *)data);
	 return(-1);
      }
   } else {
      data = image->data;
   }
/*
 * set up the mapping of DN to pixel values
 */
   for(y = 0;y < yh - yl;y++) {
      ptr = &pict[yl + y][xl];
      dptr = dbase = &data[((wysize - 1 - y) - d_ylow)*wxsize + d_xlow];
      while(dptr < dbase + xh - xl) {
	 *dptr++ = dltable[*ptr++];
      }
      if(!(y%10)) {
#if defined(pause)			/* conflict with X headers */
         pause(out);
#else
         if(dopause()) goto out;
#endif
      }
   }

   xdispsize = wxsize;			/* copy into local static variables */
   ydispsize = wysize;
   (void)strncpy(imagname,d_pfname,NAME_SIZE);
   fill_name(imagname);
/*
 * Image all allocated, scaled and written into the image structure,
 * now figure out where and how much of image to display.
 */
   if(d_cden != 1 || d_cnum != 1) {
      x11zoom(wxoff - width*d_cnum/d_cden,wyoff - height*d_cnum/d_cden,-1);
      return(0);
   }
   left_offset = ((int)width - xdispsize)/2;
   top_offset = ((int)height - ydispsize)/2;
   if(left_offset < 0) {
      left_offset = 0;
   }
   if(top_offset < 0) {
      top_offset = 0;
   }

   dest_x = left_offset;
   if(d_xlow > wxoff) {
      src_x = d_xlow;
      dest_x += d_xlow - wxoff;
   } else {
      src_x = wxoff;
   }
   dest_y = top_offset;
   if(d_ylow > wyoff) {
      src_y = d_ylow;
      dest_y += d_ylow - wyoff;
   } else {
      src_y = wyoff;
   }
   xsize = d_xhigh - src_x;
   ysize = d_yhigh - src_y;

/*debug
   printf(
"\nxsize,ysize,d_xlow,d_ylow,d_xhigh,d_yhigh,src_x,src_y,wxoff,wyoff=%d %d %d %d %d %d %d %d %d %d\n",
xsize,ysize,d_xlow,d_ylow,d_xhigh,d_yhigh,src_x,src_y,wxoff,wyoff);
*/

/*
 * keep the books
 */
   hardzoom = 1;
   hard_xorig = (xdispsize > width) ? wxoff : -left_offset;
   hard_yorig = (ydispsize > height) ? wyoff : -top_offset;
   file_xoff = xl;			/* remember the origin of the data */
   file_yoff = yl;

   src_y += ((height > ysize) ? ysize : height) - 1;
   dest_y += ((height > ysize) ? ysize : height) - 1;
   src_y = ydispsize - src_y - 1;
   dest_y = height - dest_y - 1;
   XPutImage(disp,wind,gc,image,src_x,src_y,dest_x,dest_y,xsize,ysize);
   DrawPoints(xpoint,xpoint_ptr - xpoint);
   DrawSegments(xvec,xvec_ptr - xvec);
   return(0);
 out:
   XDestroyImage(image);
   image = NULL;
   return(-1);
}

/*****************************************************************************/
/*
 * Mirella display routines for raw byte images. Mostly for graphics.
 * Need to be able to open display with a handful of colors for more
 * chance of success, so need a little routine to set NCOL (set_ncol below)
 * not compiled in. This is useful with as little as 2 color
 * ( monochrome), eight (WRGBCMYK), etc.
 */
 
void 
set_ncol(n)
int n;
{
    if(n > NCOL){
        n=NCOL;
        printf("\nSetting to max number of colors allowed = %d",NCOL);
    }
    Ncol = n;
}


/*
 * sets the vlt for 1 bit per primary, so data values run from 0 to 7;
 * 0=blk 1=red 2=grn 4=blu 3=yel 5=magenta 6=cyan 7=wht
 */
 

normal
dsp_bimage(pict,xl,yl,xh,yh)
char **pict;				/* image to display */
int xl,yl;				/* lower left corner in picture */
int xh,yh;				/* upper right corner "   "  "  */
{
   char *data;				/* data to display (scaled) */
   char *dbase,*dptr;			/* pointers to data */
   int dest_x,dest_y;			/* origin in window for display */
   char *ptr;
   int src_x,src_y;			/* origin in image for display */
   int xsize,ysize;			/* x and y size of image */
   int y;

   if(unzoomed_image != NULL) {		/* we were zoomed */
      XClearWindow(disp,wind);
      XDestroyImage(image);		/* the zoomed one */
      image = unzoomed_image;
      unzoomed_image = NULL;
   }
   if(image != NULL) {
      if(image->width != wxsize || image->height != wysize) {
	 XDestroyImage(image);
	 image = NULL;
      }
   }
   if(image == NULL) {
      if((data = malloc(wxsize*wysize)) == NULL) {
	 fprintf(stderr,"Can't allocate storage for data\n");
	 return(-1);
      }
      filln(wxsize*wysize,data,backpix);
      
      if((image = XCreateImage(disp,visual,depth,ZPixmap,0,data,
					       wxsize,wysize,8,0)) == NULL) {
	 error("\nFailed to allocate XImage");
	 free((char *)data);
	 return(-1);
      }
   } else {
      data = image->data;
   }
/*
 * set up the mapping of DN to pixel values
 */
   for(y = 0;y < yh - yl;y++) {
      ptr = &pict[yl + y][xl];
      dptr = dbase = &data[((wysize - 1 - y) - d_ylow)*wxsize + d_xlow];
      while(dptr < dbase + xh - xl) {
	 *dptr++ = pixels[(int)(*ptr++)];  /* just copy color pixel values */
      }
      if(!(y%10)) {
#if defined(pause)			/* conflict with X headers */
         pause(out);
#else
         if(dopause()) goto out;
#endif
      }
   }

   xdispsize = wxsize;			/* copy into local static variables */
   ydispsize = wysize;
   (void)strncpy(imagname,d_pfname,NAME_SIZE);
   fill_name(imagname);
/*
 * Image all allocated, scaled and written into the image structure,
 * now figure out where and how much of image to display.
 */
   left_offset = ((int)width - xdispsize)/2;
   top_offset = ((int)height - ydispsize)/2;
   if(left_offset < 0) {
      left_offset = 0;
   }
   if(top_offset < 0) {
      top_offset = 0;
   }

   dest_x = left_offset;
   if(d_xlow > wxoff) {
      src_x = d_xlow;
      dest_x += d_xlow - wxoff;
   } else {
      src_x = wxoff;
   }
   dest_y = top_offset;
   if(d_ylow > wyoff) {
      src_y = d_ylow;
      dest_y += d_ylow - wyoff;
   } else {
      src_y = wyoff;
   }
   xsize = d_xhigh - src_x;
   ysize = d_yhigh - src_y;
/*
 * keep the books
 */
   hardzoom = 1;
   hard_xorig = (xdispsize > width) ? wxoff : -left_offset;
   hard_yorig = (ydispsize > height) ? wyoff : -top_offset;
   file_xoff = xl;			/* remember the origin of the data */
   file_yoff = yl;

   src_y += ((height > ysize) ? ysize : height) - 1;
   dest_y += ((height > ysize) ? ysize : height) - 1;
   src_y = ydispsize - src_y - 1;
   dest_y = height - dest_y - 1;
   XPutImage(disp,wind,gc,image,src_x,src_y,dest_x,dest_y,xsize,ysize);
   DrawPoints(xpoint,xpoint_ptr - xpoint);
   DrawSegments(xvec,xvec_ptr - xvec);
   return(0);
 out:
   XDestroyImage(image);
   image = NULL;
   return(-1);
}
 


/*****************************************************************************/
/*
 * Map Mirella's idea of a lookup table into ours.
 */

/* Pixels holds the color number, int array, ncol entries
 * After this routine is
 * run, dlt has a color number per DN value, mapped 1-1. Next we need to
 * learn how to map the color number into video colors. ???
 */
 
void
dsp_setdlt()
{
   u_char conv[NCOLOR];			/* convert NCOLOR to ncol */
   float fac;
   int i;
   register u_char *tp;
   
   if(dltable == NULL) {
      mprintf("You must allocate a dlt before calling dsp_setdlt\n");
      return;
   }

   /* mprintf("\nExecuting dsp_setdlt()"); */
   if(ncol == 0){
       mprintf("\nncol=0; the X display is not set up properly\n");
       return;
   }
   fac = (float)((int)ncol - 1)/(NCOLOR - 1);
   /*DB mprintf("\nNCOLOR=%d ncol=%d, fac=%f pixels=%d\n",NCOLOR,ncol,fac,
        pixels);*/
   for(i = 0;i < NCOLOR;i++) {
      conv[i] = pixels[(int)(fac*i + 0.5)];
   }

    /*DB for(i=0;i<NCOLOR;i++) mprintf(" %d",conv[i]); */
       fflush(stdout);

   for(tp = dltable - 32768;tp < dltable + 32768;tp++) {
       *tp = conv[*tp];
   }
}

int 
qdsp_setup()
{
    if(dltable == NULL || ncol == 0) return 0;
    else return 1;
} 

void
putdsppts(points,n)
struct dsp_point *points;
int n;
{
   struct dsp_point *end = points + n;

   points--;
   while(++points < end) {
      x11point(points->x,points->y);
   }
}

void
dev_line(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
   x11line(x1,y1,x2,y2);
}

void
dev_templine(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
   x11_templine(x1,y1,x2,y2);
}
    

void
colvlt(r,g,b,n)
u_char *r,*g,*b;
int n;
{
   x11set_lookup(r,g,b,n);
}

void
vltsel(n)
int n;
{
   switch(n) {
    case 0:
      vlt = vlta;
      break;
    case 1:
      vlt = vltb;
      break;
    default:
      fprintf(stderr,"Illegal choice of lookup table: %d\n",n);
      return;
   }
   x11set_lookup(vlt,vlt + 256,vlt + 512,256);
}

void
cursor()
{
   reset_curflags();
   (void)x11cursor(&c_xs,&c_ys,0);
}

void
to_pal(c)
u_char *c;
{
   x11set_lookup(vlt,vlt + 256,vlt + 512,256);
}

void
dsp_erase()
{
   x11erase();
}

/*
 * Dummies (some just for now)
 */
void
dbytewrt(buf,n,x,y)
char *buf;
int n;
int x,y;
{
   fprintf(stderr,"\ndbytewrt is not yet supported by X11");
}

void
ggg(ny,line,oldflg)
int ny;
unsigned short *line;
int oldflg;
{
   fprintf(stderr,"\nggg is not (yet) supported by X11");
}

int
qqq()
{
   x11cursor(&c_xs,&c_ys,1);
   return(cursor_key);
}

void
dspwindow()
{
   fprintf(stderr,"\ndspwindow isn't yet supported by X11");
}

/*****************************************************************************/
/*
 * Save the pixels under the cursor; a no-op for X
 */
void
savepix(x,y)
int x,y;
{
   ;
}

void
dcenter(x,y)
int x,y;
{
   fprintf(stderr,"\nX11: dcenter: (%d,%d) (noop)",x,y);
}

void
cenwin(x,y)
int x,y;
{
   fprintf(stderr,"\nX11: cenwin: (%d,%d) (noop)",x,y);
}

void
setbm(x,y)
int x,y;
{
   fprintf(stderr,"\nsetbm isn't yet supported by X11");
}

void
greyscale(low, high, overlay)
int low,high;
int overlay;
{
   fprintf(stderr,"\ngreyscale isn't yet supported by X11");
}

void
hramp()
{
   fprintf(stderr,"\nhramp isn't yet supported by X11");
}

void
gryvlt(l,h,b,i,s)
int l,h,b,i,s;
{
   fprintf(stderr,"\ngryvlt isn't yet supported by X11");
}

void
panmode()
{
   fprintf(stderr,"\npanmode isn't supported by X11");
}

void
rdvlt(v,t)
u_char *v;
int t;
{
   fprintf(stderr,"\nrdvlt isn't yet supported by X11");
}


mirella void 
edgraph(x0, y0, nx, ny)
int x0, y0, nx, ny;
{
   fprintf(stderr,"\nedgraph isn't yet supported by X11");
}

int
getpix(x,y)
int x,y;
{
   fprintf(stderr,"\nX11 doesn't support getpix()");
   return(0);
}

void
setpix(x,y,val)
int x,y;
int val;
{
   fprintf(stderr,"\nX11 doesn't support setpix()");
}

#if OPENWINDOWS_BUG
#undef XQueryPointer

static void
x_query_pointer(disp,root,root_wind,child_wind,rx,ry,kx,ky,button_status)
Display *disp;
Window root,*root_wind,*child_wind;
int *rx,*ry,*kx,*ky;
unsigned int *button_status;
{
   XQueryPointer(disp,root,root_wind,child_wind,rx,ry,kx,ky,button_status);
   if(fix_openwindows_bug) {
      *kx -= 4; *ky -= 4;
      *rx -= 4; *ry -= 4;
   }
}
#endif

#endif
