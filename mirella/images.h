#if !defined(IMAGES_H)
#define IMAGES_H
/*
 * Things needed for application-independent imaging
 *
 * parameters for memory allocation
 */
#define MAXPBUF 16		/* how many can we have ?*/
/*
 * Types of data in a pbuf; the values of _data_type in HEADER
 */
#define T_UNKNOWN 0
#define T_CHAR 1
#define T_SHORT 2
#define T_LONG 3
#define T_FLOAT 4
#define T_COMPLEX 5
#define T_DOUBLE 6
#define T_RGB8 7

typedef struct {
    u_char _rrgb;
    u_char _grgb;
    u_char _brgb;
} RGBTRIPLE;


#define ISBUF(D)			/* Is D a valid buffer? */ \
  ((D) >= 0 && (D) < MAXPBUF && pbufs[D]->_pline != NULL)

/*
 * Minimal application-independent part of a pbuffer, along with a pointer
 * to an unknown structure for (e.g.) FITS information
 */
typedef struct {
   normal _serno;			/* what buffer revision am I? */
   normal _bitpix;			/* bits per pixel */
   normal _naxes;			/* how many axes? */
   normal _xsize;			/* x and */
   normal _ysize;			/*       y size of pic */
   int    _zsize;			/* for eventual multiple pictures */
   char   _pfname[72];			/* picture filename */
   char   _imname[72];                  /* picture name, if applicable */
   char   _filedate[16];		/* date of creation of the file */
   int    _decorder;			/* flag for Dec-Intel byte order */
   int    _data_type;			/* type of data: T_CHAR, T_SHORT,
					   T_LONG, T_FLOAT, T_COMPLEX,
					   T_DOUBLE, T_RGB8 */
   Void *app;				/* application specific part */
   int app_size;			/* sizeof(*app) */
} HEADER;

/*
 * The basic data structure for representing picture buffers
 */
typedef struct {
   HEADER _header;			/* header information */
   char *_mpicorig;			/* value of picorig as malloced */
   short *_picorig;			/* ptr to origin of picture, rounded
					   to a page (512by) boundary */
   short **_pline;			/* ptr to array of line origins */
   int _pline_size;			/* size of pline */
   long _pbsize;			/* size of buffer in short ints */
   Void *_scr;				/* scratch space for application */
} PBUF;

/*
 * The data structure to describe a displayed image; where it is, how it
 * is zoomed, the grayscale, etc.
 */
typedef struct display_t {
    normal _xoff,_yoff;			/* ll corner in picture space */ 
    normal _dxlow,_dylow;		/* ll corner in display bitmap space */
    normal _dxhigh,_dyhigh;		/* ur corner in display bitmap space */
    normal _wxcen,_wycen;		/* center of phys. disp. window in
					   disp. bitmap space*/
    normal _wzoom;			/* hardware zoom if any */
    normal _cnum;			/* software zoom */
    normal _cden;			/* software compression */
    normal _wrapflg;			/* 1 if wrap, 0 if clip */
    normal _zorg;			/* black level */
    normal _zrng;			/* white-black range */
    normal _dmode;			/* cursor display mode;
					   0 if displ mem, 1 if main mem */
    normal _bufno;			/* buffer from which created */
    normal _bufsno;			/* revision number of buffer at
					   time of creation */
    char   _dpfname[72];		/* picture filename */
} DISPLAY;
/*
 * Define a cursor
 */
typedef struct {
    normal _xs;				/* cursor location in picture */
    normal _ys;
    normal _cmarker;			/* cursor marker index */
    DISPLAY *_cdisp;			/* current display */
    normal _cval;			/* value at cursor */
} CURSOR;

/*
 * A point in an image
 */
struct dsp_point {
   short int x;
   short int y;
};

/*****************************************************************************/
/*
 * Prototypes/external declarations
 */
