/* jeg060410 */
/******************* MIRGRAPH.C *****************************************/
/*
 * module for making bitmap and vector graphs in a memory array;
 * mostly primitives
 * Begin adding byte graph support 04/10/31. The primitives are the
 * same independent of the number of colors, but we must find a way
 * to specify the colors. The main linegraph schemes are 3 bits, 8 colors, and
 * 6 bits, 64 colors, corresponding to 2 and 4 levels of each of the
 * primaries  (rgb). The x11 driver expects the bitmap to have byte
 * entries i :
 *
 *        r = (i & 3)*85 ;
 *        g = ((i & 12) >> 2) * 85 ;
 *        b = ((i & 48) >> 4) * 85 ;    85 = 255/3
 *
 *        i = (r >> 6) + ((g >> 6)<<2) + ((b >> 6)<<4)  r,g,b in (0,255)
 *
 *        r = (i & 1)*255 ;
 *        g = ((i & 2) >> 1) * 255 ;
 *        b = ((i & 4) >> 2) * 255 ;
 *
 *        i = (r >> 7) + ((g >> 7)<<1) + ((b >> 7)<<2)  r,g,b in (0,255)
 *  
 *        also want a standard 256-color rgb w/3 bits rg and 2 bits b, for
 *        compatibility with old code, or is it worth it? Or do we just
 *        want a lut? need to find out about 15 and 16 bit X representations.
 *
 * for the 64 and 8 color cases, respectively, where i is the graph data
 * value. Probably simplest is to specify 0-255 values for each of 
 * rgb and translate based on number of colors, which we can keep in
 * a global stored in the bitmap header (2,8,or 64) Try to make system
 * not care whether it is a bitmap or bytemap, and vectors should map
 * into any type...and going to more than 256 colors should also be
 * possible transparently.
 *
 * We will implement first a scheme which necessitates using an external
 * viewer, and which produces/uses full-up .ppm files, with 24 bits per
 * pixel and full color coverage. Later, we will see what we will see.
 *                                   
 * 0506--removed VMS and vaxstation support
 */

#include "mirella.h"
#include "graphs.h"

/*
 * refresh versions--assigned to (*m_grefresh)()
 */
/*  static normal rgtoscr();
    static normal rredscr(); */

/*
 * exported variables
 */
int g_vector=0;             /* create and maintain vector list*/
int g_bitmap=0;             /* create and maintain 'bit'map  */
                            /* flags are set at creation of graph 
                             * according to args to gallocgraph()  
                             */

int g_xsize, g_ysize;       /* size of graph array in pixels. */
int g_xbsize;               /* size of x axis in bytes:
                             * ((g_xsize-1)>>3) + 1 for bitmaps
                             * g_xsize              for bytemaps
                             * 2*g_xsize            for wordmaps
                             * 3*g_xsize            for truemaps
                             */
u_char *gpointer;           /* pointer to body of bitmat graph array */
u_char **mgraph;            /* pointer to line pointers in bitmap; 
                             * graph byte is
                             *      mgraph[y][x/8]   for bitmaps
                             *      mgraph[y][x]     for bytemaps
                             *      mgraph[y][2*x]   for wordmaps
                             *      mgraph[y][3*x]   for truemaps
                             */
int g_bitpix;               /* bits per pixel--1,8,16, or 24 */
                            /* `bit'map storage is uniquely set by this
                               parameter; convention is that if it is zero
                               there is no map */
int g_bitscol;              /* bits of color-- 1,3,6,8,15,16,or 24 */
int gchan = -1;             /* which graph ? */
int lweight = 1;            /* Mongo-type line specifiers */
int ltype = 0;

int g_xold = 0;             /* old coordinates for rm_draw() */
int g_yold = 0;
int g_xl,g_yl;
int g_xh,g_yh;              /* low and high corner for allowed drawing area;
                                bounds for ldraw() function */
int g_xlp,g_xhp;
int g_ylp,g_yhp;            /* corners of current plotting box; 
                             * note that g_xl,g_xh are current
                             * plot limits which may or may not
                             * be same as g_xlp, g_xhp, etc, 
                             * depending on whether frame or box
                             * mode is in effect.
                             */
int g_xsp,g_ysp;            /* x and y sizes of the plot box. calculated */
u_char g_lcolor[3] = {0xff,0xff,0xff}; 
                            /* current drawing color. We do something
                               rather strange with this. The colors are SET
                               with this scheme, but we use the same array
                               for storage of the working color, which
                               may be a byte or a short (or the original
                               triad of bytes. This may not be a good idea,
                               and MAY have byteorder issues, but I think
                               not the way we do it. See the code in 
                               setpoint(). I think we should keep a pristine
                               copy */
u_char *g_pcolor = g_lcolor; 
                            /* pointer for export to Mirella; see above
                               discussion; it can be a pointer to anything */
                               
normal c_gcnvec=0;          /* current vector # and max #  */
normal c_gcnmax=0;
struct v_point **c_vpptr=0; /* current memstruct pointer for vector list */
struct v_point *mvgraph=0;  /* current vector array pointer = *c_vpptr +2;
                                the first 12 bytes are for size information */
u_char *fontorigin0;        /* jeg0201 origin of pointer array into font table*/
u_char *fontorigin1;        /* jeg0102 origin of font offsets, etc-- nstroke */

/* imported variables */
                                
/* statics and static structure definitions */

int sizvpt = sizeof(struct v_point); /* 8 bytes */

/* the vector palette is a square g_xsize by g_ysize, each smaller than
32768 points; the pendown byte is 0 for pen-up motions (for which the
color values are irrelevant). The colors are 8-bit rgb.
*/

#if 0  /* defined in graphs.h */

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
					 *gmcp is origin of pointer array*/
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


#endif /* 0 */

struct genv_t g_env[NGRAPH];          /* up to NGRAPH graphs at once */

static char *namgraph(), *vnamgraph();

/* mask for setting bits in monochrome graphs */
static u_char fmgmask[8] = {128,64,32,16,8,4,2,1};
u_char *g_mask = fmgmask;

u_short        isomask16 = 0x8000;       /* anding with clears low 15 bits,
                                          * leaves sign bit alone
                                          */
/* global */

extern void rm_draw();  /* raw memory draw for bitmaps & vector lists */
extern void nulldraw(); /* null function */
static void exp_vmem();
static void ltypeset();

void (*g_rdraw)() = nulldraw; /* drawing function vector, 
                                    one of the above */

/* following pointers are in sbrk()'d space allocated in mgs_init, and
used by the Mongo font arrays */

static u_char *nstroke;
static Signed char *ladj;
static Signed char *radj;
static unsigned short *pointer;
static Signed char *font;

/*********************** INIT_MGS() **************************************/

#define FONTSIZE 19712
void init_mgs()  /* initializes memory graphics system; there is code in 
                init_mem() in mirelmem to purge graphics memory entries
                from the allocation table at startup. It is crucial that
                the system be initialized; otherwise one will scribble
                all over memory with stray pointers. */
{
    register int i;
    int fdes;
    normal nrd ;
    normal fontblk;
    
    for(i=0; i< NGRAPH; i++){
        g_env[i].gmcp = 0;
        g_env[i].vpptr = 0;
    }
    fontblk = ((FONTSIZE -1)/512 + 1)*512;
    nstroke = (u_char *)m_sbrk(fontblk);   /* need 19712, up to next 512 bdy*/
    if(nstroke == (u_char *)(-1)){
        scrprintf("\nCannot allocate font tables");
        exit(1);
    }
    /* now allocate memory for font file and load fonts */
    ladj = (Signed char *)(nstroke + 512);
    radj = (Signed char *)(ladj + 512);
    pointer = (unsigned short *)(radj + 512);
    font = (Signed char *)(pointer + 512);    /* note pointer arith. careful */
    fontorigin0 = (u_char *)pointer;
    fontorigin1 = (u_char *)nstroke;
    
    if((fdes = openblk(mcfname("mfont.dat"),O_RDONLY)) == -1){
        scrprintf("\nCannot open font file in Mirella directory");
    } else {
       nrd = readblk(fdes,(char *)nstroke,FONTSIZE);
       if(nrd < FONTSIZE) scrprintf("\nError reading font file, req=%d, read=%d",
				    FONTSIZE,nrd);
#ifdef IBMORDER
#ifdef DSI68
       for(i=0;i<512;i++) pointer[i] = swab(pointer[i]);
#else
       /* WARNING!!!! this works in bky unix on the sun, but may not on all
	  systems; it does not under IBMPC Turbo C, for example */
       swab((char *)pointer,(char *)pointer,1024);
#endif
/*debug printf("\nfont base, origin of ptr array = %d %d",nstroke,pointer);*/
#endif
       closeblk(fdes);
    }
    gd_init();    /* initialize the display device, if any */
}

