/*
 * This is the application-independent part of Mirella's imaging support
 */

#include "mirella.h"
#include "images.h"

static int valid_buf();

normal illval;				/* set in init_image() */
float filval;
 
short  **pic;                           /* pbuf->_pline */
char   **bpic;                      
normal **npic;
float  **fpic;
double **dpic;
RGBTRIPLE **rgbpic;                     /* same value for diff data types */

short  **picd;                          /* pbufd->_pline */
char   **bpicd;
normal **npicd;
float  **fpicd;
double **dpicd;
RGBTRIPLE **rgbpicd;                    /* same value for diff. data types */


short **picd;                           /* pbufd->_pline */

PBUF *pbuf = NULL;			/* the current pbuf */
PBUF *pbufs[MAXPBUF];			/* possible pbufs */
PBUF *pbufd = NULL;                     /* `destination' buffer */

normal xsize;				/* length of line */
normal ysize;				/* number of lines */
normal xsized;
normal ysized;                          /* sizes for dest buffer */

normal maxpbuf = MAXPBUF;
normal pbuffer = -1;			/* current pbuf */
normal pbufferd = -1;                   /* number of destination buffer */

normal cursssize = sizeof(CURSOR);
normal dispsize = sizeof(DISPLAY);
normal d_xsize = 100,d_ysize = 100;
normal wxsize = 100,wysize = 100;
normal wxoff = 0, wyoff = 0;		/* offset of visible part of buffer */

normal plus_pat = PLUS_PAT;		/* possible markit() types */
normal corner_pat = CORNER_PAT;

u_char *dltable = NULL;			/* Mirella's dlt */


/*****************************************************************************/
/*
 * The default values of HEADER, set in init_image()
 */
static HEADER header_templ;

/*****************************************************************************/
/*
 * these pointers are initialized by applinit() and are never moved;
 * they both point to addresses in the application area
 */
DISPLAY *display;
CURSOR *curs;     

void
init_image()
{
   int i;
/*
 * allocate space for headers, display structures, and cursor structures
 * (The latter two in the application environment)
 */
   for(i = 0;i < MAXPBUF;i++) {
      pbufs[i] = (PBUF *)malloc(sizeof(PBUF));
      init_pbuf(pbufs[i]);
   }
   
   display = (DISPLAY *)envalloc(sizeof(DISPLAY));
   curs = (CURSOR *)envalloc(sizeof(CURSOR));
   init_display();
   init_disp_stack();
   init_dlt();
/*
 * Initialise the device-indep part of header. This can't be a static
 * initialisation as illval isn't static...
 */
   header_templ._serno = 0;
   header_templ._bitpix = illval;
   header_templ._naxes = illval;
   header_templ._xsize = header_templ._ysize = 0;
   header_templ._zsize = 1;
   header_templ._pfname[0] = '\0';
   header_templ._filedate[0] = '\0';
   header_templ._imname[0] = '\0';
#ifdef IBMORDER
   header_templ._decorder = 0;
#else
   header_templ._decorder = 1; /* this is WRONG WRONG for Fits reading */
#endif
   header_templ._data_type = illval;
   header_templ.app = NULL;
   header_templ.app_size = 0;

   init_app_header();
}

/*****************************************************************************/
/*
 * Initialise the lookup table
 */
float stretch_bottom = 0.0, stretch_top = 1.0;
static u_char *dltbase = NULL;		/* base of the DLT */

void
init_dlt()
{
   if(dltbase == NULL) {
      if((dltbase = (u_char *)malloc(65540)) == NULL){
	 printf("Cannot allocate display lookup table\n");
	 exit(1);
      }
      dltable = dltbase + 32768;
      /*DB*/
      /*mprintf("\nAllocated dlt: dltbase, dltable = %u %u\n",dltbase,dltable);*/
   }
}

/*****************************************************************************/
/*
 * Copy the template into phead, so all entries start by making sense 
 */
void
init_header(header)
HEADER *header;
{
   *header = header_templ;
}

