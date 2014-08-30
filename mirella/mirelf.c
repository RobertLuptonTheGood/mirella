/*VERSION 90/04/05:Mirella 5.40                                          */
/************************** MIRELF.C *************************************/

/* Recent history:
88/05/01: Fixed several small bugs with unix systems. The Sun does not
declare errno as a global, and twrite and tread were left out of the
function array declarations for both mira and astrovax
88/05/06: Added support in Mirella for faccess() (TRUE for exist, note)
88/10/24: Went to system in which file function definitions are contained
            in a file called 'msysdef.h', to keep the proliferation of
            systems out of this file.
89/02/12: Fixed bug which created entries for non-existent files (an erret()
            in doopen() which should have been a return(-1) ), added 
            unload() support for tapes; fixed all (ugh) system headers.
89/04/21: Fixed bug in mfgetc which left rdeof set.            
89/06/10: Fixed bug in DSI tposition which miscounted and left you AFTER eov
          !Fix other versions so report files as they go. neat.
89/07/13: Added support for the blk functions and a channel stack          
89/07/14: Added support for several kinds of disk files
90/01/16: Fixed bug which caused fputs to crash on 0-length strings;
            problem with mwrite: write() crashes on dsi68 if n=0.
90/04/05: Fixed rdeof problem with tapes.            
*/

/* This is the Mirella file-handling package with new (87/10/14) short-
buffering code   */

#ifdef VMS
#include mirkern
#include errno
#else
#include "mirkern.h"
#include <errno.h>
#endif

extern int verbose;

#define SIZBUF 300
#define LENBUF 256
#define LOFFSET 32
/* physical space for 256-byte buffer; space for pushback */

static int ichan = -1;  /* some of the tape functions need to know what
                        the channel is even if the channel is not
                        active (cf close) this is it */

#if defined(BKTAPE)

#if !defined(SOLARIS)
#  undef u_char
#  undef u_short
#endif
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mtio.h>

struct mtop tgparm;
struct mtget tsparm;
#endif

/*
 * Structure to describe special files. Note that the list must be
 * terminated with a NULL, and that the name field refers to the
 * name given in the FILE_FUNC structure.
 */
struct specfile_t {
   char opsname[32];			/* the system's name or a pattern */
   char devname[32];			/* device's true name */
   short mftype;			/* flags for special file */
   char *name;				/* type for function array */
};

struct special_names special_names[] = {
   { "*tape",    "" },
   { "*tape16",  "" },
   { "*tape62",  "" },
   { "*exabyte", "" },
   { "*spare",   "" },
   { "",	 "" },
};


static struct specfile_t specfile[] = 
    {
       {"  ", "", 0, "normal"},
#if defined(UNIX)
       {"/dev/nrst17", "", FT_TAPE, "bktape"},
       {"/dev/nrst9", "", FT_TAPE, "bktape"},
       {"/dev/nrxb0", "", FT_TAPE, "scsitape"},
       {"/dev/nrmt[0-9]?", "", FT_TAPE | FT_PATTERN, "bktape"},
       {"/dev/nrxb[0-9]", "", FT_TAPE | FT_PATTERN, "scsitape"},
       {"/dev/nrst[0-9]", "", FT_TAPE | FT_PATTERN, "scsitape"},
       {"/dev/nrsx[0-9]", "", FT_TAPE | FT_PATTERN, "scsitape"},
#endif
#ifdef DOS9TAPE
       {"", "", FT_TAPE, "dos9tape"},
#endif
#if defined(DSI386) || defined(DSI386_P)
       {"  ", "", 0, "binary"},
       {"  ", "", 0, "binary_nobuff"},
       {"  ", "", 0, "block"},
#endif
#ifdef SGI
       {"/dev/rmt/tps0d[0-9]nrnsv", "", FT_TAPE | FT_PATTERN, "bktape"},
       {"/dev/rmt/tps0d6nrnsv", "", FT_TAPE, "bktape"},
#endif
#ifdef vms
       {"  ", "", 0, "blkfile"},	/* blk files */
       {"  ", "", 0, "ansi text" }, /* ansi text files */
       {"msa0:", "", FT_TAPE, "vmstape" },
       {"msb0:", "", FT_TAPE, "tk50" },
       {"mua0:", "", FT_TAPE, "tk50" }
#endif
       {"", "", 0, NULL}
    };
/*
 * various functions
 */
static int illop(),findi(),doopen(),chkspec(),frewind();
#if defined(BKTAPE) || defined(BKSCSITAPE)
#  define NEED_LILLOP
   static long lillop();
#endif
#if !defined(openb) || defined(BKTAPE) || defined(BKSCSITAPE) || defined(DOS9TAPE) || defined(VMSTAPE)
#  define NEED_NULLOP
   static int nullop();
#  define NEED_UNLOAD
   static int unload();
#endif
static void chki(),buflush();
/*
 * Binary file I/O
 */
static int bread(), bwrite(), bunread();
static long bseek(), btell();
/*
 * Tape I/O Routines
 */
#ifdef BKTAPE
static int topen P_ARGS(( Const char *, int, int )),
  tclose(),twrite(),tread(),trewind(),skip(),rskip(),
  tfwd(),tback(),weof(),rrtposition(),ugetfno();
#endif
#ifdef BKSCSITAPE
static int scsiweof();
#endif
#ifdef DOS9TAPE
static int topen(),tclose(),twrite(),tread(),trewind(),skip(),rskip(),
    tfwd(),tback(),weof(),rrtposition(),tgetfno();
#endif
#ifdef VMSTAPE
static int vtopen(),vtclose(),vtrewind(),vtskip(),vtrskip(),vtfwd(),vtback(),
    vtweof(),vtqweof(),vtposition(),vtread(),vtwrite(),vgetfno();
    
int vopenb(),vcreata(),vcreatb();
#endif

/*
 * following are function pointers for file functions
 */
typedef struct {
   char *name;
   int (*open)P_ARGS(( Const char *, int, ... ));
   int (*creat)P_ARGS(( Const char *, mode_t ));
   int (*close)();
   int (*read)();
   int (*write)();
   long (*seek)();
   long (*tell)();
   int (*access)();
   int (*rewind)();
   int (*skip)();
   int (*rskip)();
   int (*tfwd)();
   int (*tback)();
   int (*weof)();
   int (*treten)();
   int (*tposition)();
   int (*unread)();
   int (*flush)();
   int (*getfno)();
   int (*unload)();
} FILE_FUNC;

static FILE_FUNC file_func[] = {
   {
      "normal",
      (int (*)P_ARGS(( Const char *, int, ... )))open,
      (int (*)P_ARGS(( Const char *, mode_t )))creat,
      close, bread, bwrite,
      bseek, btell, access, frewind, illop,
      illop, illop, illop, illop, illop,
      illop, bunread, bflush, illop, illop,
   },
#if !defined(openb)			/* these are probably only used
					   if the functions exist */
   {
      "binary",
      openb, creatb, close, bread, bwrite,
      bseek, btell, access, frewind, illop,
      illop, illop, illop, illop, illop,
      illop, bunread, bflush, illop, illop,
   },
   {
      "binary_nobuff",
      openb, creatb, close, readblk, writeblk,
      lseek, tell, access, frewind, illop,
      illop, illop, illop, illop, illop,
      illop, bunread, nullop, illop, illop,
   },
   {
      "block",
      openb, creatb, close, read, write,
      lseek, tell, access, frewind, illop,
      illop, illop, illop, illop, illop,
      illop, bunread, nullop, illop, illop,
   },
#endif
#ifdef BKTAPE
   {
      "bktape",
      (int (*)P_ARGS(( Const char *, int, ...)))topen,
      (int (*)P_ARGS(( Const char *, mode_t)))topen, tclose, tread, twrite,
      lillop, lillop, nullop, trewind, skip,
      rskip, tfwd, tback, weof, illop,
      rrtposition, illop, nullop, ugetfno, unload,
   },
#endif
#ifdef BKSCSITAPE
   {
      "scsitape",
      (int (*)P_ARGS(( Const char *, int, ... )))topen,
      (int (*)P_ARGS(( Const char *, mode_t )))topen,
      tclose, tread, twrite,
      lillop, lillop, nullop, trewind, skip,
      rskip, tfwd, tback, scsiweof, illop,
      rrtposition, illop, nullop, ugetfno, unload,
   },
#endif
#ifdef DOS9TAPE
   {
      "dos9tape",
      topen, topen, tclose, tread, twrite,
      illop, illop, nullop, trewind, skip,
      rskip, tfwd, tback, weof, illop,
      rrtposition, illop, nullop, tgetfno, unload,
   },
#endif
#ifdef vms
   {
      "blkfile",
      vopenb, vcreatb, closeblk, readblk, writeblk,
      lseekblk, tellblk, access, rewindblk, illop,
      illop, illop, illop, illop, illop,
      illop, illop, illop, illop, illop,
   },
   {
      "ansi text",
      open, vcreata, close, read, write,
      lseek, tell, access, frewind, illop,
      illop, illop, illop, illop, illop,
      illop, illop, illop, illop, illop,
   },
#endif
#ifdef VMSTAPE
   {
      "vmstape",
      vtopen, vtopen, vtclose, vtread, vtwrite,
      nullop, vgetfno, nullop, vtrewind, vtskip,
      vtrskip, vtfwd, vtback, vtweof, illop,
      vtposition, illop, nullop, vgetfno, unload,
   },
   {
      "tk50",
      vtopen, vtopen, vtclose, vtread, vtwrite,
      nullop, vgetfno, nullop, vtrewind, vtskip,
      illop, vtfwd, illop, vtqweof, illop,
      illop, illop, nullop, vgetfno, unload,
   },
#endif
};
#define NFTYPE (sizeof(file_func)/sizeof(file_func[0]))