/******************* PLOTINALL(), PLOTINFRAME(), PLOTBOX() *****************/
/*
 * sets plotting area to whole page
 */
void
plotinall()
{
    g_xl = 0;
    g_yl = 0;
    g_xh = g_xsize;
    g_yh = g_ysize;
    g_xsp = g_xh - g_xl ;
    g_ysp = g_yh - g_yl ;
}

void plotinframe()  /* sets plotting area to plot box */
{
    g_xl = g_xlp; 
    g_yl = g_ylp; 
    g_xh = g_xhp; 
    g_yh = g_yhp;
    g_xsp = g_xh - g_xl; 
    g_ysp = g_yh - g_yl;
}

void plotbox(xl,yl,xh,yh) /* sets plotting box boundaries--note that
                            this is for reference only; the physical
                            boundaries outside of which ldraw will not
                            draw are g_xl, etc */
int xl, yl, xh, yh;
{
    if(xl < 0 || yl < 0 || xh > g_xsize || yh > g_ysize){
        erret("\nIllegal plotting boundaries");
    }
    g_xlp = xl; g_ylp = yl; g_xhp = xh; g_yhp = yh;
    g_xsp = xh - xl; g_ysp = yh - yl;
}

/************** ALLOCGRAPH(),SETGRAPH(),FREEGRAPH(),G_ISALLOC() **************/
/*
 * allocate graphics memory for channel n and *sets graph pointer* there
 * bitscol ONLY affects 'bit'mapped graphs; vector graphs always have
 * 24 bits of color. bitscol=0 results in no bitmap memory being allocated
 */

void
gallocgraph(xs,ys,n,vecflg,bitscol)
int xs,ys;    /* size */
int n;        /* channel */
int vecflg;   /* vector ? */
int bitscol;  /* NB!! this is used to set bitpix and hence storage, 0=nomap */
{
    u_char ***mp;
    struct v_point **vp;
    struct v_header *vhp;
    int xsb;
    
    irchk(n,0,NGRAPH,"No such graph");
    if((g_env[n].gmcp && *(g_env[n].gmcp)) ||
       (g_env[n].vpptr && *(g_env[n].vpptr))){
        /* there is already a graph in channel n. Free it. Should you ask??*/
        freegraph(n);
    }
    switch(bitscol){
    case 24:
        g_bitpix = g_env[n].gbitpix = 24;
        break;
    case 16:
    case 15:
        g_bitpix = g_env[n].gbitpix = 16;
        break;
    case 8:
    case 6:
    case 3:
        g_bitpix = g_env[n].gbitpix = 8;
        break;
    case 1:
        g_bitpix = g_env[n].gbitpix = 1;
        break;
    case 0:
        g_bitpix = g_env[n].gbitpix = 0;  /* no 'bit'map */
        break;
    default:
        perror("Gallocgraph: Illegal bitscol: must be one of 1,3,6,8,15,16,24");
        erret((char *)NULL);
        break;
    }
    g_bitscol = g_env[n].gbitscol = bitscol;
    

    /* set up general parameters */
    switch(g_bitpix){
    case 1:  
        xsb = ((xs-1)>>3) + 1; 
        g_xsize = xsb*8;   /* force even number of bytes */
        break;
    case 8:  xsb = xs; break;
    case 16: xsb = xs*2; break;
    case 24: xsb = xs*3; break;
    }

    g_env[n].gxbsize = g_xbsize = xsb;
    g_env[n].gxs = g_xsize = xs;
    g_env[n].gys = g_ysize = ys;
    gchan = n;

    g_lcolor[0]=g_lcolor[1]=g_lcolor[2]=0xff;  /* full on */

    /* bitmaps */
    if(g_bitpix != 0){
        mp = (u_char ***)matralloc(namgraph(n),ys,xsb,1);
        /* this allocation is correct; the element sizes are subsumed
            in xsb which is set above */
        if(!mp) erret((char *)NULL);
        g_bitmap = 1;
        g_env[n].gmcp = mp;   /* so you can memfree it */
        mgraph = *mp;        

/*debug*/ printf(
    "\nsetting up graph %d, size %dx%d bytes, xs=%d pointer %u  %d bits",
                    n,ys,xsb,xs,(unsigned)mp,bitscol); fflush(stdout);       

        g_env[n].gptr = gpointer = mgraph[0];        

    }
    if(vecflg){         /* note that it is OK to 
                           maintain both modes for a given graph */
        vp = (struct v_point **)memalloc(vnamgraph(n),
            5000*sizeof(struct v_point));
        /* initially reserve space for 5000 vectors */
        if(!vp) erret((char *)NULL);
        g_vector = 1;
        g_env[n].vpptr = vp;
        g_env[n].gcnmax = 5000;
        g_env[n].gcnvec = 0;
        c_vpptr = vp;
        c_gcnvec = 0;
        c_gcnmax = g_env[n].gcnmax;
        mvgraph = *vp + 2 ; /* N.B. !!!! first 2 loc reserved for number,size*/ 
        vhp = (struct v_header *)(*vp);
        vhp->v_xsize = g_xsize;
        vhp->v_ysize = g_ysize;
        vhp->v_nvec = 0;      /* these occupy the space of the first two
                                 entries in the vector table, to facilitate disk
                                 storage */
#ifdef ADEBUG
        scrprintf("xs,ys,mvgraph-2,mvgraph,header=%d %d %d %d %d %d %d ",
            g_xsize,g_ysize,mvgraph-2,mvgraph,*(normal *)(mvgraph-2),
            *((normal *)(mvgraph-2) + 1), *((normal *)(mvgraph-2) + 2) );
#endif
    }
    g_rdraw = rm_draw;
    plotinall();
    plotbox(0,0,g_xsize,g_ysize);
}

/* old call for monochrome bitmap graph; keep it */
void
allocgraph(xs,ys,n)
int xs,ys;
int n;
{
    gallocgraph(xs,ys,n,0,1);
}


/********************** SETGRAPH() ***********************************/
void setgraph(n)   /* sets graph "channel" to n; there must be a graph there. 
                   saves parameters of old graph that have not been saved
                   already at creation in structure array. */
int n;
{
    u_char ***mp; 
   
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp == (u_char ***)0 || *(g_env[n].gmcp) == (u_char **)0){ 
        scrprintf("Graph %d not allocated",n);
        erret((char *)NULL);
    }
    if(gchan != -1){ /*there is an active graph; save its volatile parameters*/
        /* I think this is not a good idea; the graph parameters should be
            settable only in allocgraph???? Dunno---good question.*/
        g_env[gchan].gxl = g_xl;
        g_env[gchan].gyl = g_yl;
        g_env[gchan].gxh = g_xh;
        g_env[gchan].gyh = g_yh;
        g_env[gchan].gxlp = g_xlp;
        g_env[gchan].gylp = g_ylp;
        g_env[gchan].gxhp = g_xhp;
        g_env[gchan].gyhp = g_yhp;
        g_env[gchan].gbitpix = g_bitpix;
        g_env[gchan].gbitscol = g_bitscol;
        g_env[gchan].gxbsize = g_xbsize;
        g_env[gchan].gcnvec = c_gcnvec;  /* vpptr and gcnmax are updated as
                                            neccesary by exp_vmem() */
    }
    g_xsize = g_env[n].gxs;
    g_ysize = g_env[n].gys;
    g_xl = g_env[n].gxl ;
    g_yl = g_env[n].gyl ;
    g_xh = g_env[n].gxh ;
    g_yh = g_env[n].gyh ;
    g_xlp = g_env[n].gxlp ;
    g_ylp = g_env[n].gylp ;
    g_xhp = g_env[n].gxhp ;
    g_yhp = g_env[n].gyhp ;
    g_xsp = g_xhp - g_xlp;
    g_ysp = g_yhp - g_ylp;
    g_bitpix  = g_env[n].gbitpix;
    g_bitscol = g_env[n].gbitscol;
    g_bitmap  = (g_env[n].gmcp != 0 && *(g_env[n].gmcp) != 0) ? 1 : 0 ;
    g_lcolor[0]=g_lcolor[1]=g_lcolor[2]=0xff;  /* full on */
    if(g_bitmap){
        mp = g_env[n].gmcp ;
        gpointer = g_env[n].gptr;
        mgraph = *mp;
        g_xbsize = g_env[n].gxbsize;
    }else{
        mp = 0;
        gpointer = 0;
        mgraph = 0;
    }
    g_vector = (g_env[n].vpptr != 0 && *(g_env[n].vpptr) != 0 );
    if(g_vector){
        c_vpptr = g_env[n].vpptr;
        mvgraph = *c_vpptr + 2;
        c_gcnvec = g_env[n].gcnvec;
        c_gcnmax = g_env[n].gcnmax;
    }else{
        mvgraph = 0;
        c_vpptr = 0;
    }
    gchan = n;
    g_rdraw = rm_draw;
    plotinall();
}

