/*VERSION 90/05/06: Mirella 5.50                                        */
/*********************** MIRELLIO.C *************************************/

#ifdef VMS
#include mirkern
#include dotokens
#include mireldec
#else
#include "mirkern.h"
#include "dotokens.h"
#include "mireldec.h"
#endif
#ifdef DLD
#include "dld.h"
#endif

#define NDIR 10				/* max num. of directories to search */

/* Recent history:
88/05/01: Fixed findmdir so that it aborts on either a null pointer
            or a null name for mirelladir
	    (renamed to set_dirpath at some point)
88/06/02: Added #if--#endif support for conditional compilation            
88/06/04: Added #else support for conditional compilation. Need to fix
            forth code in mirella.m --done.
88/06/18: Fixed last, which was hopelessly muddled, and added nesting
88/08/06: Added code to load_push() and load_pop() to handle file thread
            properly during nested loads
88/08/30: Added code to emit() and related functions (expect, cexpect)
            to make screen buffer as well as (if sel.) a log file. 
88/08/31: Added code to get_dic to support new memory allocation scheme;
          aborts with message if origin has moved since the image was
          created.
88/10/22: Finally wrote va_args version of mprintf for systems which
          support va_args AND vsprintf(). Used if VARARGS is defined in
          mirella.h. SPARC has to have it.          
88/10/24: Fixed get_dic to accomodate dynamically allocated break. 
88/10/27: "     "       to save name and lfa of last C-link, to enable
            checking of validity of dictionary image, and to get an image
            from MIRELLADIR if there is none in the cwd.         
89/06/10: Added mcfname() fn to return mirelladir path for basename            
90/04/17: Added primitive print functions to write to screen: scrprintf(),
            and the rawraw functions (*scrnprt)(), (*scrnput)(), (*scrnflsh)().
            The hierarchy works like this: 
                mprintf(), tputs(), emit() go through the full Mirella i/o
                    mechanism, to the log file, to the screen, and to the
                    screen buffer.
                scrprintf(), scrputs(), scrput() go everywhere except the
                    log file;
                all use the raw functions (*scrnprt)(), (*scrnput)(), and 
                (*scrnflsh)() to write to the screen, and these do nothing 
                else, and are assignable to any device with some code.
                Note that (*scrnprt) is NOT a printf clone--it just
                prints a string. NEED TO WORK ON WSCRBUF() in mirscrn.c
90/05/06: changed mcfopen,mcfopenb to support searching a 'parent' directory,
            and op_nxt_f to support naming one.
90/05/07: wrote code to deal with command-line arguments as a function
            called by mirallam().
*/            

/* Exported definitions:
 *
 * q_restart(argc,argv);            Check for dictionary input image
 * init_io();                       Initialize io system
 * emit(char);                      Output a character
 * cprint(cstr);                    Print C string
 * tputs();                         Print C string with \n      
 * m_gets();                         gets, but uses expect() and Mirella i/o
 * mrgets();                        gets, raw Mirella term input
 * error(cstr);                     Print C str on screen alone
 * get_dic(cstr), save_dic(cstr)    read, write dictionary image
 * fdot(),dfdotr()                  Output formatted float
 * actual = cexpect(addr, count);   Collect a command string
 * actual = expect(addr,count);     Collect a line of requested input
 * char = key();                    Get the next input character
 * forth_string in_fl_name[];       Name of the current input file
 * char *infilename                 pointer, same as above, not decl as array
 * FILE *input_file;                input file pointer
 * int eprompt();                   prints the prompt string, returns length
 * load(), semi_s()                 loading primitives
 * flushit();                       Flushes output
 * int _yesno();                    Waits for key; 1 if y/Y, 0 if n/N 
 * s = mdir(str);                   Rets pointer to string mdir/str
 * n = mkey_avail();                Rets number of chars in typeahead buffer,
 *                                       if system supports
 * mprintf(args);                   like printf(), but goes through Mirella
 *                                  i/o system.
 * set_dirpath(str)                 reads dirpath from .mirella
 * add_dirpath(str)                 adds directories to the search list
 *
 * some primitive i/o to screen functions:
 * int (*scrnprt)(str)              prints the C-string str to the screen
 * int (*scrnput)(c)                puts the char c to the screen
 *
 */
 
int stdprt();
int stdput();
int stdflsh();              /* standard system screen-writing functions */

int  (*scrnprt)() = stdprt;
int  (*scrnput)() = stdput;
int  (*scrnflsh)() = stdflsh;

normal grafmode=0;          /* if grafmode=0, normal terminal i/o; if 1 or 2,
                                primary screen is in line graphics mode; if
                                3 or more, image mode; used in mirgdis.c,
                                miredit.c, and mirage code */

extern int use_editor;
extern int verbose;

#define STRINGINPUT (FILE *) -5
#define LSTACKSIZE 8               
/* depth of load stack */

static struct l_stk_t{
    char infname[LEN];       /* filename */
    char in_line[132];       /* tibbuf at time of load */
    normal inbufsize;       /* #tib at time of load */
    FILE *infptr;           /* filepointer */
    normal llptr;           /* line counter */
    normal inptr;           /* >in at time of load */
    vocab_t *scurrent;      /* current at time of load */
    dic_en_t *sbefore;      /* contents of current->last_word at time of load*/
    normal olfile;          /* value of V_LASTFILE at time of load */
    normal *herbefore;      /* here at time of load */
    normal *herafter;       /* here after placing marker */
} lstk[LSTACKSIZE];          /* total overhead about 1600 bytes */

FILE            *input_file = NULL;    /* forth input */
static FILE     *out_file = NULL;      /* log file */
static int      gargc;
static char     **gargv;
static int      tls;                /* index into load stack */
static int      clload = 1;         /* flag for loading from command line */
static int      oclload = 0;        /* flag for marking bottom file of load
                                        stack as commd-line loaded */
/* static char     strbuf[128]; not used?? */  
static char     *strptr;
static normal   dot_stk[8];           /* pushback stack for '.', '.r' */
static float    fdot_stk[8];          /* pushback stack for 'f.', df.r */
static unsigned int dot_sp = 0;
static unsigned int fdot_sp = 0;
static void load_push(),
  	    load_pop(),
	    s_i_f_name(),
	    nullname(),
	    fstrcpy();
static char *filename();
static FILE *op_nxt_f();
/*
 * command_line argument and flags stuff
 */
static char m_dictfname[60] = "";

#define COMMAND_LINE "command-line"	/* name of command line `file' */
/*
 * Name of input file or null string if standard input
 */
char *in_fl_name = NULL;
char logfname[64];   /* name of logfile */
normal m_termflg=1;    /* is our output to a terminal-like device ?? */
normal *m_oldorigin = 0; /* dictionary image origin */
normal ns_dir=0;        /* number of search directories */
extern normal *m_lastlink;  /* from mirella.c...lfa of last clink */

/*********************** INITIALIZATION ROUTINES ***********************/
char *dir_list[NDIR + 1];	        /* list of directories to search */
extern char full_name[];		/* full name of programme */
char home_dir[LEN];			/* home directory */

/*********************** MIRELCMD() ***********************************/
/*
 * reads the command line and sets the global flags implied therein;
 * NB: all the flags must come first
 * mirelcmd is executed very early, before much of the  
 * Mirella underpinnings are in place. Since the command line can cause
 * the loading of files and the execution of forth command strings, much
 * of the work must be put off till later, when we are in a position to
 * call op_nxt_f (in init_io()), which handles loading and command
 * strings. This situation makes it necessary to use the switches in order:
 * The switches -d, -D (dictionary image name), -f def_file, and -v (silent
 * loading flag) must precede any filenames to be loaded, which in turn must
 * precede any string command input, headed by the switches -s or -e.
 * The string command input may be the last thing on the command line without
 * any terminator or may be optionally terminated by -, +, -&, -C (all for
 * 'continue' --ie jump to the outer interpreter; all this is handled by
 * op_nxt_f() ), or -Q (exit after execution of the command string)
 * -s input normally continues (but is overridden by a terminating -Q) and 
 * -e input always quits, and a list of filenames to load quits UNLESS
 * there is a -, +. -C, or -&. The + option is to fool various terminal
 * programs which allow a program name as the last thing on the command
 * line, but terminating with -anything makes this not work. 
 * If there IS no string command input, there MUST be a terminator as above.
 */