#define NCHAN 10

struct fchannel{
    normal floc;     /* current place in file; 
                        set by mopen,mread,mwrite,mseek,_mrewind*/
    char *buforig;   /* origin of buffer */
    char *bufptr;    /* pointer to buffer loc if used */
    char *bufend;    /* end of buffer; these two set by low-level routines */
    int ftype;	     /* type of file: index into specfile */
    short fmode;     /* mode */
    short ffdes;     /* descriptor */
    short tapefile;  /* current file if tape */
    FILE_FUNC *funcs;			/* pointer to functions */
    short lmflg;     /* flag for last-used mode; -1 none, 0 read, 1 write;
                        set by high-level routines */ 
    char fname[64];  /* filename */
};

normal sz_fchannel=sizeof(struct fchannel);    /* 92 bytes */
static struct fchannel *chan[NCHAN];
static char *fborigin;
/* channel stack quantities */
#define CHSIZ 20
static short int chstack[CHSIZ+4]= {-1,0,0,0,0} ;
static short int *chptr = chstack+1 ;

/* exported functions in this module: */
/* Mirella Procedures */
void _mfopen(),
     _mgopen(),
     _mfclose(),
     _mclose(),
     _mseek(),
     _mchannel(),
     _fchannel(),
     _mwrite(),
     _mread(),
     showchan(),
     showspec(),
     _mrewind(),
     _mtfwd(),
     _mtback(),
     _mskip(),
     _mrskip(),
     _mweof(),
     _munload(),
     _mretension(),
     _mtgetfno();
/* and other functions exported to the Mirella environment */
int     mfgetc();           
void    mfputc();          /* these two are primitives */
void    mfungetc();
char    *mfgets();
int     mfputs();
void    puteol();

/* and a variable; the allocation size for blk files */
normal blkfsiz = 200000;

/***********************************************************************

There can be 10 open files or character devices, whose characteristics
are contained in the structure array chan.  When a device is opened, its
attributes are stored in an empty slot in this array; when closed, its
descriptor is set to -1, and the name is nulled.  The structures
themselves are stored in an area adjoining the user area, and are saved
with the dictionary. 

In early versions, open files were reopened and repositioned when
a dictionary image was read it; since 5.2, this feature has been
dropped, it proving of more pain than value.

There are two modes of using the interface.  Just as in block-oriented
forth, there is only one channel designated at a given time, though
many files can be open at a given time.  The Mirella word to activate a
channel is channel:

n channel 

designates which channel; alternatively, the channel is set by 

    filename _fchannel 

or (see mirella.m)

    fchannel filename

which searches through the channel array. The file must previously have 
been opened, by

    filenamestring mode _fopen,  (always opens binary in systems where that 
                        matters; no extended Unix mode stuff, mode = 0 1 2 
                        for read, write, r/w)
or (see mirella.m) 
                        
    mode fopen filename
    fopenr filename
    fopenw filename
    fopenrw filename                        
                        
all of which push the channel number.  Under VMS and some other systems,
there are special file types, and Mirella can support those with
appropriate definitions in msysdef.h.  Standard under VMS is support for
ordinary lf_stream files (the default C filetype, type 0) using the
interface described here, and in addition 512-byte fixed record binary
files (type 2) and variable-record-length ansi text files (type 1). They are
created in Mirella using the general open function gopen(name,mode,filetype)
(C) or    " name" mode filetype _gopen (Mirella); the usual cases are
defined in vms.m to correspond to the fopenr... words in mirella.c:

    fopenbr (fopenbw, fopenbrw) filename    ( binary files )
    fopenar (fopenaw, fopenarw) filename    ( ansi text files )
    
SPECIFYING THE TYPE ONLY DOES ANYTHING WHEN THE FILE IS CREATED; OPENING
A FILE AFTER IT IS CREATED HAS NO EFFECT ON ITS ORGANZATION, but can lead
to serious errors, since the read and write functions are tailored to the
specific types. Mirella does not check when the file is opened whether 
you have opened it as the right type; this is done purposely in order to allow
block i/o on non-block-oriented files for the sake of efficiency, but you
MUST know what you are doing.

Closing the file is done by

    n close, or filename _fclose.  

Since 5.2, there is a channel stack, and this is the preferred way to
handle channels, though the old mechanism is still available. In Mirella,
one says

    fopenr filename pushchan     to open the file and make it the active chan;
    
to close it, one only need say

    popchan

the Mirella word 

    swapchan

makes the file one down on the stack active and puts it on top. If you need to
dig farther, the word    n  rollchan  does the obvious thing        
    
      
All openings and closings are for low-level i/o; the read and write commands 
are 

    addr length write    and addr length read  

length in bytes. Obviously, if the user does not want to be bothered
with channel numbers, he/she need never be, and there is only the overhead of 
searching the list for the filename. Seeks are performed by

    offset mode seek,      mode = 0,1,2 for beginning, current pos, end;

and positions reported by

    tell

Since v5.2, Mirella supports different kinds of disk files for those
systems, principally VMS, which have different disk formats. The default
(Type 0) remains Unix-type stream files. Type 1 for all systems is the
filesystem supported by the 'blk' functions and intended for large binary
files; type 2 in VMS is variable-length files for ascii
text. Larger type identifiers are used for other devices.

*************** THE C INTERFACE ***************
This mechanism can (and should) also be used by C code in the Mirella
environment, particularly any code which leaves files open after returning
to Mirella control. There are functions supporting C code, including

mopen(name,mode)        
gopen(name,mode,type)   returns the channel number
mclose(i)               closes channel i
mseek(pos,org)          seeks to pos relative to org in active channel,
                            returns position
mtell()                 returns position in active channel
mchannel(i)              sets active channel to i
mtype()			returns ftype of currectly active channel
chan_name()		returns filename for currently active channel
mread(addr,nby)         reads nby bytes from active channel to addr
mwrite(addr,nby)        writes nby bytes to active channel from addr
desi(i)                 returns descriptor(if any) for channel i; returns
                            -1 if channel is a special device with no os
                            descriptor.

The more exotic functions are mostly coded as Mirella procedures; they
can be accessed from C directly as functions if they require no parameters,
such as _mrewind(), or can be passed parameters on the Mirella stack if
they do:  push(n); _mtfwd(); causes the tape drive if there is one to
skip n records.

The tape routines are:
_mrewind()      no parameters
_mskip()        1 parm nfiles
_mrskip()       1 parm nfiles
_mtfwd()        1 parm nrecs
_mtback()       1 parm nrecs
_mweof()        no parms
_mretension()   no parms
_munload()      no parms

**************************************************************************/

normal achan;    /* Mirella achan; active channel */
normal nread;    /* Mirella bread; bytes read. can check for error or eof */
normal rdeof;    /* Mirella rdeof; flag 0 unless eof encountered */
normal mterr;    /* Mirella mterr; tape error register*/
normal mtresid;  /* Mirella mtresid; tape residual count */
normal tfileno;  /* Mirella tfileno; tape file number */
/* these variables are exported to Mirella */
static void chkachan();

/*************************** INIT_FBUF() ********************************/
char *
init_fbuf()  /* allocates buffer memory. */
{
    fborigin = (char *)m_sbrk(NCHAN * SIZBUF);
#ifdef BDEBUG
    scrprintf("\nfborigin = %d",fborigin);
#endif
    if(!fborigin){
        scrprintf("\ninit_fbuf: Cannot allocate buffer storage");
        exit(1);
    }
    return fborigin;
}

/*************************** INIT_FILE() ********************************/
void init_file()    /* initializes file structures. No longer (v5.2) attempts
                        to reopen and reposition files open when dict image
                        was made */
{
    int i;

    achan = -1;    /* no open channels */
    for(i=0; i<NCHAN; i++){    /* initialize pointers */
        chan[i] = (struct fchannel *)V_FORIG + i;
        chan[i]->buforig = fborigin + SIZBUF*i + LOFFSET;
        chan[i]->bufptr = chan[i]->bufend = chan[i]->buforig;
    }
    for(i=0; i<NCHAN; i++){       /* reopen files from dictionary image or
                                        init structures */
        chan[i]->floc = 0;
        chan[i]->fmode = 0;
        chan[i]->ffdes = -1;
        chan[i]->tapefile = 0;
	chan[i]->ftype = 0;
        chan[i]->funcs = &file_func[0];	/* normal file */
        chan[i]->lmflg = -1;
        chan[i]->fname[0] = '\0';
    }
}

/*************************** DOOPEN() *********************************/
/* NB!!!!! Unix/Linux now has named flags, and I suspect all POSIX C compilers
 * do as well, so we must be careful. This code is emphatically NOT portable.
 */

static int 
doopen(s,mode,it) /* checks mode and opens or creats as appropriate */
int mode,it;
char *s;
{
   int ret;
   
   if(mode<0 || mode > 2){
      erret("Illegal file mode");
   }
   if((*file_func[it].access)(s,0)){
      if(mode == 0){
	 scrprintf("\nFile %s does not exist",s);
	 flushit();
	 return(-1);
      }
      else ret = (*file_func[it].creat)(s,0644);
   } else {
      if(mode == 1 && it == 0) {	/* normal file opened for write,
					   so truncate it to zero length */
	 ret = (*file_func[it].creat)(s,0644);
      } else {
	 ret = (*file_func[it].open)(s,mode,0644);
      }
   }
/* WARNING !!!! the primitives may change the mode in the file structure*/
   return ret;
}