/************************* FREEGRAPH()*********************************/
void
freegraph(n)
int n;
{
    irchk(n,0,NGRAPH,"No such graph");
    if((g_env[n].gmcp == (u_char ***)0  || *(g_env[n].gmcp) == (u_char **)0)
            &&(g_env[n].vpptr == 0 || *(g_env[n].vpptr) == 0 )){ 
        scrprintf("Graph already free");    
        return;
    }
    if(g_bitmap) chfree((Void *)g_env[n].gmcp);
    if(g_vector) chfree((Void *)g_env[n].vpptr);
    g_env[n].gmcp = 0;    
    g_env[n].vpptr = 0;  /* init env. structure */
    if(gchan == n){  /* current graph*/
        g_rdraw = nulldraw;
        mgraph = 0;
        mvgraph = 0;
        gpointer = 0;
        c_vpptr = 0;
        g_bitmap = 0;
        g_vector = 0;   /* these are necessary to ensure that freed
                            graphics memory is not written to */
        gchan = -1;     /* jeg080605 */                            
    }
}
/********************* G_ISALLOC() *************************************/

int g_isalloc(n)  /* rets 1 if graph n exists and is allocated, 0 if
                    not */
int n;
{                    
    irchk(n,0,NGRAPH,"No such graph");
    if((g_env[n].gmcp == (u_char ***)0  || *(g_env[n].gmcp) == (u_char **)0)
        &&(g_env[n].vpptr == 0 || *(g_env[n].vpptr) == 0 )) return 0;
    else return 1;
}
/*************** EGRAPH(), SHOWGRAPHS() *****************************/

void egraph()   /* erases current graph */
{
    register u_char *cp;
    register int n;
    irchk(gchan,0,NGRAPH,"Illegal graphics channel");
    if((g_env[gchan].gmcp == 0 || *(g_env[gchan].gmcp) ==0) 
        &&(g_env[gchan].vpptr == 0 || *(g_env[gchan].vpptr) == 0)) 
        erret("NO GRAPH");
    if(g_bitmap){
        cp = gpointer;
        n = g_ysize*g_xbsize;  /* OK */
        while(n--) *cp++ = 0;
    }
    if(g_vector){
        c_gcnvec = 0;    /* simply trucate vector list--perhaps should
                            reduce allocation as well; will leave for now */
        *(normal *)(mvgraph-2) = 0;
    }
}

void showgraphs()
{
    int i;
    u_char **bmp;
    struct v_point *vp;
    
    mprintf(
"\nGRAPH                 xsize ysize   size     bitscol      bmptr      vecptr"
    );
    for(i=0;i<NGRAPH;i++){
        if((g_env[i].gmcp && *(g_env[i].gmcp)) || 
            (g_env[i].vpptr && *(g_env[i].vpptr))) {
            bmp = (g_env[i].gmcp ? *(g_env[i].gmcp) : 0 );
            vp = (g_env[i].vpptr ? *(g_env[i].vpptr) : 0 );
            mprintf("\n%-20s %5d %5d %7d  %9d  %10u  %10u",
                namgraph(i),g_env[i].gxs, g_env[i].gys,
                g_env[i].gys*g_env[i].gxbsize,
                g_env[i].gbitscol,bmp,vp );
        }
    }
    fflush(stdout);
}        


static char nambuf[10] = "graph_";   /* number is in 6 */
static char vnambuf[10] = "vgraph_"; /* number is in 7 */

static char *
namgraph(n)
int n;
{
    sprintf(nambuf+6,"%d",n);
    return (nambuf) ;
}

static char *
vnamgraph(n)
int n;
{
    sprintf(vnambuf+7,"%d",n);
    return (vnambuf) ;
}

/******************** RM_DRAW(), NULLDRAW() **********************************/
#define ABS(x) ( (x) > 0 ? (x) : -(x) )

/* this function is useful for lines, etc, for which the color does
 * not change; pictures are best dealt with with the function gpoint()
 * below. These are all *bitmap* functions.
 
 * thots..make a 32 bit color mask, which can be used to set the color.
 * can we do this? Yes, and with mask which has 0s in all the bits in
 * which we are interested, 1s in the rest. then or with color mask,
 * which has 0s in all bits 
 */
 
static void setpoint(x,y)
int x, y;
{
    register u_char *pixptr, *colptr;
    register int n;
    u_short val;
    
    switch(g_bitscol){
    case 24:
        colptr = g_lcolor;
        pixptr = mgraph[y] + x + (x<<1);
        n=3;
        while( n-- ) *pixptr++ = *colptr++ ;
        break;
    case 16:
        *((short *)(mgraph[y]) + x) = *(short *)g_lcolor;
        break;
    case 15:
        val = *((short *)(mgraph[y]) + x) ;
        *((short *)(mgraph[y]) + x) = ((val & isomask16) | *(short *)g_lcolor);
        break;
    case 8:
    case 6:
    case 3:
        mgraph[y][x] = *g_lcolor;
        break;
    case 1:
        n = x&7;
        mgraph[y][x>>3] |= g_mask[n];
        break;
    default:
        mprintf("\nbitscol=%d is not a legal value",g_bitscol);
        erret((char *)NULL);
    }
}

/* primitive point setter, just one point of current color. cf gpoint() */
void g_setpoint(int x, int y)
{
    setpoint(x,y);
}

/* sets line color. Primarily aimed at line graphs */

void ilinecolor(color)
char *color;   /* bytes rgb */
{
    switch(g_bitscol){
    case 24:
        g_lcolor[0]=color[0];
        g_lcolor[1]=color[1];
        g_lcolor[2]=color[2];
        break;
    case 16:
        *(short *)g_lcolor = (color[0] >> 3) + ((color[1] >> 2)<<6) +
            ((color[2] >> 3)<<11);
        break;            
    case 15:
        *(short *)g_lcolor = (color[0] >> 3) + ((color[1] >> 3)<<5) +
            ((color[2] >> 3)<<10);
        break;            
    case 8:   /* we assume for now that 8 bits is grey, and that
               * the intensity is in the first color byte (or all, but
               * we just look at the first one)
               */
        g_lcolor[0] = color[0];
        break;
    case 6:
        g_lcolor[0] = (color[0] >> 6) + ((color[1] >> 6)<<2) + 
            ((color[2] >> 6)<<4);
        break; 
    case 3:
        g_lcolor[0] = (color[0] >> 7) + ((color[1] >> 7)<<1) + 
            ((color[2] >> 7)<<2);
        break; 
    default:
        mprintf("\n%s","Bitscol=%d is illegal or as yet unsupported");
        erret((char *)NULL);
        break;  /* never reached */
    }

}

/* gpoint() routines. For now, one for each common graph type for
 * efficiency. Deal with 15/16bit color later if at all.
 * these check for validity of location and force 8-bit values
 *
 * Could be for generality that we just want a routine that
 * takes x,y,pointer and does what it does on the basis of 
 * the known nature of the graph. That would be better, if perhaps
 * a little slower (there would have to be a switch). These do not
 * check, for example, that the graph is the correct type.
 */
 
void 
pgmgpoint(x,y,value)
    int x;
    int y;
    int value;
{
    if((x)>=0 && (x)<g_xsize && (y)>=0 && (y)<g_ysize){
        mgraph[y][x]=(value&0xff);
    }
}               

/* this one takes a triad of rgb values */
void 
ppmgpoint1(x,y,r,g,b)    
    int x;
    int y;    
    int r;
    int g;
    int b;
{
    register u_char *p = mgraph[y] + x + (x<<1);
    if((x)>=0 && (x)<g_xsize && (y)>=0 && (y)<g_ysize){
        *p++ = (r & 0xff);
        *p++ = (g & 0xff);
        *p   = (b & 0xff);
    }
}

/* this one takes a pointer to a color vector */
void 
ppmgpoint2(x,y,color)    
    int x;
    int y;    
    u_char *color;
{
    register u_char *p = mgraph[y] + x + (x<<1);
    register int n=3;
    
    if((x)>=0 && (x)<g_xsize && (y)>=0 && (y)<g_ysize){
        while(n--) *p++ = *color++;
    }
}

