/*VERSION 90/05/07: Mirella 5.50                                */

/************************ MIRELLA.C *****************************/
/*  This module contains most of the kernel code                */
/*  The header is in mirella.h                                  */
/*  The #defines for the primitives are in mprims.h             */
/*  The dictionary initializer is in minit.x                    */
/*  The inner interpreter is in mirellin.c                      */
/*  The io module is in mirellio.c                              */
/*  The c interface code is in mircintf.c                       */
/*  The internal/external fn/vbl code in mirel(i/e)c.c          */
/*  And the dictionary initializer for above is mirel(i/e)c0.x  */
/*  The input editor is in miredit.c                            */
/*  The memory allocator is in mirelmem.c                       */
/*  The file handler is in mirelf.c                             */
/*  The help code is in mirhelp.c                               */
/*  Array arithmetic is in mirarray.c                           */
/*  The screen handler is in mirscrn.c                          */
/*  The graphics primitives are in mirgraph.c                   */
/*  The plotting code is in mirplot.c                           */
/*  Hardcopy device-specific graphics code is in mirgdev.c      */
/*  Display  device-specific graphics code is in mirgdis.c      */
/*  Matrix arithmetic code is in mirmatrx.c                     */
/*  Othogonal polynomial fitting code is in mirorpol.c          */
/*  Spline and integration code is in mirsplin.c                */
/*  Histogramming and analysis code is in mirhist.c             */
/*  Function minimization code is in mirameba.c                 */
/*  System-dependent utilities are in mirsys.c                  */
/*  Directory search code is in mirdir.c                        */
/*                                                              */
/*  The standalone C installation program is minstall.c         */
/*                                                              */
/****************************************************************/
/* History-v4.xx
88/04/87 -  Fixed(?) hdl_sig so repeated ^C will not bomb, and counts 
            occurrences. Changed out_intp so that the bomb flag insane is 
            reset on each invocation, whether or not any interpretation is 
            done. Also hdl_sig so that 4 signals are required to dump to OS 
            between resets of insane.
            Implemented the user variables V_RES_STK for resetting the stack
            at each line of the outer interpreter ('idiot mode') and 
            V_INTERFLG for returning to the o.i. at each keyboard interrupt,
            whether or not there is any application catching code.
88/05/20 -  Added code to find() and added l_create() for local variables,
            and defined the local_dic structure array. New user variables
            nlvar and localdic
88/06/01 -  Added VMS text library support            
88/06/10 -  Added new user variables V_NOCOMPILE, V_CCNEST, and V_CCNOFF
            for conditional control of compilation via #if, #else, and #endif
88/08/31 -  Made erret() do a gmode(0) to make sure user is left in text
            mode after an error; only viable after cleanup of mirgdis to
            do a real reset only if necessary
VERSION 5.1:
88/10/23 -  sigfpe is defined as some sort of global array in the SPARC
            world, so changed all the signals to m_ prefixes.
88/10/24 -  qt_create could create on nonaligned addresses, which crashed
            SPARC; it should never have been so, and is now fixed.
88/10/24 -  fixed init_dic so that it dynamically allocates break on next
            4K boundary.
88/10/24 -  fixed signal handler and erret() so that signals simply do
            a longjmp, passing a 1 or a string address to setjmp, which
            is subsequently acted on and printed; no functions get called
            except longjmp while the error condition exists. Much more
            robust.
89/01/27 -  fixed number() so that it correctly interprets floats of arbitrary
            precision; previous version worked incorrectly if the float could
            not be represented by a scaled normal (about 9 digits). Added
            support for Fortran 'D' format.
89/04/31 -  changed number() to support e-floats with just embedded + or -,
            no e or d.            
89/08/15 -  better debugging support--flag setdb makes setup verbose, so
            can better find errors when porting to new machines
90/05/07 -  Changed init_dic code to support secondary external clink files.
96/03    -  Various small fixes to interrupt code, so now works robustly
            under Linux. Added erret code to get1char, where it belongs, I
            think.
98/07    -  Fixed number() to correctly interpret e-format with +sign on
            exponent. HTH did this not get noticed before???????
12/08    -  Fixed number() to scale with K and M properly for integers
*/            

#include "options.h"
#ifdef VMS
#include setjmp
#include mirkern
#include mprims
#include dotokens    /* #defines for primitive tokens */
#include mircintf
#include mireldec
#include images
#else
#include <setjmp.h>
#include "mirkern.h"
#include "mprims.h"
#include "dotokens.h"    /* #defines for primitive tokens */
#include "mircintf.h"
#include "mireldec.h"
#include "images.h"
#endif /* VMS */

#ifdef SIGNALS 

#  ifdef VMS
#     include signal
#  else
#     include <signal.h>
#  endif /* VMS */


# ifdef SIGACTION
   struct sigaction sigmirella;
   struct sigaction sigtemp;
   struct sigaction sigold;
#endif
   int m_sigint = SIGINT;
   int m_sigill = SIGILL;
   
#  if defined(SIGIOT)
      int m_sigiot = SIGIOT;
#  else
      int m_sigiot = 0;
#  endif
#  if defined(SIGEMT)
      int m_sigemt = SIGEMT;
#  else
      int m_sigemt = 0;
#  endif
   int m_sigsegv= SIGSEGV;
#  if defined(SIGBUS)
      int m_sigbus = SIGBUS;
#  else
      int m_sigbus = 0;
#  endif
   int m_sigfpe = SIGFPE;
#  if defined(SIGPIPE)
      int m_sigpipe= SIGPIPE;
#  else
      int m_sigpipe= 0;
#  endif
#else
   int m_sigint = 0;
   int m_sigill = 0;
   int m_sigiot = 0;
   int m_sigemt = 0;
   int m_sigbus = 0;
   int m_sigfpe = 0;
   int m_sigpipe= 0;
#endif /* SIGNALS */

/*#define INTDEBUG*/

/* dictionary and user area sizes #defines now in mirkern.h */

#define IMMEDBIT 0x01
#define isimmediate(lfa) ((lfa)->flags & IMMEDBIT)

static void hdl_pipe P_ARGS(( int ));

extern FILE *input_file;    
extern normal sz_fchannel;      /* sizeof(struct fchannel) from mirelf.c */
extern normal sz_memarea;       /* sizeof(struct memarea) from mirelmem.c */


static normal *hashnum;         /* diagnostic array for numbers of entries
                                    in each thread */
int m_errexit = 1;              /* flag to signal exit in the event of an
                                    erret(); on until outer interpreter is
                                    set up */
static char *fborigin;          /* file buffer origin */

jmp_buf env_4th;               
normal  par_stk[PSSIZE+1];
float   fl_stk[FSSIZE+1];
normal  tem_stk[TSSIZE+1];
token_t *rtn_stk[XRSSIZE+1];
token_t **xrp_4th;

char    tibbuf_4th[TIBSIZE];   /* def. exported */
normal  *dp_4th = 0;
normal  *xsp_4th;
normal  *xtsp_4th;
float   *xfsp_4th;

static vocab_t tempvoc = {
  {
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
    (dic_en_t *)NULL,
  },
  (normal *)NULL
};				/* number of entries must agree with
				   NTHREADS (8) in mirkern.h. Hash
				   fn defined as macro in mirkern.h */
                                        
normal  *up_4th ;     /* user origin , also V_UPZERO, 'up0' This is
                            the origin of the Mirella memory area */
normal  *fp_4th;      /* file area origin, also V_FORIG, 'fp0' */
normal  *mp_4th;      /* memalloc area origin, also V_MORIG, 'mp0' */
normal  *origin = 0;  /* for init; will be set to dict_orig. 
                            pushed by 'origin' */
normal  *dict_orig;   /* dict origin, primaries and clinks. */
normal  *dictb_4th;   /* origin of normal forth dictionary. 
                            pushed  by <*> */
normal  *dict_end;    /* end */
normal  *ap_env;      /* origin of application environment area,
                            also V_APAREA, 'app0' */
normal  *m_lastlink;  /* link address of last real clink in dictionary */
#define ap_ptr (normal *)(*ap_env)  
                      /* first element of ap_env is pointer to
                        current next free element. initialized
                        in aloc_dic; allocated by envalloc() */
void  (*erretfun)() = (void (*)())cprint; /* function called with ptr passed
					     to an erret-generated longjmp */

char nmbr_buf[40];
char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
token_t comp_buf[CBUFSIZE];
char wordbuf[MAXSTRING + 5];
char *xssp_4th;				/* string stk ptr */
char *str_stk;				/* string stk, allocated by alloc_ss*/
char pstring[40] = "\2M>";		/* prompt string */
int dict_image = 0;			/* flag for loading old dictionary */
char full_name[100];			/* full name of program executable;
                                           nearly always dir/cmirella */
