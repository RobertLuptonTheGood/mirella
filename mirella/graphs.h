/* graphs.h -- header for graphics code */

#define NGRAPH 8

/* STRUCTURES */

struct genv_t{
    short gxs;
    short gys;				/* pixel x,y sizes */   
    short gxl;
    short gyl;
    short gxh;
    short gyh;				/* plotting limits */
    short gxlp;
    short gylp;
    short gxhp;
    short gyhp;				/* plot box */
    
    normal gbitpix;                     /* bits per pixel 1,8,16,24 */
    normal gbitscol;                    /* bits of color: 1,3,6,8,15,16,or 24 */
    normal gxbsize;                     /* # bytes in a line */
 
    u_char ***gmcp;			/* memalloc pointer;
					 * gmcp is origin of pointer array
					 */
    u_char *gptr;			/* pointer to base of bitmap.
					   Both NULL if none allocated */
    struct v_point **vpptr;		/* memalloc pointer to vector array;
					   NULL if none allocated;
					   *vpptr is pointer to array */
    normal gcnvec;			/* current number of vectors in array*/
    normal gcnmax;			/* current size of allocated
					   vector buffer */
};

/* v_point is 8 bytes-- header is of size 2 v_points */
struct v_point{
    short gv_x;
    short gv_y;             /* location of point */
    u_char gv_color[3] ;    /* rgb of line starting here */
    u_char gv_pendown ;     /* nonzero if drawn TO here ???? */
};

struct v_header{
    normal v_nvec;
    normal v_xsize;
    normal v_ysize;
    normal v_scratch;
};  /* this is 16 bytes, size of 2 v_point entries, and use is made of that */


struct pnm_head_a{
    char pnm_magicnos[8];
    char pnm_xsizes[8];
    char pnm_ysizes[8];
    char pnm_maxgreys[8];
};

struct pnm_head_i{
    int pnm_magicno;
    int pnm_xsize;
    int pnm_ysize;
    int pnm_maxgrey;
    int pnm_data;
};


/* VARIABLES */

extern int grafmode;			/* flag for graphics mode */

#if 0  /* ????????????????? */
extern struct pnn_head_a pnm_ahead;     /* scratch pnm headers */
#endif
extern struct pnm_head_i pnm_ihead;

extern int g_vector ;                   /* create and maintain vector list*/
extern int g_bitmap ;                   /* create and maintain 'bit'map  */
                                        /* flags are set at creation of graph*/

extern int g_xsize, g_ysize;		/* size of bit array in pixels */
extern int g_xbsize;			/* size of x axis in bytes:
					   ((g_xsize-1)>>3) + 1 */
extern u_char *gpointer;		/* pointer to body of graph array */
extern u_char **mgraph;			/* pointer to line pointers;
					   graph byte is mgraph[y][x/8] */
extern u_char *g_mask;			/* the graphics mask */
extern int g_bitpix;                    /* bits per pixel--1,8,16, or 24 */
                                        /* `bit'map storage is uniquely set by
                                         * this parameter; convention is that 
                                         * if it is zero there is no map 
                                         */
extern int g_bitscol;                   /* bits of color-- 1,3,6,8,15,16,or 24
                                         * only 1,8,24 in use now 
                                         */
extern int gchan;			/* which graph ? */
extern int lweight;			/* Mongo-type line specifiers */
extern int ltype;
extern int g_xold;			/* old coordinates for rmdraw() */
extern int g_yold;
extern int g_xl,g_yl;
extern int g_xh,g_yh;			/* low and high corner for allowed
					   drawing area; bounds for ldraw() */
extern int g_xlp,g_xhp,g_ylp,g_yhp;	/* corners of current plotting box; 
					 * note that g_xl,g_xh are current
					 * plot limits which may or may not
					 * be same as g_xlp, g_xhp, etc, 
					 * depending on whether frame or box
					 * mode is in effect.
					 */
extern int g_xsp, g_ysp;                /* size of plotting box */
extern u_char g_lcolor[3];              /* current line color for vector map */
extern normal c_gcnvec;			/* current vector # and max #  */
extern normal c_gcnmax;
extern struct v_point **c_vpptr;	/* current memstruct ptr for vec list*/
extern struct v_point *mvgraph;		/* current vec array ptr = *c_vpptr */
                                        
extern struct genv_t g_env[];		/* up to NGRAPH graphs at once */

extern float g_xls ;
extern float g_xhs ;
extern float g_yls ;
extern float g_yhs ;      /* scaled limits in units of g_xsize,g_ysize for the
                           * plotting box; normally .26,.96,.15,.95 for 
                           * 4:3 screens 
                           */
                           
/* in mirplot.c */                          
extern float g_fxorg;     /* float x val at left edge of plot box */
extern float g_fyorg;     /* float y val at lower edge of plot box */
extern float g_fxhi;      /* float x value along right edge of plot box */
extern float g_fyhi;      /* float y value along top edge of plot box */
extern float g_fxscl;     /* float scale value along x; ie i = (x - xorg)*xscl*/
extern float g_fyscl;     /* float scale value along y; ie j = (y - yorg)*yscl*/
                          /* NB!!! scales are cells per delta value, not inv  */
/*
 * These are used for bitmap devices
 */
#if 0
/* replaced by a function in mirgraph.c */
#define SETPOINT(x,y) if((x)>=0 && (x)<g_xsize && (y)>=0 && (y)<g_ysize){ \
                    xb=(x)>>3; xbt = (x)&7; mgraph[y][xb] |= g_mask[xbt];}
                    
#define CLRPOINT(x,y) if((x)>=0 && (x)<g_xsize && (y)>=0 && (y)<g_ysize){ \
                    xb=(x)>>3; xbt = (x)&7; mgraph[y][xb] &= ~(g_mask[xbt]);}
/* note must have defined xb and xbt in routines which use these macros */ 
#endif
                    
extern int gd_xsize, gd_ysize;		/* size of display */
extern char *gd_base;			/* address of base of display memory */
extern normal gd_xbsize;		/* xsize of display in bytes */

/* FUNCTIONS */

extern void (*g_rdraw)();		/* raw drawing function */ 
extern void rm_draw();			/* raw memory bitmap drawing function*/
extern void nulldraw();
char *ggetpnmhead();                    /* general pnm header reader */