void
mirelcmd(argc,argv)
int argc;
char **argv;
{
   char *argv0;
   char c;

   argv0 = *argv;			/* name of executable */

   while(--argc > 0){
      if((*(++argv))[0] == '-'){  /* switch */
	 switch(c = argv[0][1]){
	  case 'v':
	    if(isdigit(c = argv[0][2])) {
	       verbose = c - '0';
	    } else {
	       verbose = 1;
	    }
	    break;
	  case 'd':
	    strcpy(m_dictfname,*(++argv)); --argc;
	    break;
	  case 'D':
	    if(argc == 0) {
	       fprintf(stderr,"You must give a directory name with -D\n");
	    } else {
	       argc--; argv++;
#if defined(vms)
	       sprintf(full_name,"%s%s",*argv,argv0);
#else
	       if((*argv)[strlen(*argv) - 1] == '/') {
		  sprintf(full_name,"%s%s",*argv,argv0);
	       } else {
		  sprintf(full_name,"%s/%s",*argv,argv0);
	       }
#endif
	    }

	    if(access(full_name,1) != 0) {
	       fprintf(stderr,"%s is not executable: ",full_name);
	       perror("");
	       return;
	    }
	    break;
	  case 'f':		/* choose name of defaults_file */
	    if(argc <= 1) {
	       fprintf(stderr,"You must specify a file with -f\n");
	    } else {
	       set_defs_file(argv[1]);
	       argc--;
	       argv++;
	    }
	    break;
	  default:	  
	    /* if there is an unrecognized switch at this point, it may
	     * be a command or continuation or be illegal. We do not
	     * know at this point; it is passed to op_nxt_f() to decide
	     * through the statics gargc and gargv
	     */
            ++argc;         /* some other switch; set back and exit */
            --argv;
            gargc = argc;
            gargv = argv;   /* transfer to statics */
            break;
	 }
      } else {				/* last switch or no switches */
	 ++argc;
	 --argv;
	 gargc = argc;
	 gargv = argv;			/* transfer to statics */
	 break;
      }
   }

   set_dirpath();
   if(m_dictfname[0] != '\0') {		/* read the preamble */
      m_oldorigin = get_pdic(m_dictfname);
   }

#ifdef DLD
    if(*full_name == '\0') {
       strcpy(full_name,dld_find_executable(argv0));
    }
#endif
}
 
/**************************** Q_RESTART() ************************************/
/*
 * does an ordinary dictionary initialization; then reads image if
 * present over it. returns 1 if 'd' flag is present and dictionary
 * image was read
 */
int
q_restart()
{
    init_dic();				/* GET THIS OUT OF HERE */

    dict_image = 0;
    if(*m_dictfname){
        get_rdic();			/* get rest of the dictionary image */
	do_init_2();			/* Allow application code to call its
					   second initialisation functions */
        dict_image = 1;
    } 
    return dict_image;
}


/**************************** INIT_IO() ******************************/
void
init_io()
{
    if(gargc <= 1) clload = 0;   /* no files for command-line loading */
    lstk[0].infptr = stdin;
    nullname(lstk[0].infname);
    tls = 0;

    if((in_fl_name = malloc(LEN)) == NULL) {
       fprintf(stderr,"Can't allocate storage for in_fl_name\n");
       exit(1);
    }
    in_fl_name[0] = 0;
    in_fl_name[1] = '\0';

    input_file = (gargc <= 1) ? stdin : op_nxt_f() ;
}

/********************* MIRELLA DIRECTORY ROUTINES ************************/
/*
 * this routine is executed early and only once; use stdio functions
 * FIXME!!!!! Do NOT include home_dir; useless (I think); first entry should be
 * "./", not null
 */
void
set_dirpath()
{
   int i;
   char *md;
   char *ptr;
   
   i = 0;
   dir_list[i++] = "";			/* current directory */
   
   if(!(getcwd(home_dir,100))){
      printf("\nError finding home directory");
      erret((char *)NULL);
   }
   if(strlen(home_dir) > LEN){
      printf("\nHome directory name too long");
      erret((char *)NULL);
   }
#if !defined(vms)
   {
      int len;
      if((len = strlen(home_dir)) > 0 && home_dir[len - 1] != '/') {
	 home_dir[len++] = '/';
	 home_dir[len] = '\0';
      }
   }
#endif
   dir_list[i++] = home_dir;		/* initial working directory */

   if((md = get_def_value("dirpath")) == NULL || *md == '\0'){ 
      printf("\nCannot find dirpath or MIRDIRPATH; I quit");
      erret((char *)NULL);
   } else {
      while(isspace(*md)) md++;
      if((ptr = malloc(strlen(md) + 1)) == NULL) {
	 printf("\nCan't get space for dirpath=%s",md); flushit();
	 erret((char *)NULL);
      }
      strcpy(ptr,md);
   }
   for(;i < NDIR;i++) {
      while(isspace(*ptr)) ptr++;
      if(*ptr == '\0') break;
      
      dir_list[i] = ptr;
      while(*ptr != '\0' && !isspace(*ptr)) ptr++;
      if(*ptr != '\0') *ptr++ = '\0';
   }
   ns_dir = i;
   dir_list[i] = NULL;			/* dimen is NDIM + 1 so this is safe */
}

/*
 * This routine allows an application to add to the dirpath list
 * s can be a string holding one directory or a blank-delimited list.
 */
 
void
add_dirpath(s)
   char *s;
{
   int i;
   char *ptr;
   
    for(i=0; i < NDIR + 1; i++){
        if(dir_list[i] == NULL) break;
    }
    if( i == NDIR ) return;  /* full up */
   
    if( s == NULL || *s == '\0' ){ 
        return;
    } else {
       while(isspace(*s)) s++;
       if((ptr = malloc(strlen(s) + 1)) == NULL) {
	    printf("\nCan't get space for dirpath=%s",s); flushit();
	    erret((char *)NULL);
        }
        strcpy(ptr,s);
    }
    for(;i < NDIR;i++) {
        while(isspace(*ptr)) ptr++;
        if(*ptr == '\0') break;
      
        dir_list[i] = ptr;
        while(*ptr != '\0' && !isspace(*ptr)) ptr++;
        if(*ptr != '\0') *ptr++ = '\0';
    }
    ns_dir = i;
    dir_list[i] = NULL;			/* dimen is NDIM + 1 so this is safe */
}



/*********************************************************************/
/*
 * extracts basename + ext from full pathname. Expects and returns C strings
 *
 * Because we have to be able to handle foreign filenames, assume that the
 * directory can be in VMS, unix, or dos format; that is, the directory
 * part extends to the last ], \, or / in the full path.
 */
static char *
filename(str)
char *str;
{
   char *backslash = strrchr(str,'\\');
   char *slash = strrchr(str,'/');
   char *rbracket = strrchr(str,']');

   if(backslash != NULL) backslash++;	/* skip the character itself */
   if(slash != NULL) slash++;
   if(rbracket != NULL) rbracket++;

   if(backslash == NULL) {
      if(slash == NULL) {
	 return(rbracket == NULL ? str : rbracket);
      } else if(rbracket == NULL) {
	 return(slash);
      } else {
	 return(slash > rbracket ? slash : rbracket);
      }
   } else if(slash == NULL) {
      if(rbracket == NULL) {
	 return(backslash);
      } else {
	 return(backslash > rbracket ? backslash : rbracket);
      }
   } else {
      return(rbracket == NULL ? str : rbracket);
   }
}

/*********************************************************************/
/*
 * extracts basename (NO ext)from full pathname. 
 * returns pointer to FORTH string
 */
#if 0 
char *
fbasename(fstr)
char *fstr;				/* input is also forth string */
{
    static char basestring[LEN];
    char *str;

    strcpy(basestring+1,filename(fstr + 1));
    str = strchr(basestring+1,'.');	/* look for ext separator */
    if(str != NULL) *str = '\0';	/* kill it if it is there */
    *basestring = strlen(basestring + 1);
    return(basestring);
}
#endif

/* this was broken--it did NOT remove the extension, which broke reload */
char *
fbasename(fstr)  /* extracts basename from full pathname. returns pointer
                to FORTH string WHY????? */
char *fstr;  /* input is also forth string */

{
    static char basestring[40];
    char *cp,*cp1,*cp2;
            
    cp = fstr + 1;
               
    cp1 = cp;
    if((cp = strrchr(cp1,']'))  != 0) cp1 = cp+1;   /* look for dir sep ']'*/
    if((cp = strrchr(cp1,'/'))  != 0) cp1 = cp+1;   /* look for last '/' */
    if((cp = strrchr(cp1,'\\')) != 0) cp1 = cp+1;   /* or last '\' */
    /* Ok, cp1 is pointing to first char of base string */
    strcpy(basestring+1,cp1);
    cp2 = strrchr(basestring+1,'.'); 
    if(cp2 != (char *)NULL) *cp2 = '\0';
    *basestring = strlen(basestring+1);     /* count byte */
    return( basestring);
}
                                            
                                                                            
char *
fbasext(fstr)    /* extracts basename+ext from full pathname. returns pointer
                    to FORTH string. */
    char *fstr;  /* input is also forth string */
{
    static char basestring[40];
    char *cp,*cp1;
                                    
    cp = fstr + 1;
    cp1 = cp;
    if((cp = strrchr(cp1,']'))  != 0) cp1 = cp+1;   /* look for dir sep ']'*/
    if((cp = strrchr(cp1,'/'))  != 0) cp1 = cp+1;   /* look for last '/' */
    if((cp = strrchr(cp1,'\\')) != 0) cp1 = cp+1;   /* or last '\' */
    /* Ok, cp1 is pointing to first char of base string */
    strcpy(basestring+1,cp1);
    *basestring = strlen(basestring+1);     /* count byte */
    return( basestring);
}