char progname[100];                     /* name of program (mirella,mirage,
                                           vtra, etc; set in primary Mirella
                                           program for any extension */
#define OUT_STR_SIZE MAXSTRING          
char out_string[MAXSTRING];		/* string for 'printf' output */
int insane;
int sigflg = 0;				/* set by signal handler */
normal interrupt = 0;			/* flag for ^C (^D in DSI systems) */
int verbose = 0;			/* verbosity level */
int use_editor = 1;			/* should I use the history editor? */
int tibsize = TIBSIZE;                  /* input buffer size */

/* system definition constants; #defines picked up from msysdef.h */
char *m_terminal;       /* picks up from MIRTERM in the environment */
char *g_display = G_DISPLAY;    /* graphics display */
char *im_display= IM_DISPLAY;   /* image display */
/*char *g_printer = G_PRINTER;*/ /* graphics hardcopy */
char *m_opsys   = M_OPSYS;      /* operating system */   
char *m_cpu     = M_CPU;        /* cpu type */
char *m_ccomp   = M_CCOMP;      /* c compiler */
/* these are read from .mirella */
char m_system[64];              /* system NAME */
char m_distro[64];              /* distribution if Linux */
char m_distrotype[64];          /* distro type (Debian, RedHat) */
char m_graphsys[64];            /* for X, Xvnc or X2 */

#ifdef IBMORDER
normal m_ibmorder = 1;
#else
normal m_ibmorder = 0;
#endif

struct ld_en_t{
    char   l_name[8];
    normal l_cfa;
}local_dic[64];

extern void    erret(),clrint(),stack_init() ;
extern char *init_fbuf();
static normal j_ret;               /* return for erret longjmp() */
static void fixerror();            /* errfix for erret longjmp() */

static vocab_t *forth_vocab, *root_vocab; /* forth and root vocabularies;
					     these variables are the PFAs of
					     the vocabulary words */

/********************************************************************/
/* the user array is normally initialized as follows:               */
/*  0   NEXT_VAR*sizeof(normal),    #user                           */
/*  1   0,                          span                            */
/*  2   0,                          >in                             */
/*  3   10,                         base                            */
/*  4   0,                          blk                             */
/*  5   0,                          #tib                            */
/*  6   (normal)INTERPRETING,       state                           */
/*  7   (normal)&tempvoc,           current                         */
/*  8   (normal)&tempvoc,           context                         */
/*  9   0, 0, 0, 0,                 rest of context list            */
/*  13  0,                          delimiter                       */
/*  14  (normal)ps_top,             sp0                             */
/*  15  (normal)&rtn_stk[RSSIZE],   rp0                             */
/*  16  (normal)&variable[0],       up0                             */
/*  17  0,                          last                            */
/*  18  0,                          #out                            */
/*  19  0,                          #line                           */
/*  20  0,                          voc-link                        */
/*  21  -1,                         dpl                             */
/*  22  0,                          warning                         */
/*  23  0,                          caps                            */
/*  24  0,                          errno                           */
/*  25  (normal)ts_top,             tsp0                            */
/*  26  (normal)fs_top,             fsp0                            */
/*  27  (normal)ss_top,             ssp0                            */
/*  28  1,                          Mirella flag; f83 if zero       */
/*  29  (normal)&pstring[0],        pstring (prompt string)         */
/*  30  ap_env                      app0  (application env origin)  */
/*  31  0                           fpf (float-only input flag)     */
/*  32  (normal)fp_4th              fp0 (origin of file struc area) */
/*  33  sizeof(struct fchannel)     fssize                          */
/*  34  (normal)mp_4th              mp0   (origin of memalloc area) */
/*  35  sizeof(struct memarea)      mssize                          */
/*  36  0                           ital                            */
/*  37  0                           io                              */
/*  38  0                           noecho (flag to suppress echo)  */
/*  39  0                           #linein (input line counter)    */
/*  40  0                           lastfile                        */
/*  41  0                           silentout (suppresses mprint)   */
/*  42  fborigin                    fb0                             */
/*  43  0                           nocompile                       */
/*  44  0                           remin (flag for rem input in in */
/*  45  0                           res_stk (reset stk each line)   */
/*  46  0                           interflg (erret each ^C)        */
/*  47  0                           nlvar                           */
/*  48  &local_dic[0]               localdic                        */
/* some of these are not yet implemented; ie they don't do anything */
/********************************************************************/
/*
 * Support for linked C-code, files mirel[ie123].c
 */
static void mircl_init();		/* init the C-linked stuff */
static void mircl_read();		/* read clset for mircl_init() */
static void get_sysinfo();              /* read run-time info about distro, 
                                         * graphics */

static struct {
    int nvar;				/* number of variables */
    int nfunc;				/* and number of functions */
    char **varar;			/* array for variables */
    void (**funcar)();			/* and for functions */
    MIRCINTF *(*next_cif)();
    void (*init0)();			/* initialises variables */
    void (*init1)();			/* called before directory init */
    void (*init2)();			/* called after directory init */
    void (*init3)();			/* called just before out_intp */
} clset[5];
#define N_CLSET (sizeof(clset)/sizeof(clset[0]))

/********************************************************************/


static int setdb=0;
/*flag for making the initialization procedure verbose, so can localize
errors */

/* appl code begins by calling */

void
mirelinit(argc,argv)  
int argc;
char **argv;
{
    int i;
#ifdef DSI32
    extern long FILEBUF;
    FILEBUF = 32640;    /* set buffer size for 32032-80286 xfrs */
#endif
    mirelcmd(argc,argv);
    mircl_read();			/* read the clset struct */

    if(setdb){printf("\nInitializing line editor"); fflush(stdout);}
    init_ed();  /* initialize line editor and get MIRTERM from env.*/
    
    if(setdb){printf("\nInitializing display");fflush(stdout);}
    init_scr(); /* init screen stack and restore_screen functions */
    /* at this point, the basic mirella i/o functions are in place */
    
    if(setdb){printf("\nAllocating string stack"); fflush(stdout);}
    alloc_ss();  /* allocate space for string stack */
    
    if(setdb){printf("\nInitializing file buffers");fflush(stdout);}
    fborigin = init_fbuf(); /* init file buffers */    

    if(setdb){
       printf("\nhw mark before dic.alloc=%x",(int)sbrk(0)); fflush(stdout);
    }
    aloc_dic(); /* allocate dictionary space & define pointers to user
                    area, dictionary, break, and application env. clear
                    all that space. */

    if(setdb){printf("\nInterpreting command line"); fflush(stdout);}
#ifdef DLD
    setup_dld();
#endif
/*
 * set illegal values
 */
    {
       union {
	  float f;
	  long i;
       } bad;
       
       bad.i = I_FILVAL;
       filval = bad.f;
       illval = -77777777;
    }
    init_image();
/*
 * Allow application code to call its first initialisation functions
 */
    if(setdb){
       printf("\nhw mark before first init functions=%x",(int)sbrk(0));
       fflush(stdout);
    }

    for(i = 0;i < N_CLSET;i++) {
       if(clset[i].init1 != NULL) {
	  (*clset[i].init1)();
       }
    }
    /* get system information */
    get_sysinfo();
}

/*
 * application code can perform an applinit() at this point, if it
 * hasn't declared a first init function to minstall
 *
 * Application code must then perform a q_restart (rets 1 if 'd'
 * flag is present) at this point, which does an init_dic() and
 * calls do_init_2 to run the applications second initialisation
 * functions (if declared to minstall). Alternatively (and if required)
 * application code can perform an oldd_init at this point
 */
void
do_init_2()
{
   int i;
   
   for(i = 0;i < N_CLSET;i++) {
      if(clset[i].init2 != NULL) {
	 (*clset[i].init2)();
      }
   }
}

/*
 * Next we must call:
 */
static void do_init3();