/********************* GOPEN(), MOPEN() ***********************************/
/*
 * C function for opening a file; returns channel # 'it' is file type for
 * special disk file formats; if 0, type is 0 or special depending on name;
 */
int 
gopen(name, mode, it)
int mode;
char *name;
int it;					/* index into file_func[]; unused */
{
    int ft;				/* type of special file */
    int i;
    int des;

    /* find an empty slot if there is one, and check already open */
    for(i=0; i<NCHAN; i++){
        if(!chan[i]->fname[0]) break;
        if(!strcmp(name,chan[i]->fname)){
            /* fixed jeg0312--rhl got the precedence wrong */
            if(mode == chan[i]->fmode || (mode == 1 && chan[i]->fmode == 2)){
                scrprintf("File %s already open\n",chan[i]->fname);
                flushit();
                return  i;
            }else{
                scrprintf("File %s already open and not in requested mode\n",
                    chan[i]->fname);
                flushit();
                return -1;
            }
        }
    }
    if(i == NCHAN){
        error("Too many open files");
        return -1;
    }
    it = chkspec(name,&ft);
    
    /* special disk-file typing overrides chkspec only if the name is
        NOT special */
    ichan = i;              /* low-level open may need this */
    chan[i]->floc = 0;
    chan[i]->fmode = mode;
    chan[i]->ftype = it;
    chan[i]->funcs = &file_func[ft];
    chan[i]->bufptr = chan[i]->bufend = chan[i]->buforig;
    chan[i]->tapefile = 0;    /* this needs to be rethought, perhaps */
    chan[i]->lmflg = -1;
    strcpy(chan[i]->fname,name);
    /* we do all of this before trying to physically open the file because
    under some conditions (eg for the tape drives) the low-level routines
    manipulate the contents of the file structure. This is emphatically not
    a good idea, but that is the way it is for now. */    
    
    if((des=doopen(name,mode,ft)) == -1){
        scrprintf("\nCannot open %s",name);
        flushit();
        chan[i]->ffdes = -1;        
        chan[i]->fname[0] = '\0';
        return -1;
    }
    /* success !!! */
    chan[i]->ffdes = des;
    return i;
}

int mopen(name,mode)  /* old mopen(); type is 0 or spec depending on name */
char *name;
int mode;
{
    return gopen(name,mode,0);
}

void _mfopen()    /* Mirella procedure for opening a file, Mname _fopen */
{
    int mode;
    char *name;
    int ich;

    mode = pop;
    name = cspop;
    if((ich = mopen(name,mode)) == -1) erret((char *)NULL);
    push(ich);
}

void _mgopen()  /*Mirella procedure for opening a special file, Mname _gopen*/
{
    int mode;
    char *name;
    int ich;
    int it;
    
    it = pop;
    mode = pop;
    name = cspop;
    if((ich = gopen(name,mode,it)) == -1) erret((char *)NULL);
    push(ich);
}

/*************************** _FACCESS() ******************************/
void _faccess()     /* Mirella procedure; pushes TRUE (-1) if file exists
                        and is readable; FALSE (0) otherwise */
{
    int ft;
    int it;				/* type of file */
   
    char *name = cspop;     
    it = chkspec(name,&ft);
    if(ft != 0) push(-1);   /* special files always exist */
    else push( ((*file_func[it].access)(name,0)) ? 0 : (-1) );
}

/*************************** MCLOSE() ********************************/
void 
mclose(i)   /* closes file i  if it is open */
    int i;
{
 
    chki(i);
    ichan = i;
    if(chan[i]->fname[0]){
        buflush(i);
        (*chan[i]->funcs->close)(chan[i]->ffdes);
        chan[i]->fname[0] = '\0';
        chan[i]->ffdes = -1;
        if(i == achan) achan = -1; /* close active channel if this is it */
    }else{
        scrprintf("\nChannel %d not open",i);
        flushit();
    }
}

/*****************************************************************************/
/*
 * Return a unique filename 
 * gcc complains; fix it someday. Security is not an issue
 */
mirella char *
mir_mktemp(template)
char *template;
{
   static char sdate[10];
   static char ndate[10];
   static char tempname[64];
   static int idx=0;
   char istr[32];
   char * datestr();

   strncpy(tempname,template,32); /* truncate template at 32; no XXX are used */
   strncpy(ndate,datestr(),9);
   if( strcmp(ndate,sdate)){
      idx = 0;
      strcpy(sdate,ndate);
   } else {
      idx++;
   }
   sprintf(istr,"_%s_%05d",sdate,idx);
   strcat(tempname,istr);
   return(tempname);
}

/*************************** FINDI() *********************************/
static int 
findi(s)   /* finds the array index for the name s if exists */
    char *s;
{
    int i;
    for(i=0; i<NCHAN; i++){
        if(!strcmp(s,chan[i]->fname)) break;
    }
    if(i == NCHAN){
        scrprintf("\nNo such open file as %s",s);
        flushit();
        erret((char *)NULL);
    }
    return i;
}

/****************************** _MFCLOSE() **************************/
void _mfclose()  /* Mirella procedure for closing a file by name */
{
    char *name;
    int i;

    name = cspop;
    i = findi(name);
    mclose(i);
}

/**************************** _MCLOSE() ******************************/
void _mclose()   /* Mirella procedure for closing a file by channel number;
                    Mirella name is close */
{
    int i;

    i = pop;
    mclose(i);
}

/***************************** _MSEEK(), MSEEK() ***************************/

normal mseek(pos,org)
    normal pos;
    int org;
{
    normal posd;
    register struct fchannel *fcp = chan[achan];

    chkachan();
    posd = (*fcp->funcs->seek)(fcp->ffdes,(long)pos,(int)org);
    if(posd<0) {
        V_ERRNO=errno;
        scrprintf("\nSeek failure, seek to %d, channel %d",pos,achan);
        flushit();
    }else fcp->floc = posd;
    fcp->lmflg = -1;
    return posd;
}


void _mseek()   /* Mirella procedure for seeking in the active channel 
                    mname is seek;   pos org seek   */
{
    normal org;
    normal pos;
    normal posd;

    org = pop;
    pos = pop;
    posd = mseek(pos,org);
    push(posd);
}

/**************************** _MTELL() ********************************/
void _mtell()   /* Mirella procedure for finding position in file */
{

    chkachan();
    push( (*chan[achan]->funcs->tell)(chan[achan]->ffdes) );
}

normal mtell()
{
    chkachan();
    return  (*chan[achan]->funcs->tell)(chan[achan]->ffdes);
}

/******************* FREWIND() ***********************************/
static int frewind(des)
int des;
{
    chkachan();
    return (int)(*chan[achan]->funcs->seek)(chan[achan]->ffdes,0L,0);
}

/******************* DESI() ************************************/
int
desi(i)
int i;
{
    int des;

    chki(i);
    des = chan[i]->ffdes;
    return (des >= 0 ? des : -1 );
}


/**************** CHKACHAN(), CHKI(), CHKSPEC() *******************/
static void 
chkachan()   /* checks for existence of open channel */
{
    if( achan < 0 ) erret("No active I/O channel\n");
}

static void 
chki(i)
    int i;
{
    if(i<0 || i>NCHAN-1){
        scrprintf("\nIllegal I/O channel number %d",i);
        flushit();
        erret((char *)NULL);
    }
}

/*
 * returns special file index if s is a special file name, 0 otherwise
 * Also returns the index of the special file (allowing access to its
 * real name, flags, and so forth).
 */
static int 
chkspec(s,type)
char *s;
int *type;
{
    char buff[80];
    int i,j;

    for(i = 1;specfile[i].name != NULL;i++){
       if(specfile[i].mftype & FT_PATTERN) {
	  if(match_pattern(s,specfile[i].opsname,(char **)NULL) != NULL) {
	     strcpy(specfile[i].devname,s);
	     break;
	  }
       } else {
	  if(strcmp(s,specfile[i].opsname) == 0) {
	     strcpy(specfile[i].devname,specfile[i].opsname);
	     break;
	  }
       }
    }

    if(specfile[i].name == NULL) {	/* not a special file's real name */
       for(i = 0;*special_names[i].mfname != '\0';i++){
	  if(strcmp(s,special_names[i].mfname) == 0) {
	     if((j = chkspec(special_names[i].opsname,type)) <= 0) {
		sprintf(buff,"\n%s --> %s not found as a special file",
			s,special_names[i].opsname);
		erret(buff);
	     }
	     return(j);
	  }
       }
       *type = 0;
       return(0);
    } else {
       for(j = 0;j < NFTYPE;j++) {
	  if(!strcmp(specfile[i].name,file_func[j].name)) {
	     *type = j;
	     return(i);
	  }
       }
       sprintf(buff,"\nUnknown file type: %s",specfile[i].name);
       erret(buff);
       return(0);
       /*NOTREACHED*/
    }
}


/**************** MCHANNEL(),_MCHANNEL(),FCHANNEL() ************************/
void mchannel(i)
int i;
{
    chki(i);
    achan = i;
}

void _mchannel() /* Mirella procedure for setting channel  by number 
                    Mname channel */
{
    int i;

    i = pop;
    chki(i);
    achan = i;
}

