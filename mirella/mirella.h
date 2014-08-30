#if !defined(MIRELLA_H)
#define MIRELLA_H
/*version 89/06/10:Mirella 5.20                                         */

/************************** MIRELLA.H ***********************************/

/* system-dependent definitions for Mirella library.  Use for Mirella
application modules is flagged by lack of prior definition of 'MIRKERN_H'
....see mirkern.h and definitions at end of this file. This file now
DOES #include stdio.h and math.h for applications */

/*****************  !!!!!!!!!!!!!!!!!!!!!!! *****************************/
/* This header file is loaded by all mirella applications; please       */
/* be careful when you muck with it, so you don't break old code        */
/************************************************************************/

/* recent history:
 * 88/08/20:  Added SISYPHUS Convex system
 * 88/08/27:  Added switches DEFUCHAR and LOGCSHIFT #defined if the cpu
 *              (or compiler) uses default unsigned chars or logical
 *              instead of arithmetic shifts.
 * 88/08/28:  Added new system definitions M_CCOMP and M_CPU
 * 88/10/24:  Went to "msysdef.h" include file scheme for this and mirelf.c;
 *              added function declarations from Mirella modules and
 *              trig constants.
 * 89/01/12:  Added time structure definition
 * 89/06/10:  Added fn array def, some utility macros, updated fn decl.
 */
#include "options.h" 

#define MIREDIT  1            
                                /* defined 1 if RHL line editor is being used;
                                if 0, just uses opsys line editor. Less grief
                                will be had in porting if set to 0 until done
                                 */

/* This file contains all the preprocessor flags used in Mirella,
and some macros needed for system compatibility. The msysdef.h file
#included here contains all such special definitions for a given 
system and, in addition,  a section included in mirelf.c which
contains the special i/o function definitions for any special 'files'
(devices, usually) which the system may have, like tape drives, etc,
to make them compatible with the Mirella i/o system.

When porting Mirella to a new system, the flag MIREDIT should be defined
to be 0 and SIGNALS should be undefined until you are pretty sure there
are no gross errors; life will be easier. 

So far the system has been tested on the DSI32, the 68020-based DSI780, an
Ultrix VAX, a Sun 3 running Berkeley Unix, a Ridge 32 running a funny
amalgam of Berkeley and system V, microVAXs and a vax780 running VMS, 
a CONVEX running "Berkeley Unix" (most painfully), a Sun 4
running Berkeley 4.2-4.3., and most recently 386 AT systems running
Phar Lap tools and NDP (Greenhills)  C.   Good luck.     */


#define SYSHEADER
#ifdef vms
#include stdio
#include math
#include ctype
#include msysdef
#include charset.h
#include declare
#else
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "msysdef.h"
#include "charset.h"
#include "declare.h"
#endif /* vms */
#undef SYSHEADER

/* definition of normal int */
/* these work for 32-bit machines; revisit for 64-bit */
#if 0                                   // old code; works on 32-bit machines
#define  normal     int
#define  u_normal   unsigned int 
#else					// works on 64-bit machines (and should work on 32-bit too)
#define  normal     long
#define  u_normal   unsigned long
#endif

#define LEN 100				/* length of directory names */
/*
 * system parameters
 */ 
extern char *m_terminal;
extern char *g_display;
extern char *im_display;
extern char *m_opsys;
extern char *m_cpu;
extern char *m_ccomp;
extern normal m_ibmorder;
extern char *g_printer;
extern char m_system[];              /* system name */
extern char m_distro[];              /* distribution if Linux */
extern char m_distrotype[];          /* distro type (Debian, RedHat) */
extern char m_graphsys[];            /* for X, Xvnc or X2 */

/*
 * exported from mirsys.c
 */
/*********************** UTILITY DEFINITIONS ******************************/

#if !defined(MAX)
#  define MAX(a,b)  ( (a) < (b) ? (b) : (a))
#endif
#if !defined(MIN)
#  define MIN(a,b)  ( (a) < (b) ? (a) : (b))
#endif
#define cr()      scrprintf("\n")
#define Debug     scrprintf
/* many/most unix-like systems (now POSIX?)have a math function round() 
 * which returns
 * double or float. The mirella version returns an int. This should be
 * fixed eventually, but for now do this, so we do not have to change
 * a lot of source code
 */
#define round     mround

/******************* NUMERICAL CONSTANTS *******************************/
#ifdef PI
#undef PI
#endif

#define LOG10   2.302585093
#define TWOPI   6.283185308
#define PI      3.141592654 
#define PIO2    1.570796327
#define PIO4    0.785398164
#define SQTWOPI 2.506628275
#define DEGPRAD 57.29577951
#define LN2     0.693147180