void alloc_app_pbuf P_ARGS(( PBUF * ));
void bsbump P_ARGS(( void ));
void bsbumpn P_ARGS(( int ));
void copy_full_header P_ARGS(( HEADER *, HEADER * ));
int do_cursor_key P_ARGS (( void ));
void dsup P_ARGS(( void ));
DISPLAY *findwind P_ARGS(( int x, int y ));
void free_app_pbuf P_ARGS(( PBUF * ));
int get_value P_ARGS (( int *x, int *y, short *value ));
void headup P_ARGS(( void ));
void init_app_header P_ARGS(( void ));
void init_app_pbuf P_ARGS(( PBUF * ));
void init_disp_stack P_ARGS(( void ));
void init_dlt P_ARGS(( void ));
void init_display P_ARGS(( void ));
void init_image P_ARGS(( void ));
void init_header P_ARGS(( HEADER * ));
void init_pbuf P_ARGS(( PBUF * ));
int  isacbuf P_ARGS(( int d ));
void isokbuf P_ARGS(( int d ));
void pclear P_ARGS(( void ));
void qq P_ARGS(( void ));
void qql P_ARGS(( void ));
void reset_disp_stack P_ARGS(( void ));
void dsp_setdlt P_ARGS(( void ));
/* moved from mirage.h 1208 */
void plptr P_ARGS(( void ));
void bplptr P_ARGS(( void ));
void splptr P_ARGS(( void ));
void fplptr P_ARGS(( void ));
void nplptr P_ARGS(( void ));
void gplptr P_ARGS(( int n ));
void dbufchk P_ARGS(( int n ));
void pdest P_ARGS(( int n ));
void bpdest P_ARGS(( int n ));
void fpdest P_ARGS(( int n));   
void npdest P_ARGS(( int n));   
void spdest P_ARGS(( int n));   
void rgbpdest P_ARGS(( int n));   
void seldest P_ARGS(( int n));
void selqdest P_ARGS(( int n)); 
void pcopy P_ARGS(( int n));
void pchange P_ARGS(( int xs, int ys, int n ));
void bpchange P_ARGS(( int xs, int ys, int n ));
void headtobuf P_ARGS(( int n ));
extern void (*copy_app_header)(int);

void dbytewrt P_ARGS(( char *, int, int, int ));
void ccur P_ARGS(( void ));
void cenwin P_ARGS(( int, int ));
void dcenter P_ARGS(( int, int ));
void dsp_erase P_ARGS(( void ));
int dspinit P_ARGS(( void ));
int dsp_image P_ARGS((short **pict, int xl, int yl, int xh, int yh ));
int dsp_bimage P_ARGS((char **pict, int xl, int yl, int xh, int yh ));
int dspprint P_ARGS(( char * ));
int dsp_zoom P_ARGS(( int x, int y, int zoom ));
void reset_curflags P_ARGS(( void ));
void savepix P_ARGS(( int, int ));

/*****************************************************************************/
/*
 * these sizes are variable or constant depending on the display.
 * The actual display need not be d_xsize*d_ysize
 */
extern normal d_xsize;
extern normal d_ysize; 
/*
 * these are the window sizes, which may or may not be the same as the
 * d_[xy]size sizes, and the desired offsets of the window
 */
extern normal wxsize;
extern normal wysize;

extern normal wxoff,wyoff;

extern PBUF *pbuf;			/* the current pbuf */
extern PBUF *pbufd;
extern PBUF *pbufs[MAXPBUF];		/* the possible picture buffers */
extern normal pbuffer;			/* current pbuf */
extern normal pbufferd;                 /* current destination buffer */

extern normal cursssize;
extern normal dispsize;

extern short  **pic;
extern char   **bpic;
extern normal **npic;
extern float  **fpic;
extern double **dpic;
extern RGBTRIPLE **rgbpic;

extern short  **picd; 
extern char   **bpicd;
extern normal **npicd;
extern float  **fpicd;
extern double **dpicd;
extern RGBTRIPLE **rgbpicd;