void _fchannel()  /* Mirella procedure for setting channel by name */
{
    char *s;
    int i;

    s = cspop;
    i = findi(s);
    achan = i;
}

/************************* _MWRITE(), _MREAD() ***************************/
void _mwrite()     /* Mirella proc for writing: addr nby write */
{
    normal nby;
    char *addr;

    nby = pop;
    addr = cppop;
    if(mwrite(addr,nby) == -1) erret((char *)NULL);
}

int mwrite(addr,nby)     /* writes nby bytes to active channel from addr */
    int nby;
    char *addr;
{
    int  iw ;
    register struct fchannel *chp = chan[achan];

    chkachan();
    V_ERRNO = 0;

    if(chp->fmode == 0) erret("\nWrite-mode error: write to read-only file");
    if(chp->lmflg == 0) (*chp->funcs->flush)();
    chp->lmflg = 1;  /* writing now */

    if(nby == 0) return 0;

    iw = (*chp->funcs->write)(chp->ffdes,addr,nby);   
    chp->floc += iw;
    
    if(iw != nby){
        scrprintf(
            "\nError writing to channel %d, req %d, written %d, type %s",
            achan, nby, iw, chp->funcs->name);
        flushit();
        V_ERRNO = errno;
        return -1;
    }
    return iw;
}

/*
 * Return ftype flag of currently active channel
 */
int
mtype()
{
   chkachan();
   return(specfile[chan[achan]->ftype].mftype);
}

/*
 * Return name of currently active channel
 */
char *
chan_name()
{
   chkachan();
   return(chan[achan]->fname);
}

void _mread()     /* Mirella proc for reading: addr nby read */
{
    int nby ;
    char *addr;

    nby = pop;
    addr = cppop;
    if(mread(addr,nby) == -1) erret((char *)NULL);
}

int mread(addr,nby)     /* reads nby bytes from active channel to addr */
    int nby;
    char *addr;
{
    register struct fchannel *chp = chan[achan];
    
    rdeof = 0;
    V_ERRNO = 0;
    chkachan();
    
    if(chp->fmode == 1) erret("\nRead-mode error: read from write-mode file");
    if(chp->lmflg == 1) (*chp->funcs->flush)();
    chp->lmflg = 0;  /* reading now */
    nread = (*chp->funcs->read)(chp->ffdes,addr,nby);
    chp->floc += nread;
    if(nread == 0) rdeof = 1;
    else if(nread != nby){
        if(nread >= 0 && !errno) rdeof = 1;
        else V_ERRNO = errno;
    }
    if(rdeof) tfileno = ++(chp->tapefile);
    return nread;
}

int munread(addr,nby)     /* unreads nby bytes to active channel from addr;
                            TAKE CARE!! affects only buffer; unread data
                            never appears in the file, and is lost with
                            seeks */
    int nby;
    char *addr;
{
    register struct fchannel *chp = chan[achan];
    int nunread;
    
    rdeof = 0;
    V_ERRNO = 0;
    chkachan();
    
    if(chp->fmode == 1 )erret("\nRead-mode error: unread to write-mode file");
    if(chp->lmflg == 1) (*chp->funcs->flush)(); /* changing modes */
    chp->lmflg = 0;  /* readmode now */
    
    nunread = (*chp->funcs->unread)(addr,nby);
    chp->floc -= nunread;
    nread = -nunread;
    return nread;
}

void _munread()     /* Mirella procedure for unreading. note takes
                        address of first char, not count byte, so
                        " blat" count unread works */
{
    int nby = pop;
    char *s = cppop;
    munread(s,nby);
}

/************************* SHOWCHAN() ********************************/
void showchan()   /* Mirella procedure for displaying channels */
{
    int i,j;
    int oflg = 0;
    int c;
    int stkdp = chptr - chstack;
    int stackpos;

    for(i=0; i<NCHAN; i++)
        oflg |= chan[i]->fname[0];
    if(!oflg){
        mprintf("\nNo open channels");
        flushit();
        return;
    }
    mprintf(
"\n Ch  Filename                      imode mode desc    loc    type");
    flushit();
    for(i=0; i<NCHAN; i++){
        if(chan[i]->fname[0]){
            c = (i == achan ? '*' : ' ');
            stackpos = 0;
            for(j=1;j<stkdp;j++){
                if(chstack[j] == i) stackpos = j;
            }
            mprintf("\n%c%2d %-32s %2d   %2d  %3d  %6ld",c,i,
                chan[i]->fname,chan[i]->lmflg,chan[i]->fmode,chan[i]->ffdes,
                chan[i]->floc);
            if(stackpos) mprintf("    %2d",stackpos);                
            flushit();
        }
    }
}

/************************** SHOWSPEC() ******************************/
/*
 * Mirella procedure for displaying special files
 */
void
showspec()
{
    int i;

    for(i = 0;*special_names[i].mfname != '\0';i++){
       if(*special_names[i].opsname != '\0') break;
    }
    if(!verbose && *special_names[i].mfname == '\0') {
        mprintf("\nNo special file types are currently defined");
        return;
    }
    
    mprintf("\n    Special Files\nMirellaname          Opsysname\n\n");
    for(i=0;*special_names[i].mfname != '\0';i++){
       if(verbose || *special_names[i].opsname != '\0') {
	  mprintf("%-20s %-20s\n",special_names[i].mfname,
					special_names[i].opsname);
       }
    }
    flushit();
}

/************************ ILLOP(), NULLOP() *************************/
#if defined(NEED_LILLOP)
static long
lillop(a,b,c)
normal a,b,c;
{
   (void)illop(a,b,c);
   return(0);
   /*NOTREACHED*/
}
#endif

static int 
illop(a,b,c)
normal a,b,c;
{
    chkachan();
    scrprintf("Illegal operation for files of type %s\n",
	      chan[achan]->funcs->name);
    flushit();
    erret((char *)NULL);
    return(0);
    /*NOTREACHED*/
}

#if defined(NEED_NULLOP)
static int 
nullop(a,b,c)
    normal a,b,c;
{
    return 0;
}
#endif

/******************* CHANNEL STACK FUNCTIONS *******************************/

void pushchan(n)
int n;
{
    int i;
    
    chki(n);
    achan = n;
    *chptr++ = n;
    if((chptr - chstack) >= CHSIZ){
        scrprintf("\nChannel stack overflow");
        chstack[0] = -1;
        for(i=1;i<5;i++){
            chstack[i] = chstack[(chptr-chstack)+i-4];
            chptr = chstack + 4;
        }
    }
}

void popchan()
{
    int cchan = *(--chptr);
    
    if(cchan == -1 || chptr == chstack){
        chstack[0] = -1;
        chptr = chstack+1;
        scrprintf("\nChannel stack empty");
        return;
    }
    mclose(cchan);    
    if((cchan = *(chptr-1)) >= 0) achan = cchan;
}

void rollchan(i)
int i;
{
    int scr;
    short int *j;
    if((chptr-chstack) < i + 2){
        scrprintf("\nChannel stack empty");
        return;
    }
    scr = *(chptr -1 - i);  /* select the channel from the stack */
    for(j=chptr-1-i; j< chptr-1; j++){
        *j = *(j+1);
    }
    *(chptr-1) = (achan = scr);
}
    
void swapchan()
{
    int scr;
    if((chptr-chstack) < 3){
        scrprintf("\nChannel stack empty");
        return;
    }
    scr = *(chptr-2);
    *(chptr-2) = *(chptr-1);
    *(chptr-1) = (achan = scr);
}

/******************* SHORT-BUFFERED FILE I/O FUNCTIONS *********************/

/******************** BREAD() *******************************************/

static int bread(des,buf,n)
int des;
char *buf;
normal n;
{
    register struct fchannel *chp = chan[achan];
    register int nbchar = chp->bufend - chp->bufptr;
    int ret, nrd, nres;

    if(n>LENBUF){
        if(nbchar) bufcpy(buf,chp->bufptr,nbchar);
        chp->bufend = chp->bufptr = chp->buforig;
        ret = nbchar + read(des,buf+nbchar,n-nbchar);
        return ret;
    }else if(n<=nbchar){
        bufcpy(buf,chp->bufptr,n);
        chp->bufptr += n;
        if(n==nbchar) chp->bufend = chp->bufptr = chp->buforig;
        return n;
    }else{   /* need small number of chars, but more than are in current buf*/
        if(nbchar > 0) bufcpy(buf,chp->bufptr,nbchar);
        chp->bufend = (chp->bufptr = chp->buforig);
        /* fill buffer */
        nres = n-nbchar;                        /* how many more do I want ? */
        nrd = read(des,chp->buforig,LENBUF);
        if(nres > nrd) nres = nrd;              /* all there are, sorry */
        chp->bufend += nrd;
        if(nres>0) bufcpy(buf + nbchar, chp->buforig, nres);
        chp->bufptr += nres;
        ret = nbchar + nres ;
        return ret;
    }
}

/**************************** BWRITE() **************************************/