/* and the corresponding routines to plot in the plot box, with coordinates
 * referenced to the plot box corner. These routines check for validity
 * of location and force value to 8 bits
 */

void 
pgmplpoint(x,y,value)
    int x;
    int y;
    int value;
{

    if((x)>=0 && (x)<g_xsp && (y)>=0 && (y)<g_ysp){
        mgraph[y+g_ylp][x+g_xlp]=(value&0xff);
    }
}               

/* this one takes a triad of rgb values */
void 
ppmplpoint1(x,y,r,g,b)    
    int x;
    int y;    
    int r;
    int g;
    int b;
{
    register u_char *p;
    x += g_xlp;
    p = mgraph[y+g_ylp] + x + (x<<1);


    if((x)>=0 && (x)<g_xsp && (y)>=0 && (y)<g_ysp){
        *p++ = (r & 0xff);
        *p++ = (g & 0xff);
        *p   = (b & 0xff);
    }
}

/* this one takes a pointer to a color vector */
void 
ppmplpoint2(x,y,color)    
    int x;
    int y;    
    u_char *color;
{
    register u_char *p; 
    register int n=3;
    
    x += g_xlp;
    p = mgraph[y+g_ylp] + x + (x<<1);
    
    if((x)>=0 && (x)<g_xsp && (y)>=0 && (y)<g_ysp){
        while(n--) *p++ = *color++;
    }
}


/*XXXXXXXX FIXME--this is the basic routine. incorporate color, knowledge
of kind of graph, etc, etc,
I think all we need to do is to generalize SETPOINT--make it a function,
with all the necessary knowledge. Need g_lcolor, current point color.*/

void 
rm_draw(sx,sy,pen)    /* basic raw memory line drawer; 
                             Tek 4010 conventions. Draws single weight
                             line from g_xold, g_yold to sx,sy. If g_bitmap,
                             makes bitmap; if g_vector, makes vector list;
                             if both, both */
int sx,sy;
int pen;
{
    register int x,y;
    int delx, dely, adel, step=1;
/*    register int xb, xbt; */  /* used in SETPOINT() macro */
    int i;

    if(g_vector){
        mvgraph[c_gcnvec].gv_x = sx;
        mvgraph[c_gcnvec].gv_y = sy;
        for( i=0; i<3; i++){
            mvgraph[c_gcnvec].gv_color[i] = (pen ? g_lcolor[i] : 0);
        }
        mvgraph[c_gcnvec].gv_pendown = pen;
        c_gcnvec++;
        *(normal *)(mvgraph-2) = c_gcnvec;
        if(c_gcnvec >= c_gcnmax) exp_vmem(0);  /* get more memory */
    }
    if(!pen || !g_bitmap) goto out;
    
    /* pen down(1). draw a line. Do not care whether points are in box or not */
    delx = sx - g_xold;
    dely = sy - g_yold;
    if(ABS(delx) >=  ABS(dely)){
        if(delx <0) step = -1;
        adel = ABS(delx);
        if(!adel){               /* final point same as init */
            setpoint(sx,sy);
            g_xold = sx;
            g_yold = sy;
            return;
        }
        for(x=g_xold; x != sx; x += step){
            y = g_yold + (dely*(x-g_xold))/delx ;
            setpoint(x,y);
        }
    }else{
        if(dely <0) step = -1;
        adel = ABS(dely);
        for(y=g_yold; y != sy; y += step){
            x = g_xold + (delx*(y - g_yold))/dely ;
            setpoint(x,y);
        }
    }
out:    
    g_xold = sx;
    g_yold = sy;
}

void nulldraw(x,y,pen)
int x,y,pen;
{ }

/*********************** EXP_VMEM() ************************************/
/*
 * get more vector memory
 */
static void
exp_vmem(flg)
int flg;                     /* if 0, doubles memory; if >0, gets that
                                many vector locations */
{
    int wanted = (flg ? flg : 2*c_gcnmax);
    struct v_point **vpptr;
    
    if(wanted <= c_gcnmax) return;
    scrprintf("WAIT: enlarging vector list\n");
    vpptr = (struct v_point **)memalloc(vnamgraph(gchan),
                                          wanted*sizeof(struct v_point));
    if(c_vpptr == 0){
        scrprintf("\nCannot enlarge vector list; turning off vector drawer");
        g_vector = 0;   /* turn off vector drawer */
        return;
    }
    bufcpy((char *)*vpptr,(char *)*c_vpptr,c_gcnmax*sizeof(struct v_point));
    chfree((Void *)c_vpptr);
    g_env[gchan].vpptr = vpptr;
    c_gcnmax = wanted;
    g_env[gchan].gcnmax = c_gcnmax;
    c_vpptr = vpptr ;
    mvgraph = *vpptr + 2;   /* first loc is number, recall */
}
    

/********************  DRAW(), GPOINT() ***********************************/

/* this routine draws within (g_xl,g_yl),(g_xh,g_yh) box and
treats the boundaries intelligently; ie it draws along the desired
vector TO THE BOUNDARY and stops; the saved previous value on the next
call is the input value for the last one, so the points are always where
you think they are, whether or not they are in the box and hence
actually drawn. This routine draws a single-weight solid line; ldraw, at
one layer higher, draws a line of weight lweight and type ltype.
draw() uses the raw drawing function (*g_rdraw)() */

#define INSP(x,y) ( (x) >= g_xl && (y) >= g_yl && (x) <= g_xh && (y) <= g_yh ) 
/* point inside plotting boundary */

void 
draw(x,y,pen)  /* checks range to plot only within PLOT boundary;
                        plots to boundary if instruction was to cross */
int x;
int y;
int pen;
{
    int xb,yb,xin,yin,xout,yout,dx,dy;
    int inn = INSP(x,y);
    int ino = INSP(g_xold,g_yold);
    
    if( inn && ino && pen){    /* both in, pen down */
        (*g_rdraw)(x,y,1);
        return;
    }
    if((!inn && !ino) || !pen ){   /* neither in or pen up  */
        (*g_rdraw)(x,y,0);
        return;
    }
    /* OK, one in, one out, pen down. Now the hard part. */
    if(inn){ xin = x; yin = y; xout = g_xold; yout = g_yold; }
    if(ino){ xin = g_xold, yin = g_yold; xout = x; yout = y; }
    dx = xout - xin;
    dy = yout - yin;
    if(dx > 0){   /* line goes to right */
        yb = yin + dy*(g_xh-1-xin)/dx;
        xb = g_xh-1;
        if(yb < g_yl){  /* note guarantees dy != 0 */
            xb = xin + dx*(g_yl-yin)/dy;
            yb = g_yl;
        }else if(yb >=g_yh){  /* ditto */
            xb = xin + dx*(g_yh-1-yin)/dy;
            yb = g_yh-1;
        }
    }else if(dx < 0){  /* line goes to left */
        yb = yin + dy*(g_xl-xin)/dx;
        xb = g_xl;
        if(yb < g_yl){
            xb = xin + dx*(g_yl-yin)/dy;
            yb = g_yl;
        }else if(yb >= g_yh){
            xb = xin + dx*(g_yh-1-yin)/dy;
            yb = g_yh-1;
        }
    }else{  /* line is vertical */
        if(dy > 0){ xb = x; yb = g_yh-1;}
        else      { xb = x; yb = g_yl;  }
    }
    if(inn){
        (*g_rdraw)(xb,yb,0);
        (*g_rdraw)(x,y,1);
    }else{
        (*g_rdraw)(xb,yb,1);
        (*g_rdraw)(x,y,0);
    }
    return;
}


/************************** LDRAW() **************************************/
static int pcount;

/* FIXME --- The pattern counter stuff is ALL screwed up; may always have 
 * been
 */

void ldraw(sx,sy,pen)   /* draws a line of type ltype and weight lweight 
                        you must draw to the first point with ldraw with
                        pen = 0 to reset the pattern counters */