void
init_pbuf(buf)
PBUF *buf;
{
   init_header(&(buf->_header));
   buf->_mpicorig = NULL;
   buf->_picorig = NULL;
   buf->_pline = NULL;
   buf->_pline_size = 0;
   buf->_pbsize = 0;
   buf->_scr = NULL;
}

/*****************************************************************************/
/*
 * Initialise the display struct and device
 */
void
init_display()
{
   dspinit();
   d_xoff = d_yoff = 0;
   d_xlow = d_ylow = 0;
   d_xhigh = wxsize = d_xsize;
   d_yhigh = wysize = d_ysize;
   d_wxcen = (d_xsize)/2;
   d_wycen = (d_ysize)/2;		/* center the window if relevant */
   d_wzoom = 1;				/* set hardware zoom */
   
   d_cnum = d_cden = 1;
   d_wrapflg = 0;
   d_zorg = 0;  
   d_zrng = 32767;
   d_dmode = 1;
   d_bufno = 0;
   d_bufsno = 0;
   d_pfname[0] = '\0';
/*
 * init cursor/window structure
 */
   ccur();
   c_marker = 0;
   c_disp = NULL;
}

/*****************************************************************************/
/*
 * Copy a header, including its application part
 */
void
copy_full_header(to,from)
HEADER *to,*from;
{
   Void *to_app = to->app;		/* don't copy this! */

   *to = *from;
   to->app = to_app;

   bufcpy((char *)to->app,(char *)from->app,from->app_size);
}

/*****************************************************************************/
/*
 * BUFFER ALLOCATION ROUTINES
 *
 * allocates new pic buffer, sets up header and line pointer array. dsize
 * is the data size in bytes. The old (short only) pballoc is below.
 */
 
void
gpballoc(xsz,ysz,nbuf,dsize)
int xsz,ysz;
int nbuf;
int dsize;
{
   int i;
   unsigned long porig;
   PBUF *buf;
    
   dsize = dsize < 0 ? -dsize : dsize ; /* can be passed negative for float */
   if(nbuf < 0 || nbuf >= MAXPBUF){
      printf("\nPicture buffer %d is outside range",nbuf);
      erret((char *)NULL);
   }
   buf = pbufs[nbuf];
   
   if(buf->_pline != NULL) {
      scrprintf("\nPicture buffer %d is already allocated, size %d",
		nbuf,buf->_pbsize);
      return;
   }
/*
 * allocate enough for the picture and 511 extra bytes.  The 511
 * allows us to put picorig on a page boundary for vms's sake
 */
   if((buf->_mpicorig = malloc(xsz*ysz*dsize + 511)) == NULL) {
      erret("\nCannot allocate picture memory");
   }
   if((buf->_pline = (short **)malloc(ysz*sizeof(short *))) == NULL) {
      free((char *)buf->_mpicorig);
      erret("\nCannot allocate line origins' memory");
   }
   buf->_pline_size = ysz;
   
   porig = (long)buf->_mpicorig;
   if(porig%512 != 0) {		/* move it up to a page boundary */
      porig += (512 - porig%512);
   }
   buf->_picorig = (short *)porig;
   buf->_pbsize = xsz*ysz*dsize/2;
   pbufs[nbuf]->_header._xsize = xsz;
   pbufs[nbuf]->_header._ysize = ysz;
   pbufs[nbuf]->_header._bitpix = 8*dsize;
   pbufs[nbuf]->_header._data_type = T_UNKNOWN;
   pbufs[nbuf]->_header._naxes = 2;
   
   for(i=0;i<ysz;i++) {
      buf->_pline[i] = (short *)((char *)(buf->_picorig) + i*xsz*dsize);
   }
   alloc_app_pbuf(buf);
   init_app_pbuf(buf);
}

void pballoc(xsz,ysz,nbuf) /* short pictures */
int xsz,ysz,nbuf;
{
    gpballoc(xsz,ysz,nbuf,2);
    pbufs[nbuf]->_header._data_type = T_SHORT;          
}
    