static int bwrite(des,buf,n)
int des;
char *buf;
normal n;
{
    register struct fchannel *chp = chan[achan];
    register int nbchar = chp->bufptr - chp->buforig;
    int ret;
    
    if(n>LENBUF){     /* long write */
        if(nbchar > 0){
            write(des,chp->buforig,nbchar);  /* empty buffer */
            chp->bufend = chp->bufptr = chp->buforig;
        }
        ret = write(des,buf,n);
        return ret;
    }else if( n < LENBUF-nbchar){   /* short enough to fit in buffer */
        bufcpy(chp->bufptr,buf,n);
        chp->bufptr += n;
        chp->bufend = chp->bufptr;        
        return n;
    }else{        /* too long to fit in buffer; flush buffer and put rest in */
        ret = write(des,chp->buforig,nbchar);
        bufcpy(chp->buforig,buf,n);
        chp->bufend = chp->bufptr = chp->buforig + n;
        return (n + ret - nbchar);
    }
}

/*********************** BUFLUSH() , BFLUSH() *****************************/

static void
buflush(i)
int i;
{
    register struct fchannel *fcp = chan[i];
    normal nbuf;
    int error;
    
    chki(i);
    if(fcp->lmflg == 1 && fcp->fmode > 0){
        nbuf = fcp->bufptr - fcp->buforig;
        error = write(fcp->ffdes,fcp->buforig,nbuf);
        if(error != nbuf){
            scrprintf("\nError flushing channel %d",i);
            bells(2);
        }
    }
    fcp->bufptr = fcp->bufend = fcp->buforig;
}

/*
 * resets buffer pointers
 */
int
bflush()
{
    chkachan();
    buflush(achan);
    return(0);
}

/*********************** BSEEK(), BTELL() **********************************/

static long
bseek(des,loc,org)
int des;
normal loc;
int org;
{
    buflush(achan);
    return(lseek(des,loc,org));
}

static long
btell(des)
int des;
{
    return(chan[achan]->floc);
}

/************************* BUNREAD() ***************************************/

static int bunread(s,n)    /* pushes back n chars from s onto file buffer. Does
                        not affect file; works only if in (instantaneous)
                        read mode. Safe only for a total of 32 characters,
                        the size of the buffer lead-in. returns number
                        actually pushed back.  */
register char *s;
int n;
{
    register struct fchannel *fcp = chan[achan];
    int i;

    chkachan();    
    for(i=n-1;i>=0;i--){
        if((fcp->bufptr - fcp->buforig) < -LOFFSET + 1){
            return (n-1 - i);
        }
        *(--(fcp->bufptr)) = s[i];
    }
    return n;   /* if get here, entirely successful */
}

/* ASCII STRING ROUTINES:
    MFGETC()
    MFUNGETC()
    MFPUTC()
    MFGETS()
    PUTEOL()
    MFPUTS()
    *******************************************************************/
    
int mfgetc()   /* this is a primitive */
{   
    char c;
    register struct fchannel *fcp = chan[achan];
    register int n;

    chkachan();
    rdeof = 0;
    if(fcp->fmode == 1 || fcp->lmflg == 1)erret(
"\nRead-mode error: read from write-mode file or w/r file last written to");
    fcp->lmflg = 0;    
    n = (*fcp->funcs->read)(fcp->ffdes,&c,1);
    nread = n;
    if(n == 1){
        (fcp->floc)++;
    }else if(n == 0){
        rdeof = 1;
        return EOF;
    }else{
        scrprintf("\nRead error on channel %d",achan);
        erret((char *)NULL);
    }
    return (int)c;
}

void mfungetc(c)
int c;
{
    char cc = c;
    register struct fchannel *fcp = chan[achan];
    register int n;

    chkachan();
    if(fcp->fmode == 1 || fcp->lmflg == 1)erret(
"\nRead-mode error: unread to write-mode file or w/r file last written to");
    fcp->lmflg = 0;    
    n = (*fcp->funcs->unread)(&cc,1);
    nread = -n;
    fcp->floc -= n;
    if(n == 0) erret("\nungetc error: too many characters");
}

/*
 * this is a primitive
 */
void
mfputc(c)
int c;
{
    char buff[80];
    char d;
    int n;
    register struct fchannel *fcp = chan[achan];
    
    chkachan();
#if 0
    if(itype == 0 && (fcp->fmode == 0 || fcp->lmflg == 0)) erret(
"\nWrite-mode error: write to read-only file or to r/w file last read from");
#else
    if(fcp->fmode == 0 || fcp->lmflg == 0) erret(
"\nWrite-mode error: write to read-only file or to r/w file last read from");
#endif
    fcp->lmflg = 1;

    d = c&255;

    n = (*fcp->funcs->write)(fcp->ffdes,&d,1);
    fcp->floc += n;
    if(n != 1) {
       sprintf(buff,"\nError writing to channel %ld", (long)achan);
       erret(buff);
    }
}

/* This fundamental routine modified 090606; before, it would read at
 * most n chars, and the file pointer would be left in mid-line if
 * n was smaller than the line length. The word is normally used to
 * read a whole line, and the behavior has been changed to only 
 * RETURN n chars in s, but ALWAYS to read till a \n or null.
 * This may break some old code. Be warned.
 */
 
char *
mfgets(s,n)   /* reads C string into s; stops at lf, EOF, or null; does
                NOT return cr or lf; null-terminates string; returns
                argument address  */
register char *s;
int n;             /* max # chars, EXCLUDING term. null ; 
                        s must have room for n+1 chars !!!! */
{   
    register int c;
    register char *ptr = s ;
    if(!n){
        nread = 0;
        *s = '\0';
        return 0;
    } 
    do{
        c = mfgetc();
        if(c == '\n' || c == EOF || !c) break;
        if(c != '\r'){  /* this is for DOS/Windoze */
            /* *ptr++ = c;
            if(ptr - s >= n) break; old code */
            /* 
             * new code; we stuff the string until we see a terminator or
             * until we have stuffed n chars, but keep reading until
             * we see a terminator.
             */
            if(ptr - s < n) *ptr++ = c; 
        }
    }while(1);
    /* termination code */    

    *ptr = '\0';
    nread = ptr - s;
    return (c != EOF ? s : (char *)NULL );   
}

/*jeg9812*/
/* This is a long-needed Mirella routine to get strings from files IGNORING
 * Mirella comments. Note that it returns an INT, which is zero for a normal
 * read, -1 for an error, or a positive integer which gives the number of
 * comment strings read. The last comment is stored in the Mirella global
 * string fgetscomment.
 */
 
char fgetscomment[256];
int nfgcom;

/* mod jeg0006--did not return #comments properly, nor deal with multiple
 * comment strings properly 
 * and jeg0209--caved in and made # a comment char as well
 */
 
int
mfgetsnc(s,n)
char *s;
int n;
{
    char *ret;
    int c = 0;
    nfgcom = 0;
    
    do{
        s[0] = '\0';
        ret = mfgets(s,n);
        if(ret == NULL) return -1;
        if((c = s[0]) != '\\' && c != '#' ) return nfgcom;
        strncpy(fgetscomment,s,255);
        fgetscomment[255] = '\0';
        nfgcom++;
    }while( c == '\\' || c == '#' );
    /* it should never fall through; EOF will exit with the mfgets */
    return -1;
}

/* 
 * Like mfgetsnc, but also skips blank lines. This is a new function,
 * for fear of breaking past code.
 */
 
int
mfgetsncb(s,n)
char *s;
int n;
{
    char *ret;
    int c = 0;
    nfgcom = 0;
    
    do{
        s[0] = '\0';
        ret = mfgets(s,n);
        if(ret == (char *)NULL) return -1;  /* EOF */
        /* mprintf("\ns[0]=%d",s[0]); */
        if((c = s[0]) != '\\' && c != '#' && c != 0 ) return nfgcom;
        if(c != '\0' ){  /* not a blank line */
            strncpy(fgetscomment,s,255);
            fgetscomment[255] = '\0';
            nfgcom++;
        }
    }while( c == '\\' || c == '#' || c == '\0');
    /* it should never fall through until it reaches the EOF */
    return -1;
}

void puteol()      /* writes an EOL to the active channel */
{
    mwrite(EOL,LEOL);
    /* EOL is a string defined in mirella.h; LEOL is its length */
}

int mfputs(s)  /* writes the C string at s to the active channel,
                and terminates with an EOL */
char *s;
{
    int len;
    
    len = strlen(s);
    len = mwrite(s,len);
    puteol();
    return len;
}

#if 0
/***************** INTERFACE TO BLK FILE FUNCTIONS *******************/
/* I think these are obsolete with new file interface...check with VMS
systems.  jeg 900405 */

int blkdes;    /* block file descriptor...careful !! */
int blkbread;  /* block # read */
int blkbwrt;   /* block # write */

void
_mopenblkr()
{
    char *name = cspop;
    blkdes = openblk(name,O_RDONLY);
    if(blkdes == -1){
        scrprintf("\nCannot open %s",name);
        erret((char *)NULL);
    }
}

void
_mopenblkw()
{
    char *name = cspop;
    blkdes = openblk(name,O_RDWR);
    if(blkdes == -1){
        scrprintf("\nCannot open %s",name);
        erret((char *)NULL);
    }
}

void
_mcreatblk()
{
    int len = pop;
    char *name = cspop;
    blkdes = creatblk(name,len);
    if(blkdes == -1){
        scrprintf("\nCannot create %s",name);
        erret((char *)NULL);
    }
}


void
_mreadblk()
{
    int n = pop;
    char *buf = cspop;

    blkbread = readblk(blkdes,buf,n);
}

void 
_mwriteblk()
{
    int n = pop;
    char *buf = cspop;
    blkbwrt = writeblk(blkdes,buf,n);
}

void
_mlseekblk()
{
    int orig = pop;
    int n = pop;
    if(lseekblk(blkdes,orig,n) == -1){
        scrprintf("\nBlock seek error");
        erret((char *)NULL);
    }
} 
   