int sx,sy,pen;
{
    int drawint;
    int patint;
    int delx,dely,adx,ady,adel; 
    int xb,yb,xe,ye;
    int scr;
    int i;
    int firstx = g_xold;
    int firsty = g_yold;
    int endflg = 0;
    int xdir,ydir;
    int xb0, yb0;      /* first marked point on this call */
    
    if(!pen){
        pcount = 0;
        draw(sx,sy,0);
        return;
    }
    delx = sx - firstx;
    dely = sy - firsty;
    adx = ABS(delx);
    ady = ABS(dely);
    xdir = (delx < 0 ? -1 : 1);
    ydir = (dely < 0 ? -1 : 1);
    adel = (adx > ady ? adx : ady);
    if(adel==0){
        gpoint(sx,sy);
        return;
    }
    if(ltype) ltypeset(delx,dely,&drawint,&patint);
    else drawint = patint = 1000000;

    if(adx>=ady){   /* line q_horizontal */
        xb0 = firstx - xdir*pcount;
        yb0 = firsty - ydir*((pcount*ady)/adx);
        if(pcount > drawint){
            xb0 += xdir*patint;
            yb0 += ydir*((patint*ady)/adx);
        }
        for(i=0;!endflg;i++){
            xb = xb0 + xdir*i*patint;
            yb = yb0 + ydir*((i*patint*ady)/adx);
            xe = xb + xdir*drawint ;
            ye = yb + ydir*((drawint*ady)/adx);
            if(i == 0 && pcount <= drawint){     /* begins in draw mode */
                xb = firstx;
                yb = firsty;
            }
            scr = xb - firstx;
            if(ABS(scr) > adx){ /* first point is beyond end */
                scr = sx-g_xold;   /* nb g_xold updated by last draw*/
                pcount += ABS(scr);  
                pcount %= patint; 
                draw(sx,sy,0);
                endflg = 1;
            }else{    /* something to draw */
                if(i != 0) pcount += (patint - drawint);  /* count the spaces */
                scr = xe - firstx;
                if(ABS(scr) > adx){  /* line would extend beyond end */
                    xe = sx;
                    ye = sy;
                    endflg = 1;
                }
                if(xb == xe) gpoint(xb,yb);
                else{
                    if(lweight >1){
                        draw(xb,yb+1,0);
                        draw(xe,ye+1,1);
                    }
                    if(lweight > 2){
                        draw(xb,yb-1,0);
                        draw(xe,ye-1,1);
                    } 
                    draw(xb,yb,0);
                    draw(xe,ye,1);
                    scr = xe-xb;
                    pcount += ABS(scr);    
                }
                pcount %= patint;
            }
        }
    }else{         /* line q_vertical; same code with x,y exchanged */
        yb0 = firsty - ydir*pcount;
        xb0 = firstx - xdir*((pcount*adx)/ady);
        if(pcount > drawint){
            yb0 += ydir*patint;
            xb0 += xdir*((patint*adx)/ady);
        }
        for(i=0;!endflg;i++){
            yb = yb0 + ydir*i*patint;
            xb = xb0 + xdir*((i*patint*adx)/ady);
            ye = yb + ydir*drawint ;
            xe = xb + xdir*((drawint*adx)/ady);
            if(i == 0 && pcount <= drawint){     /* begins in draw mode */
                yb = firsty;
                xb = firstx;
            }
            scr = yb - firsty;
            if(ABS(scr) > ady){ /* first point is beyond end */
                scr = sy-g_yold;
                pcount += ABS(scr);  /* nb g_yold updated by last draw*/
                pcount %= patint;
                draw(sx,sy,0);
                endflg = 1;
            }else{    /* something to draw */
                if(i != 0) pcount += (patint - drawint);  /* count the spaces */
                scr = ye - firsty;
                if(ABS(scr) > ady){  /* line would extend beyond end */
                    xe = sx;
                    ye = sy;
                    endflg = 1;
                }
                if(yb == ye) gpoint(xb,yb);
                else{
                    if(lweight >1){
                        draw(xb+1,yb,0);
                        draw(xe+1,ye,1);
                    }
                    if(lweight > 2){
                        draw(xb-1,yb,0);
                        draw(xe-1,ye,1);
                    } 
                    draw(xb,yb,0);
                    draw(xe,ye,1);
                    scr = ye-yb;
                    pcount += ABS(scr);    
                }
                pcount %= patint;
            }
        }
    }
    return;
}

/*
 * sets length of pattern for different ltypes
 */
static void
ltypeset(dx,dy,pdraw,ptot)
int dx,dy;   /* line slopes */
int *pdraw, *ptot;   /* length of dash, total length of pattern */
{
    float rat;
    float unit;
    
    rat = (fabs((double)dx)+ 0.01)/(fabs((double)dy) + 0.01) ;
    if(rat > 1.0) rat = 1./rat;
    rat = 1./sqrt(1.+rat*rat);     /* cos of angle of line to nearest axis */
    unit = g_ysize/40 + 1;         /* length of short dash */
    switch(ltype){
        case 1:  /* dots */
            *pdraw = 0;
            *ptot = rat*unit;
            break;
        case 2:  /* short dashes */
            *pdraw = rat*unit;
            *ptot = 2*(*pdraw);
            break;
        case 3:   /* long dashes */
            *pdraw = 2*rat*unit;
            *ptot = 3*rat*unit;
            break;
        case 0:
        default:
            *pdraw = *ptot = 1000000;
            break;
    }
    return;
}

/******************** GPOINT() **********************************/
            
void gpoint(x,y)
register int x,y;
{
    int xc = x;
    int yc = y; 
   
    draw(x,y,0);
    if(lweight == 2){
        draw(x,++y,1); draw(++x,y,1); draw(x,--y,1);
    }else if(lweight >= 3){
        draw(x,++y,1); draw(++x,--y,1); draw(--x,--y,1); draw(--x,++y,1);
    }
    draw(xc,yc,1);
    g_xold = xc;
    g_yold = yc;
}


/***************** EXGRAPH(), INSGRAPH() **********************************/
/* these functions work on BITMAPS only */

/*XXXXXX--fixme so works with all types of graphs, maybe even converts
on the fly.???? */

void exgraph(x0,y0,xsz,ysz,n)   /* extracts a graph of size xsz,ysz at x0,y0
                                and puts it into graph n. Does no range
                                checking. Be careful */
int x0,y0;
int xsz,ysz;
int n;
{
    u_char **og = mgraph;
    register int j, c, m;
    int i, iy0;
    
    if(!g_bitmap) erret("\nThis graph has no bitmap data");
    allocgraph(xsz,ysz,n);  /* allocate new graph. changes mgx... */
    for(i=0;i<ysz;i++){
        iy0 = i + y0; 
        for(j=0;j<g_xsize;j++){
            m = j+x0;
            c = og[iy0][m>>3] & g_mask[m&7];    /* point ? */
            if(c) mgraph[i][j>>3] |= g_mask[j&7];
        }
    }
}

void insgraph(x0,y0,n)  /* inserts the present graph into graph n at x0,y0 */
int x0,y0;
int n;
{
    int xsz = g_xsize;
    int ysz = g_ysize;
    u_char **og = mgraph;
    int i,iy;
    register int j,jx,c ;
    
    if(!g_bitmap) erret("\nSource graph has no bitmap data. Use bdrawlist");    
    setgraph(n);   /* changes mgx...*/
    if(!g_bitmap) erret("\nDest. graph has no bitmap data. Use bdrawlist");
    if(x0 + xsz > g_xsize) xsz = g_xsize - x0;
    if(y0 + ysz > g_ysize) ysz = g_ysize - y0;
    for(i=0;i<ysz;i++){    
        iy = i + y0;
        for(j=0;j<xsz;j++){
            jx = j + x0;
            c = og[i][j>>3] & g_mask[j&7];
            if(c) mgraph[iy][jx>>3] |= g_mask[jx&7];
            else mgraph[iy][jx>>3] &= ~(g_mask[jx&7]);
        }
    }
}            

/************* VSTRING(), GSTRING(), GSTRLEN() ****************************/

/* this is a translation of Tonry's string handling routines from Mongo */

static int slant=51;         /* italic slope * 256 */
static int supfrac=154;      /* red to super or sub * 256 */

struct senv_t{
    int itflg;          /* italic flag */
    int ioffs;          /* font style offset */
    int isupr;          /* superscript level */
    int suprf;          /* superscript size factor * 256 */
    int suprsh;         /* superscript shift * 256 */
};
static struct senv_t senvtem = {0,0,0,256,0};

/*#define VDEBUG*/

#ifdef DEFUCHAR
#define SCHAR(b) ( ((cc = ( b ))&0x80) ? (int)(cc&0x7f) - 0x80 : (cc&0x7f) )
#else
#define SCHAR(b) ( b )
#endif

#ifdef LOGCSHIFT    /* system does logical shifts, not arith, so cannot
                        shift negative numbers */
#define RSH8(x)  (( x )/256)
#define RSH16(x) (( x )/65536)
#else
#define RSH8(x)  (( x )>>8)                        
#define RSH16(x) (( x )>>16)
#endif