void bpballoc(xsz,ysz,nbuf) /* byte pictures */
int xsz, ysz, nbuf;
{
    gpballoc(xsz,ysz,nbuf,1);
    pbufs[nbuf]->_header._data_type = T_CHAR;    
}
        
void fpballoc(xsz,ysz,nbuf)  /* float pictures */
int xsz, ysz, nbuf;
{
    gpballoc(xsz,ysz,nbuf,-4);
    pbufs[nbuf]->_header._data_type = T_FLOAT;    
}
                
void npballoc(xsz,ysz,nbuf)  /* normal (int) pictures */
int xsz, ysz, nbuf;
{
    gpballoc(xsz,ysz,nbuf,4);   
    pbufs[nbuf]->_header._data_type = T_LONG;    
}

void rgbpballoc(xsz,ysz,nbuf)  /* normal (int) pictures */
int xsz, ysz, nbuf;
{
    gpballoc(xsz,ysz,nbuf,4);
    pbufs[nbuf]->_header._data_type = T_RGB8; 
}   
        
    
/************************ PBFREE() ***********************************/

void
pbfree(nbuf)
int nbuf;
{
   PBUF *buf;
   
   if(nbuf<0  || nbuf >=MAXPBUF){
      printf("\nPicture buffer %d is outside range",nbuf);
      erret((char *)NULL);
   }
   if(nbuf == pbuffer) {
      printf("\nYou cannot free the current pbuf");
      erret((char *)NULL);
   }
   buf = pbufs[nbuf];
   
   if(buf->_pline == NULL){
      printf("\nPicture buffer %d is already free",nbuf);
      return;
   }
   free(buf->_mpicorig);
   free((char *)buf->_pline);
   buf->_pline = NULL;
   buf->_pline_size = 0;
   buf->_picorig = NULL;
   buf->_pbsize = 0;
   
   free_app_pbuf(buf);
}


/************** SET(D)POIINTERS() **************************************/
/* code to set all the varieties of picture matrix pointers */

/* for the current buffer */

static void setpointers(p)
short **p;
{
    pic  = (short  **)p;
    bpic =   (char **)p;
    npic = (normal **)p;
    fpic = (float  **)p;
    dpic = (double **)p;
    rgbpic = (RGBTRIPLE **)p;
}   

/* and the current dest buffer */

static void setdpointers(p)
short **p;
{
    picd  = (short  **)p;
    bpicd =   (char **)p;
    npicd = (normal **)p;
    fpicd = (float  **)p;
    dpicd = (double **)p;
    rgbpicd = (RGBTRIPLE **)p;
}   

/************************ PICBUFFER() *******************************/

/*
 * Choose a new buffer and update the current header struct with the
 * array entry for the new buffer number.  If the filename for the new
 * buffer is empty, the buffer is assumed to be empty and the header
 * is copied from the current header.  This the the ONLY routine that
 * can change the pointer to the current picture buffer.
 */
void
picbuffer(n)
int n;
{
   if(!valid_buf(n)) {
      if(n <= -100 || n == -1) {	/* assume they know what they want */
	 ;
      } else {   
	 printf("\n%d is an invalid pbuf -- select it anyway? ",n);
	 if(!_yesno()) {
	    erret((char *)NULL);
	 }
      }
      if(n == -1) printf("\nDeselecting all buffers");
      headup();
      pbuf = NULL;
      pbuffer = n;
      xsize = ysize = 0;
      setpointers((short **)NULL);
      return;
   }
   isokbuf(n);				/* check that it really _is_ OK */
   /* save current header in array for current pbuffer */
   headup();
   pbuffer = n;
   pbuf = pbufs[n];
   xsize = pbuf->_header._xsize;
   ysize = pbuf->_header._ysize;
   setpointers(pbuf->_pline);
   return;
}

/************************ SHOWPBUFFERS() ******************************/