char *          /* same but c strings */
basename(str)
char *str;
{
    static char dirstring[72];
    
    strcpy(dirstring+1,str);
    *dirstring = strlen(str);
    return (fbasename(dirstring) + 1);
}

/* added jeg9605  We *need* to clean up some of these static strings */

char *          /* same but c strings */
basext(str)
char *str;
{
        static char dirstring[72];
        strcpy(dirstring+1,str);
        *dirstring = strlen(str);
        return (fbasext(dirstring) + 1);
}
                
/************************** MCFOPEN(), MCFOPENB(), APPENDM() *****************/
/*
 * Return the full path name of a file in one of the directories in the
 * search list
 */
char *
mcfname(name)
char *name;
{
   static char fullname[LEN + 40];
   char **dptr;
   for(dptr = dir_list;*dptr != NULL;dptr++) {
      sprintf(fullname,"%s%s",*dptr,name);
      if(access(fullname,0) == 0) {
	 return(fullname);
      }
   }
   return(NULL);
}

/*
 * opens textfile fname for reading in current dir if exists; if not in
 * home, parent, and then Mirella dir; np is a pointer to a pointer to
 * the name actually opened; if np is zero, nothing is returned there.
 * fname is a cstring
 */
FILE *
mcfopen(name,np)
char *name;
char **np;
{
   FILE *file;
   char *fname;
   
   if((fname = mcfname(name)) == NULL || (file = fopena(fname,"r")) == NULL){
      error("\nCan't open ");
      error(name);
      error(" in any of current, home, or dirpath directory");
      V_NR_OUT = 1;     /* ????? */
      return(NULL);
   }
   if(np != NULL) *np = fname;
   return(file);
}

/*
 * opens fname for binary reading in current dir if exist; if not, in home,
 * parent, and then Mirella dir; in Unix systems, these fns are identical. 
 * fname is a cstring
 */
FILE *
mcfopenb(fname,np)
char *fname;
char **np;
{
   FILE *file;
   
   if((fname = mcfname(fname)) == NULL || (file = fopenb(fname,"r")) == NULL) {
      error("\nCan't open ");
      error(fname);
      error(" in any of current, initial, or dirpath directory");
      V_NR_OUT = 1;     /* ????? */
      return(NULL);
   }
   if(np != NULL) *np = fname;
   return(file);
}

/* 1208: this was broken..filenames with '.' not in an extension
 * such as ../blat.m did not work (need strrchr, and look only at
 * basename
 */
 
/* this is STILL broken; even if look explicitly for '.m' it breaks.
 * problem is that we need to open things like dotokens.h-- anything
 * else??? Apparently not, See how this works........
 */
 
char *
appendm(name) /* if filename has no extension, appends .m (for load) */
char *name;
{
   char *baseptr();
   char *pp, *bp ;
   static char namebuf[80];
    
   bp = baseptr(name);

   pp = strrchr(bp,'.');  
   if(pp == (char *)NULL || (*(pp+1) != 'm' && *(pp+1) != 'h')){ 
    /* ext is not '.m' or '.h' or no ext */
/*  if(!pp){  */               /* if no '.' in BASENAME, add a .m */
      strcpy(namebuf,name);
      strcat(namebuf,".m");
      return(namebuf);
   } else {
      return(name);
   }
}

/** FILENAME UTILS: BASEPTR(), HASDIR(), EXTPTR(), HASEXT() *******/
/*
 * rets pointer to basename in the filename s
 */
char *
baseptr(s)
char *s;
{
    char *p, *po;
    
    po = strrchr(s,':');
    if((p = strrchr(s,'/')) != NULL && (po == NULL || p > po))  po = p;
    if((p = strrchr(s,'\\')) != NULL && (po == NULL || p > po)) po = p;
    if((p = strrchr(s,']')) != NULL && (po == NULL || p > po))  po = p;
    return(po == NULL ? s : po + 1);
}

int hasdir(s)  /* rets -1 if s is a filename with a prepended dir */
char *s;
{
    return( baseptr(s) == s ? 0 : -1 );
}

/* returns pointer to static directory string *prefix* for 
 * filename s; ptr to empty string if none.
 */
 
char *
dirname(s)  
char *s;
{
    char *cp;
    static char fname[128];
    strncpy(fname,s,127);
    cp = baseptr(fname);
    *cp = '\0';  /* terminates at beginning of base, so rets dir prefix */
    return fname;
}                        

char *  
extptr(s)   /* rets pointer to 1st char of extension if s contains a '.' 
                    followed by a char after directory separators if any; 
                    rets 0 if file has no extension */
char *s;
{
    char *p, *p1;
    int ret;
    
    if((p1 = strrchr(s,']')) != NULL) s = p1;  /* vms, pointer at dir sep.*/
    if((p1 = strrchr(s,'/')) != NULL) s = p1;  /* unix, pointer at last dir sep.*/
    /* if a DOS filename has a '.', it MUST be the extension separator */
    ret = (((p = strrchr(s,'.')) != 0) && !isspace(*(p+1))) ;
    return (ret ? p+1 : 0 );
}

int
hasext(s)
char *s;
{
    return(extptr(s) ? -1 : 0 );
}


void
/*
 * checks fname for extension ext. if no extension, appends ext; if otherwise,
 * asks user for guidance. returns fixed- up name in filename
 */
extchk(fname,ext,filename)
char *fname;
char *ext;
char *filename;
{
    char buf[LEN], *cp;
    int len;
    
    strncpy(buf,fname,35);
    buf[35] = '\0';
    cp = extptr(buf);
    len = strlen(buf);
    if(!cp){
        strcat(buf,".");
        strcat(buf,ext);
    }else{
        if(strcmp(cp,ext)){
            mprintf("\nWrong extension:%s, should be %s. Change it ? ",
                cp,ext);
            if(_yesno()) strcpy(cp,ext);
            else erret((char *)NULL);
        }
    }
    if(len<2) erret("\nMirella filenames must be at least 2 characters long");
    strcpy(filename,buf);
}

/******************* CONSOLE I/O ROUTINES *************************/

/*********************** EMIT() ***********************************/
/* new (03/88) version with internal buffering. MUCH faster */

static char linebuf[257];
static char *lbp=linebuf;
static char *lbend = linebuf + 255;

#define putbuf(c)   *lbp++ = (c); if(lbp>lbend) flushit() 

void
emit(c)
int c;
{
   if(V_SILENTOUT == 0){
      if(c == '\n' || c == '\r') {
	 V_NR_OUT = 0;
	 V_NR_LINE++;
	 putbuf(c);
	 flushit();
      } else { 
	 V_NR_OUT++;
	 putbuf(c);
      }
   }
}

/************************* FLUSHIT() **************************************/

void
flushit()  
{
    *lbp = '\0';
    (*scrnprt)(linebuf);   /* write to screen */
    wscrbuf(linebuf);       /* write to screen buffer */
    if(out_file){
        fputs(linebuf,out_file);   /* write to log file */
        fflush(out_file);
    }
    lbp = linebuf;    

    edit_outfun(linebuf);		/* do any other output things currently
					   installed */
}

/************************ STDPRT() **************************************/
/* NB!!!!!!
 * this does not handle output from res_scr properly for wide terminals
 * ?????
 */

int 
stdprt(str)
register char *str;
{
    register int c;
    int ret;
    char *str0 = str;
    
    if(!grafmode){
       while((c = *str++) != '\0') putchar(c);
       ret = str - str0 - 1;
       fflush(stdout);
    }else ret = 0;
    return ret;
}

/************************ STDPUT() **************************************/
/* there are various flushing issues under dosinWindows which have been
 * hacked in the current Windoze version of Mirella; we leave them out
 * here. Basically if you are in graphics mode and do a flush on stdout
 * nothing happens, so if this is the case you need to set a flag and
 * do the flush when you return to textmode. */
 
int 
stdput(c)
int c;
{
    if(!grafmode){
        return(putchar(c));
    }else return 0;
}

/************************ STDFLSH() ********************************/
int 
stdflsh()
{
    int ret;
    ret = fflush(stdout);
    return ret;
}

/************************ _LOGFILE() ********************************/
/*
 * Mirella procedure for setting logging file or device, mname logfile
 */
void
_logfile()
{
    char *name = (char *)(*xsp_4th++); /* addr of cname on stack */

    if(out_file){
        flushit();
        /*scrprintf("\nClosing logfile");*/ /* debug */
        fclose(out_file);
    }
    out_file = 0;
    logfname[0] = '\0';
    if(name[0] == '\0') return ;
    if((out_file = fopena(name,"a")) == NULL) {
        mprintf("Cannot open %s",name);
        return ;
    }
    strcpy(logfname,name);
    /*scrprintf("\n%s open, pointer=%u\n",logfname,out_file); flushit();*/ /* debug*/
}

/* this was NUTS; there is no way to set logfiles in C, so we do this for now; 
 * this should just be minstalled as a function. 
 * remove from mirella.m ?? check functionality. For now change the mname
 * of this function to clogfile
 */
 