static int 
vstring(string,angle,expand,drawit)  
/* if drawit, draws string from g_xold,g_yold at angle angle (deg) with
 * chars expanded by factor expand from default. If not drawit, just returns 
 * length of string in pixels, which it does in any case. */
char *string;
float angle;    /* in degrees */
float expand;   /* factor to apply to default size */
int drawit;     /* flag to draw; if not, just return length */
{
#ifdef DEFUCHAR
    register int cc;   /* used in macro SCHAR() */
#endif
    int xp = g_xold ;
    int yp = g_yold ;
    int i,j,c;
    int n = strlen(string);
    int co = cos(angle*PI/180.)*256.;
    int si = sin(angle*PI/180.)*256.;
    int x0 = xp;
    int y0 = yp;                    /* starting location */
    int slength = 0;                /* string length in ????? */
    int icomm = 0;                  /* "command" flag */
    
    struct senv_t senv ;            /* "environment"; font, etc */
    struct senv_t senvsave ;
    int oldtype;                    /* saved type  */

    int jchar;                      /* extended ASCII index of char */
    int iladj,iradj,stroke;
    int pen;                        /* down is 1, up is 0 */
    
    int ix,iy;                      /* coords in letter 'box', -127 to 127 */
    int x,y;                        /* coords in natural orient, real size */
    int xc,yc;                      /* coords in rotated frame, pixels */
    int xa;                         /* letter space in pixels */
    
    int cdef = 0.4 * g_ysize;       /* TEST!!!!!!!!!!!!!!*/
    int magf = expand*cdef;         /* expand*cdef; magf*21/256 is ht of
                                     *  typical capital letter,
                                     *  g_ysize*expand/30  
                                     */ /*jeg9510*/
    senv = senvtem ;
    senvsave = senvtem ;
    oldtype = ltype;
    ltype = 0;
    
    /*** CONTROL CODES FOR STRINGS ******
    *  \\x - set mode x		        *
    *  \x  - set mode x for next char   *
    *  \r - roman font		        *
    *  \g - greek font		        *
    *  \s - script font		        *
    *  \t - tiny font		        *
    *  \i - toggle italics      	* 
    *  \u - superscript		        *
    *  \d - subscript		        *
    *  \b - backspace		        *
    *  \e - end string		        *
    * ***********************************
    */
  
    for(j=0;j<n;j++){
        c = string[j];
        if(c == '\\'){
            icomm++;
            continue;
        }
        if(icomm > 0){
            switch(tolower(c)){
            case 'r': senv.ioffs = 0;   break;
            case 'g': senv.ioffs = 128; break;
            case 's': senv.ioffs = 256; break;
            case 't': senv.ioffs = 384; break;
            case 'i': senv.itflg = 1 - senv.itflg ; break;
            case 'u': 
                senv.isupr++;
                senv.suprsh += 16.*senv.suprf;
                if(senv.isupr > 0) senv.suprf = (senv.suprf * supfrac)>>8 ;   
                else               senv.suprf = (senv.suprf>>8)/supfrac ;  
                break;
            case 'd':
                senv.isupr--;
                senv.suprsh -= 16.*senv.suprf;
                if(senv.isupr < 0) senv.suprf = (senv.suprf * supfrac)>>8 ;
                else               senv.suprf = (senv.suprf>>8)/supfrac ; 
                break;
            case 'e':
                goto out;
                break;
            default:
                mprintf("\nIllegal embedded commmand:%c",c);
                break;
            }
            if(icomm > 1) senvsave = senv;  /* change of state after '\\' */
            icomm = 0;
            continue;
        }
        jchar = c + senv.ioffs;
        iladj = SCHAR(ladj[jchar]);  /* signed */
        iradj = SCHAR(radj[jchar]);
#ifdef VDEBUG
        scrprintf("\nInterpreting %c, %d of %d chars: j,ladj,radj=%d %d %d",
            c,j,n,jchar,iladj,iradj);         
#endif
        
        if(drawit){
            pen = 0;
            stroke = nstroke[jchar]&0xff;
#ifdef VDEBUG
            scrprintf("  strokes=%d",stroke);
#endif
            for(i=0;i<stroke;i++){
                ix = SCHAR(font[pointer[jchar]+ 2*i -1]);  
                iy = SCHAR(font[pointer[jchar] + 2*i ]); 
                        /*offset from fortran index*/ 
#ifdef VDEBUG    
                if(!(i%4))scrprintf("\n");
                if(ix == 31)scrprintf("R ");
                else scrprintf("i|xy=%3d %3d %3d  ",i,ix,iy);
                if(i==stroke-1)scrprintf("\n");
#endif
                if(ix == 31){     /* flag to pick up pen and move */
                    pen = 0;
                    continue;
                }
                x = ix - iladj;
                if(senv.itflg) x += RSH8(slant*(iy+9)) ;
                x = RSH16((int)( magf * x * senv.suprf )) ;
                y = RSH16((int)(magf*(iy * senv.suprf + senv.suprsh))) ;
                xc = RSH8(co*x - si*y) + x0;
                yc = RSH8(si*x + co*y) + y0;
/*?*/           ldraw(xc, yc, pen);  /* draw the stroke */                
                if(!pen) pen = 1;     /* draw the next one */
                xp = xc;             
                yp = yc;             /* move current to new location */
            }    /* end for loop on strokes */
        }        /* end if drawit */
        xa = RSH16((int)(magf*(iradj - iladj)*senv.suprf));
        slength += xa;
        x0 += RSH8(co*xa);
        y0 += RSH8(si*xa);
        senv = senvsave;
    }  /* end of for loop over chars */
out:   /* cleanup */
    if(drawit){
        g_xold = x0;
        g_yold = y0;
    }
    ltype = oldtype;
    return (slength);
}    

void gstring(string,angle,expand)   /* draws string */
char *string;
double angle;
double expand;
{
    vstring(string,angle,expand,1);
}

int gstrlen(string,angle,expand)   /* returns length of string */
char * string;
double angle;
double expand;
{
    return vstring(string,angle,expand,0);
}

/***************** BITMAP OUTPUT ROUTINES *****************************/

/* first, general ones for disk i/o */

/******************* OUTGRAPH(), INGRAPH() **************************/
/* XXXXXX I think we can kill these--at least make them fail gracefully
if the graph is not a bitmap */
void
outgraph(fname)   /* writes a "natural" image of the memory array  to disk;
                    not useful for much but saving a graph quickly. Demands
                    an extension .grp to the filename */
char *fname;
{
    int fdes;
    char filename[64];
    u_char size[6];
    
    if(!g_bitmap)erret("\nGraph has no bitmap data");
    extchk(fname,"grp",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,DBLKSIZE + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char *)NULL);
    }
    size[0] = g_xsize&0xff;
    size[1] = g_xsize>>8;
    size[2] = g_ysize&0xff;
    size[3] = g_ysize>>8;
    size[4] = g_xbsize&0xff;
    size[5] = g_xbsize>>8;   /* DEC-INTEL byte order enforced */
    writeblk(fdes,(char *)size,6);
    writeblk(fdes,(char *)mgraph[0],g_xbsize*g_ysize);
    closeblk(fdes);
}

void
ingraph(fname,n)   /* reads an outgraph image back in to graph n; 
                    if there is not a graph n, allocates one; if there
                    is, frees and overwrites it */
char *fname;
int n;
{
    int fdes;
    u_char size[6];
    char name[64];
    int gxs, gys; 
    
    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".grp");
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)NULL);
    }
    readblk(fdes,(char *)size,6); 
    gxs = size[0] + (((int)size[1])<<8);
    gys = size[2] + (((int)size[3])<<8);
    /* gxbs = size[4] + (((int)size[5])<<8); */
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocgraph(gxs,gys,n);
    readblk(fdes,(char *)mgraph[0],g_xbsize*g_ysize);
    closeblk(fdes);
}

/********************** VECTOR ROUTINES ************************************/

void drawlist()    /* draws the graph in the current vector list, using
                        (*g_rdraw)(); if g_rdraw is rm_draw, prepares bitmap */
{
    int i;
    normal vsave = g_vector;
    normal n = *(normal *)(mvgraph-2);
    u_char ***mp;
    
    if( g_rdraw == (void (*)())rm_draw ){
        g_vector = 0; /*don't remake the list from itself*/
        if(!g_bitmap){ /* must allocate memory for bitmap */
            mp = (u_char ***)matralloc(namgraph(gchan),g_ysize,g_xbsize,1);
            if(!mp) erret("\nCannot allocate bitmap memory");
            g_bitmap = 1;
            g_env[gchan].gmcp = mp;   /* so you can memfree it */
            mgraph = *mp;        
            g_env[gchan].gptr = gpointer = mgraph[0];        
        }
    }
    for(i=0;i<n;i++){
        (*g_rdraw)(mvgraph[i].gv_x,mvgraph[i].gv_y,mvgraph[i].gv_color);
    }
    g_vector = vsave;
}

