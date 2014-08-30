/*********************** UNIX.H  ***********************************/
#ifdef SYSHEADER

#if defined(SGI) || defined(SOLARIS)
#  define SAVED_PCS _POSIX_C_SOURCE
#  undef _POSIX_SOURCE			/* define timeval in sys/time.h */
#  undef _POSIX_C_SOURCE		/* define timeval in sys/time.h */
#  include <sys/time.h>
#  define _POSIX_SOURCE
#  define _POSIX_C_SOURCE SAVED_PCS
#else
#  include <sys/time.h>
#endif
#include <sys/file.h>
#include <string.h>

/*system*/
#undef unix
/* use ours */
#define unix
/*#define bkunix*/

/* graphics display--4010 graphics on suitable terminals. */
#define NOGDISPLAY   /* for now */
/*#define G4010*/
/*#define XTERM*/				/* a subset of G4010 */
/* graphics printer. none*/
#define NOGPRINT
/* image display */
#define X11


/* id strings--possible values are in minstall.def, and can be added to there*/ 

#define G_DISPLAY  "xv"
#define IM_DISPLAY "x11"
/* 
 *The operating system of unix type is expected to be one of
 * bkunix
 * sysV
 * unix (some generic)
 * linux
 * osX 
 */
#define M_OPSYS    "linux"
#define M_CPU      "i386"
#define M_CCOMP    "gcc"

/* definition of normal--int for 32 bit machines, long for 16-bit ints */
/* moved to mirella.h -- what do we do for OSX ?? */
/* #define normal int */

/*hardware/compiler characteristics: default byteorder is DEC-Intel, #define
IBMORDER if not; default shifts are ARITHMETIC, #define LOGCSHIFT if logical;
default chars are SIGNED; #define DEFUCHAR if unsigned */
#define IBMORDER
#ifdef __BYTE_ORDER
# if __BYTE_ORDER == __LITTLE_ENDIAN
#   undef IBMORDER
# endif
#endif

/* Mirella uses strchr/strrchr; sufficiently ancient systems may still
 use index/rindex */

/* these fix screwups in the runtime library and headers for the system */
#if !defined(_POSIX_SOURCE)
#  define getcwd(p,n)  getwd( p )
#endif

/* Mirella uses clearn(n,buf) to bytewise clear an n-byte region with origin
buf, filln(n,buf,ch) to fill such a region with char ch, and bufcpy(dest,src,n)
to move n bytes from src to dest */
#if defined(__STDC__)
#  define bufcpy(dest,src,n)  memcpy((dest),(src),(n))
#  define clearn(n,buf)       memset((buf),'\0',(n))
#else
#  define bufcpy(dest,src,n)  bcopy((src) , (dest) , n )
#  define clearn(n,buf)       bzero( (buf) , (n) )
#endif
/* filln is a C fn in mirsys.c in the unix section */

/* some systems make distinctions between opening and creating text files
and binary files, and so Mirella must make the distinction; if your system
does not, you must make macros defining openb, creatb, fopenb, fopena
(which Mirella uses) appropriately */
#define readblk     read
#define writeblk    write
#define creatb(file,mod)  open((file) ,O_RDWR|O_CREAT|O_TRUNC,mod)
#define openb(file,flag)  open((file) , (flag) , 0666)
#define fopenb(file,mod)  fopen((file), mod )
#define fopena(file,mod)  fopen((file), mod )

/* below are end-line strings for the character functions in mirelf;
DOS files use cr-lf. LEOL is the length of the line terminator. */
#define EOL "\n"
#define LEOL 1

 /* file parameters */

/* blocksize -- this should be tailored to the system buffer size for
systems which do not use fixed block sizes, and should be the block size
for those systems */
#define DBLKSIZE 4096

/* flag for fixed-length disk records, in readblk() and writeblk() */
#define DFIXREC 0

/* These macros are for creating and opening large binary files; see the
functions readblk and writeblk in mirsys.c */
#define creatblk(file,siz) creat( file ,0666)
#define openblk(file,flags) open( file ,(flags),0666)
#define tellblk  tell
#define lseekblk lseek
#define closeblk close

/* some unices define tolower and toupper to work only on upper-case
    and lower-case ascii chars, respectively. Mirella assumes they
    work on all all chars, and return the argument if it is not an
    upper-case or lower-case char, respectively. if the former, redefine
    as below: (SUNOS is one) */

#undef tolower
#undef toupper
#define	tolower(ch) (isupper(ch) ? (ch)+('a'-'A') : (ch))
#define	toupper(ch) (islower(ch) ? (ch)+('A'-'a') : (ch))

#endif /* SYSHEADER */