/* AND should be idempotent??? I think it effectively IS ... just closes
 * and reopens
 */

void
logfile(name)
char *name;
{

    if(out_file){
        flushit();
        /*scrprintf("\nClosing logfile");*/ /* debug */
        fclose(out_file);
    }
    out_file = 0;
    logfname[0] = '\0';
    if(name[0] == '\0') return ;
    if((out_file = fopena(name,"a")) == NULL) {
        mprintf("Cannot open %s",name);
        return ;
    }
    strcpy(logfname,name);
    /*scrprintf("\n%s open, pointer=%u\n",logfname,out_file); flushit();*/ /* debug*/
}

/* closes the logfile */
void logoff()
{
    logfile("");
}


/**************** DOT_PUSH(),_POP(),FDT_PUSH(),_POP() ******************/
/* There are two little circular stacks which save the last quantities which 
have been output via . or f. (or associated words), to eliminate a major
annoyance I have with Forth. Bear with me. */

void dot_push(x)
normal x;
{
    dot_stk[(dot_sp++)&7] = x;
}

/*
 * Mirella procedure to get it back , mname '-.'
 */
void
dot_pop()
{
    *(--xsp_4th) = dot_stk[(--dot_sp)&7];
}

void
fdt_push(x)
double x;
{
    fdot_stk[(fdot_sp++)&7] = x;
}

/*
 * Mirella procedure to get it back, mname '-f.'
 */
void
fdt_pop()
{
    *(--xfsp_4th) = fdot_stk[(--fdot_sp)&7];
}

/*************** UDOT() , DOT(), and PRINT-STRING() ***********************/
static void 
rawudot(u)
unsigned normal u;
{
    register char *p = &nmbr_buf[40];
    register unsigned normal a = u;

    *--p = ' ';     /* gratuitous space at the end */

    do {
    *--p = digits[ a % V_BASE ];
    } while ((a /= V_BASE) != 0 );

    --p;
    *p = &nmbr_buf[40] - p - 1;

    print_string(p);
}

/* jeg 060403 */
static void 
rawudotr(u, len)
unsigned normal u;
normal len;
{
    register char *p = &nmbr_buf[40];
    register unsigned normal a = u;
    normal n;

    /* *--p = ' '; */    /* gratuitous space at the end  removed 121226  */

    do {
    *--p = digits[ a % V_BASE ];
    } while ((a /= V_BASE) != 0 );


    n = len - (&nmbr_buf[40] - p );
    while (n-- > 0) *--p = ' ';
    --p;
    *p = &nmbr_buf[40] - p - 1;    

    print_string(p);
}

/* jeg 130130 */
static void 
rawndotr(i, len)
normal i;
normal len;
{
    register char *p = &nmbr_buf[40];
    register int a = i;
    normal n;
    normal sgn = a < 0 ? 1 : 0 ;
    
    if(a < 0) a = -a;

    do {
    *--p = digits[ a % V_BASE ];
    } while ((a /= V_BASE) != 0 );

    if (sgn) *--p = '-';

    n = len - (&nmbr_buf[40] - p );
    
    while (n-- > 0) *--p = ' ';
    --p;
    *p = &nmbr_buf[40] - p - 1;    

    print_string(p);
}

void
udot(u)
unsigned normal u;
{
    dot_push(u);
    rawudot(u);
}

void
udotr(u,len)
unsigned normal u;
normal len;
{
    dot_push(u);
    rawudotr(u,len);
}

void ndot(n)
    normal n;
{
    dot_push(n);
    if ( n < 0 ) {
    emit('-');
    n = -n;
    }
    rawudot(n);
}

/* .r as implemented in util.m is broken */
/* Fixed?? 130130 (wrote rawndotr() ) */
void ndotr(n,len)
    normal n;
    normal len;
{
    dot_push(n);
    rawndotr(n, len);
}
       

/*
 * prints forth strings
 */
void
print_string(str)
char *str;
{
    int count = *str++;
    while (count--)
    emit(*str++);
    flushit();
}

/*************************** DFDOTR(), FDOT() ************************/
void
dfdotr(flt,dec,width)
double flt;
int dec,width;
{
    fdt_push(flt);
    cprint(fltstr(flt,width,dec));
}

void
fdot(flt)
double flt;
{
    char buf[80];

    fdt_push(flt);
    if(flt < 1. && flt >-1.) sprintf(buf,"%8.4e ",flt);
    else sprintf(buf,"%6.5g ",flt);
    cprint(buf);
}

/**************** CPRINT(), ERROR(), TPUTS(), MGETS() ***********************/
/*
 * prints c strings through Mirella i/o; rets #
 */ 
int
cprint(str)
char *str;
{
   register char *p;
   register int ch;
   
   p = str;
   if( p == (char *)NULL) return 0 ;
   if(up_4th == NULL) {
      while((ch = *p++) != '\0') putchar(ch);
   } else {
      while((ch = *p++) != '\0') emit(ch);
      flushit();
   }
   return (p - str);
}

/*
 * mirella procedure
 */
void
m_cprint()
{
    char *str = (char *)(*xsp_4th++);
    cprint(str);
}

/*
 * prints c strings with '\n' through M i/o, use like puts
 */
int
tputs(str)
char *str;  
{
    register char *p;
    register int ch;

    p = str;    
    while((ch = *p++) != '\0') emit ( ch );
    emit('\n');
    flushit();
    return 1;  /* success */
}

void
error(str)
char *str;
{
    flushit();
    scrprintf(str);
}

/* like gets(), but through all of Mirella i/o */
char *
m_gets(str) 
char *str;
{
   expect(str,1000);
   emit('\n');
   return str;
}

/* like m_gets, but no entry in log file */
char *
mrgets(str)             
char *str;
{
    rexpect(str,1000);
    (*scrnput)('\n');
    return str;
}

/* jeg0112 */
/********************* SCRPUT(), SCRPUTS() ***************************/
/* 
 * these write to the screen only, despite claims to the contrary
 * in the header in this file. FIXME?? 
 */
void scrput(c)
int c;
{
    (*scrnput)(c);
}

void scrputs(s)
char *s;
{
    (*scrnprt)(s);
}

/********************** MPRINTF(), SCRPRINTF() *****************************/
/* mprintf() works like printf() and goes through the Mirella i/o system;
   scrprintf() goes ONLY to the screen and to the screen buffer */

#ifdef STDARGS
/*
 * this is the ansi standard way of doing things
 */
#include <stdarg.h>

int 
mprintf(char *fmt, ...)
{
   va_list ap;

   va_start(ap,fmt);
   vsprintf(out_string,fmt,ap);
   va_end(ap);
   return cprint(out_string);
}

int 
scrprintf(char *fmt, ...)
{
    va_list ap;
    int ret;
   
    va_start(ap,fmt);
    vsprintf(out_string,fmt,ap);
    va_end(ap);
    ret = (*scrnprt)(out_string);
    wscrbuf(out_string);
    edit_outfun(out_string);		/* we need this  here since we don't
					   go thru 'emit'*/
    (*scrnflsh)();
    return ret;
}

#endif
#ifdef VARARGS
/*
 * This is the old way of doing things `properly' -- please use stdargs
 * if you can
 */
#include <varargs.h>

int 
mprintf(va_alist)
va_dcl
{
    va_list ap;
    char *fmt;
   
    va_start(ap);
    fmt = va_arg(ap,char*);
    vsprintf(out_string,fmt,ap);
    va_end(ap);
    return cprint(out_string);
}


int 
scrprintf(va_alist)
va_dcl
{
    va_list ap;
    char *fmt;
    int ret;
   
    va_start(ap);
    fmt = va_arg(ap,char*);
    vsprintf(out_string,fmt,ap);
    va_end(ap);
    ret = (*scrnprt)(out_string);
    wscrbuf(out_string);
    (*scrnflsh)();
    return ret;
}
#endif

#if !defined(STDARGS) && !defined(VARARGS)

/* old version for systems without varargs facility or vsprintf;
assumes arguments are stacked contiguously on stack, in the direction SDIR */

#define SDIR +
/* direction of stack growth; one or the other of + or - will work
for most machines unless the compiler is clever enough to pass the first
few arguments of function calls in registers, or has 24-bit pointers, or
something else weird. */

int 
mprintf(str,args)   /* this version is nice if it works, but portable it
		       is emphatically not */
char *str;                        
normal args;
{
    normal *aptr = (&args);
  
    sprintf(out_string,str,
        *(aptr       ),
        *(aptr SDIR 1),
        *(aptr SDIR 2),
        *(aptr SDIR 3),
        *(aptr SDIR 4),
        *(aptr SDIR 5),
        *(aptr SDIR 6),
        *(aptr SDIR 7),
        *(aptr SDIR 8),
        *(aptr SDIR 9),
        *(aptr SDIR 10),
        *(aptr SDIR 11),
        *(aptr SDIR 12),
        *(aptr SDIR 13),
        *(aptr SDIR 14),
        *(aptr SDIR 15),
        *(aptr SDIR 16));
    /* hokey,hokey, hokey --at most 16 normal-sized arguments, but it
    should not be impossible to figure out how to add more. */
    {
       int ret = (cprint(out_string));	/* goes through 'emit()' and flushes */
       clearn(OUT_STR_SIZE,out_string);
       return ret;
    }
}