void bdrawlist()   /* create bitmap from vector list */
{
    void (*drawf)() ;
    drawf = g_rdraw;
    g_rdraw = rm_draw;
    drawlist();
    g_rdraw = drawf;
}
    

void outvector(fname)    /* demands extension '.gvc' for filename */
char *fname;
{
    int fdes;
    char filename[64];
    
    if(!g_vector) erret("\nNo vector data for this graph");
    extchk(fname,"gvc",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,
        DBLKSIZE + (*(normal *)(mvgraph-2))*sizeof(struct v_point))) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char *)NULL);
    }
    writeblk(fdes,(char *)(mvgraph-2),3*sizeof(normal));
    writeblk(fdes,(char *)mvgraph,(*(normal *)(mvgraph-2))*sizeof(struct v_point));
    closeblk(fdes);
}

void invector(fname,n)  /* reads a vector file back in to graph n; if
                           there is not a graph n, creates one; if there
                           is one, frees and overwrites it */
char *fname;
int n;
{
    int fdes;
    normal size[3];
    char name[64];
    int gxs,gys,nvec;
    char *cp;
    
    strcpy(name,fname);
    if((cp = strchr(name,'.')) == NULL) strcat(name,".gvc");
    else if(strcmp(cp,".gvc")) erret("\nFile is not a vector file");
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)NULL);
    }
    readblk(fdes,(char *)size,3*sizeof(normal));
    nvec = size[0];
    gxs = size[1];
    gys = size[2];
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){    /* already a graph */
        scrprintf("\nFree existing graph ? ");
        if(!_yesno()) erret((char *)NULL);
        freegraph(n);
    }
    gallocgraph(gxs,gys,n,1,0);
    (*(normal *)(mvgraph-2)) = (g_env[n].gcnvec) = c_gcnvec = nvec;
    exp_vmem((nvec*3)/2);
    readblk(fdes,(char *)mvgraph,nvec*sizeof(struct v_point));
    closeblk(fdes);
}

/************************ graph transposition-added jeg9411 ******************/

/************************ TRANSGRAPH() *************************************/
/* 
 * This routine is slow, but is useful only for printing, so is probably
 * OK. To is an empty allocated graph with the y and x sizes flipped 
 * into which the transposed CURRENT graph is to be written. 
 * This is checked; the routine bombs if either receiving axis is too 
 * small. The routine works for bitmaps only, and is reasonably
 * efficient if the graph is mostly white space. 
 */

void
transgraph(to)
int to;
{
    u_char **from = mgraph;
    register char c;
    register int k;
    int ib,it;
    int oxs = g_xsize;
    int oys = g_ysize;
    int oxsb = g_xbsize;
    int i,is,j;
    
    setgraph(to);
    egraph();    
    if(g_xsize < oys || g_ysize < oxs){
        erret("\nNew graph too small to receive transposition");
    }
    for(i=0;i<oys;i++){
        is = i;
        ib = is>>3;
        it = is & 7;
        for(j=0;j<oxsb;j++){
            if((c=from[i][j]) != 0){
                for(k=0;k<8;k++){
                    if(c & g_mask[k]){
                        mgraph[oxs - (j<<3) - k - 1][ib] |= g_mask[it];
                    }
                }
            }
        }
    }
}

/* all this stuff has been moved to mirpnm.c */
#if 0
/* pnm support added jeg9808 */
/**************************************************************************/
/*********************** PNM stuff ****************************************/
/**************************************************************************/
#if 0   /* in graphs.h */
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
#endif


struct pnm_head_i pnm_ihead;
struct pnm_head_a pnm_ahead;

/* ftp at oak.oakland.edu: pub/msdos/djgpp, pub/msdos/graphics  
    pbmp191d.zip is full pbmplus distribution. */


/* jeg0003 */