/* update this to be more informative--x,ysiz,bitpix,pfname */
void
showpbuffers()
{
    int i;
        
    printf("\nPicture buffers\n nbuf   origin    picorig   size(shorts)");
    for(i = 0;i < MAXPBUF;i++){
        if(pbufs[i]->_pline != NULL) {
	   printf("\n%c%2d   %8ld   %8ld   %8ld",(pbuffer == i ? '*' : ' '),
		  i,(long)pbufs[i]->_pline,(long)pbufs[i]->_picorig,
		  pbufs[i]->_pbsize);
	}
    }
}


/*****************************************************************************/
/*
 * BUFFER SELECTION ROUTINES
 *
 * generates line pointer array for CURRENT picture buffer
 * XXXXXXXXXXXX This is a mess. xize and ysize are decoupled from the
 * header quantities. this may be saveable, but may not be. Mirella
 * talks to the header quantities, so, for example, psetup, does not work
 * properly or reliably.
 */
void
gplptr(dsize)
int dsize;
{
    register int i;
   
    h_bitpix = 8*dsize;
    
    dsize = abs(dsize);
    
    if(xsize*ysize*dsize > pbsize*sizeof(short)){
        scrprintf("\npicture too big, x,ysize,bitpix,bufsiz = %d %d %d %d",
		xsize,ysize,h_bitpix,pbsize);
        erret((char *)NULL);
    }
    if(ysize != pbuf->_pline_size) {
        if((pbuf->_pline = (short **)realloc((char *)pbuf->_pline,
					 ysize*sizeof(short *))) == NULL) {
	    erret("Can't realloc pline");
        }
        pbuf->_pline_size = ysize;
    }
    for(i = 0;i < ysize;i++) {
        pbuf->_pline[i] = (short *)((u_char *)picorig + xsize*i*dsize);
    }
    picbuffer(pbuffer);			/* update the pbuf struct etc. */
    h_xsize = xsize;			/* [xy]size CAN change, so do this */
    h_ysize = ysize;
    return;
}

void plptr() /* automatic one, used in rfits, etc */
{
    int dsize = h_bitpix/8;
    gplptr(dsize);
    return;
}

void bplptr()
{
    gplptr(1);
    pbuf->_header._data_type = T_CHAR;
    return;
}

void splptr()
{
    gplptr(2);
    pbuf->_header._data_type = T_SHORT;
    return;
}

void fplptr()
{
    gplptr(4);
    pbuf->_header._data_type = T_FLOAT;
    return;
}

void nplptr()
{
    gplptr(4);
    pbuf->_header._data_type = T_LONG;
    return;
}

/*****************************************************************************/
/*
 * update the header array entry with the current header struct, if it exists.
 */
void
headup()
{
   if(valid_buf(pbuffer)) {
      h_xsize = xsize;			/* [xy]size CAN change, so do this */
      h_ysize = ysize;
   }
}

/************************ ISOKBUF() ***************************************/
/*
 * checks for legality of buffer index d and complains
 */
void
isokbuf(d)
int d;
{
   if(d < 0 || d >= MAXPBUF){
      printf("\nBuffer %d does not exist",d);
      erret((char *)NULL);
   }
   if(pbufs[d]->_pline == NULL){
      printf("\nBuffer %d is not active",d);
      erret((char *)NULL);
   }
}

/* 
 * checks for legality AND existence of pointer array (active); returns
 * 0 if not OK, 1 if OK
 */
normal
isacbuf(d)
int d;
{
   if(d < 0 || d >= MAXPBUF || pbufs[d]->_pline == NULL ){
      return 0;
   } else {
      return 1;  /* OK */
   }
}


static int
valid_buf(d)
int d;
{
   return((d >= 0 && d < MAXPBUF) ? 1 : 0);
}

/************************ HEADTOBUF() *******************************/
/*
 * writes the current header to buffer n, if legal
 */
void
headtobuf(n)
int n;
{
   isokbuf(n);
   /* these CAN change, so make sure CURRENT header is up-to-date*/
   h_xsize = xsize;
   h_ysize = ysize;
   copy_full_header(&pbufs[n]->_header,&pbuf->_header);
}


/*****************************************************************************/
/*
 * Clear current pbuf
 */