int
scrprintf(str,args)   /* this version is nice if it works, but portable it
                        is emphatically not */
char *str;
normal args;
{
    normal *aptr = (&args);
    int ret;

    sprintf(out_string,str,
        *(aptr       ),
        *(aptr SDIR 1),
        *(aptr SDIR 2),
        *(aptr SDIR 3),
        *(aptr SDIR 4),
        *(aptr SDIR 5),
        *(aptr SDIR 6),
        *(aptr SDIR 7),
        *(aptr SDIR 8),
        *(aptr SDIR 9),
        *(aptr SDIR 10),
        *(aptr SDIR 11),
        *(aptr SDIR 12),
        *(aptr SDIR 13),
        *(aptr SDIR 14),
        *(aptr SDIR 15),
        *(aptr SDIR 16));
    /* hokey,hokey, hokey --at most 16 normal-sized arguments, but it
    should not be impossible to figure out how to add more. */

    {
       int ret = (*scrnprt)(out_string); /* goes only to screen */
       wscrbuf(out_string);
       edit_outfun(out_string);
       (*scrnflsh)();
       clearn(OUT_STR_SIZE,out_string);
       return ret;
    }
}

#endif

/*************************** EXPECT() ********************************/

#define EXPSAV 01        /* set if wish to save string in buffer and log */
#define EXPMOD 02        /* set if wish to modify existing string */
#define ELINELEN 252

static int
gexpect(addr,count,flg)			/* count must include terminating nul*/
u_char *addr;
normal count;
normal flg;				/* mode flag; see above */
{
   char line[ELINELEN+2];

   if(count>ELINELEN) count = ELINELEN;
#if MIREDIT
   if((flg&EXPMOD) == 0 || V_NOECHO){
      clearn(ELINELEN,line);
   } else {
      strncpy(line,(char *)addr,ELINELEN);	/* mod existing line */
   }
   if(V_NOECHO){
      int c;
      char *lp = line;
      
      while(((c = key()) != '\n') && (c != '\r')){
	 *lp++ = c;
      }
      *lp = '\0';
   } else {
      edit_line(line,0);
   }
#else
   clearn(ELINELEN,line);		/* always clear if no editor */
   fgets(line,ELINELEN,stdin);
#endif

   strncpy((char *)addr,line,count);
   addr[count - 1] = '\0';
   count = strlen((char *)addr);

   if((flg & EXPSAV) != 0) {
      wscrbuf((char *)addr);			/* write in screen buffer */
      if(out_file) fputs((char *)addr,out_file); /* write in log */
   }
   return count;
}

int
expect(addr,count)
char *addr;
normal count;
{
   return gexpect(addr,count,EXPSAV);
}

/*
 * raw--no saving in screen buffer or log file
 */
int
rexpect(addr,count)
char *addr;
normal count;
{
   return gexpect(addr,count,0);
}
 
/*
 * modifies (allows edit of) existing string
 */
int
mexpect(addr,count)
char *addr;
normal count;
{
   return gexpect(addr,count,EXPSAV|EXPMOD);
}

/*************************** CEXPECT() *******************************/
static char endifstr[] = "#endif";
static char elsestr[] = "#else";
static char hashifstr[] = "#if";

/* For use with command line input only. Returns actual count */

#define LCOMM 254 /* max length of command line */

int
cexpect(addr, count)
char *addr;
normal count;
{
    int c;
    char *p, *cp;
    int len;
    char chkstr[LCOMM];
    void prompt();
    
    if(input_file == stdin) {
       if(use_editor && MIREDIT) {
	  len = edit_hist(addr,1);
	  /* copy command line for log and screen buffer */
	  p = addr;
	  cp = chkstr;
	  while((c = *p++) != '\n' && c) {
	     *cp++ = c;
	  }
	  *cp++ = ' ';   /* replace newline with blank for log and scrbuffer*/
	  *cp++ = '\0';  /* terminate */
	  wscrbuf(chkstr);
	  if(out_file){   
	     fputs(chkstr,out_file);
	  }
       } else {
	  prompt();
	  fgets(addr,count,stdin);
	  len = strlen(addr);
	  if(out_file) fputs(addr,out_file);   /* write in log */
	  wscrbuf(addr);                       /* write in screen buffer */
       }
       return(len);
    } else {				/* reading from file */
        if(V_NOCOMPILE==0){    /* comp_ignore flag off; normal compilation */
            for (p = addr; count > 0; count--) {
               c = key();
               *p++ = c;
/* This violates the standard which says that the newline is not stored */
/* but storing the newline makes backslash SO much easier */
               if ( c == '\n')
                   break;
            }
            *p = '\0';
            return ( p - addr);
        }else{        /* comp_ignore flag on..check for #endif or #else */
            chkstr[0] = '\0';
            /*FIXME: this is a possible problem with edited input if anyone
                ever uses the #symbols in typed input--fgets(...stdin) is
                a nono */
            if(input_file != NULL && input_file != stdin) {
	       fgets(chkstr,LCOMM,input_file);
	    } else{ 
                expect(chkstr,LCOMM); emit('\n');
            }
            c = chkstr[0];
            if(c == '#'){   /* first char is '#' */
                for(count = 0; count < 7; count++){
                    c = chkstr[count];
                    if(isspace(c) || c == '\n' || c == '\r'){
                        chkstr[count] = '\0';
                        break;
                    }
                }   
                if(!strcmp(chkstr,endifstr)){
                    if(V_CCNEST == V_CCNOFF) V_NOCOMPILE = 0;   
                        /* turn on compilation if this endif is the one which
                           belongs to the #if or #else that turned it off */
                    V_CCNEST-- ;    /* decrement the level */
                }else if(!strcmp(chkstr,elsestr)){
                    if(V_CCNEST == V_CCNOFF) V_NOCOMPILE = 0; /* as above */
                }else if(!strcmp(chkstr,hashifstr)){
                    V_CCNEST++;    /* increment the level */
                }                    
                /* note that this code never turns OFF compilation, so never
                sets ccnoff; this can only be done by the outer interpreter
                in the code in mirella.m */
#ifdef CCDEBUG
                flushit(); 
                scrprintf("\ngot :%s: nocompile= %d, ccnest = %d, ccnoff = %d",
                    chkstr,V_NOCOMPILE, V_CCNEST, V_CCNOFF);
                flushit();
#endif
            }
            p = addr;
            *p++ = '\n';   /* return a blank line in any case */
            *p = '\0';
            return (p - addr);
        }
    }
}

/******************** LOADING ROUTINES() ******************************/
/* These routines are an abomination and should be replaced by a newly
 * thought-out set which are SIMPLE.
 */

/************************* LOAD() ************************************/
/*
 * filename is a forth string
 */
void
load(filename)
char *filename;
{
   char fname[80];
   FILE *file;
   char *fn;
   
   strcpy(&fname[1],appendm(&filename[1]));
   *fname = strlen(&fname[1]);
   if((file = mcfopen(&fname[1],&fn)) != NULL){   
      load_push(fname);			/* push current environment onto load
					   stack and create dictionary marker*/
      V_LN_IN = 0;			/* line counter */
      tibbuf_4th[V_TO_IN] = '\n';	/* terminate current copy of input
					   to force loading; load_push has
					   saved input buffer and pointer */
      tibbuf_4th[V_TO_IN + 1] = '\0' ;
      V_SPAN = V_NR_TIB = V_TO_IN + 1;
      
      s_i_f_name(fn);
      if(verbose){
	 error("\nFile: ");
	 error(in_fl_name + 1);
	 error(" ");
	 flushit();
      }
      input_file = file;
   }
}

/************************* LOAD_POP **********************************/
/*
 * pops top of load stack to active input status  and removes marker
 * placed by load_push if no definitions have been added
 */