extern normal illval;			/* illegal values */
extern float filval;
#define I_FILVAL (-1L)			/* filval's bitpattern */
#define IS_FILVAL(F) (*(long *)(&F) == I_FILVAL)

/****************** SOME STRUCTURE DEFINITIONS *************************/

struct mir_tm{
    normal tm_sec;
    normal tm_min;
    normal tm_hour;
    normal tm_mday;
    normal tm_mon;
    normal tm_year;
    normal tm_wday;
    normal tm_yday;
    normal tm_isdst;
    char *tm_zone;
    normal tm_gmtoff;
};

struct ftab_t{          /* mirsplin.c */
    float *ft_xptr;
    float *ft_yptr;
    float *ft_kptr;     /* pointer to spline arrays */
    int ft_n;           /* # pts */
    int ft_ord;         /* order of interpolation: 0:lin >=1:splin */
    float ft_minx;      /* minimum x */
    float ft_maxx;      /* maximum x */
    int ft_ser;         /* serial number--monotonically increasing identfier */
};


/*
 * An array of functions that need to be called to refresh windows
 */
typedef int (*CALLBACK_FUNC_PTR)();

/* unsigned types */
#define  u_char     unsigned char
#define  u_short    unsigned short int

      
/*******************  MIRELLA DEFINITIONS *****************************/

#define  mirella

#define  fpop       (double)(*xfsp_4th++)
#define  pop        (*xsp_4th++)
#define  push(xx)   (*(--xsp_4th)) = (normal)( ( xx ) )
#define  fpush(xxx) (*(--xfsp_4th)) = (float)( ( xxx ) )
#define  cppop      (char *)(*xsp_4th++)
#define  cspop      ((char *)(*xsp_4th++))


/*
 * from mirelf.c
 *
 * Types of special file
 */
#define FT_TAPE 02			/* special file is a tape drive */
#define FT_PATTERN 04			/* this is a filename pattern, not a
					   real device */
struct special_names {
   char mfname[16];			/* mirella's name for special file */
   char opsname[32];			/* the system's name */
};
extern struct special_names special_names[]; /* mapping from Mirella's special
						names to real device names */

/*
 * if 'MIRKERN_H' is defined, the following Mirella-specific definitions are 
 * suppressed. This is used in the kernel of Mirella itself, where there would
 * be conflicts, and can be used in code which has nothing to do with Mirella
 * if the user should want to use any of these names for something else
 */
#ifndef MIRKERN_H

extern   normal *   xsp_4th;
extern   float  *   xfsp_4th;
extern   normal *   ap_env;
extern   char     out_string[];
extern int use_editor;			/* should I use the history editor? */
extern int verbose;			/* shall I be chatty? */

#define  pause(str) if(dopause()) goto str
    /* str is a label. It MUST point to code that eventually does an
    erret(), or the interrupt signal handler will not be reset.*/
#define  pauseerret() if(dopause()) erret((char *)NULL)
#define  pausemsg(str)  if(dopause()) erret(str) 

#define ap_ptr (normal *)(*ap_env)  
                        /* first element of ap_env is pointer to
                            current next free element. initialized
                            in aloc_dic */
extern struct sigaction sigmirella;
extern struct sigaction sigtemp;
extern struct sigaction sigold;
extern void raiseint();
extern normal rdeof;
extern normal nread;
extern normal achan;
extern normal tfileno;
extern normal mblk_fsz;
/* from mirscrn.c */
extern normal m_cwid;                          /* font stuff */
extern normal m_chgt;                          /* " */
extern normal m_ctsz;                          
extern normal m_fontindex;
extern u_char *m_cmask;
extern u_char **c_tab;                         /* " */       
extern normal m_termflg;                       /* flag for terminal output */
extern normal t1flg,t2flg,t3flg,t4flg,t5flg;   /* gp test flags */
extern normal grafmode;                        /* display graphics mode */
extern normal (*m_grefresh)();                 /* graphics refresh fn */
extern normal (*m_irefresh)();                 /* image refresh fn */
/* from mirellio.c */
extern int      (*scrnprt)();
extern int      (*scrnput)();
extern int      (*scrnflsh)();

/*
 * from editor.c
 */
extern char	prompt2[];

/************ FUNCTION DECLARATIONS FROM MIRELLA MODULES ************/
#endif /* MIRKERN_H */

#if !defined(NO_DECLS)
#  include "mireldec.h"
#endif
#endif  /* MIRELLA_H */