mirella void
pclear()
{
   register char *pl, *pend;
   int i;
   int xsizeb = xsize * h_bitpix/8; /* length of line in bytes */
   
   for(i = 0;i < ysize;i++){
      pl = (char *)pic[i];
      for(pend = pl + xsizeb;pl < pend;) {
	 *pl++ = 0;
      }
   }
   *d_pfname = '\0';
   *h_pfname = '\0';
   bsbump();
}

/************** REVISION STUFF -- NEVER USED (???) *************************/
/*
 * bumps revision number of buffer in current header struct
 */
mirella void
bsbump()
{
   (pbuf->_header._serno) = rand();	/* new rev in current header */
   (pbufs[pbuffer]->_header._serno) = h_serno; /* also corresponding array
						  element -- this needs to be
						  done because if you display
						  from the current buffer, the
						  array entry may be old */
}

/*
 * does the same for buffer n
 */
mirella void 
bsbumpn(n)
int n;
{
   (pbufs[n]->_header._serno) = rand();
}

/*********************** 'DESTINATION' BUFFER ROUTINES ********************/
/************************* DBUFCHK() **************************************/
/*
 * complains if d(est) buffer is same as source
 */
void
dbufchk(d)
int d;
{
    isokbuf(d);
    if( d == pbuffer ){
        mprintf("destination buffer is same as source. OK? (y/n)");
	flushit();
        if (!_yesno()) erret((char *)NULL);
    }
}

/************************* PDEST(), BPDEST() *******************************/
/*
 * set up header and line ptr array for buffer d to receive pic of the
 * same dimensions as the current one but data size dsize bytes. Used
 * for such ops as smoothing, rotating, shifting, etc
 */
mirella void 
gpdest(d,dsize)
int d,dsize;
{
    register int i;

    seldest(d);
    if(pbufs[d]->_pbsize*sizeof(short int) < xsize*ysize*dsize) 
        erret("\nDestination buffer is not big enough");
    /* set up line pointers */
    pbufd->_header._xsize = xsized = xsize;
    pbufd->_header._ysize = ysized = ysize;
    for(i=0; i<ysize; i++) {
       picd[i] = (short *)((u_char *)picorigd + xsize*i*dsize);
    }
    /* copy header and FITS array*/
    copy_full_header(&pbufs[d]->_header,&pbuf->_header);
    copy_app_header(d);
    pbufferd = d;
}

mirella void 
pdest(d)                /* same data size as current buffer */
int d;
{ 
    gpdest(d,h_bitpix/8);
}

mirella void 
bpdest(d)               /* destination is byte array */
int d;
{
    gpdest(d,1);
    pbufs[d]->_header._bitpix = 8;
}

mirella void
spdest(d)              /* destination is short array jeg9402 */
int d;
{
    gpdest(d,2);
    pbufs[d]->_header._bitpix = 16;    
}

mirella void
npdest(d)              /* destination is normal array jeg9803 */
int d;
{
    gpdest(d,4);
    pbufs[d]->_header._bitpix = 32;    
}

mirella void
fpdest(d)              /* destination is float array jeg9803 */
int d;
{
    gpdest(d,-4);
    pbufs[d]->_header._bitpix = -32;    
}

mirella void            /* destination buffer is rgb triple */
rgbpdest(d)
int d;
{
    gpdest(d,6);
    pbufs[d]->_header._bitpix = 24;
}

/*****************************************************************************/
/*
 * Copy application part of current pbuf's header to buffer d; both
 * the mask and the FITS information
 */
static void donothing(d)
int d;
{ 
    return;
}


mirella void (*copy_app_header)(int) = donothing;

/************************* SELDEST() *********************************/
/*
 * simply selects buffer d as the one referred to by the pointer *
 * picd; user is responsible for any size mismatch, and for the
 * existence of the line pointer array; this routine is normally used
 * simply to select a buffer which already has a picture in it for use
 * as a template of some sort for another buffer. DOES NOT COMPLAIN IF
 * DEST BUFFER IS SAME AS SOURCE.
 */