static void 
load_pop()
{
    char bname[LEN];
    normal *scr;
    normal wscr;

#ifdef LDEBUG
    flushit();
    scrprintf("\npopping load stack: tls = %d current = %s, last = %s", 
        tls,lstk[tls].infname +1,in_fl_name+1);
#endif
    fstrcpy( in_fl_name, lstk[tls].infname);
    input_file = lstk[tls].infptr;
    V_LN_IN = lstk[tls].llptr;    
    if(tls == 1 && oclload == 1){
        clload = 1;    /* if at bottom of stack and bottom file was command-
                        line loaded, proceed with command-line loading */
    }else{             /* normal loading; restore input buffer and pointer */
        strncpy(tibbuf_4th,lstk[tls].in_line,132);
        V_TO_IN = lstk[tls].inptr;
        V_SPAN = V_NR_TIB = lstk[tls].inbufsize;
        if(V_TO_IN < V_NR_TIB){   /* still some unread input */
            V_SPAN = -1;          /* flag to read more from current line;
                                    see the outer interpreter. Kludge */
        }
    }
    /* leave or remove dictionary marker ??? */
    if( dp_4th == lstk[tls].herafter && 
        (vocab_t *)V_CURRENT == lstk[tls].scurrent ){ 
    /*loaded file neither added definitions nor changed def'n vocabularies;
            remove marker */
        /* restore vocab entry */
        *(hash( (vocab_t *)V_CURRENT,
            ((char *)(lstk[tls].herbefore) + sizeof(normal) + 1) )) 
                /* ptr to name of marker */
            = lstk[tls].sbefore;      
        /* restore dp */
        dp_4th = lstk[tls].herbefore; 
        /* restore file-link */
        V_LASTFILE = lstk[tls].olfile;
    }else{
    /* new defn's. leave marker and place new dp in 2nd parameter field of 
        marker to mark end of this file's contribution to dictionary */
        scr = (lstk[tls].herbefore + 1);  /* nfa of marker -1 (precedence) */
        scr = (normal *)name_from((char *)scr + 1); /* cfa of marker */
        scr += 2;                       /* 2nd pfa of marker */
        *scr = (normal)dp_4th;          /* link across all defn's from 
                                               just-loaded file */
    }
    /* end marker code */
    tls--;
    if(tls < 0) {
        error("\nLoad stack empty");
        tls = 0;
        norm_input();			/* cleans up */
        erret((char *)NULL);		/* and returns to outer interpreter */
    }
    if(input_file == stdin){
        if(tls != 0){
            mprintf("\nLoad stack error, tls = %d",tls);
        }
    }else{  /* this file is being loaded from another: write continuation */
        if(verbose){
            error("\nFile: ");
            error(in_fl_name + 1);
            error(" (cont) ");
            flushit();
        }

        /* subsidiary marker code: check if defs added or vocab
         * changed since primary marker in mother file; if not do nothing; 
         * if so place subs. marker with code just like that in load_push 
         * below, but with filename with addnl blank at end and char count 
         * incremented 
         */
        
        if( dp_4th != lstk[tls].herafter || 
                    (vocab_t *)V_CURRENT != lstk[tls].scurrent ){     
                /* last module loaded from this one did something, so original
                marker is obscured. make a new one. */
            strcpy(bname + 1,filename(in_fl_name + 1));
            *bname = strlen(bname + 1);
            (*bname)++; bname[(int)(*bname)]= ' ';
            lstk[tls].scurrent = (vocab_t *)V_CURRENT;
            lstk[tls].sbefore = *(hash((vocab_t *)V_CURRENT,bname));
                /* this vocab entry, and only this one, will be changed by
                the creation of the marker */
            lstk[tls].herbefore = dp_4th;
            wscr = V_WARNING; V_WARNING = 0;  /* turn off dupl def warnings */
            qt_create(bname,DOCON);
            V_WARNING = wscr;
            scr = dp_4th;
            *dp_4th++ = lstk[tls].olfile = V_LASTFILE;
            V_LASTFILE = (normal)scr;
            *dp_4th++ = (normal)((normal *)scr + 2);  /* lookahead tentatively
                                                    placed just after marker */
            lstk[tls].herafter = dp_4th;
        }  
        /* end subs. marker code */
    }
#ifdef LDEBUG
    scrprintf("\nLeaving load_pop()   file:%d,%s; span=%d, #tib=%d, >in=%d",
        input_file,in_fl_name+1,V_SPAN,V_NR_TIB,V_TO_IN);
    if(V_SPAN == -1) scrprintf("\nRemnant buffer:%s",tibbuf_4th + V_TO_IN);
#endif
}

/************************* LOAD_PUSH() ********************************/
/*
 * pushes current environment onto load stack. subsequently load() or
 * op_nxt_f() sets up new input filename and pointer. need filename
 * (new file) only to make dictionary marker entry.
 */
static void 
load_push(fname)
char *fname;  /* forth string */
{
    char bname[68];
    int scr;
    normal *scr2;
    
    tls++;
    if(tls == LSTACKSIZE){
        error("Load stack overflow\n");
        norm_input();			/* cleans up */
        erret((char *)NULL);		/* and returns to outer interpreter */
    }
    if(strlen(fname+1) > 64){
        error(&(fname[1]));
        error(" :filename too long\n");
        norm_input();
        erret((char *)NULL);
    } 
#ifdef LDEBUG
    flushit();
    scrprintf("\nPushing load stack: tls= %d, file  %s;ptr,stdin= %d %d",
        tls,&in_fl_name[1],input_file,stdin);
    scrprintf("\n#tib,>in = %d %d",V_NR_TIB,V_TO_IN);
#endif
    /* save filename, pointer, line count, tibbuf, pointer */
    fstrcpy(lstk[tls].infname,in_fl_name);
    lstk[tls].infptr = input_file;
    lstk[tls].llptr = V_LN_IN;
    lstk[tls].inptr = V_TO_IN;            
    strncpy(lstk[tls].in_line,tibbuf_4th,V_NR_TIB);  
    lstk[tls].in_line[V_NR_TIB] = '\0';
    lstk[tls].inbufsize = V_NR_TIB;
    /* set flag to indicate command-line loading of bottom file */
    if((tls == 1) && (clload == 1)){
        oclload = 1;
    }
    clload = 0;
    /* now place dictionary marker */
    strcpy(bname + 1,filename(fname + 1));
    if(strrchr(bname + 1,'.') == NULL) {
       (void)strcat(bname + 1,".m");
    }
    *bname = strlen(bname + 1);

    /*printf("\n %d %s\n",*bname, bname+1); This is OK */

    lstk[tls].scurrent = (vocab_t *)V_CURRENT;
    lstk[tls].sbefore = *(hash((vocab_t *)V_CURRENT,bname));
        /* this vocab entry, and only this one, will be changed by
        the creation of the marker */
    lstk[tls].herbefore = dp_4th;
    qt_create(bname,DOCON);
    scr = (normal)dp_4th;
    *dp_4th++ = lstk[tls].olfile = V_LASTFILE;
    /* if this file is being loaded from another file, must delimit its
    load range with herebefore. do so */
    if((lstk[tls-1].infptr) != stdin ){  /* this is a nested load; connect the
            file thread with the last marker entry */ 
        scr2 = ((normal *)V_LASTFILE + 1);
        *scr2 = (normal)(lstk[tls].herbefore);
    }
    V_LASTFILE = scr;
    *dp_4th++ = (normal)((normal *)scr + 2); /* tentatively place lookahead
                                                just after marker */
    lstk[tls].herafter = dp_4th;
    /* end marker code */
}

/*********************** SEMI_S() *************************************/
void semi_s()       /* graceful exit from  loading; it seems to work */
{
    if(input_file == stdin) return;  /* jeg9707 -- semi_s crashes the OI */
    fclose(input_file);
    input_file = NULL;
    if(!clload){
        load_pop();
    }else{
        load_pop();  /* tryit */
        input_file = op_nxt_f();
    }
}

/*********************** NORM_INPUT() *********************************/
void 
norm_input()     /* reestablishes keyboard input from load errors */
{
    if(input_file != stdin){
        if(*in_fl_name != 0){
            scrprintf("\nClosing %s: error line %d",in_fl_name + 1,
                V_LN_IN);
	    if(strcmp(&in_fl_name[1],COMMAND_LINE) != 0) 
	      if(input_file != NULL) {
		 fclose(input_file);
		 input_file = NULL;
	      }
        }
        for(;tls>0;tls--){   /* tls goes from 1 up; load_push incr tls first
                                thing */
            if(*lstk[tls].infname != 0){ /*close the files on the load stack*/
                scrprintf("\nClosing %s, tls = %d",lstk[tls].infname +1,tls);
                fclose(lstk[tls].infptr);
            }
        }
    }
    nullname(in_fl_name);
    input_file = stdin;
    clload = 0;
    oclload = 0;
    tls = 0;
    V_LN_IN = 0;
    erret((char *)NULL);	/* the outer interpreter sets INTERPRETING */
}


/********************** KEY(), MKEY_AVAIL() ******************************/

normal
key()
{
    register int c;

    do {
        if ( input_file == STRINGINPUT ) {
            if( (c = *strptr++) != '\0')
                return(c);
        } else {                                    /* normal input*/
            if((input_file != stdin && input_file != NULL) || !MIREDIT) {
	       /* from file or no editor*/
	       c = getc(input_file);
            }
#if MIREDIT
            else{                                   /* from kbd with editor */
                get1char(1);    /* set raw mode */
                c = readc();    /* in the editor */
                get1char(EOF);  /* unset raw mode */
                return (c);
            }
#endif /* MIREDIT */
            if(c != EOF ) return(c);
        }
        /* have char and it IS an EOF; first close input file */

        if (input_file != stdin){
	    if(input_file != NULL && input_file != STRINGINPUT) {
	       fclose(input_file);
	       input_file = NULL;
	    }	       
            if(V_NOCOMPILE){       /* ignore-compile flag left on; turn it
                                        off and complain */
                V_NOCOMPILE = 0;
                mprintf("Missing #endif in %s\n",in_fl_name+1);
            }
        }

        /* now need to return a newline; this termination causes normal 
        * processing of the input line by cexpect() and thence by the 
        * outer interpreter; if the span=-1 flag has been set, (load_pop())
        * the NEXT pass thru the outer interpreter picks up the 
        * remnant input buffer. */

        load_pop();
        if(clload) input_file=op_nxt_f();   /* command-line loading */
        if(input_file) return('\n');  /* EOF should act like newline 
                                        to cexpect */

    } while (input_file != NULL);
    putchar('\n');
    exit(0);
    /*NOTREACHED*/
}