void 
_mtellblk()
{
    push(lseekblk(blkdes,0,1));
}    
   
void
_mcloseblk()
{
    closeblk(blkdes);
}

#ifndef VMS   /* FIXME!!!!!*/

void
showblk()
{
    scrprintf("\nBlk files in this system are ordinary files");
}

#endif
#endif

/******************** MIRELLA TAPE PROCEDURES **************************/
normal tstrict=1;    /* flag for strict adherence to "no maneuver on
                                writable tape" protocol */

void twarn(s)
char *s;
{
    struct fchannel *fcp = chan[achan];
    
    if(fcp->fmode > 0 && tstrict ){
        scrprintf("\nTHIS TAPE IS OPEN FOR WRITING. YOU MAY NOT %s IT",s);
        erret((char *)NULL);
    }
}

void _mrewind()
{
    register struct fchannel *fcp = chan[achan];
    
    chkachan();
    /* this is used for both tapes and files, so protocol must be
        handled by low-level routines */
    (void)(*fcp->funcs->rewind)(fcp->ffdes);
    fcp->tapefile = tfileno = 0;
    fcp->floc = 0;
    fcp->lmflg = -1;
}

void _mskip()
{
    int n = pop;
    register struct fchannel *fcp = chan[achan];

    chkachan();
    (void)(*fcp->funcs->skip)(fcp->ffdes,n);
    fcp->tapefile += n;
    tfileno = fcp->tapefile;
    fcp->floc = 0;
}

void _mrskip()
{
    int n = pop;
    register struct fchannel *fcp = chan[achan];    

    chkachan();
    twarn("RSKIP");
    (void)(*fcp->funcs->rskip)(fcp->ffdes,n);
    fcp->tapefile -= n;
    if(chan[achan]->floc <0) fcp->tapefile = 0;
    tfileno = fcp->tapefile;
}

void _mtfwd()  /* if you tfwd over a filemark, you screw up the file
                    accounting in some systems (eg unix).
                    This should be implemented as a read, but that might
                    or might not work for SCSI tapes */
{
    int n = pop;
    register struct fchannel *fcp = chan[achan];
    
    chkachan();
    (void)(*fcp->funcs->tfwd)(fcp->ffdes,n);
}

void _mtback()  /* see comments above */
{
    int n = pop;
    register struct fchannel *fcp = chan[achan];
    
    chkachan();
    twarn("BACKSPACE");
    (void)(*fcp->funcs->tback)(fcp->ffdes,n);
}

void _mweof()
{
    register struct fchannel *fcp = chan[achan];
    chkachan();
    if(fcp->fmode == 0)
        erret("This tape is not open for writing");
    (void)(*fcp->funcs->weof)(fcp->ffdes);
    fcp->tapefile++;
    tfileno = fcp->tapefile;    
    fcp->floc = 0;
}

void _mretension()
{
    register struct fchannel *fcp = chan[achan];    
    chkachan();
    (void)(*fcp->funcs->treten)(fcp->ffdes);
}

void _munload()
{
    register struct fchannel *fcp = chan[achan];

    chkachan();
    (void)(*fcp->funcs->unload)(fcp->ffdes);  /* low-level close is in unload*/
    fcp->tapefile = 0;
    tfileno = fcp->tapefile;
    fcp->floc = 0;
    fcp->fname[0] = '\0';
    fcp->ffdes = -1;
    achan = -1; /* close active channel */
}

/*
 * positions  active unit at EOV. ONLY for reel-to reel tapes which
 * normally have an EOV
 */
void
_mtposition()
{
    register struct fchannel *fcp = chan[achan];    
 
    chkachan();
    chan[achan]->tapefile = tfileno = (*fcp->funcs->tposition)(fcp->ffdes);
}

/*
 * pushes tape file number
 */
void
_mtgetfno()
{
    int ret;

    chkachan();
    ret = (*chan[achan]->funcs->getfno)(chan[achan]->ffdes);
    push(ret);
}

void reopenr()  /* reopens tape for reading. In most systems, will
                    write an EOV if tape is open for writing. 
                    If so, will ask */
{
    register struct fchannel *chp = chan[achan];
    chkachan();
    ichan = achan;
    (*chp->funcs->close)(chp->ffdes);
    chp->fmode = 0;
    chp->lmflg = 0;
    chp->ffdes = (*chp->funcs->open)("",0,0774);
}    

void reopenw() /* reopens tape for writing */
{
    register struct fchannel *chp = chan[achan];
    chkachan();
    ichan = achan;
    (*chp->funcs->close)(chp->ffdes);
    chp->fmode = 1;
    chp->lmflg = 1;
    chp->ffdes = (*chp->funcs->open)("",1,0774);
}    


/********************** BKUNIX TAPE STUFF *****************************/
#ifdef BKTAPE
/* routines to support tape drives on Berkeley systems */

static void
get_tstat()
{
    ioctl(chan[achan]->ffdes,MTIOCGET,(char *)&tsparm);
    mtresid = tsparm.mt_resid;
    mterr = tsparm.mt_erreg;
}

/*
 * this does not work well,because of problem with tfwd'ing over filemarks. ???
 */
static int
ugetfno(des)			
int des;
{
    return(chan[achan]->tapefile);
}
       
  
static int 
tread(des,addr,nby)
int des,nby;
char *addr;
{
    int ret;
    
    ret = read(des,addr,nby);
    get_tstat();
#ifdef XYTAPE
    /* following works for MIRA xylogics tape controller */
    if(tsparm.mt_erreg & 0x1e) rdeof = 1;
#endif
    return ret;
}

static int
twrite(des,addr,nby)
int des,nby;
char *addr;
{
    int ret;
    
    ret = write(des,addr,nby);
    get_tstat();
    return ret;
}    
    

static int 
topen(s,mode,dummy)    /* opens drive for reading or writing */
Const char *s;
int mode;
int dummy;
{
    if(mode == 2 && tstrict){ 
        mode =0;      /* cannot read/write */
        chan[ichan]->fmode = 0;        
        scrprintf(
        "\nMirella tapes cannot be read/write; I am opening for reading");
    }
    if(mode==1){
        mode =2;     /* cheat */
        chan[ichan]->fmode = 2;
    }
    return(open(specfile[chan[ichan]->ftype].devname,mode));
}

static int 
tclose(des)      /* closes it */
    int des;
{
    if(chan[ichan]->fmode > 0 && tstrict){
        scrprintf(
"\nThis tape is open for writing. I am going to write an EOF Right Here. OK? ");
        flushit();
        if(!_yesno()) erret((char *)NULL);
        weof(des);
    }
    return close(des);
}

static int 
trewind(des)    /* rewinds it. CAREFUL: if a tape open for writing is
                rewound and then closed, an EOF is written at the
                beginning of the tape. This is seldom the desired result;
                either use the drives in the (usually default) mode of
                rewind-upon-close, or be sure to physically dismount the
                tape before closing the file, either on purpose or not--
                by, for instance, exiting Mirella, or doing something
                sufficiently outrageous that Mirella crashes. The only
                SAFE thing to do is never to maneuver on a tape open
                for writing; such a tape should always be positioned
                at the end of the data. This is assured in the new
                (4/88) code unless one maliciously reopens a rewound
                tape for writing */
int des;                
{
    int ret;
    chkachan();
    twarn("REWIND");    
    tgparm.mt_op = MTREW;
    tgparm.mt_count = 1;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
/*    get_tstat();  */
    return ret;
}

static int 
skip(des,n)  /*  skips n files */
    int des;
    int n;
{
    int ret;
    chkachan();
    tgparm.mt_op = MTFSF;
    tgparm.mt_count = n;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
/*    get_tstat(); */
    return ret;
}

static int 
rskip(des,n)  /* reverse skips n files */
int des;
int n;
{
    int ret;
    
    chkachan();
    tgparm.mt_op = MTBSF;
    tgparm.mt_count = n;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
/*    get_tstat();   */
    return ret;
}

static int 
tfwd(des,n)     /* skips forward n records */
int n;
int des;
{
    int ret;
    chkachan();
    tgparm.mt_op = MTFSR;
    tgparm.mt_count = n;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
/*    get_tstat();  */
    return ret;
}

static int 
tback(des,n)   /* skips back n records */
int des;
int n;
{
    int ret;
    chkachan();
    tgparm.mt_op = MTBSR;
    tgparm.mt_count = n;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
    get_tstat();
    return ret;
}

static int 
weof(des)     /* writes a file mark; actually writes two file marks
                and skips back over the second, so a tape always
                has an EOT. See the remarks above about rewind */
int des;
{
    int ret;
    tgparm.mt_op = MTWEOF;
    tgparm.mt_count = 2;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
    tback(des,1);
    get_tstat();
    return ret;
}

#if defined(NEED_UNLOAD)
static int 
unload(des)  /*  rewinds and takes offline */
    int des;
{
    int ret;
    chkachan();
    tgparm.mt_op = MTOFFL;
    tgparm.mt_count = 1;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
    if(ret==-1){
        scrprintf("\nUNLOAD:Error unloading tape");
        return(-1);
    }
    ret = close(des);        
/*    get_tstat(); */
    return ret;
}
#endif