mirella void 
seldest(d)
int d;
{
    isokbuf(d);
    pbufd = pbufs[d];
    pbufferd = d;
    setdpointers(pbufd->_pline);
    xsized = pbufd->_header._xsize;
    ysized = pbufd->_header._ysize;
}

/************************* SELQDEST() *********************************/
mirella void 
selqdest(d)     /* simply selects buffer d as the one referred to
                        by the pointers plined,(picd), and picorigd;
                        user is responsible for any size mismatch, and
                        for the existence of the line pointer array; this
                        routine is normally used simply to select a buffer
                        which already has a picture in it for use as a
                        template of some sort for another buffer
                        this routine is identical to seldest() but does
                        no checking */
int d;
{
    pbufd = pbufs[d];
    pbufferd = d;
    setdpointers(pbufd->_pline);
    xsized = pbufd->_header._xsize;
    ysized = pbufd->_header._ysize;
}

/************************ PCOPY() *************************************/
/*
 * copies picture buffer from current to destination d, making it contiguous
 * in the process if it wasn't
 */
mirella void 
pcopy(d)
int d;
{
    int dsize = h_bitpix/8;
    int ysz = ysize;
    int xszb = xsize * dsize;
    int i;

    pdest(d);
    /* copy picture line by line in case not contiguous */
    for(i=0;i<ysz;i++){
        bufcpy((char *)(picd[i]),(char *)pic[i],xszb);
    }
}

/******************* PCHANGE(), BPCHANGE()  **********************************/
/*
 * set up header and ptr array for buffer d to receive picture of xsize 
 * xsz, ysize ysz, data size dsize bytes. Used for extractions, 
 * repixellations, etc. 
 */
static void 
gpchange(xsz,ysz,d,dsize)
int xsz, ysz, d, dsize;
{
    register int i;

    seldest(d);
    /* set up line pointers*/
    xsized = xsize;
    ysized = ysize;
    if(xsz*ysz*dsize > pbsized*sizeof(short int)){
        mprintf(
            "\n%d x %d x %d picture will not fit in buffer %d; size %ld bytes",
            xsz,ysz,dsize,d,pbsized*dsize);
        erret((char *)NULL);
    }
    for(i=0; i<ysz; i++)
        picd[i] = (short *)((u_char *)picorigd + xsz*i*dsize) ;
    /* copy header */
    copy_full_header(&pbufs[d]->_header,&pbuf->_header);

/*    bufcpy(fitsorigs(d),fitsorig,FITSHSIZ); FITS */

    xsized = pbufs[d]->_header._xsize = xsz;
    ysized = pbufs[d]->_header._ysize = ysz;
    pbufferd = d;    
}

/*
 * prepares for a picture with the same data size as the current one
 */
mirella void 
pchange(xsz,ysz,d)
int xsz,ysz,d;
{
    gpchange(xsz,ysz,d,h_bitpix/8);
}

/*
 * prepares for a byte picture of the dim xsz, ysz
 */
mirella void 
bpchange(xsz,ysz,d)
int xsz,ysz,d;
{
    gpchange(xsz,ysz,d,1);
    pbufs[d]->_header._bitpix = 8;
}

/*
 * prepares for a short picture of the dim xsz, ysz
 */
mirella void 
spchange(xsz,ysz,d)
int xsz,ysz,d;
{
    gpchange(xsz,ysz,d,2);
    pbufs[d]->_header._bitpix = 16;
}

/*
 * prepares for a normal picture of the dim xsz, ysz
 */
mirella void 
npchange(xsz,ysz,d)
int xsz,ysz,d;
{
    gpchange(xsz,ysz,d,4);
    pbufs[d]->_header._bitpix = 32;
}

/*
 * prepares for a float picture of the dim xsz, ysz
 */
mirella void 
fpchange(xsz,ysz,d)
int xsz,ysz,d;
{
    gpchange(xsz,ysz,d,4);
    pbufs[d]->_header._bitpix = 32;
}

/******************** END, IMAGES.C ************************************/