int mkey_avail()
{
    if(input_file != NULL && input_file != stdin) return (key_avail()) ;
    else return ( 0 );
}
    
/******* S_I_F_NAME() (SAVE_INPUT_FILE_NAME()), NULLNAME(), FSTRCPY() ******/
static void
s_i_f_name( str )
char *str;
{
    strcpy ( &in_fl_name[1], str );
    in_fl_name[0] = strlen(str);
}

static void 
nullname(fstr)    /* nulls both the forth count and c name */
char *fstr;
{ 
    *fstr = *(fstr+1) = '\0';
}

static void 
fstrcpy(tar,src)  /* copies forth str correctly even for null strings */
char *tar,*src;
{
    strcpy(tar+1,src+1);
    *tar = *src;
}

/*************************** OP_NXT_F() *******************************/
static FILE *
op_nxt_f()
{
    FILE *file;
    char fname[60];
    char *fn;
    int c, c1;
    int exitflg=0;

    while (--gargc > 0) {
        if ( (*++gargv)[0]=='-' || *gargv[0]=='+' ) {  /* try this to foil
                                                        * xterm -e behavior
                                                        */
            switch(c = (*gargv)[1]){
            case '\0':
            case '&' :    
            case 'C' :
                /* terminating - returns control to stdin; vms objects
                    to terminating -, so use -& there */
                input_file = stdin;
                clload = oclload = 0;
                norm_input();   /* clean up and jump to outer interpreter;
                                   control never returns here */
                erret((char *)NULL);
            /*
             * the flags -e and -s accept string input to the end of the 
             * command line; the acceptance is ALMOST general, the exceptions 
             * being that no colon definitions may be made, no words (like 
             * 'ascii'), which temporarily change the input state, may be used,
             * and that no words called '-', '+'. '-Q', '-C' or '-&' may be 
             * executed as the LAST tokens in the string (but '"' strings
             * MAY be used through a special case in the interp_tokarr()
             * routine). The terminating flags are interpreted as the usual 
             * control flags.
             */
                                                
            case 'e' :
                /* string input; exits at end */                
                exitflg = 1;
                /* fall through */
            case 's' : 
                /* string input; control returned to outer interp at end */
                ++gargv;
                if( --gargc>0){
                    if(gargv[gargc-1][0] == '-' && 
                      ((c1 = gargv[gargc-1][1]) == 'C'|| c1 == '\0'|| c1 == '&'
                      || c1 == 'Q')){  /* is command string terminated ?? */
                      if(c1 == 'Q'){
                        exitflg = 1;
                      }else exitflg = 0;
                      gargc--;  /* remove terminator token */
                    }
                    interp_tokarr(gargc,gargv); /* interpret all remaining
                                                    tokens as forth words */

                    if(exitflg) (void)cinterp("bye");
                    input_file = stdin;
                    clload = oclload = 0;
                    norm_input();   /* clean up and jump to outer interpreter;
                                       control never returns here */
                    erret((char *)NULL);
                }
                break;
            case 'Q':
                cinterp("bye");
            default:
                error("\nUnknown flag ");
                error(*gargv);
                break;
            }
        }else{
            /* *gargv is a c-string filename; open and take input */
            if((file = mcfopen(appendm(*gargv),&fn)) != NULL) {
                /* new code 7/28/87 */                
                input_file = stdin;
                nullname(in_fl_name);
                V_TO_IN = V_NR_TIB = V_SPAN = 0;
                strcpy(fname+1,*gargv); 
                *fname = strlen(*gargv);  /* fname is a forth string */
                load_push(fname);
                /* end */
                V_LN_IN = 0;
                s_i_f_name(fn);
                if (verbose) {
                    error("\nFile: ");
                    error(*gargv);
                    error(" ");
                }
                clload = 1;     /* flag for command-line loading */
                return(file);
            }
        }
    }
    return(NULL);
}

/************************* PROMPT() ***********************************/
/*
 * used in editor; does not check for output and always prints prompt string;
 * returns length of prompt string actually printed
 */
int
eprompt()
{
    int ret;

    if( MIREDIT){
        wscrbuf("\n");
        if(out_file) putc('\n',out_file);
        edit_outfun("\n");
    }
    if (V_STATE == (normal)INTERPRETING){
        cinterp("prompt");
        print_string(pstring);
        if(*pstring != '\0') emit(' ');
        flushit();
        ret = pstring[0] + 1;
    }else{
        cprint(" ] ");
        flushit();
        ret = 3;
    }
    V_NR_OUT = 0;
    return ret;
}

void 
prompt()  /* prints prompt only if input is stdin; \n if output 
                    on current line */
{
    if ( *in_fl_name == '\0' ) {
        if (V_NR_OUT != 0)
            emit( '\n' );
        eprompt();
    }
}


#if 1  /* we no longer use any of this; should remove it */

/******************* SAVE_DIC(), GET_DIC() ********************/
/* see comment below on preamble */
#define PRESIZE 256  

void
save_dic(file)
char *file;
{
    int fdes;
    normal bufsize = DBLKSIZE;  /* buffer size defined in mirella.h*/
    normal nbuf;
#ifdef DICDEBUG
    normal nbuf,fs;
#endif
    int ivl, ivle;
    normal *vlp;
    normal preamble[PRESIZE];  /* this is big enough for about 25 vocabularies
                                in the clinks section for eight threads; 
                                should suffice. */
    char * cp;
    normal i;
    normal fsize ;
#if 0
    normal presiz,usrsiz,appsiz; */   /* sizes of file req for the
                                            4 writes to the dictionary */
#endif
    if((cp = strrchr(file,'.')) != NULL && !strncmp(cp+1,"dic",3)) ;
    else{
        error(file);
        erret(": illegal dictionary name");
    }
/*  mprintf("\nSaving dictionary in %s",file); */
    fsize = (dp_4th - dictb_4th + 1) * sizeof(normal);
    nbuf = (fsize -1)/bufsize + 1;
    fsize = nbuf*bufsize;          /* make filesize an integral # of blocks*/
#if 0
    presiz = ((PRESIZE*sizeof(normal) -1)/bufsize + 1)*bufsize;
    usrsiz = (((USER_AREA+FILE_AREA+MEM_AREA)*sizeof(normal) -1)/bufsize + 1)
            *bufsize;    
    appsiz = ((APP_AREA*sizeof(normal) -1)/bufsize + 1)*bufsize;
#endif    
    /* create file in block mode */
    if((fdes=creatblk(file,presiz+fsize+usrsiz+appsiz)) == -1){ 
        scrprintf("\nI can't open %s to write a dictionary; sorry",file);
        return;
    }
    /* clear and then stuff preamble array */
    for(i=0;i<PRESIZE;i++) preamble[i] = 0;
    preamble[0] = nbuf;
    preamble[1] = bufsize;
    preamble[2] = (normal)dp_4th;
    preamble[3] = (normal)up_4th;
    preamble[4] = (normal)origin;
    preamble[5] = (normal)dictb_4th;
    preamble[6] = (normal)ap_env;
    preamble[7] = (normal)m_lastlink;
    bufcpy((char *)(&(preamble[8])),(char *)m_lastlink + 5,
        MIN(8,*((char *)m_lastlink + 5)+1 ));
    /* copy 1st 8 char of name of lastlink into preamble[8] and [9] */
/*debug scrprintf("\nlastlink=%d, name:%d:%-7s",preamble[7],
            *(char *)(&(preamble[8])),(char *)(&(preamble[8])) + 1); */
    /* 10-19 are up for grabs */
    /* now follow the voc-link chain to find the first vocabulary
    before the break */
    vlp = (normal *)V_VOC_LINK;
    while(vlp > dictb_4th) vlp = *(normal **)vlp;  
    /* vlp is pointer to first voc in clink area; now gather entries
    and place in preamble. We do not really need to save the voc-links */
    preamble[20] = (normal)vlp;   
    ivl = 21;
    do{
        for(ivle = ivl+NTHREADS; ivl <= ivle; ivl++){  /* NTHREAD threads 
                                                            + voc-link */
            preamble[ivl] = *((normal *)vlp - ivle + ivl); 
        }
        vlp = *(normal **)vlp;        
    }while(vlp != 0);  /* terminates on forth, for which the voc-link = 0 */

#ifdef DICDEBUG
    scrprintf("\nPreamble:");
    for(i=0;i<ivl;i++){
        if(!(i%5)) scrprintf("\n");
        scrprintf("%11d ",preamble[i]);
    }
#endif
   
    writeblk(fdes,(char *)preamble,PRESIZE*sizeof(normal));
    writeblk(fdes,(char *)up_4th,
	     (USER_AREA+FILE_AREA+MEM_AREA)*sizeof(normal));
    writeblk(fdes,(char *)dictb_4th,fsize);
    
#ifdef DICDEBUG
    fs = tellblk(fdes);
    scrprintf("writing %d bytes from %s, location %d,  from %x \n",
        APP_AREA*sizeof(normal),file,fs,ap_env);
#endif
    writeblk(fdes,(char *)ap_env,APP_AREA*sizeof(normal));
    closeblk(fdes);
}