static int 
rrtposition(des)
int des;
{
    char **memchan = (char **)memalloc("tbuffer",32768);
    char *buf = *memchan;
    int error,ret=0;

    tback(des,1);
    do{
        skip(des,1);
        error = read(des,buf,32768);
        if(error){
            ret++;
        }
        if(!rdeof && errno != 0){
            memfree("tbuffer");
            erret("tape read error");
        }
    }while(error);
    memfree("tbuffer");
    return ret;
}


#ifdef BKSCSITAPE
/* routines to support QIC streaming drives on Berkeley systems */

#define SCSIBLKERR  777
static int 
scsiweof(des)  /* writes an EOF; tape cannot back up, so only one */
int des;
{
    int ret;
    tgparm.mt_op = MTWEOF;
    tgparm.mt_count = 1;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
/*    get_tstat();*/
    return ret;
}

#ifndef MTRETEN
#define MTRETEN 0
#endif

#if defined(BKSTREAMING)
static int
retension(des)  /* retensions the cartridge */
int des;
{
    int ret;
    
    if(!MTRETEN) erret("\nI do not know how to retension this drive");   
    tgparm.mt_op = MTRETEN;
    tgparm.mt_count = 1;
    ret = ioctl(des,MTIOCTOP,(char *)&tgparm);
/*    get_tstat();*/
    return ret;
}

static int 
scsiwrite(des,pt,n)  /* SCSI tapes write in 512 byte blocks whether
                        you like it or not; if you write a record which
                        is not a multiple of 512 bytes in length, this
                        routine pads the end with zeros */
    int des;
    char *pt;
    int n;
{
    u_char ebuf[512];
    int nblk = (n-1)/512;
    int nhead = nblk*512;
    int ntail = n - nhead;
    int nwrt = 0;
    if(ntail == 512){
    	ntail = 0;
    	nhead += 512;
    	nblk += 1;
    }


    bzero((char *)ebuf,512);  /* clear ebuf */
    if(nblk){
        nwrt = write(des,pt,nhead);
/*debug*/ scrprintf("\nnwrt,nblk,nhead,ntail: %d %d %d %d",nwrt,
                nblk,nhead,ntail);
	if(nwrt != nhead){
            return nwrt;
	}
    }
    if(ntail){
        bufcpy((char *)ebuf,&pt[nhead],ntail);
        nwrt += write(des,(char *)ebuf,512);
/*debug*/ scrprintf("\nnwrt,nblk,nhead,ntail: %d %d %d %d",nwrt,nblk,nhead,
                ntail);
    }
/*    get_tstat();       trouble    */
    if (nwrt < nhead+512) return nwrt;
    if(nwrt != n) errno = SCSIBLKERR;
    return n;
}

static int 
scsiread(des,pt,n)  /* this routine returns exactly n bytes, whether
                    or not n is divisible by 512. Be warned, however, that
                    a physical read always reads an integral number of
                    blocks, so you may not be where you think you are on
                    the tape. both this and the write routine return
                    an error code if n is not a multiple of 512. */
    int des,n;
    u_char *pt;
{
    u_char ebuf[512];
    int nblk = (n-1)/512;
    int nhead = nblk*512;
    int ntail = n - nhead;
    int nrd= 0;
    if(ntail == 512){
    	ntail = 0;
    	nhead += 512;
    	nblk += 1;
    }

    if(nblk){
        nrd = read(des,(char *)pt,nhead);
    }
    if(nrd != nhead) return nrd;
    if(ntail){
        nrd += read(des,(char *)ebuf,512);
        if (nrd < nhead+512) return nrd;
        bufcpy((char *)&pt[nhead],(char *)ebuf,ntail);
    }
    if(nrd != n) errno = SCSIBLKERR;
/*    get_tstat();*/    /* trouble */
    return nrd;
}
#endif

#endif
#endif

/************* VMS TAPE STUFF (USES R. DEVERILL'S MIRTPKG) *****************/
/* These routines require phy_io privilege */

#ifdef VMSTAPE

extern int mtwtva[];    /* current file position in mtpckg; array with
                        entry for each drive */
                        
static int cur_unit = -1;
static int *ptu = &cur_unit;
static int result;
static int *pres = &result;
static int vstatus;
static int *pstat = &vstatus;

static int vgetfno(des)
int des;
{
    return mtwtva[des];
}

static int 
vtopen(s,mode,dummy)
char *s;
int mode;
int dummy;
{
    int sresult;
    if(mode == 2 && tstrict){ 
        mode =0;   /* No r/w tapes in Mirella */
        scrprintf(
        "\nMirella tapes cannot be read/write; I am opening for reading");
        chan[ichan]->fmode = 0;
    }
    if(mode == 1){
        mode == 2;  /* cheat */
        chan[ichan]->fmode = 2;
    }
    mtopenq(ptu,_sstrdes(specfile[chan[ichan]->ftype].devname),pres,pstat);
    if(result) vmsmsg(vstatus);
    mtsens(ptu,pstat,&sresult);
    if(sresult == 2) scrprintf("\n\007\007WARNING: TAPE UNIT IS OFFLINE !!");
    if(sresult)vmsmsg(vstatus);
    return (result ? -1 : cur_unit);   /* current unit plays role of des */
}

static int 
vtclose(des)
int des;
{
    int ret;
    if((chan[ichan]->fmode) > 0 && tstrict){
        scrprintf(
"\nThis tape is open for writing. I am going to put an EOF RIGHT HERE. OK?");
        if(!_yesno()) erret((char *)NULL);
        vtweof();
    }
    ret = mtclos(ptu,pres);
    return(result ? -1 : 0);
}

static int 
unload(des)
{
    if( mtunld(ptu,pres)== -1){
        scrprintf("\nError unloading tape");
    }
    return(result ? -1 : 0);
}

static int 
vtread(des,addr,nby)
    int des,nby;
    char *addr;
{
    int count;
    mtread(&des,&nby,addr,&count,&result);
    switch(result){
    case 0: return count; 
    case 1: return (-1); 
    case 2: erret("\nTAPE UNIT OFFLINE");
    case 3:
        rdeof = 1;
        error("\nEOF encountered");
        return count;
        break;
    case 4: erret("\nEOT encountered");
    default: erret("\nIllegal return code from mtwrite = %d ????",result);
    }
}

static int 
vtwrite(des,addr,nby)
    int des,nby;
    char *addr;
{
    int count;
    mtwrit(&des,&nby,addr,&count,&result);
    switch(result){
    case 0: return count; break;
    case 1: return (-1); break;
    case 2: erret("\nTAPE UNIT OFFLINE");
    case 3: erret("\nTAPE UNIT WRITE PROTECTED");
    case 4: erret("\nEOT encountered");
    default: erret("\nIllegal return code from mtwrite = %d ????",result);
    }
}

static int    
vtrewind(des)
int des;
{
    twarn("REWIND");    
    mtrewi(&des,&result);
    if(result) error("\nError in rewinding tape");
    return (result ? -1 : 0);
}

static int 
vtfwd(des,n)
    int des,n;
{
    int count;
    mtskre(&des,&n,&count,&result);
    n= ( n<0 ? -n: n);
    if(count != n)scrprintf("\nSkip count: %d does not match input: %d",
        count,n);
    switch(result){
    case 0: return count; 
    case 1: erret("\nError skipping records"); 
    case 2: erret("\nTAPE UNIT OFFLINE");
    case 3: error("\nEOF encountered"); return count;
    case 4: erret("\nEOT encountered");
    default: return(-1);
    }
}

static int vtback(des,n)
int des,n;
{
    return (vtfwd(des,-n));
}

static int 
vtskip(des,n)
    int des,n;
{
    int count;
    mtskfi(&des,&n,&count,&result);
    n= ( n<0 ? -n : n);
    if(count != n)scrprintf("\nSkip count: %d does not match input: %d",
        count,n);
    switch(result){
    case 0: return count; 
    case 1: erret("\nError skipping files"); 
    case 2: erret("\nTAPE UNIT OFFLINE");
    case 4: erret("\nEOT encountered");
    default: return(-1);
    }
}

static int vtrskip(des,n)
int des,n;
{
    return(vtskip(des,-n));
}
    
static int vtweof(des)
int des;
{
    int i;
    for(i=0;i<2;i++){
        mtweof(&des,&result);
        if(i==1) switch(result){
        case 0: break; 
        case 1: scrprintf("\nError writing EOF"); return(-1); 
        case 2: scrprintf("\nTAPE UNIT OFFLINE"); return(-1);
        case 3: scrprintf("\nTAPE UNIT WRITE PROTECTED"); return(-1);
        default: 
            scrprintf("\nIllegal return code from mtweof = %d ????",result);
            return (-1);
        }
    }
    vtrskip(des,1);
    return 0;
}

static int vtqweof(des)  /* for streamer tapes; only one eof, no backspace */
int des;
{
    mtweof(&des,&result);
    switch(result){
    case 0: return 0; 
    case 1: erret("\nError writing EOF"); 
    case 2: erret("\nTAPE UNIT OFFLINE");
    case 3: erret("\nTAPE UNIT WRITE PROTECTED");
    case 4: erret("\nEOT encountered");
    default: erret("\nIllegal return code from mtweof = %d ????",result);
    }
}

    
static int vtposition(des)  /*Find the EOV; Hester's routine*/
int des;
{
char tape_buf[100];
int skpd_files, count, res;

scrprintf("\n  0 files");
for(skpd_files=0; ;skpd_files++) {
   mtread(&des,&100,tape_buf,&count,&res);
   if(res==3) {			/* Read an EOF */
      if(skpd_files!=0) {	/* Skip initial EOF */
         mtskfi(&des, &(-2), &count, &res);
         mtskfi(&des, &1, &count, &res);
         return(skpd_files+1);
         }
      }
   mtskfi(&des, &1, &count, &res);
   scrprintf("\r%3d",skpd_files+1);
   }
}