#if 0
Notes on formats: A ppm, pbm, pgm header consists of a set of ascii
lines (sep by whitespace, not necc. newlines or cr/newlines:

P3  (magic number, P1 P2 P3 for pbm, pgm, ppm ascii, P4 P5 P6 for binary
# comments
78 54 (image width and height in pixels )
255   (maxgrey, or max color for pnm, but only one entry for pnm; 
            no entry for pbm)
1 character of whitespace
Data-- bytes with MSb first, and ( I think, MSb=leftmost dot in pbm)
ppm data is R-G-B

For DOS files, use spaces to be sure, bcz cf/lf pairs are 2 bytes
#endif


/************************* GETPNMTOK() *************************************/
/* This routine does all the work of reading PNM headers and ascii files.
 * it returns a count of the number of comment characters it has collected
 * in commentptr or zero if none. If it finds no token, str is set to null.
 * if commentptr is NULL, ignores comments.
 * if token pointer is null, returns position in file; this is the data offset;
 * the call is made after the last token is obtained.
 */

/* made global jeg0005*/ 

mirella int   
getpnmtok(str,commentptr)
char *str;
char *commentptr;
{
    char commentstr[512];
    char *cp = str;
    int c;
    int len=0;
    int clen;
    int tclen = 0;
    
 

    if(str == NULL){
        return mtell();
    }

    *str = '\0';        
    do{
        c = mfgetc();
        if(rdeof){
            *cp++ = '\0';
            return tclen;
        }
        if(!isspace(c) && c != '#'){
            *cp++ = c;
            len++;
            if(len == 7) erret("\nGETPNMTOK: Token too long");
        }else if(len != 0){   /* got something; end whitespace */
            *cp = '\0';
            return tclen;  /* success, not comment */
        }
            
        if(c == '#'){  /* comment */
            mfgets(commentstr,512);
            clen = strlen(commentstr);
            strcpy(commentstr + clen,EOL);
            if(commentptr && tclen + clen + LEOL < 512){
                strcpy(commentptr + tclen,commentstr);
                tclen += (clen+LEOL);
            }
            if(rdeof) return(tclen);
        }
    }while(1);
}

/********************* GETPNMHEAD() **************************************/
/* This one uses arbitrary structures */

mirella char *
ggetpnmhead(file,pa,pi)
char *file;
struct pnm_head_a *pa;
struct pnm_head_i *pi;
{
    int fchan;
    char tokstr[10];
    int m;
    /*char *cp;*/
    int cco;
    static char comment[2048];
    char *comptr = comment;
    int c;
    
    fchan = mopen(file,0);
    pushchan(fchan);
    
    cco = getpnmtok(tokstr,comptr);
    comptr += cco;

    if((((c=*tokstr) != 'P') && (c != 'p') ) || (m=atoi(tokstr+1)) < 1 || m>6){
        erret("\nGETPNMHEAD:Not a legal pnm file");
    }
    *tokstr = 'P';
    strcpy(pa->pnm_magicnos,tokstr);
    pi->pnm_magicno = m;
    

    cco = getpnmtok(tokstr,comptr);
    comptr += cco;

    if(*tokstr != '\0'){
        strcpy(pa->pnm_xsizes,tokstr);
        pi->pnm_xsize = atoi(tokstr);
    }else{
        erret("\nGETPNMHEAD: No valid x size");
    }

    cco = getpnmtok(tokstr,comptr);
    comptr += cco;

    if(*tokstr != '\0'){
        strcpy(pa->pnm_ysizes,tokstr);
        pi->pnm_ysize = atoi(tokstr);
    }else{
        erret("\nGETPNMHEAD: No valid y size");
    }

    if(m != 1 && m != 4){  /* PGM or PPM */
        cco = getpnmtok(tokstr,comptr);
        comptr += cco;
    
        if(*tokstr != '\0'){
            strcpy(pa->pnm_maxgreys,tokstr);
            pi->pnm_maxgrey = atoi(tokstr);
        }else{
            erret("\nGETPNMHEAD: No valid maxgrey");
        }
    }

    pi->pnm_data = getpnmtok(NULL,NULL);
    popchan();
    return comment;
}

/* this one uses pnm_ahead, phm_ihead */

mirella char *
getpnmhead(file)
{
    return ggetpnmhead(file,&pnm_ahead,&pnm_ihead);
}

/******************* MAKEPBMHEAD(), MAKEPNMHEAD() **************************/
/* 
 * Makes ascii headers for pbm, pnm files; entries are sep by spaces, 
 * terminated with a newline; pbm header is exactly 16 chars long, pnm
 * header 20 chars long.
 */
 
mirella char * 
makepbmhead(xsize,ysize)
int xsize;
int ysize;
{
    static char header[16];
    int i;
    int xlen,ylen;
    
    strcpy(pnm_ahead.pnm_magicnos,"P4");
    sprintf(pnm_ahead.pnm_xsizes,"%d",g_xsize);
    sprintf(pnm_ahead.pnm_ysizes,"%d",g_ysize);
    for(i=0;i<16;i++) header[i] = ' ';
    strncpy(header,pnm_ahead.pnm_magicnos,2);
    
    xlen = strlen(pnm_ahead.pnm_xsizes);
    if(xlen > 5) erret("\nMAKEPBMHEAD:x size too big");
    strncpy(header+3,pnm_ahead.pnm_xsizes,xlen);
    
    ylen = strlen(pnm_ahead.pnm_ysizes);
    if(ylen > 5) erret("\nMAKEPBMHEAD:y size too big");    
    strncpy(header + 15 - ylen,pnm_ahead.pnm_ysizes,ylen);
    
    header[15]='\n';
    return header;
}


mirella char * 
makepnmhead(magicno,xsz,ysz,maxgrey)
int magicno;
int xsz;
int ysz;
int maxgrey;
{
    static char header[20];
    int i;
    int xlen,ylen,glen;
    
    sprintf(pnm_ahead.pnm_magicnos,"P%d",magicno);
    sprintf(pnm_ahead.pnm_xsizes,"%d",xsz);
    sprintf(pnm_ahead.pnm_ysizes,"%d",ysz);
    sprintf(pnm_ahead.pnm_maxgreys,"%d",maxgrey);
    
    for(i=0;i<20;i++) header[i] = ' ';
    
    strncpy(header,pnm_ahead.pnm_magicnos,2);

    xlen = strlen(pnm_ahead.pnm_xsizes);
    if(xlen > 5) erret("\nMAKEPNMHEAD:x size too big");
    strncpy(header + 3,pnm_ahead.pnm_xsizes,xlen);

    ylen = strlen(pnm_ahead.pnm_ysizes);
    if(ylen > 5) erret("\nMAKEPNMHEAD:y size too big");    
    strncpy(header + 9,pnm_ahead.pnm_ysizes,ylen);

    glen = strlen(pnm_ahead.pnm_maxgreys);
    if(glen > 3) erret("\nMAKEPNMHEAD:maxgrey too big");    
    strncpy(header + 19 - glen,pnm_ahead.pnm_maxgreys,glen);

    header[19]='\n';
    return header;
}

/******************* PUTPBM(), GETPBM() **************************/
/* These routines write and read binary pbm files; the pbm spec does
 * not insist that xsize be a multiple of 8, which is the case for
 * Mirella bitmap graphs; the input code throws up its hands if
 * this is not the case. Also, the pbm origin is at the top left,
 * and ours is at the bottom left, so we have to read/write a line
 * at a time.
 */
 
mirella void
putpbm(fname) /* writes a binary PBM file for the current graph */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i;
    
    if(!g_bitmap)erret("\nGraph has no bitmap data");
    extchk(fname,"pbm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,16 + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    if(8*(g_xsize/8) != g_xsize){
        erret("\nPUTPBM:Mirella graph does not have xsize divisible by 8");
    }
    cp = makepbmhead(g_xsize,g_ysize);
    writeblk(fdes,cp,16);
    for(i=0;i<g_ysize;i++){
        writeblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
getpbm(fname,n)   /* reads a binary pbm file into to graph n; 
                   * if there is not a graph n, allocates one; if there
                   * is, frees and overwrites it. NB!!! only handles BINARY
                   * pbm graphs--it would be simple to handle general ones,
                   * but someday. Note that pbm image is 'upside down'; its
                   * origin is at the top left, Mirella's at the bottom left.
                   */
char *fname;
int n;
{
    int fdes;
    char name[100];
    int gxs, gys; 
    int datoffs;
    int dt;
    int i;

    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".pbm");

    /* get header */
    getpnmhead(name);
    if(pnm_ihead.pnm_magicno != 4){
        erret("\nNOT A BINARY PBM FILE");
    }
    gxs = pnm_ihead.pnm_xsize;
    gys = pnm_ihead.pnm_ysize;
    datoffs = pnm_ihead.pnm_data;
    
    if(gxs != 8*(gxs/8)) erret("\nGETPBM:linelength is not a multiple of 8");
    
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)1);
    }
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocgraph(gxs,gys,n);
    dt = lseekblk(fdes,datoffs,0);
    if(dt != datoffs) erret("\nGETPBM: seek error for reading graph");
    for(i=0;i<g_ysize;i++){
        readblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}


/******************* PUTPGM(), GETPGM() **************************/
/* These routines write and read binary pgm files; 
 * The pgm origin is at the top left,
 * and ours is at the bottom left, so we have to read/write a line
 * at a time.
 */
 
mirella void
putpgm(fname) /* writes a binary PGM file for the current graph */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i;
    
    if(!g_bitmap) erret("\nGraph has no bitmap data");
    if(g_bitscol != 8) erret("\nLo siento. Graph is not a PGM graph");
    extchk(fname,"pgm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    cp = makepnmhead(5,g_xsize,g_ysize,255);
    writeblk(fdes,cp,20);
    for(i=0;i<g_ysize;i++){
        writeblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
getpgm(fname,n)   /* reads a binary pgm file into to graph n; 
                   * if there is not a graph n, allocates one; if there
                   * is, frees and overwrites it. NB!!! only handles BINARY
                   * pgm graphs. Note that pgm image is 'upside down'; its
                   * origin is at the top left, Mirella's at the bottom left.
                   */
char *fname;
int n;
{
    int fdes;
    char name[100];
    int gxs, gys; 
    int datoffs;
    int dt;
    int i;

    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".pbm");

    /* get header */
    getpnmhead(name);
    if(pnm_ihead.pnm_magicno != 5){
        erret("\nNOT A BINARY PGM FILE");
    }
    gxs = pnm_ihead.pnm_xsize;
    gys = pnm_ihead.pnm_ysize;
    datoffs = pnm_ihead.pnm_data;
    
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)NULL);
    }
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocpgmgraph(gxs,gys,n);
    dt = lseekblk(fdes,datoffs,0);
    if(dt != datoffs) erret("\nGETPBM: seek error for reading graph");
    for(i=0;i<g_ysize;i++){
        readblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}


/******************* PUTPPM(), GETPPM() **************************/
/* These routines write and read binary ppm files; 
 * The ppm origin is at the top left,
 * and ours is at the bottom left, so we have to read/write a line
 * at a time.
 */
 
mirella void
putppm(fname) /* writes a binary PPM file for the current graph */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i;
    
    if(!g_bitmap) erret("\nGraph has no bitmap data");
    if(g_bitscol != 24) erret("\nLo siento. Graph is not a PPM graph");
    /* XXXXX FIXME--should be able to write ANY graph as a PPM graph */
    extchk(fname,"ppm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)NULL);
    }
    cp = makepnmhead(6,g_xsize,g_ysize,255);
    writeblk(fdes,cp,20);
    for(i=0;i<g_ysize;i++){
        writeblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
getppm(fname,n)   /* reads a binary ppm file into to graph n; 
                   * if there is not a graph n, allocates one; if there
                   * is, frees and overwrites it. NB!!! only handles BINARY
                   * ppm graphs. Note that ppm image is 'upside down'; its
                   * origin is at the top left, Mirella's at the bottom left.
                   */
char *fname;
int n;
{
    int fdes;
    char name[100];
    int gxs, gys; 
    int datoffs;
    int dt;
    int i;

    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".pbm");

    /* get header */
    getpnmhead(name);
    if(pnm_ihead.pnm_magicno != 6){
        erret("\nNOT A BINARY PPM FILE");
    }
    gxs = pnm_ihead.pnm_xsize;
    gys = pnm_ihead.pnm_ysize;
    datoffs = pnm_ihead.pnm_data;
    
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)NULL);
    }
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocppmgraph(gxs,gys,n);
    
    dt = lseekblk(fdes,datoffs,0); 
    
    if(dt != datoffs) erret("\nGETPBM: seek error for reading graph");
    for(i=0;i<g_ysize;i++){
        readblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
putpnm(fname)
char *fname;
{
}

mirella void
getpnm(fname, n)
char *fname;
int n;
{
}
#endif
    
/************************** END, MIRGRAPH.C **********************************/