/***********************************************************/
/*
 * gets the dictionary; if flg is 0, gets it all; if 1, just gets preamble,
 * and the next call with any flg and filename gets the rest; the
 * preamble-only call returns the user origin, the full call 0
 */
static normal *
gget_dic(file,flg)
char *file;
int flg;
{
    char *fname;			/* full name of directory */
    normal nbuf, bufsize;
    int ivl, ivle;
    normal *vlp;
    static normal preamble[PRESIZE];
    static int fdes = -1;
    char *cp;
    normal fsize;
   
    if(fdes >= 0) {			/* file is open and preamble has been
					   read; skip file stuff and finish */
       ;
    } else {
       if((cp = strrchr(file,'.')) != NULL && !strncmp(cp+1,"dic",3)){
	  if(verbose) {
	     sprintf(out_string,"\nReading dictionary from %s",file) ;
	     cprint(out_string);
	  }
       } else{
	  scrprintf("\nIllegal dictionary name: %s",file);
	  exit(1);
       }
       if((fdes = openblk(file,O_RDONLY)) == -1){
	  if(hasdir(file)){   /* you specified directory; quit if not found*/
	     scrprintf("\nCannot open %s",file);
	     exit(1);
	  } else if((fname = mcfname(file)) == NULL ||
				(fdes = openblk(fname,O_RDONLY)) == -1) {
	     scrprintf("\nCannot open %s in this or Mirella directory",file);
	     exit(1);
	  }
       }
       readblk(fdes,(Void *)preamble,PRESIZE*sizeof(normal));
       if(flg){   
	  /* only read preamble; leave file open for finishing later */
	  return((normal *)preamble[3]);
       }
    }

    if(preamble[3] != (normal)up_4th){  
        scrprintf(
            "\nInvalid dictionary image; Origin has moved: new=%d, old=%d\n",
                    up_4th,preamble[3]);
        scrprintf("Please recompile dictionary\n");
        exit(1);
    }
    if(preamble[5] != (normal)dictb_4th){  
        scrprintf(
            "\nInvalid dictionary image; Break has moved: new=%d, old=%d\n",
                     dictb_4th,preamble[5]);
        scrprintf("Please recompile dictionary\n");
        exit(1);
    }
    nbuf    = preamble[0];
    bufsize = preamble[1];
    dp_4th  = (normal *)preamble[2];
    up_4th  = (normal *)preamble[3];
    origin  = (normal *)preamble[4];
    dictb_4th = (normal *)preamble[5];
    ap_env = (normal *)preamble[6];
    /* check to see whether m_lastlink is OK */
    if(m_lastlink < (normal *)(preamble[7])  ||
       strncmp((char *)(&(preamble[8])),(char *)(preamble[7]) + 5,
            MIN( (*(char *)(&(preamble[8])) + 1) , 8))  ) {
        scrprintf(
"\nDictionary image found is incompatible with Clinks. Please recompile dict.");
        fflush(stdout);
        exit(1);
    }
    fsize = nbuf*bufsize;

    /* now replace vocabulary entries in clink area */
    vlp = (normal *)preamble[20]; 
    ivl = 21;
    do{
        for(ivle = ivl+NTHREADS; ivl <= ivle; ivl++){
            *((normal *)vlp - ivle + ivl) = preamble[ivl];
        }
        vlp = *(normal **)vlp;
    }while( vlp != 0);
   
    readblk(fdes,(Void *)up_4th,(USER_AREA+FILE_AREA+MEM_AREA)*sizeof(normal));
    readblk(fdes,(Void *)dictb_4th,fsize);

    readblk(fdes,(Void *)ap_env,APP_AREA*sizeof(normal));
    closeblk(fdes);
    fdes = -1;
    return(NULL);
}

void
get_dic(filename)
char *filename;
{
    gget_dic(filename,0);
}

static int gotpreamble = 0;

normal *
get_pdic(filename)
char *filename;
{
    normal *ret;
    ret = gget_dic(filename,1);
    gotpreamble = 1;
    return ret;
}

void 
get_rdic()
{
    if(gotpreamble){
        gget_dic((char *)NULL,0);
        gotpreamble = 0;
    }
    else erret("\nGET_RDIC: Called with no previous call to get_preamble");
}

#endif  /* 0, all get/save dic functions */

/************************ _YESNO() ***********************************/

int _yesno()
{
    char c;
    int ret;
    void flushit();
    
    flushit();
    get1char(1);   /* initialize raw mode */
    do{
        c = get1char(0);
        if(c == 'y'|| c == 'Y'){
            emit('y');
            ret = 1;
            break;
        }
        if(c == 'n'|| c == 'N'){
            emit('n');
            ret = 0;
            break;
        }
        if(c == 3  || c == 4  ){     /* escape */
            emit('^'); emit('C');
            interrupt = 1;
            ret = 0;
            break;
        }
        else scrprintf("\nPlease say y(ae) or n(ae). OK? :");
    }while(c != 'y' && c != 'n' && c != 3 && c != 4);
    get1char(EOF);
    flushit();
    return ret;
}

/********************** KWAIT() **************************************/
#define  push(xx)   (*(--xsp_4th)) = (normal)( ( xx ) )

/* key() does not pause when reading from scripts, since it gets its
next char from the stream. kwait() simply pushes get1char(). */

void kwait()
{
    push(get1char(0));
}

/*********************** VWAIT() *****************************************/
/* waits for char from keyboard if NOT reading from a file; this is
 * for situations in which one wishes interaction selectively; does
 * not disturb the stack.
 */

void vwait()
{
    if(*in_fl_name == 0){
        (void)get1char(0);
    }
}

/*********************** LWAIT() *****************************************/
/* waits for char from keyboard if NOT reading from a file AND no logfile
 * is open; this is for situations in which one wishes to pause to look
 * at output as it scrolls by, but is not so interested if logging anyhow. 
 * does not disturb the stack
 */

void lwait()
{
    if(*in_fl_name == '\0' && *logfname == '\0'){
        (void)get1char(0);
    }
}

/*********************** M_BYE() *************************************/

void m_bye(exit_code)   /* exit routine */
int exit_code;
{
   int i;
    
   get1char(EOF);
   reset_kbd();
#if 0
   num_keypd();			/* reset keypad */            
#endif
   gmode(0);				/* get out of graphics mode if in it */
   if(chdir(home_dir) != 0) {
      if(home_dir[i = strlen(home_dir) - 1] == '/') {
         home_dir[i] = '\0';
      }
      if(chdir(home_dir) != 0) {
         error("\nCannot change to initial directory: ");
         error(home_dir);
      }
   }
   write_hist();
   mprintf("\nAddio  \n");
   exit(exit_code);
}

/***************************** BELLS() **********************************/

int 
bells(n)
int n;
{
    int i;
    for(i=0;i<n;i++){
        if(dopause()) return 1;
        emit(7);
        flushit();
    }
    return 0;
}

void 
ebells(n)
int n;
{
    bells(n);
    erret((char *)NULL);
}

/************************* MFCOPY() ******************************************/
/* this is a general file copy routine which just goes to the OS to do
 * the work, except for VMS, which I no longer remember why we had to do
 * this. It is borrowed from minstall, but the name is changed (fcopy)
 */

void
mfcopy(fsrc,fdest)
char *fsrc;
char *fdest;
{
    char command[80];

    if(access(fsrc,0) == -1) {		/* can't open file */
       return;
    }
#ifdef DSI
    strcpy(command,"copy ");
    strcat(command,fsrc);
    strcat(command," ");
    strcat(command,fdest);
    system(command);
#endif
#ifdef unix
    strcpy(command,"cp ");
    strcat(command,fsrc);
    strcat(command," ");
    strcat(command,fdest);
    system(command);
#endif
#ifdef vms
    FILE *ifp,*ofp;
    char *buf;
    int len;
    ifp = tryopen(fsrc,"r");
    ofp = tryopen(fdest,"w");
    fseek(ifp,0L,2);    
    len = ftell(ifp);
    fseek(ifp,0L,0);
    buf = (char *)malloc(len + len/2);   /* careful !! */
    if(!buf){ 
        fprintf(stderr,"\nCannot allocate copy buffer");
        exit(1);
    }
    fread(buf,1,len,ifp);
    fwrite(buf,1,len,ofp);
    free(buf);
    fclose(ifp);
    fclose(ofp);
#endif
}

/********************** END, MIRELLIO.C *************************************/