#endif

#ifdef DOS9TAPE

/********************** DOS/OVERLAND TAPE ROUTINES ************************/
/* This section was extensively revised in 92june to accomodate 386 control
 * of the overland 9-track tape system. Some of the functions were moved
 * to mir386pl.c and mirdsi68.c where the interrupt-handling differences
 * in the two systems could be hidden. The routines moved were dos_bin_mode(),
 * dos_ioctl(), and two general read/write routines dosread() and doswrite().
 */
 

#define TBASE 0x0360
/* base i/o address of Overland port set */
#define TRDY 0x04
#define TFPT 0x08
#define TONL 0x10
#define TBOT 0x40
#define TEOT 0x80
/* bit masks for status word, port 0 */
#define TERR 0x2000
#define TFMK 0x8000
/* bit masks for status word, port 1 */
#define TOVF 0x080000
#define TPAR 0x100000
#define TMPAR 0x200000
/* bit masks for error word, port 2 */
#define DSIBUF 7168
/* intrinsic size of DSI68 buffer */
#define MAXBUF 32768
/* maximum supported blocksize */

static int
tstatus() /* gets the 24-bit status bit map from the Overland controller*/
{
    int stat=0;
    
    stat = (INPUT(TBASE)&0xff) + ((INPUT(TBASE+1)&0xff)<<8);
    if(stat & TERR) stat |= ((INPUT(TBASE+2)&0xff)<<16);   /* error */
    return stat;
}
    

static unsigned ctl_str=0; /* pointer to control string in 8086 address space */
static unsigned short int dsstr;
static unsigned short int dxstr;  /* segment and offset for ctl_str */
static unsigned tbuff=0;   /* pointer to tape buffer in 8086 address space */
static unsigned short int dstbuff;
static unsigned short int dxtbuff; /* segment and offset for tbuff */

static int
topen(fname,mode,dum)
int mode,dum;
char *fname;
{
    int ret;
    int tstat;
    int plow;
    int phigh;
    int plen = 0x10000;
    int i;
    
    
    if(!((tstat = tstatus()) & TRDY)){
        scrprintf("\nCANNOT OPEN TAPE:  NOT READY");
        return (-1);
    }
    if( mode >0 && (tstat & TFPT)){
        scrprintf("\nCANNOT OPEN FOR WRITING: TAPE WRITE PROTECTED");
        return (-1);
    }
    if(mode == 2 && tstrict){
        scrprintf(
    "\nMirella tapes cannot be opened for read/write. I am opening for read");
        mode = 0;
        chan[ichan]->fmode = 0;
    }
    if(mode == 1){ 
        mode = 2;   /* cheat */
        chan[ichan]->fmode = 2;
    }
    if(!tbuff){                       /* first time thru */
        tbuff = malloc86(MAXBUF);   /* allocate space in 8086 land */
        if(!tbuff){ 
            erret("\nTOPEN: Cannot alloc 8086 space for buffer");
            tbuff = 0;
        }
        /* calc seg ond offset */
        dstbuff = tbuff >> 4;
        dxtbuff = tbuff - (dstbuff<<4);
#ifdef TODEBUG
        scrprintf("\nTOPEN:tbuff,ds,dx: %x %x %x",tbuff,dstbuff,dxtbuff);
#endif
    }
    ret = open(specfile[itype].opsname,mode);
    if(ret >=0) dos_bin_mode(ret);
    return ret;
}

static int
unload(fdes)
int fdes;
{
    int i;
    register struct fchannel *chp = chan[ichan];  /* careful !! */
        
    if((chp->fmode) > 0 && tstrict){ 
        scrprintf(
    "\nI am going to write an EOV RIGHT HERE. Is that what you want??");
        if(!_yesno()) erret(1);
        weof();
    }
    dos_ioctl(fdes,"tu");   /* unloads */
    close(fdes);
    if(tbuff) free86(tbuff);   /* free the buffer space */
    tbuff = 0;         /* mark as deallocated */
}

static int
tclose(fdes)
int fdes;
{
    int i;
    register struct fchannel *chp = chan[ichan];  /* careful !! */
        
    if((chp->fmode) > 0){ 
        scrprintf(
    "\nI am going to write an EOF RIGHT HERE. Is that what you want??");
        if(!_yesno()) erret(1);
    }
    dos_ioctl(fdes,"t0");
    close(fdes);
    if(tbuff) free86(tbuff);   /* free the buffer space */
    tbuff = 0;         /* mark as deallocated */
}

static int
tread(fdes,buf,n)
u_char *buf;
int n;
int fdes;
{
    int ret;
    int tstat;
    R86 regs;
    
    register struct fchannel *chp = chan[achan];
    if(!(tstat = tstatus()) & TRDY) erret("\nTAPE NOT READY");
    /* must in general use 8086 buffer and transfer */
    if(n > 0x8000) erret("\nTREAD: Max buffer size 32K. Perdon!");
    ret = dosread(fdes,dstbuff,dxtbuff,n);    
    if(ret > 0){
#ifdef DSI68
        MOVE_FROM(buf,tbuff,n);   /* get the data. ARG ORDER screwed up! */
#else        
        MOVE_FROM(tbuff,buf,n);   /* get the data. */
#endif
    }
    if(ret == 0){   /* have read file mark; must deal with mickey-mouse
                        Overland interface */
        close(fdes);       
        if(chp->fmode == 2) chp->fmode = 1; /*mickeymouse to suppress message*/
        fdes = topen(specfile[itype].opsname,chp->fmode,0);
        if(fdes== -1){    /* trouble!!! */
            chp->ffdes = -1;
            chp->fname[0] = '\0';
            erret("\nCANNOT REOPEN TAPE AFTER READING FILE MARK");
        }
        chp->ffdes = fdes;
        rdeof = 1;
    }
    return ret;   /* note that mread() handles file number accounting in this
                        case */
}

static int
twrite(fdes,buf,n)
u_char *buf;
int n;
int fdes;
{
    int ret;
    int tstat;
    R86 regs;
    
    register struct fchannel *chp = chan[achan];
    if(!(tstat = tstatus()) & TRDY) erret("\nTAPE NOT READY");
    if(tstat & TFPT) erret("\nTAPE WRITE-PROTECTED");
    /* must in general use 8086 buffer and transfer */
    if(n > 0x8000) erret("\nTWRITE: Max buffer size 32K. Perdon!");
    MOVE_TO(buf,tbuff,n);   /* move the data to the 8086 */
    ret = doswrite(fdes,dstbuff,dxtbuff,n);
    return ret;
}
            
static int
skip(fdes,n)
int n;
int fdes;
{
    int i;
    for(i=0;i<n;i++){
        dos_ioctl(fdes,"f+");
    }
    return n;
}

static int
rskip(fdes,n)
int fdes;
int n;
{
    int i;
    for(i=0;i<n;i++){
        dos_ioctl(fdes,"f-");
    }
    return n;
}

static int
tfwd(fdes,n)
int fdes;
int n;
{
    int i;
    register struct fchannel *chp = chan[achan];    
        
    for(i=0;i<n;i++){
        dos_ioctl(fdes,"r+");
    }
    if(tstatus() & TFMK){
        (chp->tapefile)++;
        tfileno++;
    }
    return n;
}
        
static int
tback(fdes,n)
int fdes;
int n;
{
    int i;
    register struct fchannel *chp = chan[achan];    
    
    for(i=0;i<n;i++){
        dos_ioctl(fdes,"r-");
    }
    if(tstatus() & TFMK){ 
        (chp->tapefile)--;    
        tfileno--;
    }
    return n;
}

static int
trewind(fdes)
int fdes;
{
    register struct fchannel *chp = chan[achan];

    twarn("REWIND");        
    /* this is tricky because Overland does not provide a simple 
    rewind command, so concerned are they with hand-holding */
    close(fdes);
    fdes = topen(fdes,0,0);
    if(fdes== -1){    /* trouble!!! */
        chp->ffdes = -1;
        chp->fname[0] = '\0';
        erret("\nCANNOT REOPEN TAPE TO REWIND");
    }
    dos_ioctl(fdes,"t-");
    fdes = topen(fdes,chp->fmode,0);
    if(fdes== -1){    /* trouble!!! */
        chp->ffdes = -1;
        chp->fname[0] = '\0';
        erret("\nCANNOT REOPEN TAPE AFTER REWIND");
    }        
    chp->ffdes = fdes;
}

static int
weof(fdes) /* writes two file marks and backspaces over the second*/
int fdes;
{
    dos_ioctl(fdes,"fw");
    dos_ioctl(fdes,"fw");
    waitfor(100);
    dos_ioctl(fdes,"r-");
}

static int
rrtposition(des)
int des;
{
    int error,ret=0;
    int tstat;

    rskip(des,1);
    scrprintf("\n  0  files");
    do{
        skip(des,1);
        tfwd(des,1);
        tstat = tstatus();
        ret++;
        scrprintf("\r%3d",ret);
        if(tstat & TERR){
            erret("tape read error");
        }
    }while(!(tstat & TFMK));
    tback(des,1);
    return ret;
}

static int
tgetfno(des)  
int des; 
{
    return(chan[achan]->tapefile);
}

#endif /* DOS9TAPE */
