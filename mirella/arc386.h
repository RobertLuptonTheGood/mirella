/************** NPD/PHAR LAP for ARC AMI 386 ********************/
#ifdef SYSHEADER

#include <strings.h>

/* system */
#define NDP_PL
#define DOS32
#define I386

/* i/o definitions: */


/*signals*/
#define DSISIGNALS

/* text display adapter for DOS systems */
#define VGA
/* graphics display */
/*#define VGA*/
/* graphics printer */
#define NOGPRINT
/* image display. ati vga */
#define ATIVGA

/* id strings */
#define G_DISPLAY  "vga"
#define IM_DISPLAY "ativga"
#define G_PRINTER  ""
#define M_SYSTEM   "386dos"
#define M_OPSYS    "pl386dos"
#define M_CPU      "i386"
#define M_CCOMP    "NDP"

/* definition of normal--int for 32 bit machines, long for 16-bit ints */
#define normal int

/*hardware/compiler characteristics: default byteorder is DEC-Intel, #define
IBMORDER if not; default shifts are ARITHMETIC, #define LOGCSHIFT if logical;
default chars are SIGNED; #define DEFUCHAR if unsigned */
/* i386 -- all default, I think */

/* Mirella uses index/rindex; your system may use strchr/strrchr  */
#define index   strchr
#define rindex  strrchr

/* these fix screwups in the runtime library and headers for the system */

/* the Greenhills C compiler thinks it is a unix compiler */
#ifdef unix
#undef unix
#endif   
#define iscntrl iscntl
/* someuns cant spel */

/* Mirella uses clearn(n,buf) to bytewise clear an n-byte region with origin
buf, and filln(n,buf,ch) to fill such a region with char ch: */
/* the NDP system is OK */

/* some systems make distinctions between opening and creating text files
and binary files, and so Mirella must make the distinction; if your system
does not, you must make macros defining openb, creatb, fopenb, fopena
(which Mirella uses) appropriately */

/* the NDP environment determines whether files are to be opened ascii or
binary by the setting of a global, _pmode (!!!). fopena(), fopenb() are
appropriately defined in mirpl386.c, but are declared here; also
creatb(), openb() */

FILE *fopena();
FILE *fopenb();

/* below are end-line strings for the character functions in mirelf;
DOS files use cr-lf. LEOL is the length of the line terminator. */
#define EOL "\r\n"
#define LEOL 2

/* file parameters */
/* there is no file.h or fctrl.h in the NDP world; we define the 
flags here. */

#define O_RDONLY    000
#define O_WRONLY    001
#define O_RDWR      002

/* blocksize -- this should be tailored to the system buffer size for
systems which do not use fixed block sizes, and should be the block size
for those systems */
#define DBLKSIZE 8192

/* flag for fixed-length disk records, in readblk() and writeblk() */
#define DFIXREC 0

/* These macros are for creating and opening large binary files; see the
functions readblk and writeblk in mirsys.c */
#define creatblk(file,siz) creatb(( file ) , 2)
#define openblk(file,flags) openb(( file ) , (flags))
#define tellblk  tell
#define lseekblk lseek
#define closeblk close

/****** DOS stuff ******************************************************/

/* this is for uniformity with the register structure definitions in
doscalls.h in the DSI systems */

struct r86arr {
    unsigned short ax, l_ax;
    unsigned short bx, l_bx;
    unsigned short cx, l_cx;
    unsigned short dx, l_dx;
    unsigned short si, l_si;
    unsigned short di, l_di;
    unsigned short flags, l_flags;
};
/* the flag bits are :
    carry       1
    parity      4
    auxcarry    16
    zero        64
    sign        128
    trap        256
    interrupt   512
    dirf        1024
    ovflow      2048
*/

#define R86 struct r86arr

struct rregs86{
    unsigned short int   intno;
    unsigned short int   ds;
    unsigned short int   es;
    unsigned short int   fs;
    unsigned short int   gs;
    unsigned short int   eax;
    unsigned short int   eaxdum;
    unsigned short int   edx;
    unsigned short int   edxdum;    
    unsigned short int   ecx;
    unsigned short int   ecxdum;    
    unsigned short int   ebx;
    unsigned short int   ebxdum;    
    unsigned short int   ebp;
    unsigned short int   ebpdum;
    unsigned short int   esi;
    unsigned short int   esidum;
    unsigned short int   edi;
    unsigned short int   edidum;
    unsigned short int   flgs;
};


#define RR86 struct rregs86

#define EXTEND(x) *(u_char **)(&(x))
/* for addressing the extended 386 registers in regs; usage is
     EXTEND(regs.bx) = ebx;  or vice versa; it may not work. but seems to.
*/


/* memory moves to/from DOS memory; the DOS address (to in MVE_TO, from in
MOVE_FROM) is an absolute address (offset from 0x0000)  */

#define MOVE_TO(from,to,n)      blk_bm(( from ),0x034,( to ),( n ))
#define MOVE_FROM(from,to,n)    blk_mb(( to ),0x034,( from ),( n ))

/* in/out from 386 ports */

#define OUTPUT outp
#define INPUT inp
       
#endif SYSHEADER


/* is part for mirelf file function declaration */
#ifdef MIRELF

/*************************  NDP386 version *******************************/
/* NPD/PL 386 version, no special devices. The only difference from
the unix version is the existence of openb and creatb as functions
in this environment */

#define NFTYPES 3
#define NSTYPES 3
/* only standard opsys files */
extern int openb(),creatb();   /* these are FUNCTIONS in the NDP
                                environment; they are defined as macros
                                in mirella.h for most other systems */
extern int opena(),creata(),readblk(),writeblk(); 

static struct specfile_t specfile[NFTYPES] = 
{   {" ","  "},     /* binary files */
    {" ","  "},     /* ditto, no buffering; these are actually not accessible */
    {" ","  "} }    /* blk files */  
;

/* following are function pointer arrays for file functions; on the
    DSI, only opsys files are supported */
int     (*_open[NFTYPES])()     = {openb,openb,openb};
int     (*_creat[NFTYPES])()    = {creatb,creatb,creatb};
int     (*_close[NFTYPES])()    = {close,close,close};
int     (*_read[NFTYPES])()     = {bread,readblk,read};
int     (*_write[NFTYPES])()    = {bwrite,writeblk,write};
normal  (*_seek[NFTYPES])()     = {bseek,lseek,lseek};
normal  (*_tell[NFTYPES])()     = {btell,tell,tell};
int     (*_access[NFTYPES])()   = {access,access,access};
int     (*_rewind[NFTYPES])()   = {frewind,frewind,frewind};
int     (*_skip[NFTYPES])()     = {illop,illop,illop};
int     (*_rskip[NFTYPES])()    = {illop,illop,illop};
int     (*_tfwd[NFTYPES])()     = {illop,illop,illop};
int     (*_tback[NFTYPES])()    = {illop,illop,illop};
int     (*_weof[NFTYPES])()     = {illop,illop,illop};
int     (*_treten[NFTYPES])()   = {illop,illop,illop};
int     (*_tposition[NFTYPES])()= {illop,illop,illop};
int     (*_unread[NFTYPES])()   = {bunread,illop,illop};
int     (*_flush[NFTYPES])()    = {bflush,nullop,nullop};
int     (*_getfno[NFTYPES])()   = {illop,illop,illop};
int     (*_unload[NFTYPES])()   = {illop,illop,illop};


#endif MIRELF
/************************** END, ARC386.H ********************************/