#define pline pic
#define plines(N) pbufs[N]->_pline
extern normal xsize;
extern normal ysize;
extern normal xsized;
extern normal ysized;                          /* sizes for dest buffer */


extern DISPLAY *display;
extern CURSOR *curs;

#define NCOLOR 256			/* Number of colours used by Mirella */
extern u_char *dltable;			/* Mirella's dlt */

/* extern Void **m_ctab; */
/*****************************************************************************/
/*
 * definitions for "current" structure members
 */
#if 0 
#define BPIC ((u_char **)pic)		/* buffer thought of as bytes */
#define bpic ((u_char **)pic)           /* ditto */
#define npic ((normal **)pic)           /* buffer thought of as normals */
#define fpic ((float **)pic)            /* buffer thought of as floats */
#define rgbpic ((RGBTRIPLE**)pic)       /* buffer thought of as byte triples*/
#define dpic ((double **)picd)          /* buffer thought of as doubles */
#define bpicd ((u_char **)picd)         /* destbuffer though of as bytes */
#define npicd ((normal **)picd)         /* destbuffer thought of as normals */
#define fpicd ((float **)picd)          /* destbuffer thought of as floats */
#define rgbpicd ((RGBTRIPLE**)picd)     /* destbuffer thought of as byte triples*/
#define dpicd ((double **)picd)         /* destbuffer thought of as doubles */
#endif

#define picorig (pbuf->_picorig)
#define picorigd (pbufd->_picorig)
#define pbsize (pbuf->_pbsize)
#define pbsized (pbufd->_pbsize)

#define h_serno pbuf->_header._serno
#define h_bitpix  pbuf->_header._bitpix
#define h_naxes pbuf->_header._naxes
#define h_xsize pbuf->_header._xsize
#define h_ysize pbuf->_header._ysize
#define h_zsize pbuf->_header._zsize
#define h_filedate pbuf->_header._filedate
#define h_decorder pbuf->_header._decorder
#define h_data_type pbuf->_header._data_type
#define h_pfname pbuf->_header._pfname

/*****************************************************************************/
/*
 * defines for the cursor
 */
#define CORNER_PAT 1			/* possible patterns for markit() */
#define PLUS_PAT 2
#define CONTROL       0200		/* lower numbers are 7-bit ascii */
#define META          0400
#define SPECIAL	     07000		/* if c&SPECIAL c is a special key */
#define BUTTON1      01000		/* defines for mouse keys */
#define BUTTON2      02000
#define BUTTON3      03000
#define KEYPAD_CEN   04000
#define N_ARROW     010000
#define NE_ARROW    010001
#define E_ARROW     010002
#define SE_ARROW    010003
#define S_ARROW	    010004
#define SW_ARROW    010005
#define W_ARROW     010006
#define NW_ARROW    010007

#define c_disp curs->_cdisp
#define c_xs curs->_xs
#define c_ys curs->_ys
#define c_marker curs->_cmarker
#define c_val curs->_cval
extern int cursor_key;

/*****************************************************************************/
/*
 * Defines for the display structure
 */
#define d_xoff display->_xoff
#define d_yoff display->_yoff
#define d_xlow display->_dxlow
#define d_ylow display->_dylow
#define d_xhigh display->_dxhigh
#define d_yhigh display->_dyhigh
#define d_wxcen display->_wxcen
#define d_wycen display->_wycen
#define d_wzoom display->_wzoom
#define d_cnum display->_cnum
#define d_cden display->_cden
#define d_wrapflg display->_wrapflg
#define d_zorg display->_zorg
#define d_zrng display->_zrng
#define d_dmode display->_dmode
#define d_bufno display->_bufno
#define d_bufsno display->_bufsno
#define d_pfname display->_dpfname

#endif					/* !defined(IMAGES_H) */