void
mirellam()
{

#ifdef SIGNALS    
    void hdl_sig();
#ifdef SIGACTION
    static int sa_err;
#endif    
#endif

    if(setdb){printf("\nInitializing user vbls"); fflush(stdout);}    
    init_var();             /* initialize volatile user variables */
    
    if(setdb){printf("\nInitializing file system"); fflush(stdout);}        
    init_file();            /* initialize file system  */
    
    if(setdb){printf("\nInitializing memalloc system"); fflush(stdout);}
    init_mem();             /* initialize memalloc system */

    if(setdb){printf("\nInitializing stack system"); fflush(stdout);}        
    stack_init();           /* initialize stacks */
    m_errexit = 0;          /* enable longjmp in erret() */

    if(setdb){printf("\nSetting up longjump to OI");fflush(stdout);}
    if ( (j_ret = setjmp(env_4th)) == 0 ) {  
    /* this is the setup code, executed on the call to setjmp() */
#ifdef DSISIGNALS
        trapinit() ;            /* turn off ^C, revector float exception,
                                div by 0, illegal, flag, and undefined 
                                inst traps */
#endif /* DSISIGNALS */
#ifdef SIGNALS 
#ifdef SIGACTION
        sigmirella.sa_handler=(void (*)P_ARGS((int)))hdl_sig;
        sa_err=sigaction(SIGINT,&sigmirella,&sigold);
        if(sa_err){
           puts("\nCannot initialize SIGINT handler");
        }
#else        
        signal(SIGINT,(void (*)P_ARGS((int)))hdl_sig);
#endif        
        signal(SIGILL,(void (*)P_ARGS((int)))hdl_sig);
#if defined(SIGIOT)
        signal(SIGIOT,(void (*)P_ARGS((int)))hdl_sig);
#endif
#if defined(SIGWINCH)
        signal(SIGWINCH,(void (*)P_ARGS((int)))hdl_sig);
#endif
#if defined(SIGEMT)
        signal(SIGEMT,(void (*)P_ARGS((int)))hdl_sig);
#endif
        signal(SIGSEGV,(void (*)P_ARGS((int)))hdl_sig);
#if defined(SIGBUS)
        signal(SIGBUS,(void (*)P_ARGS((int)))hdl_sig);
#endif
        signal(SIGFPE,(void (*)P_ARGS((int)))hdl_sig);
#if defined(SIGQUIT)
	signal(SIGQUIT,hdl_quit);
#endif
#if defined(SIGPIPE)
	signal(SIGPIPE,hdl_pipe);
#endif
        sigflg = 0;
#endif /* SIGNALS */
	if(setdb){printf("\nInitializing load system"); fflush(stdout);}
        init_io();              /* init Mirella i/o (load) system. may
                                    do a longjmp to env_4th;  setjmp
                                    will return 1 in that case, so OK */

	do_init3();
    }else{ 
    /* this is a longjmp return */
        fixerror((char *)j_ret);
#ifdef SIGNALS
#ifdef SIGACTION
        if(sigflg){
            sigtemp.sa_handler=(void (*)P_ARGS((int)))hdl_sig;
            sa_err=sigaction(sigflg,&sigtemp,(struct sigaction*)NULL);
        if(sa_err){
           printf("\nCannot initialize signal %d handler",sigflg);
        }
#else        
        if(sigflg) signal(sigflg,(void (*))hdl_sig);
#endif /* SIGACTION */
        sigflg = 0;
#endif /* SIGNALS */
    }
    /* now call the outer interpreter; never return except for erret()s
        or exit */
#ifdef INTDEBUG     
        printf("\nCalling outer interpreter\n");
#endif    
    out_intp();
}

/*
 * This function is separate to avoid a probable bug in gcc; if it is
 * included inline a subsequent longjmp gives a SEGV
 */
static void
do_init3()
{
   int i;
   
   for(i = 0;i < N_CLSET;i++) {
      if(clset[i].init3 != NULL) {
	 (*clset[i].init3)();
      }
   }
}

/*******************  TO_CSTR()  ******************************************/
/*
 * converts Forth string to c string; uses internal buffer; careful
 */
char *
to_cstr( from )
register char *from;
{
    static char cstr[256];
    register char *to = cstr;
    register int length = *from++;

    if(length) {
       do {
	  *to++ = *from++;
       } while (--length);
       *to++ = '\0';
    }
    return(cstr);
}

/************************   CANON()  *************************************/
char *
canon(fstr)         /* If V_CAPS, converts Forth string to lower(!) case */
char *fstr;
{
   register char *str = fstr;
   register int length;
   register char c;
   
   if ( V_CAPS ) {
      for ( length = *str++; length--; str++) {
	 c = *str;
	 /* Ascii-dependent */
	 if ( c >= 'A' && c <= 'Z' )
	   *str = c - 'A' + 'a';
      }
   }
   return( fstr );
}

/*
 * Initialize the dictionary
 */


/***********************   ALOC_DIC()  *******************************/
void aloc_dic()     /* allocates the dictionary and user areas */
{
    /* following is departure from Bradley--we assume never to load an
    executable image, but will only use dictionary image--thus origin is
    always initially zero. */
    int needed;
    normal * end;
    
    needed = DICT_SIZE + APP_AREA ;  /* in normals. Note that DICT_SIZE
                                    contains user area, file area, memalloc
                                    area, and break(clink) area. */
    if((up_4th= (normal *)init_malloc(needed*sizeof(normal))) == (normal *)-1){
        error("Can't allocate memory for the dictionary\n");
        exit(1);
    }
    /* clear it all */
    clearn(needed*sizeof(normal),(char *)up_4th);
    
    /* up_4th is never modified and is the origin of the Mirella memory area*/
    fp_4th    = up_4th + USER_AREA;     /* pointer to file area */
    mp_4th    = fp_4th + FILE_AREA;     /* pointer to memalloc area */
    dp_4th    = mp_4th + MEM_AREA;      
    dict_orig = dp_4th ;                /* pointer to dictionary origin */
    dict_end  = up_4th + DICT_SIZE -1;  /* end of dictionary */
    ap_env    = up_4th + DICT_SIZE ;
    end       = ap_env + APP_AREA;
    /* all of these quantities are pointers to normal   */
    *ap_env = (normal)((normal *)ap_env + 1);  
        /* first entry is pointer to next free */
    if(setdb){
       printf("\nup_4th, end= %d %d",(int)up_4th,(int)end); fflush(stdout);
    }
}


/************************************************************************/
/*  The Mirella memory map looks something like this:                   */
/*                                                                      */
/*      0       -   x7ffff      C code (512 Kby )                       */
/*      x80000  -   x801ff  user area (512b, len USER_AREA normals)     */
/*  (USERORIGIN)                                                        */
/*      x80200  -   x803ff  file structure area (512 bytes)             */
/*          (x80200 is USERORIGIN + USER_AREA*sizeof(normal)            */
/*      x80400  -   x807ff  memory alloc structure area (1 kbytes),     */
/*                     MEM_AREA                                         */
/*      x80800  -   x83fff  Mirella primitives and C fns and vbls       */
/*          (14 kb) (x80500 is origin (dict_orig)   )                   */
/*      x84000  -   x9ffff  Mirella dictionary (~112Kb)                 */
/*          (x84000 is USERORIGIN + BRK_SIZE*sizeof(normal)             */
/*      xa0000  -   xa3fff  Application communication area (globals)    */
/*          (xa0000 is ap_env)                                          */
/*      xa4000  -   end     application data and heap                   */
/*                                                                      */
/************************************************************************/


/***********************   INIT_USER() *******************************/
void init_user()   /* routine to initialize user area for cold start */
{
    int i;

    for(i = 0;i < USER_AREA;i++)	/* clean user and file struct area*/
      up_4th[i] = 0;
    V_NR_USER   = NEXT_VAR * sizeof(normal);
    V_BASE      = 10;
    V_STATE     = (normal) INTERPRETING;
    V_CURRENT   = V_CONTEXT = (normal)&tempvoc;
    V_DPL       = -1;
    V_MIRELLA   = 1;
    V_APAREA    = (normal)ap_env;
    V_NLVAR     = 0;  
}

/************************  INIT_DIC() ********************************/
static char d_mark[] = "\3<*>";

void
init_dic()    /* normal dictionary initialization  */
{
    register int j;
    normal scr;

    init_user();
    origin = dp_4th;

    if(setdb){printf("\nINIT_DIC:origin=%d",(int)origin); fflush(stdout);}
    /* reserve space for an array of pointers to the cfa of prim headers */
    dp_4th += NEXT_PRIM;

    hashnum = dp_4th + 200;          /* scratch area for initial entry */
    /* Make the initial dictionary entry */
    qt_create("\5forth",(token_t)DOVOC);

    V_CONTEXT = (normal)dp_4th;
    V_CURRENT = (normal)dp_4th;

    for(j=0;j<NTHREADS;j++)   /* hashing */
        *dp_4th++ = (normal) tempvoc.last_word[j];
        
    V_VOC_LINK = (normal) dp_4th;
    *dp_4th++ = (normal) 0;     /* voc-link */
    hashnum = dp_4th;           /* pointer to array of numbers of entries
                                    in each thread */
    dp_4th += NTHREADS;                                    
    /* compile primitives */
    if(setdb){printf("\nINIT_DIC:setting up primitives"); fflush(stdout);}
    init_cif(init_prims,(normal *)NULL,(void (**)())NULL);

    scr = (normal)dp_4th;                                            
    qt_create("\12<endprims>",(token_t)DOCON); /* marker for end of prims */
    *dp_4th++ = scr;

    if(setdb){printf("\nINIT_DIC:setting up Clinks"); fflush(stdout);}    
    mircl_init();

    m_lastlink = (normal *)V_LASTP;       /* save last 'real' clink lfa;
                                             used to chk validity of dicimage*/
/*
 * Find some addresses to allow us to reset the dictionary to root and forth
 * in case of emergency
 */
    forth_vocab = (vocab_t *)V_CONTEXT;
    {
       char *root = "\004root";
       
       if(vfind(&root,forth_vocab) == 0) {
	  printf("\nCan't find root vocabulary in forth vocabulary");
	  root_vocab = NULL;
       } else {
	  root_vocab = (vocab_t *)((normal *)root + 1);
       }
    }

    /* move dp to next even 4K boundary; break is dynamically set */
    if(setdb){printf("\nsetting dp_4th"); fflush(stdout);}    
    dp_4th = (normal *)( (((normal)dp_4th)/4096 + 1)*4096 );
    dictb_4th = dp_4th + 5*NTHREADS;
    /* place markers */
    if(setdb){printf("\nsetting up threads"); fflush(stdout);}    
    for(j=0;j<NTHREADS;j++){
        d_mark[2] = '0' + j;
        qt_create(d_mark,(token_t)DOCON); /* marker word to keep link
					     OK if add more c or prims,
					     one for each thread */
        *dp_4th++ = (normal)dictb_4th;  /* marker word pushes break address */
    }
    if(setdb){
       printf("\nINIT_DIC:dp_4th at end=%d",(int)dp_4th); fflush(stdout);
    }
}

/* *******************  INIT_VAR() *********************************/
void 
init_var()  /* this one wants to be executed whenever mirella starts */
{
    V_SPAN = V_TO_IN = V_BLK = V_NR_TIB = 0;
    V_NR_OUT = V_NR_LINE = V_LN_IN = 0;
    V_NOECHO = 0;
    V_BASE = 10;
    V_STATE = INTERPRETING;
    V_NLVAR = 0;
    V_WARNING = 1;
    V_FORIG     = (normal)fp_4th;
    V_FSSIZE    = sz_fchannel;      /* sizeof(struct fchannel) */
    V_MORIG     = (normal)mp_4th;
    V_MSSIZE    = sz_memarea;       /* sizeof(struct memarea) */
    V_APAREA    = (normal)ap_env;
    /* lines below tie mirella to code addresses; can now change code and
        use old dictionary image */
    V_SPZERO    = (normal)ps_top;
    V_RPZERO    = (normal)&rtn_stk[RSSIZE];
    V_UPZERO    = (normal)&up_4th[0];
    V_TSPZERO   = (normal)ts_top;
    V_FSPZERO   = (normal)fs_top;
    V_PSTRING   = (normal)&pstring[0];
    V_FBZERO    = (normal)fborigin;
    V_LOCALDIC  = (normal)&local_dic[0];
}

static void 
get_sysinfo()
{
    char *p;
    p = get_def_value("distro");
    if(p!= (char *)NULL ){
        strncpy(m_distro,p,63);
    } else {
        m_distro[0] = '\0';
    }
    p = get_def_value("distrotype");
    if(p!= (char *)NULL ){
        strncpy(m_distrotype,p,63);
    } else {
        m_distrotype[0] = '\0';
    }
    p = get_def_value("graphsys");
    if(p != (char *)NULL ){
        strncpy(m_graphsys,p,63);
    } else {
        m_distrotype[0] = '\0';
    }
}

/*
 * Implements vocabularies, searching, and vocabulary search order
 * using linked headers.
 */

/*
 * Header format:
 * Field                Type        Description
 * 
 * Link Field:        normal *      Points to previous link field
 * Flag byte:         u_char        Attribute bits (immediate bit)
 * Name Field:        packed-string Name of this entry
 * Alignment:         n * '\0'      Padding to align to next normal (-1)
 * Code Field:        normal        Determines what kind of work
 * Parameter Field:   n * normal    Word-specific parameters
 *
 */

/*************************  ALIGNED() ******************************/
static normal ALBM1 = ALIGN_BOUNDARY - 1;
static normal MALB = -ALIGN_BOUNDARY;

/*
 * returns the next address at addr or foll aligned on a normal boundary
 */
normal *
aligned(addr)
Void *addr;
{
    /* This calculation assumes twos-complement representation */
    return( (normal *) ( ((normal)addr + ALBM1) & MALB ) );
}

/* macro to do the same thing */
#define ALIGNED(x) ( (normal *)( ((normal)( x ) + ALBM1) & MALB ) )

/************************ NAME_FROM() *******************************/
/*
 * This should be called skip-string; it returns the next aligned
 * address after the forth string at nfa
 */
token_t *
name_from( nfa )
char *nfa;
{
    return ( (token_t *)ALIGNED( &nfa[(*nfa)+1] ) );
}

/************************ MAKEIMMEDIATE() **************************/
void makeimmediate()
{
    V_LAST->flags |= IMMEDBIT;
}

/***********************  VFIND() ************************************/
/*
 * Look for a name within a vocabulary.  Sp is a copy of the stack pointer.
 * The top of the stack contains a pointer to a string which is the name to
 * be searched-for.
 *
 * If the name is found, the string address on top of the stack is replaced
 * by the token associated with that name and vfind returns either
 * 1 (if the name has the immediate attribute) or -1.
 *
 * If the name is not found, the top of the stack is left unchanged, and
 * vfind returns 0 (Forth FALSE).
 *
 * Forth system keeps the true top of the stack in a register.  The stack
 * pointer passed to vfind is actually the address of the stack item
 * just below the top of stack.  This turns out to be quite convenient,
 * since the string passed to vfind is supposed to be below the vocabulary.
 */

int
vfind(sp, voc)
char **sp;
vocab_t *voc;
{
    /* The first character in the string is the Forth count field. */
    register char *s,*p;
    register int length;
    register char *str = *sp;
    register dic_en_t *dictp;
    token_t * cfa;
 
    for( dictp = *hash(voc, str); dictp != (dic_en_t *)0; ) {
        s = &dictp->name;
        p = str;
        length = *s++;
        if ( *p++ != length )
            goto  nextword;
        while ( length-- )
            if ( *s++ != *p++ )
                goto nextword;
    
        cfa = name_from( &dictp->name );
        *(token_t *)sp = (token_t) cfa;

        return ( isimmediate(dictp) ? 1 : -1 );
nextword:
        dictp = dictp->link;
    }
    return (0);        /* Not found */
}

/********************** LFIND() ***********************************/
/* search local dictionary; assumes caller checks that V_NLVAR > 0 */

int
lfind(sp)
char **sp;
{
    register int n;
    register char *cp1, *cp2;
    register struct ld_en_t *spt = local_dic;
    register int nlv = V_NLVAR;
    
    while(nlv--){
        cp1 = *sp;
        cp2 = spt->l_name;
        n = *cp2++;
        if(n != *cp1++) goto nextword;
        if(n>7) n=7;
        while(n--){ if(*cp2++ != *cp1++) goto nextword;}
        /* got it */
        *(token_t *)sp = (token_t)(spt->l_cfa);        
        return (-1);   /* no IMMEDIATE local entries, of course */
nextword:
        spt++;
    }        
    return (0);   /* not found */
}                

/********************** FIND() ************************************/
int
find(sp)
char **sp;
{
    int i, found = 0;
    vocab_t *voc, *last_voc;

    /* check in local dictionary first if there are any entries */
    if(V_NLVAR != 0){
        if((found = lfind(sp))!= 0 ) return (found);
    }
    /* none, so search vocabularies in search order */
    last_voc = (vocab_t *)0;
    for (i = 0; i < NVOCS; i++) {
    voc = ((vocab_t **)&V_CONTEXT)[i];
    if ( voc == 0 )
        continue;
    if ( (voc != last_voc) && ((found = vfind(sp, voc)) != 0) )
        break;
    last_voc = voc;
    }
    return (found);
}

/***********************  ALIGN() ********************************/
/*
 * fills out from current dp to next normal bdy with nulls,
 * sets dp to next normal bdy
 */
void
align()
{
    register int length = (char *)ALIGNED(dp_4th) - (char *)dp_4th;
    register char *rdp = (char *)dp_4th;
    
    while ( length-- )
      *rdp++ = '\0';
    dp_4th = (normal *)rdp;
}

/******************** COMMA_STRING() *****************************/
void
comma_string(str)   /* Places a string in the dictionary */
char *str;
{
    register char *s = str;
    register char *rdp = (char *)dp_4th;
    register int length = (*s) + 1;

    while ( length-- )
     *rdp++ = *s++;
    dp_4th = (normal *)rdp;
    align();
}

/****************** QT_CREATE() **********************************/
/* This was changed to set the number of local variables to zero if
 * NOT compiling. Be Careful,
 */

/*#define QTDEBUG*/
void
qt_create(str, cf)
char *str;
token_t cf;
{
    dic_en_t ** threadp = hash ((vocab_t *) V_CURRENT, str);
    register char *rdp = (char *) dp_4th;
    char *tmpstr = str;
    
    /* increment thread counter */    
    normal nthr = (normal *)threadp - (normal *)V_CURRENT;
    (hashnum[nthr])++;    /* nb--this is lost for first entry, since the 
                          hashnum area has not been set up yet-cf init_dic()*/
    /* there is no guarantee that rdp is aligned; if not, fix it */
    if((normal)rdp & 3) rdp = (char *)ALIGNED(rdp);
    
#ifdef QTDEBUG
    scrprintf("\nqt_create:creating %s   dp=0x%x",str+1,rdp); flushit();
#endif
    if (V_WARNING && vfind(&tmpstr,(vocab_t *)V_CURRENT)) {
        error("\n");
        error(to_cstr(str));
        error(" isn't unique ");
    }

    *(dic_en_t **) rdp = *threadp; /* Place link field */

 /* Link into vocabulary search list and remember lfa for hide/reveal */
    *threadp = (dic_en_t *) rdp;
    V_LASTP = (normal)rdp;

    rdp += sizeof (dic_en_t *);

    *rdp++ = 0;			/* Place flag byte, default not immediate */

    dp_4th = (normal *) rdp;
    comma_string(str);		/* Place name in dictionary */
    *dp_4th++ = (normal) cf;
    if(V_STATE == (normal)INTERPRETING) V_NLVAR = 0;   
        /* turn off any existing local variables if NOT compiling */
#ifdef QTDEBUG
    scrprintf("  done,cf=%d",cf);
#endif
}

/*********************** MAR_CREATE() ********************************/
/* creates dictionary entry which when executed returns the dictionary
pointer at its creation, a 'marker' */

void
mar_create(str)
char *str;				/* FORTH string */
{
    normal scr;
    
    scr = (normal)dp_4th;                    
    qt_create(str,(token_t)DOCON);
    *dp_4th++ = scr;
}

/*********************** L_CREATE() **********************************/
/* code to create 'local' dictionary entries. These exist only for
 * the duration of a colon definition. lcreate() compiles a P_LVAR,
 * which simply causes the inner interpreter to skip the following token
 * AND pfa, the token it is passed (usually DOVAR, DOCON, or DOFCON), which is
 * compiled, and makes an entry in the local_dic array which find() searches 
 * before it searches the normal dictionary.
 * There is a problem. After a word with local variables is defined, V_NLVAR
 * is still the number of local vars until a new colon definition comes along.
 * This is INCREDIBLY useful for debugging, because the local dictionary
 * can still be searched. BUT a subsequent create with the same name as a
 * local variable does not work (that is to say, its value is not valid,
 * because the local entry is searched first.
 */
 
/*#define LCDEBUG*/

void
l_create(str,cf)
register char *str;
token_t cf;
{
    register struct ld_en_t *lsp = &(local_dic[V_NLVAR]);
    register int n;
    register char *dest;
    
    *dp_4th++ = (normal)P_LVAR;      /* place skip token in real dict. */
    lsp->l_cfa = (normal)dp_4th;     /* place cfa in local dict */
    (lsp->l_name)[0] = (n = *str++); /* place name (1st 7 chars) in local dict*/
    dest = lsp->l_name + 1;
    if(n>7) n=7;
    if(n){ while(n--) (*dest++ = *str++);}    
    *dp_4th++ = (normal)cf;         /* place DO-token in real dict. */
    V_NLVAR++;                      /* bump number of local variables */
#ifdef LCDEBUG
    scrprintf("\n"); flushit();
    scrprint_string(lsp->l_name);
    scrprintf(" compiled; nlvar=%d cfa = %d, name = %d",V_NLVAR,lsp->l_cfa,
        lsp->l_name);
#endif
}
    
/* 
 * Hm. P_LVAR normally just skips dotoken and pfa; if we want more things,
 * need more variants on P_LVAR. But how about a new DOword DOLVAR which
 * evokes 
 *     k_push ( ++token ); continue;
 * in mirellin.c; ie pushes the address of the cell *2* cells after the
 * DOLVAR token.
 * The ordinary lvar word would stuff zeros into both the pfa and the
 * following word. But we could populate the first cell with a skip
 * number which P_LVAR could read at execution time; zero evokes the
 * old behavior, any number greater skips that number in addition to the
 * two that P_LVAR normally does.
 * So how to generate this number?
 * One way would be to set up another lcreate primitive which pops
 * the stack, so one would say [ 5 ] larray fivelong, and maybe have
 * THAT evoke a DOLARR word which has proper array behavior, or
 * to change the naming convention so something like lvar 5_long 
 * sets the length parameter to 5, and maybe evokes array behavior;
 * then this opens the way to xlvar for two.
 * SO: this involves 
 *      1. Change the definition of P_LVAR to skip two plus the contents
 *          of the cell following the P_LVAR token cells. Trivial
 *      2. make a DOLVAR to honor this (pushes relevant dic. address)
 *      3. fix l_create to create new dictionary cell and zero it.
 *      ( this restores current fuctionality, using a bit more memory, but
 *          allows room for expansion)
 *
 *      THEN:  think
 *      options. Make a general lcreate which takes a size, and make
 *      new tokens LARR and DOLARR to manipulate. lcreate can take the
 *      size from the stack at compilation, or abstract from the name.
 *      Can add XLVAR token. This does not require a new DOLVAR,
 *      but does require invoking lcreate with size 2. Could make
 *      lcreate general, and do various things with various size
 *      arguments..such as 0 and positive for placing DOLVAR and allocating
 *      space and skipping for n normals, -1 for getting the size from
 *      the stack and placing DOLARR. This would allow for a xlvar
 *      I like this better than changing
 *      the naming convention, though in that case larr could invoke
 *      another l_create entirely with those mods. Very un-Forthy, however.
 */

/*********************** HIDE(), REVEAL() ***************************/
/*
 * prevents a colon redefinition from finding itself while compiling
 */
void
hide()
{
   char *str = (char *)&V_LAST->name;

   dic_en_t **threadp = hash( (vocab_t *)V_CURRENT, str );

   *threadp = V_LAST->link;
}

void
reveal()
{
   char *str = (char *)&V_LAST->name;

   dic_en_t **threadp = hash( (vocab_t *)V_CURRENT, str );

   *threadp = V_LAST;
}

/*****************************************************************************/
/*
 * restore the vocabulary search order to a decent state; usually called from
 * erret
 *
 * If <force> is true, reset the search order to
 *	forth forth root,
 * and make forth the current vocabulary (i.e. `only forth also definitions')
 *
 * Otherwise, look to see if forth is already in the order. If it is, do
 * nothing; if it isn't add it to the order as the first dictionary searched
 * and make it the current vocabulary (i.e. `also forth definitions')
 */
void
reset_vocabs(force)
int force;
{
   int i = 0;

   if(force) {
      ((vocab_t **)&V_CONTEXT)[i++] = forth_vocab;	/* transient */
      ((vocab_t **)&V_CONTEXT)[i++] = forth_vocab;	/* begin resident */
      ((vocab_t **)&V_CONTEXT)[i++] = root_vocab;
      while(i < NVOCS) {
	 ((vocab_t **)&V_CONTEXT)[i++] = NULL;
      }
   } else {
      for(i = 0;i < NVOCS;i++) {
	 if(((vocab_t **)&V_CONTEXT)[i] == forth_vocab) {
	    return;
	 }
      }
/*
 * move the vocabularies down; equivalent to `also'
 */
      for(i = 1;i < NVOCS;i++) {
	 ((vocab_t **)&V_CONTEXT)[i] = ((vocab_t **)&V_CONTEXT)[i - 1];
      }
      ((vocab_t **)&V_CONTEXT)[0] = forth_vocab;
   }
   V_CURRENT = V_CONTEXT;
}

/*************** OUTER INTERPRETER STUFF ******************************/
/*
 * Forth outer (text) interpreter and
 * compile-time actions for immediate words.
 */

/************************* BLWORD() **************************************/
/* Read the next delimited word from the input stream */
/* FIXME  The standard says strings from word should end with a blank */

char *
blword()             /* returns the next blank-delimited word from tibbuf */
{
    register char *bufend = &tibbuf_4th[V_NR_TIB];
    register char *nextc = &tibbuf_4th[V_TO_IN];
    register int c;
    register int i = 1;

    do {
       if ( nextc >= bufend ) {
	  V_DELIMITER = FEOF;
	  goto finish;
       }
    } while ( (c = *nextc++) <= ' ' );

    /* Now c contains a non-delimiter character. */

    do {
    /*
     * If the string collected is longer than the maximum length
     * we can store, it is silently truncated.  This is generally
     * okay because such a string is probably either an error,
     * in which case the interpreter will almost certainly not find a
     * match, or a comment, in which case the string will be thrown
     * away anyway.
     */
       if ( i < MAXSTRING )
	 wordbuf[i++] = c;
       if ( nextc >= bufend ) {
	  V_DELIMITER = FEOF;
	  goto finish;
       }
    } while ( (c = *nextc++) > ' ' );    /* note that \n \r \t are all valid 
                                            delimiters */
    V_DELIMITER = c;

finish:
    wordbuf[0] = i-1;
    wordbuf[i] = '\0';
    V_TO_IN = nextc - tibbuf_4th;
    return (wordbuf);
}

/**************************** WORD() ************************************/
/*
 * '\n' is a universal delimiter (87/04/02)
 */
char *
word(delim)
int delim;
{
    register char *bufend = &tibbuf_4th[V_NR_TIB];
    register char *nextc = &tibbuf_4th[V_TO_IN];
    register int c;
    register int i = 1;

    if (delim == ' ')  /* Only skip leading delimiters if delim is space */
     return (blword());

    for (;;) {
        if ( nextc >= bufend ) {
            V_DELIMITER = FEOF;
            goto finish;
        }
        if ( (c = *nextc++) == delim || c == '\n' )
            break;
        /*
         * If the string collected is longer than the maximum length
         * we can store, it is silently truncated.  This is generally
         * okay because such a string is probably either an error,
         * in which case the interpreter will almost certainly not find a
         * match, or a comment, in which case the string will be thrown
         * away anyway.
         */
        if ( i < MAXSTRING )
        wordbuf[i++] = c;
    }
    V_DELIMITER = c;

finish:
    wordbuf[0] = i-1;
    wordbuf[i] = '\0';
    V_TO_IN = nextc - tibbuf_4th;
    return (wordbuf);
}

/************************ SCOMPILE() ******************************/
void
scompile(str)
Void *str;
{
    normal tp = (normal)str;

    if( find((char **)&tp) ) {
/*
 * If the word we found is a primitive, compile its primitive number
 * instead of its cfa
 */
    if ( *(token_t *)tp < MAXPRIM )
        tp = *(token_t *)tp;
    *dp_4th++ = tp;
    } else {
        V_STATE = (normal)COMPILING;  /* forces where() to compile a LOSE */
        where();  /* does an erret() */
    }
}

/************************* EXEC_ONE() ******************************/
void
exec_one( token )
token_t token;
{
    comp_buf[0] = token;
    comp_buf[1] = FINISHED;
    in_intp(comp_buf);
}

/************************* INTP_WORD() **************************/
int
intp_word(str)
char *str;
{
    normal tp = (normal)canon(str);
    int immed;

    if ((immed = find((char **)&tp)) != 0) {
        /*
             * If the word we found is a primitive, use its primitive 
             * number instead of its cfa
        */
        if ( *(token_t *)tp < MAXPRIM )
            tp = *(token_t *)tp;

        if ( immed > 0 || V_STATE == (normal)INTERPRETING )
            exec_one(tp);
        else
            *dp_4th++ = tp;
    } else if ( !hand_lit(str) ) {
        where();
    }
    return(1);
}


/************************* INTP_EWORD() **************************/
/* this is like intp-word but does not handle literals and does not
 * bomb if the word is not found; it just returns 0 (forth false) 
 * and an error message. This may be incorrect behavior.
 */
 
int
intp_eword(str)
char *str;
{
    normal tp = (normal)canon(str);
    int immed;

    if ((immed = find((char **)&tp)) != 0) {
        /*
             * If the word we found is a primitive, use its primitive 
             * number instead of its cfa
        */
        if ( *(token_t *)tp < MAXPRIM )
            tp = *(token_t *)tp;

        if ( immed > 0 || V_STATE == (normal)INTERPRETING )
            exec_one(tp);
        else
            *dp_4th++ = tp;
        return(-1);
    } else {
        /* just go on */
        scrprintf("\nForth word %s not found",tp+1);
        return(0);
    }
}


/*********************** WHERE() *********************************/
/*FIXME--get rid of erret() and redundant code */

void
where()
{
    if ( V_STATE == (normal)COMPILING ) 
        *dp_4th++ = (normal)LOSE;
    strcat(&wordbuf[1]," ? ");        
    if(in_fl_name[0] != 0) {		/* loading from file */
       scrprintf(" line %d: ",V_LN_IN); fflush(stderr);
    }
    if(!MIREDIT) error("\n");		/* editor always cr before prompt */
    erret(&wordbuf[1]);
}

/************************ HAND_LIT ********************************/
int
hand_lit(str)
char *str;
{
    normal n;
    int ok ;

    ok = number( str, &n);
    if ( ok == 1 ) {
        if ( V_STATE == (normal)COMPILING ) {
            if(V_DPL < 0) {      /* int or pointer */
                compile(P_LIT);
                *dp_4th++ = n;
            }else{
                compile(P_FLIT); /* float */
                *dp_4th++ = n;  /* the routine thinks n is an int; ok */    
            }            
        }else if(V_DPL < 0){
            *--xsp_4th = n;      /* int or pointer, push on parameter stk */
        }else{
            *--xfsp_4th = *(float *)(&n) ; /* float, push on float stk */
        }
        return (1);
    }
    return (0);
}

/*********************** STACK_INIT() *******************************/
void stack_init()
{
    xsp_4th  = ps_top;
    xfsp_4th = fs_top;
    xtsp_4th = ts_top;
    ssinit();
}

/*****************************************************************************/

void
reset_tib()
{
   V_SPAN=0;
   V_TO_IN=0;
}


/**********************OUTER INTERPRETER ******************************/
void out_intp()
{
    char *thisword;
    normal onum;  

    V_STATE = (normal)INTERPRETING;
    while (1) {
     /* I think this is redundant; let's try leaving it out */   
/*     if(interrupt){   
        erret((char *)NULL);
     } */
     xrp_4th = &rtn_stk[XRSSIZE];
     if(V_SPAN >= 0){     /* load_pop() sets span to -1 when there is
                            input left from a line in a previous file */
         V_LN_IN++;
         V_TO_IN = 0;
         V_SILENTOUT = 0;  /* if the previous line has turned off printing,
                              turn it back on */
         if(V_RES_STK) stack_init();  /* if in idiot mode, reset the stacks*/
         onum = cexpect(tibbuf_4th, TIBSIZE);
             /* if cexpect evokes a load_pop(), the input buffer is set to the
                old popped one if something is left in it, and span is set to 
                -1; otherwise should use the value returned by cexpect
                for the number of input chars. */
                /* an erret was added to GET1CHAR, where (I think) it
                 * belongs
                 */
#if 0                 
                /* code added here jeg0603, still mysteries */
                if(interrupt){
                    interrupt=0;
                    V_SPAN = V_NR_TIB = onum = 0;
                    erret((char *)NULL);
                }
#endif
                
         if(V_SPAN >= 0) V_NR_TIB = onum;
         else V_SPAN = V_NR_TIB;  /* get rid of negative span */
     }else{
        V_SPAN = V_NR_TIB;
     }
     insane = 0;         /* reset bailout at each invocation */
     while ( ((thisword = blword())[0]) && intp_word(thisword) ) {
        /* a load_pop can also occur in the while, from the execution
            of a ;s . In that case span=-1 is caught and the old input
            buffer is interpreted */  
        insane = 0; /* reset bailout again at each interpreted word */
        if ( xsp_4th > ps_top ) {
        error("Stack Empty");  
        xsp_4th = ps_top;
        goto stackerror;
        }
        if( xfsp_4th > fs_top ){
        error("Floating Stack Empty");  
        xfsp_4th = fs_top;
        goto stackerror;
        }
        if( xtsp_4th > ts_top){
        error("Temporary Stack Empty");  
        xtsp_4th = ts_top;
        goto stackerror;
        }
        if ( xsp_4th < par_stk ) {
        xsp_4th = ps_top;
        error("Stack Overflow");
        goto stackerror;
        }
        if ( xfsp_4th < fl_stk ) {
        xfsp_4th = fs_top;
        error("Floating Stack Overflow");
        goto stackerror;
        }
        if ( xtsp_4th < tem_stk ) {
        xtsp_4th = ts_top;
        error("Temporary Stack Overflow");
        goto stackerror;
        }
        continue;
stackerror:
        if(!MIREDIT ) error("\n");
        break;
     }
    }
}

/************************ NUMBER ************************************/

/*#define NDEBUG*/
/*
 * str is a forth string, n a pointer to a normal for the return value;
 * V_DPL is set to the decimal point location if float (contains .
 * and optionally a following e or an embedded + or -; e-scientific 
 * notation no work if base >15; sorry. In fpf (float only) mode,
 * integers and pointers are terminated by 'i'. Integers beginning with
 * '0x' are interpreted as hex integers, a la C, but integers with
 * leading 0s are *NOT* interpreted as octal.
 * integers with terminating K or M are scaled, but beware of maximum
 * size of a 32-bit int. NOTE!!! that this does NOT break arbitrary bases for
 * which K or M might be a valid int, because mirella only recognizes
 * lower-case characters, thus 98a5, not 98A5.
 */
 
int
number( str, n )
char *str;
normal *n;
{
    int base = V_BASE;
    int len = *str++;
    int eflag = 0;
    int eaccum = 0;
    char c;
    int d;
    int isminus = 0;
    int esign = 1;
    char *eptr;
    unsigned normal accum = 0;
    double faccum = 0.;
    float fnum;
    double fac;
    int expon;
    int fflag = 0;
    int iflag = 0;     /* flag for integer or pointer if fpf is true */
    int ifac = 1;      /* factor for K, M, etc */

    V_DPL = -100;
    if( *str == '-' ) {
        isminus = 1;
        len--;
        ++str;
    }else if( *str == '+' ) {   /* correctly interpret gratuitous leading + */
        len--;
        ++str;
    }else if( *str == '0' ) {   /* code added jeg 9507 */
        len--; 
        ++str;
        if( *str == 'x' ) {     /* if second char is 'x', set base to 16; if
                                 * not, just ignore leading zero--we do NOT
                                 * treat lz numbers as octal as C does   
                                 */
            base = 16;
            len--;
            ++str;  
        }
    }
    for( ; len > 0; len-- ) {
        c = *str++;
        if( (c == '.') && fflag == 0 ){
            V_DPL = 0;
            fflag = 1;
            faccum = (double)accum;
        }else if(c == 'i' && len == 1 && fflag == 0){
            iflag = 1;      /* integer if term by 'i'; optional unless fpf */
        }else if((c == 'K' || c == 'M') && len == 1 && fflag == 0){
            ifac = 1024;
            if(c == 'M') fac *= 1024;
        }else if( fflag && !eflag &&
            ((( c == 'e' || c == 'E') && (base < 15)) ||
            (( c == 'd' || c == 'D') && (base < 14))) ){  /* &^%*# Fortran */
            eflag = 1;        
            eptr = str ;  /* one greater than pointer to e */
        }else if(fflag && !eflag && (c == '+' || c == '-')){
            eflag = 1;
            eptr = str-1;
            if(c == '-') esign = -1;
        }else if(eflag && ((c == '-') || (c == '+')) && ((str - eptr) == 1)){
            if ( c == '-' ) esign = -1;  /*negative exponent, else skip but ok*/
        }else{
            if( -1 == (d = digit( base, c )) ){
                fflag = 0;
                break;      /* no good */
            }
            if(!eflag){
                ++V_DPL;
                if(fflag) faccum = faccum * (double)base + (double)d;
                else accum = accum * base + d;
                /* this nonsense is to accomodate float numbers with more
                'precision' than can be represented by a scaled integer. Bah. */
            }else{
                eaccum = eaccum * base + d;
            }

        }
    }
    if (len) return ( 0 );     /* not a legal number */
    
    if ((!V_FPF && V_DPL < 0) || iflag){      /* int or pointer */
        V_DPL = -1;
        *n = isminus ? -accum*ifac  : accum*ifac;
#ifdef NDEBUG
        scrprintf("\nOutput number is %d ",*n);
#endif
    }else if(V_FPF && V_DPL < 0){  /* float-only input and integer; convert */
        V_DPL = 0;        
        fnum = (double)accum;
        *(float *)n = isminus ? -fnum : fnum;
    }else{   /* bona fide float */
        expon = -V_DPL + esign * eaccum;
        fac = (double)base;
        if (expon <0 ){ 
            fac = 1./fac ;
            expon = -expon;
        }
        fnum = faccum;
        for(d=0; d<expon; d++)  fnum *= fac ;
        *(float *)n = isminus ? -fnum : fnum;
#ifdef NDEBUG
        scrprintf("\nOutput number is %f ",fnum);
#endif
    }
    return ( 1 );
}


/************************ DIGIT() *******************************/
/*
 * Converts the character c into a digit in base 'base'.
 * Returns the digit or -1 if not a valid digit.
 * Accepts either lower or upper case letters for bases larger than ten.
 * Only works for ASCII.
 */
int
digit( base, c )
int base;
int c;
{
    register int ival = c;

    ival -= '0';
    if( ival < 0 )
     return(-1);
    if( ival > 9 ) {
    ival -= ('A' - '0' - 10);
    if( ival < 10 )
        return(-1);
    if( ival > ('Z' - 'A' + 11) ) {
        ival -= ('a' - 'A');
        if( ival < 10 )
        return(-1);
    }
    }
    return( ival >= base ? -1 : ival );
}

/********************* CMOVE(), CMOVE_UP() *****************************/
void
cmove(from, to, length)
register char *from, *to;
register normal length;
{
    if ( length )
    do {
        *to++ = *from++;
    } while (--length);
}

void
cmove_up(from, to, length)
register char *from, *to;
register unsigned length;
{
    from += length;
    to += length;

    if ( length )
    do
        *--to = *--from;
    while (--length);
}

/**************************** FILL ***********************************/
void
ffill(to, length, with)
register char *to;
register unsigned length;
register int with;
{
    if ( length )
    do
        *to++ = with;
    while (--length);
}

/*************************** HDL_SIG() *********************************/
#ifdef SIGNALS 

/*#define SIGDEBUG*/

/* this did not work under Ubuntu 10.04, desktop gcc 4.4.3-4ubuntu5; 
 * ^C just exits, no matter
 * what you do. It DOES work under 10.04 server, which has the same gcc????
 * but is a virtual machine.
 * does not work on arianna or lilinoe, does on nimue
 * it is never executed. So `signal' is not working properly
 * 110205: trouble was with tee in output to logfile; works if do not
 * tee, but is still a bug somewhere. I do not think the `better' sigaction
 * option works at all.
 */
 
static char *errcp;

void
hdl_sig(sig)
int sig;
{
    void reset_scr();
    char *screensize();
    char c;

#ifdef SIGACTION
    /* this should not be necessary */
    if(sigflg){
        sigtemp.sa_handler=(void (*)P_ARGS((int)))hdl_sig;
        sa_err=sigaction(sigflg,&sigtemp,(struct sigaction*)NULL);
    if(sa_err){
       printf("\nCannot initialize signal %d handler",sigflg);
    }
#else
    signal(sig,(void (*)())hdl_sig);   /* reset--you may not do an erret()
                                        * before you get another one 
                                        */
#endif /*SIGACTION*/

    sigflg = sig;    /*  global last signal id flag*/
    switch(sig) {
#ifdef SIGDEBUG        
    printf( "\n hdl_sig:Got an interrupt, sig=%d, SIGINT=%d, insane=%d",
        sig, SIGINT,insane);
    fflush(stdout);
#endif /*SIGDEBUG*/

    case SIGINT:
        interrupt++;
#ifdef SIGDEBUG
        printf("\nGot a SIGINT, #=%d\n",interrupt); fflush(stdout); 
#endif
        /* if outer interpreter has not been started, out */
        if( m_errexit ) exit(1);                                    
        /* if interrupt user flg is on, return to outer interpreter */        
        if(V_INTERFLG) longjmp(env_4th,(int)"\n^C");  
        /* otherwise just go your merry way with interrupt incremented */
        else{
            if(interrupt == 1){
                mprintf("\n^C ");
            }else{
                mprintf("\n^C%d\n ",interrupt); 
            }
            flushit();
        }
        if(interrupt > 3){ 
error("\nYou are VERY impatient, Amore. Shall I return to the interpreter (y)");
error("\n  or wait for the code to handle the interrupt gracefully (n) ?"  );

        /* 
         * jeg0603: This used a 'yesno', but it does not pause for some reason
         * under Linux...so back to getchar() 
         */
            if((c= getchar()) == 'y' || c=='Y'){
                longjmp(env_4th,(int)"\nI refuse to be responsible for this");
            }
        }
#ifdef SIGDEBUG
        printf("\nreturning from hdl_sig");
#endif
        return;
    case SIGILL:
        errcp = ("\nIllegal Instruction");
        break;
    case SIGSEGV:
        errcp = ("\nSegmentation violation");
        break;
#if defined(SIGBUS)
    case SIGBUS:
        errcp = ("\nBus Error");
        break;
#endif
#if defined(SIGIOT)
    case SIGIOT:
        errcp = ("\nIOT Instruction Trap");
        break;
#endif
#if defined(SIGEMT)
    case SIGEMT:
        errcp = ("\nEMT Instruction Trap");
        break;
#endif
    case SIGFPE:
        errcp = ("\nArithmetic exception");
        break;
    case SIGWINCH:
        errcp = (screensize()); 
        reset_scr();
        insane = 0;
        break;
    }
    /* get 4 interrupts per word; see out_intp(); if early, leave */
    if ( (insane++)/4 || m_errexit ) {
        exit(1);
    }else{
        longjmp(env_4th,(int)errcp);
    }
}

/*****************************************/

/* SETSIGINT(): simple to call routine to reestablish hdl_sig as handler */

void setsigint()
{
#ifdef SIGNALS
    void * hdl_ret;
#ifdef SIGACTION
    /* this should not be necessary */
    sigmirella.sa_handler=(void (*)P_ARGS((int)))hdl_sig;
    sa_err=sigaction(SIGINT,&sigmirella,&sigtemp);
    if(sa_err){
       printf("\nCannot initialize SIGINT handler");
    }
    if(sigtemp.sa_handler != hdl_sig){
        printf(
        "SIGINT handler:handler=%u,hdl_sig=%u,SIG_DFL=%u\n",
        (unsigned)sigtemp.sa_handler,(unsigned)hdl_sig,(unsigned)SIG_DFL);
    }
#else
    hdl_ret = (void *)signal(SIGINT,(void (*)())hdl_sig);
    if( hdl_ret != (void *)hdl_sig ){
         printf(
        "SIGINT handler:handler=%u,hdl_sig=%u,SIG_DFL=%u\n",
        (unsigned)hdl_ret,(unsigned)hdl_sig,(unsigned)SIG_DFL);
    }
#endif /*SIGACTION*/
#else  /*SIGNALS*/
    ;
#endif /*SIGNALS*/
}

/****************************************/

void
hdl_quit(i)				/* we've found a ^\ */
int i;					/* unused */
{
   char ans[10];
   ans[0] = i;				/* quieten compilers */

   (void)get1char(EOF);
   reset_kbd();
   error("Do you want to return to the prompt? y or n ");
   (void)scanf("%s",ans);
   if(ans[0] == 'y') {
      set_kbd();
      erret((char *)NULL);
   }

   error("Do you want to save the dictionary? y or n ");
   (void)scanf("%s",ans);
   if(ans[0] == 'y') {
      save_dic("debug.dic");
      error("The dictionary image is in debug.dic\n");
   }

   error("Do you want a core dump? y or n ");
   (void)scanf("%s",ans);
   if(ans[0] == 'y') {
      get1char(EOF);
      abort();
   } else {
      m_bye(1);
   }
}

static void
hdl_pipe(sig)
int sig;
{
   signal(sig,(void (*)())hdl_pipe);
   fprintf(stderr,"\nBroken Pipe");
}
   
void setsigdfl(sig)   /* reestablishes default signal handling; if sig=-1,
                        resets all; if sig = SIGINT ... SIGFPE, resets that
                        one. */
int sig;                        
{
    switch(sig){
    case SIGINT:    signal(SIGINT,SIG_DFL);  break;
    case SIGILL:    signal(SIGILL,SIG_DFL);  break;
#if defined(SIGIOT)
    case SIGIOT:    signal(SIGIOT,SIG_DFL);  break;
#endif
#if defined(SIGEMT)
    case SIGEMT:    signal(SIGEMT,SIG_DFL);  break;
#endif
    case SIGSEGV:   signal(SIGSEGV,SIG_DFL); break;
#if defined(SIGBUS)
    case SIGBUS:    signal(SIGBUS,SIG_DFL);  break;
#endif
    case SIGFPE:    signal(SIGFPE,SIG_DFL);  break;
#if defined(SIGPIPE)
    case SIGPIPE:   signal(SIGPIPE,SIG_DFL);  break;
#endif
    case (-1):
        signal(SIGINT,SIG_DFL);
        signal(SIGILL,SIG_DFL);
#if defined(SIGIOT)
        signal(SIGIOT,SIG_DFL);
#endif
#if defined(SIGEMT)
        signal(SIGEMT,SIG_DFL);
#endif
        signal(SIGSEGV,SIG_DFL);
#if defined(SIGBUS)
        signal(SIGBUS,SIG_DFL);
#endif
        signal(SIGFPE,SIG_DFL);
#if defined(SIGPIPE)
        signal(SIGPIPE,SIG_DFL);
#endif
        break;
    default:
        scrprintf("\n%d is not a valid signal code",sig);
        break;
    }        
}
#endif /* SIGNALS */


#ifndef SIGNALS

void setsigdfl(sig)
int sig;
{
#ifdef DSI68
    _ABORT(1);
    scrprintf("\nResetting ^C only");
#else
#ifdef NDP_PL
    trapreset(sig);
#else    
    scrprintf("\nCannot reset signals on this system");
#endif
#endif
    flushit();    
}
#endif

/********************** ERRET() ******************************************/
/*
 * does a longjmp(env_4th,arg) or exit(1), whichever is appropriate
 */
static char s_no_msg, *no_msg = &s_no_msg; /* signal a return with no text */

void
erret(arg)
char *arg;
{
   if(m_errexit){			/* outer interpreter isn't set up */
      printf("%s","\nReceived interrupt before outer interpreter started\n"); 
      if(arg != (char *)NULL) {
	 printf("%s",arg);
      }
      exit(1);
   } else {
      gmode(0);				/* clean up graphics */
      if(arg == (char *)NULL) {
	 arg = no_msg;			/* NULL means the call to setjmp */
      }
      longjmp(env_4th,(int)arg);
   }
}

/********************** FIXERROR(), CLRINT() ******************************/
/*
 * does fixing up after a call to erret()
 */
static void 
fixerror(s)
char *s;
{
    clrint();    /* check for interrupt and clear it */
    gmode(0);
    if(s != no_msg) {
       (*erretfun)(s);
    }
    interrupt = 0;
    if(input_file != stdin) norm_input(); 
}

void clrint()
{
    if(interrupt){
        interrupt = 0;
        /*cprint("\nInterrupt");*/
    }
}

/*********************** ENVALLOC() *********************************/
char *envalloc(s)  /* allocates space in appl. environment area. s in bytes */
int s;
{
    /* try to put in some protection */
    char *ret;
    int size;

    if(s<0)erret("\nNegative argument to envalloc()");
    size = (s-1)/sizeof(normal) + 1; /* aligns on normal boundary */
    ret = (char *)ap_ptr ;
    /* FIXME: addresses below cast as normals. should be u_norm or token_t */
    *ap_env = (normal)(ap_ptr + size);     /* ap_ptr is normal *   */
        /* note cannot use #define of ap_ptr as lvalue because of cast */
    if(ap_ptr >= ap_env + APP_AREA){   /* ap_env is normal *  */
        *ap_env = (normal)(ap_ptr - size);   /* ditto re #def */ 
                /* if too big, reset and exit */
        scrprintf(
            "\nApplication environment full, ptr = 0x%x, ceiling = 0x%x",
            (int)ap_ptr,(int)(ap_ptr + APP_AREA)); fflush(stderr);
        erret((char *)NULL);
    }
    if(ap_ptr < ap_env + 1){      /* aaarrgh */
        scrprintf(
        "\nApplication env pointer corrupted, ptr=%x, range 0x%x - 0x%x",
            (int)ap_ptr,(int)(ap_env + 1),(int)(ap_env + APP_AREA));
        fflush(stderr);
        erret((char *)NULL);
    }
#ifdef DEBUG
    scrprintf("\ns,old, new env pointers = %d %x %x",s,ret,ap_ptr);
#endif
    return ret;
}

/***********************************************************************/
/*
 * Initialise C-linked code
 */
static void
mircl_read()
{
   static void (*setup[N_CLSET])() = {
      misetup, mesetup, m1setup, m2setup, m3setup
     };
   int i;

   for(i = 0;i < N_CLSET;i++) {
      clset[i].init0 = clset[i].init1 = clset[i].init2 = clset[i].init3 = NULL;
      (*setup[i])(&clset[i].nvar,&clset[i].nfunc,&clset[i].varar,
		  &clset[i].funcar,&clset[i].next_cif,&clset[i].init0,
		  &clset[i].init1,&clset[i].init2,&clset[i].init3);
   }
}

static void
mircl_init()
{
   int i;

   for(i = 0;i < N_CLSET;i++){
      if(clset[i].init0 != NULL) {
	 (*(clset[i].init0))();
      }
   }
/*
 * the pointer and function arrays are in static arrays set up in
 * the various mirel#c.c files; they need to be given to init_cif
 * in turn.
 *
 * First the interns, then the 0-level and secondary C-links
 * Note that any or all of them can contribute
 */
   for(i = 0;i < N_CLSET;i++){
      if(clset[i].nvar || clset[i].nfunc) {
	 init_cif(clset[i].next_cif,(normal *)clset[i].varar,clset[i].funcar);
      }
      if(i == 0) {
	 mar_create("\10<endint>");	/* marker for end of internals */
      }
   }
}

