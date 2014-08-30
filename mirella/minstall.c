/*VERSION 90/05/06, Mirella 5.50                                        */

/************************* MINSTALL.C ***********************************/
/* this awful standalone program installs the C interface to Mirella */

/* recent history:
 * 88/08/06: Installed conditional installation.
 * 88/08/30: fixed logic error which caused crash in VMS (only!) if first 
 *  command line defined a variable. Reworked conditional installation.
 * 89/01/14: Made end message a bit more informative; perhaps (?) fixed
 *  trouble with end-line comments causing random hangs
 * 89/02/12: Got rid of some of the paranoid double-backing up
 * 89/02/24: Fixed bug in variable code (poffset) which caused name
 *  difference warnings when there were none; cleaned up end reporting.   
 * 90/05/06: extended code to produce 1st, 2nd, and 3rd level secondary
 *  files
 * 26th June 1990 (rhl) added -k, -q, -T, and -u options, and \Expect.
 * 4th July 1990 (Rebel Day) (rhl) Cleaned up code; Removed interactive
 * mode and *7.x files; renamed *0.x file *.x; removed backups
 * 13th July 1990 (Fri) (rhl) Replaced conditional code by real implementation
 */

#ifdef VMS
#include mirella
#include mircintf
#else
#include "mirella.h"
#include "mircintf.h"
#endif

#define NFRAG 10			/* number of fragments */
#define SIZE 80				/* length of filenames */
#define N_LINE 5			/* max. number of continued lines */

static int argc;
static char *argv[20];
static int exit_status = 0;		/* exit from main */
static FILE *cfile;			/* source file, mirel[ie123]c.c  */
static FILE *elfp;			/* pointer to error log file */
static char *bxp[NFRAG];		/* pointers to orig of images of
					   x frags in memory */
static char *fxp[NFRAG];		/* pointers to start of fragment text*/
static char *xp[NFRAG];			/* pointers to current loc in x frag */
static int flen[NFRAG];			/* lengths of fragments */
static int falloc[NFRAG] = {
   4000, 4000, 64000, 32000, 32000, 64000, 64000, 4000, 64000, 4000
  };					/* allocated amounts for fragments;
					   enlarge if neccessary. What the
					   hell are you DOING ? */
static int maxifun = 0;			/* how many functions? */
static int maxiv = 0;			/* how many variables? */
static char savbuf[N_LINE*128];
static char iec;			/* i, e, 1, 2, or 3 */
static char line[128];   /* buffer for comments and control lines
			    Comments are lines beginning with '\'; they must
			    preceed relevant line, or be appended to the end
			    of lines; everything following ANY backslash which
			    is preceded by whitespace is ignored */
static int commentflg = 0;		/* flag for presence of comment string;
					   0 if none, 1 if standalone comment,
					   2 if end-of-line comment */
static char sname[40];  /* name of structure or pointer */

static char *tindex();
static int read_line();
static void mparse(), usage(), takeitapart();
static void addbuf(),addbuf6(),showline(),fcopy();
static int strip_c_comments();
static int isstruct(),bfwrite();
static void print_warnings();
static int iflg= 0;			/* flag for internal, ext, or
					   secondary assignment */
static void killold();
char *parse_cpp_line();			/* process a CPP (`#') line */
static void process_input_file();
static void putback();
static void read_include();
static FILE *tryopen();

static int skipflg = 0;			/* flag set by false #if; ignores
					   input lines until matching #endif */
static int ctrlflg = 0;			/* flag 1 if control line */
static int nestflg = 0;			/* nesting level of #if's */
static char *usr_include = "";		/* include directory for <...> */
/*
 * Variables for warnings and such
 */
static int expect_cm = -1;		/* number of C-M name diffs */
static int expect_no_size = -1;		/* number on non-dimensioned arrays */
static int expect_typeless = -1;	/* number of typeless variables */
static int nwarn;			/* number of nonfatal errors */
static int nnamdif;			/* number of C & Mirella name diffs */
static int nnone;			/* number of vars of type 'none' */
static int narsiz;			/* number of omissions of array sizes*/
static int nproc = 0;			/* number of procedure installations */
static int nfun  = 0;			/* number of proper func installs */
static int nvbl  = 0;			/* number of variable installs */
static int quiet = 0;			/* if true, suppress some output */

/************************ MAIN() ***************************************/
/*
 * Syntax: minstall [-ei123fkq] [files]
 *
 * The default interface is `external', but this can be modified with
 * the -e, -i, -1, -2, or -3 flags. Additionally, -i (`internal') is
 * assumed if the file is "intern.msc".
 *
 * -Asym=val #alias a symbol
 * -Dsym[=val] #define a symbol
 * -h print a usage message (the same as -u)
 * -k is equivalent to a `k' line in a file; kill all previous definitions.
 * -o file specifies an output filename (default: mirel[ei123]c.c)
 *    The corresponding .x file will be the -o file with a .x suffix
 * -q suppresses some output
 * -s skel specifies a skeleton file to use (default: mirel[ei]c.skl)
 * -u print a usage message (the same as -h)
 * -v (verbose) report un-#define'd symbols.
 * -w enable warnings about undefined symbols
 *
 * Comment Lines of the form
 *	\Expect (C != M) == expr
 *      \Expect typeless == expr
 *      \Expect no_size == expr
 * can be used to tell minstall how many of the warnings
 *	C & M name differences
 *	typeless vbl entries
 *	missing array sizes
 * to expect. If you guess right, the warnings will be suppressed
 * (expr can be any expression allowed by #if)
 */
int
main(largc,largv)
int largc;
char *largv[];
{
   void alias();
   char cfbak[SIZE];			/* backup file */
   char *cp;
   void define();
   int c;
   int eval_string();
   int i;
   static char iestr[] = "ie123";                                
   int is_defined();
   int kill_flag = 0;			/* was there a -k option? */
   char *name;				/* name of a #define */
   char xbuf[160];			/* buffer for 'writing' to fragments */
/*
 * file skeletons
 */
   static char cfname[SIZE] = "mirel?c.c";  /* default output file;
					    char 5 is [ie123] */
   static char skel_file[SIZE] = "mirelec.skl"; /* name of skeleton */
   int have_c_file = 0;			/* was an output filename given? */

/************************************************************/
/*
 * Start of executable code, system characteristics for #ifs;
 * see mirella.h, msysdef.h
 */
   alias("G_DISPLAY",G_DISPLAY);
   alias("IM_DISPLAY",IM_DISPLAY);
   alias("M_OPSYS",M_OPSYS);
   alias("M_CPU",M_CPU);
   alias("M_CCOMP",M_CCOMP);
/*
 * Read command-line args
 */
   iflg = 0;				/* external interface, default */
   
   largv++;
   largc--;				/* point to first arg. (if any) */
   while(largc > 0 && (*largv)[0] == '-') {    /* flag */
      for(i = 1;(c = (*largv)[i]) != '\0';i++) {
	 switch(c) {
	  case 'i':
	    iflg = -1;
	    break;
	  case 'A':
	    if(i != 1) {
	       fprintf(stderr,"-Aname=value must be a complete argument\n");
	       break;
	    }
	    cp = name = *largv + 2;
	    while(*cp != '\0' && !isspace(*cp) && *cp != '=') cp++;
	    if(*cp != '\0') {
	       *cp++ = '\0';
	    }
	    if(*cp == '\0') {
	       fprintf(stderr,"You must provide a name to alias %s to\n",name);
	    } else {
	       alias(name,cp);
	    }

	    for(i = 0;(*largv)[i] != '\0';i++) ; /* exit the for loop */
	    i--;			/* will be ++'d */
	    break;
	  case 'D':
	    if(i != 1) {
	       fprintf(stderr,"-Dname[=value] must be a complete argument\n");
	       break;
	    }
	    cp = name = *largv + 2;
	    while(*cp != '\0' && !isspace(*cp) && *cp != '=') cp++;
	    if(*cp != '\0') {
	       *cp++ = '\0';
	    }
	    if(*cp == '\0') {
	       define(name,1);
	    } else {
	       define(name,atoi(cp));
	    }

	    for(i = 0;(*largv)[i] != '\0';i++) ; /* exit the for loop */
	    i--;			/* will be ++'d */
	    break;
	  case 'e':
	    iflg = 0;
	    break;
	  case 'I':			/* choose name of include directory */
	    if(largv[0][++i] == '\0') {
	       fprintf(stderr,"You must specify a directory with -I\n");
	    } else {
	       usr_include = &largv[0][2];
	       for(i = 0;(*largv)[i] != '\0';i++) ; /* exit the for loop */
	       i--;			/* will be ++'d */
	    }
	    break;
	  case '1':
	  case '2':
	  case '3':
	    iflg = c - '0';
	    break;
	  case 'h':			/* print usage */
	  case 'u':
	    usage();
	    break;
	  case 'k':			/* equivalent to `k' in a script */
	    kill_flag = 1;
	    break;
	  case 'o':			/* output C file */
	    if(largc == 1) {
	       fprintf(stderr,"You must specify a file name with the -o option\n");
	    } else {
	       have_c_file = 1;
	       largc--;
	       largv++;
	       strncpy(cfname,*largv,SIZE);
	       for(i = 0;(*largv)[i] != '\0';i++) ; /* exit the for loop */
	       i--;			/* will be ++'d */
	    }
	    break;
	  case 'q':			/* be quiet */
	    quiet = 1;
	    break;
	  case 's':			/* skeleton file */
	    if(largc == 1) {
	       fprintf(stderr,"You must specify a skeleton file with the -s option\n");
	    } else {
	       largc--;
	       largv++;
	       strncpy(skel_file,*largv,SIZE);
	       for(i = 0;(*largv)[i] != '\0';i++) ; /* exit the for loop */
	       i--;			/* will be ++'d */
	    }
	    break;
	  case 'w':			/* warnings about undefined symbols */
	    define("__verbose",1);
	    break;
	  default:
	    fprintf(stderr,"Illegal flag %s\n",*largv);
	    break;
	 }
      }
      largc--;
      largv++;
   }
   if(largc > 0) {			/* must be input filename */
      if(!strcmp(*largv,"intern.msc")){  /* "intern.msc" is reserved for
                                            internal interface */
	 iflg = -1;
      }
   }
   
   iec = iestr[iflg+1];  /* i or e or 1 or 2 or 3 identifier for (i)nternal or
                            (e)xternal files or levels of secondary files */  
   
   /* OK. now open c file and take it apart */
   if(!have_c_file) {
      cfname[5] = iec;
   }
   
   cfile = tryopen(skel_file,"r",1);   /* binary because of DSI ftell trouble*/

   takeitapart();
   elfp = tryopen("minstall.log","w",0);	/* open Minstall error log */
/*
 * all files are open. Now start reading input
 */
   if(kill_flag) killold();
   
   addbuf(0,"/*\n * Input files:\n");
   for(i = 0;i < largc;i++) {
      sprintf(xbuf," *   %s\n",largv[i]);
      addbuf(0,xbuf);
   }
   addbuf(0," */\n");

   while(largc-- > 0) {			/* loop through input files */
      nwarn = nnamdif = nnone = narsiz = 0;
      process_input_file(*largv);
      print_warnings(*largv++);
   }
  
   fclose(cfile);
   fclose(elfp);
/*
 * and dimension frag
 */
   sprintf(xbuf,"#define MAXIFUN  %3d  \n",maxifun);
   addbuf(1,xbuf);
   sprintf(xbuf,"#define MAXIV    %3d \n",maxiv); 
   addbuf(1,xbuf);
/*
 * and put it all back together
 */
   strcpy(cfbak,cfname);			/* rename input file */
   if((cp = strrchr(cfbak,'.')) != NULL) {
      *cp = '\0';
   }
   strcat(cfbak,".bak");
   fcopy(cfname,cfbak);
   
   putback(cfname);			/* write output file */
   
   if(!quiet) {
      printf("\nThis session:  %d variables  %d procedures  %d functions",
	     nvbl,nproc,nfun);
      printf("\nTotal numbers: %d variables  %d procedures + functions",
	     maxiv,maxifun);
      printf("\nREMEMBER TO RECOMPILE %s !!!\n",cfname);
   }
   return(exit_status);
}

/************************************************************/

static void
process_input_file(file)
char *file;
{
   int as1=0,as2=0;			/* size of array,
					   nrow, ncol for matrices */
   char base[40],*elem;			/* base and element of a struct,
					   i.e. base->elem */
   int c;
   int c_code_flg = 0;			/* are we in a section of C code? */
   char clin[20][100];  
   static int depth = 0;		/* depth of nesting */
   int dynamic;				/* is function loaded dynamically? */
   char *fmt;				/* the format for structure offsets */
   FILE *ifp;
   char inbuf[N_LINE*128];
   char infile[64];
   int ino = 0;
   int i;
   int linecount;			/* current line number */
   int delim, input, output, endin;
   int dont_declare;			/* don't declare extern functions */
   int ignore_out;			/* ignore a return value */
   int nestflg0 = nestflg;		/* initial value of nestflg */
   char otype;				/* type of function */
   int poffset;				/* fudge for + or * prefix for cname */
   int swc;				/* `SWitch Char' */
   char type[50];
   char *types;				/* pointer to type string */
   char xbuf[160];			/* buffer for 'writing' to fragments */
/*
 * Flags for types of object
 */
   int procflg;				/* flag for "procedure"--ie, function
					   which does own stack handling */
   int pflg;				/* pointer, declared variable */
   int ptr_type;			/* object's type is `pointer to ...' */
   int no_decl;				/* don't declare this object */
   int aflg;				/* array, declared variable */
   int maflg;				/* flag for Mirella array, above
					   but not string */
   int mmatflg;				/* Mirella matrix; array of pointers */
   int cflg;				/* C constant */
   int cfflg;				/* float C constant */
   int csflg;				/* C string */
   int ctype;				/* constant or variable type */
   int sflg;				/* var or const struct member */
/*
 * code fragments
 */
   static char fargs[] = "\n    float farg%d = fpop;  ";
   static char iargs[] = "\n    normal iarg%d = pop;   ";
   static char pargs[] = "\n    Void *parg%d = cppop; ";
   static char sargs[] = "\n    char *sarg%d = cspop; ";
/*
 * n_?arg == next arg to use in stack manipulations
 */
   int nfarg;
   int niarg;
   int nparg;
   int nsarg;

   if(depth++ > 10) {
      fprintf(stderr,"Include file nesting depth is 10 -- probable loop\n");
      exit(1);
   }
   
   strcpy(infile,file);
   if(!strrchr(infile,'.')) {	/* if no extension, add ".msc" */
      strcat(infile,".msc");
   }
   if((ifp = tryopen(infile,"r",0)) == NULL) {
      depth--;
      return;
   }
   linecount = 0;
   
   if(!quiet) printf("\nReading from %s\n",infile);
   
   while((i = read_line(inbuf,ifp)) > 0) {
      linecount += i;
      if(inbuf[0] == '\0') continue;
      
      mparse(inbuf);   /* break inbuf into words, set argc, argv; mparse()
			  also finds comments and sets flags thereto */
      
      if(!skipflg && commentflg == 1 && argc == 1) {
	 char temp[70];
	 
	 if(sscanf(line,"\\Expect (C != M) == %s",temp) == 1) {
	    expect_cm = eval_string(temp);
	 }
	 if(sscanf(line,"\\Expect typeless == %s",temp) == 1) {
	    expect_typeless = eval_string(temp);
	 }
	 if(sscanf(line,"\\Expect no_size == %s",temp) == 1) {
	    expect_no_size = eval_string(temp);
	 }
	 continue ;
      }
/*
 * if whole line is a comment, skip all processing;
 */
      if(ctrlflg || skipflg) continue;   /* if line is a control line or if
					    previous control line has set
					    skipflg, skip all processing */
      if(*savbuf == '{') {
	 if(c_code_flg) {
	    fprintf(stderr,"You cannot nest sections of C code\n");
	    showline(infile,linecount);
	 }
	 c_code_flg = 1;
	 continue;
      } else if(*savbuf == '}') {
	 if(!c_code_flg) {
	    fprintf(stderr,"You are not in a section of C code\n");
	    showline(infile,linecount);
	 }
	 c_code_flg = 0;
	 continue;
      }
      if(c_code_flg) {			/* copy line to fragment 0 */
	 for(i = 0;isspace(savbuf[i]);i++) continue;
	 addbuf(0,&savbuf[i]);
	 continue;
      }

      cflg = cfflg = csflg = dynamic = ptr_type = no_decl =
	maflg = mmatflg = procflg = 0;
/*
 * see whether there is a second letter in command, and if so, if it's i or e
 * This construction is obsolete, but we still check for old script files
 * Alternatively, it could be '@' for dynamic loading
 */
      if(strlen(argv[1]) == 2) {
	 if(((c = argv[1][1]) == 'e' || c == 'i')) {
	    if(c != iec) {
	       fprintf(stderr,"Illegal or inconsistent (ext/int) specification\n");
	       showline(infile,linecount);
	       exit(1);
	    }
	    argv[1][1] = '\0';
	 } else if(argv[1][1] == '@') {
#ifndef DLD
	    fprintf(stderr,"\nDynamic loading is not supported");
	    goto error;
#else
	    dynamic = 1;
#endif
	 }
      }
/*
 * now interpret input line
 */
      switch (swc = *argv[1]) {
       case 'k':			/* kill all entries */
	 killold();
	 break;
       case 'q':			/* quit */
	 fclose(ifp);
	 depth--;
	 return;
       case 'V':			/* new vocabulary */
	 if(argc < 2){
	    fprintf(stderr,"Vocabulary entries must have a name\n");
	    goto error;
	 }
	 addbuf6(argv[2],CIF_VOCAB,0,0,0,NULL);
	 break;
       case 'F':			/* return to entry vocabulary */
	 addbuf6(" ",CIF_FORTH,0,0,0,NULL);
	 break;
       case 's':
	 csflg = 1;
	 /* fall through */
       case 'c':
	 if(*argv[1] == 'c') cflg = 1;
	 /* fall through */
       case 'v':			/* variable; cname prefix
					   `*' if pointer, `+' if array */
	 if(argc < 5) {			/* c constant or vbl. Must have type */
	    fprintf(stderr,"missing type for constant or variable\n");
	    goto error;
	 }
	 if(dynamic) {
	    fprintf(stderr,
	   "Sorry, only functions and procedures may be dynamically loaded\n");
	    goto error;
	 }
	 pflg = (*argv[3] == '*');	/* cname has pointer flag */
	 maflg = (*argv[3] == '+' && !csflg); /* 'real' Mirella array */
	 aflg = (maflg || csflg );  /* array or string : declare as array */
	 if(maflg && *(argv[3]+1) == '+') { /* matrix; careful with decl */
	    mmatflg = 1;
	    maflg = 0;
	 }
	 
	 while(argv[4][0] == '*' && !isspace(argv[4][1])) {
	    ptr_type++;
	    argv[4]++;
	 }
	 if((ctype = *argv[4]) == 'f') { /* float; if cconst, so mark */
	    if(cflg && !pflg) {		/* but not if ptr */
	       cfflg = 1;
	       cflg = 0;
	    }
	 } else if(ctype == 'i' || ctype == 'p') {
	    ;
	 } else if(ctype == 'c' || ctype == 's' || ctype == 'u') {
	    if(cflg && !(pflg || ptr_type)) {
	       fprintf(stderr,"Sorry, you can't have `%c' constants.\n",ctype);
	       goto error;
	    }
	 } else if(ctype == 'n'){
	    if(ptr_type) {
	       ;			/* OK */
	    } else {
	       fprintf(elfp,"I will not declare %s; you must\n",
		       argv[3] + poffset);
	    }
	 } else {
	    fprintf(stderr,"%s%s\n",
		   "Illegal type. Must be c(har), u(unsigned char),",
		   " s(hort), i(nt), f(loat), or p(tr)");
	    goto error;
	 }
	 if(aflg && ( cflg || cfflg)){
	    fprintf(stderr,"Arrays cannot be constants.\n");
	    goto error;
	 }
	 if((types = tindex(ctype,ptr_type)) == NULL){
	    fprintf(stderr,"Something rotten:\n");
	    fprintf(stderr," ctype = %c, types:%s tindex(ctype,ptr):%s\n",
		   *argv[4],types,tindex(ctype,ptr_type));
	    goto error;
	 }
	 poffset = 0;
	 if(pflg){
	    poffset = ( *argv[3] == '*' ? 1 : 0 );  /* pointer? */
	 }
	 if(aflg){ 
	    poffset = ( *argv[3] == '+' ? 1 : 0 );  /* array? */
	    poffset = ( *(argv[3] + 1) == '+' ? 2 : poffset ); /* matrx? */
	 }        
	 /* generate warning if Mname is not same as Cname */
	 if(strcmp(argv[2],argv[3]+poffset)){
	    fprintf(elfp,"Mirella name: %s is different from C name: %s ",
		    argv[2],argv[3]+poffset);
	    nwarn++;
	    nnamdif++ ;
	    fprintf(elfp,"\n");
	 }
	 
	 if(isstruct(argv[3]) != 0) {
	    no_decl = 1;
	    fprintf(elfp,
    "C variable is a structure member. You must declare %s in a header file\n",
		    sname);
	 }

	 if(ctype == 'n') {		/* flag not to declare */
	    no_decl = 1;
	 }
	 if(no_decl){
	    nnone++;
	    nwarn++;
	 }
	 /* now check for array size if entry is an array or matrix */
	 if(maflg){
	    if(argc < 6) {
	       fprintf(stderr,"Should supply array size for %s\n",argv[3]);
	       goto error;
	    } else {			/* OK */        
	       as1 = atoi(argv[5]);
	    }
/*	    if(ctype == 'n' && !ptr_type && as1 != 0) {
	       fprintf(stderr,"%s: %s\n",argv[3],
		       "Arrays of type n must be declared with 0 length");
	       goto error;
	    }
	    Actually, struct arrays can be accommodated. See below. -- MKE  */
	 }
	 if(mmatflg){
	    if(argc < 7) {
	       fprintf(stderr,"Should supply nrow and ncol for %s\n",argv[3]);
	       goto error;
	    }else{    /*  OK */
	       as1 = atoi(argv[5]);  
	       as2 = atoi(argv[6]);
	    }
/*	    if(ctype == 'n' && !ptr_type && (as1 != 0 || as2 != 0)) {
	       fprintf(stderr,"%s: %s\n",argv[3],
		       "Matrices of type n must be declared as size 0 0");
	       goto error;
	    }
	    This isn't really the problem. The problem is that Robert hasn't
	    included CIF_STRUCMATRIX / DOSTRUCCMAT in the C interface. */
	 }
	 if(mmatflg &&
	    ctype != 'i' && ctype != 'f' && ctype != 's' && ctype != 'c') {
	    /* Illegal type for matrices */
	    fprintf(stderr,"Illegal type: %s\n",
		   "Matrices must be float, int, short, or char");
	    goto error;
	 }    
	 nvbl++;        
	 maxiv++;   /* got one. bump number */
/*
 * all is OK. Now write new entries
 *
 * write to Mirella initialization file, mirel[ie123]c0.x
 */
	 if(cflg) {			/* C constant; push value on pstack */
	    addbuf6(argv[2],CIF_CONSTANT,0,0,0,NULL);
	 } else if(cfflg) {		/* float c constant;
					   push value on fstack */
	    addbuf6(argv[2],CIF_FCONSTANT,0,0,0,NULL);
	 } else if(csflg) {		/* C string. M pushes address */
	    addbuf6(argv[2],CIF_STRING,0,0,0,NULL);
	 } else if(maflg) {
/*
 * real array. M does array thing; must know type (size) of entry.
 */
	    if(ptr_type || ctype == 'i' || ctype == 'f' || ctype == 'p') {
	       addbuf6(argv[2],CIF_NARRAY,0,as1,0,NULL);
	    } else if(ctype == 's') {
	       addbuf6(argv[2],CIF_SARRAY,0,as1,0,NULL);
	    } else if(ctype == 'c' || ctype == 'u') {
	       addbuf6(argv[2],CIF_CARRAY,0,as1,0,NULL);
	    } else if(ctype == 'n') {
	       char size[40];
	       sprintf(size,"sizeof(%s[0])",&argv[3][1]);
	       addbuf6(argv[2],CIF_STRUCARRAY,0,as1,0,size);
	    }
	 } else if(mmatflg){
	    /* C matrix. M does matrix thing. Must be short or float 
	       or int or char */
	    if(ctype == 'i' || ctype == 'f') {
	       addbuf6(argv[2],CIF_NMATRIX,0,as1,as2,NULL);
	    } else if(ctype == 's') {
	       addbuf6(argv[2],CIF_SMATRIX,0,as1,as2,NULL);
	    } else if(ctype == 'c' || ctype == 'u') {
	       addbuf6(argv[2],CIF_CMATRIX,0,as1,as2,NULL);
	    } else if(ctype == 'n') {
	       fprintf(stderr,
		       "Robert hasn't put in handler for struct matrix %s\n",
		       argv[3]);
	       fprintf(elfp,
		       "Robert hasn't put in handler for struct matrix %s\n",
		      argv[3]);
	       goto error;
	    }
	    types = tindex(ctype,ptr_type);  /* BE CAREFUL. THIS IS A KLUDGE */
	 } else {			/* C variable, push address on stack */
	    addbuf6(argv[2],CIF_VARIABLE,0,0,0,NULL);
	 }
/*
 * now do extern declaration--only non-structure members and non
 * 'n'-type constants and variables. Fragment 3.x
 */
	 if(!no_decl) {
	    if(aflg){
	       char *indir = "";
	       char *after = "";
	       if(!pflg && argv[3][0] == '+') {
		  if(argv[3][1] == '+') {
		     poffset = 2;
		     indir = "**";
		  } else {
		     poffset = 1;
		     after = "[]";
		  }
	       } else {
		  if (csflg){
		     after = "[]";
		  }
		  poffset = 0;
	       }
	       sprintf(xbuf,"extern %s%s%s%s;\n",types,indir,argv[3]+poffset,
		       						       after); 
	       addbuf(3,xbuf);
	    } else if(pflg){
	       sprintf(xbuf,"extern %s*%s  ;\n",types,argv[3]+1);
	       addbuf(3,xbuf);
	    } else{
	       sprintf(xbuf,"extern %s%s ;\n",types,argv[3]); 
	       addbuf(3,xbuf);
	    } 
	 }
/*
 * and initialization; 5.x
 */
	 if(aflg){			/* explicitly declared array or string;
					   Mirella variable returns address of
					   1st element */
	    poffset = ( *argv[3] == '+' ? 1 : 0 );
	    poffset = ( *(argv[3] + 1) == '+' ? 2 : poffset ); /* matrx? */
	    sprintf(xbuf,"    cvarr[%3d] = (char *)(&%s) ;\n",
		    maxiv-1,argv[3]+poffset);
	    addbuf(5,xbuf);
	 } else if(pflg){		/* pointer; Mirella variable returns
					   its address; not its value;
					   Mirella cconstant returns value */
	    sprintf(xbuf,"    cvarr[%3d] = (char *)(&%s) ;\n",
		    maxiv-1,argv[3]+1);
	    addbuf(5,xbuf);
	 } else {			/* Mirella variable or constant.
					   Mirella variable returns address;
					   M cconstant returns value on pstack,
					   M fcconstant on fstack */
	    sprintf(xbuf,"    cvarr[%3d] = (char *)(&%s) ;\n",
		    maxiv-1,argv[3]);
	    addbuf(5,xbuf);
	 }
	 
	 break;
/*
 * end of const, variable, string section
 */
       case 'o':			/* structure offset */
	 if((sflg = isstruct(argv[3])) == 0) {
	    fprintf(stderr,"You must provide a structure reference with `o'\n");
	    goto error;
	 }
	 nvbl++;        
	 maxiv++;
	 
	 strcpy(base,argv[3]);		/* take name apart */
	 for(elem = base;;elem++) {
	    if(*elem == '.') {
	       *elem++ = '\0';
	       break;
	    } else if(*elem == '-') {
	       *elem++ = '\0';
	       *elem++ = '\0';
	       break;
	    }
	 }
	 ctype = (argc >= 5 ? *argv[4] : 'p');

	 if(strcmp(argv[2],elem)){
	    fprintf(elfp,"Mirella name: %s is different from C name: %s ",
		    argv[2],elem);
	    nwarn++;
	    nnamdif++ ;
	    fprintf(elfp,"\n");
	 }

	 addbuf6(argv[2],CIF_OFFSET,0,0,0,NULL);

	 fmt = (ctype == 's' ?
		"     cvarr[%3d] = (char *)(%s - (char *)%c%s);\n" :
		"     cvarr[%3d] = (char *)((char *)&(%s) - (char *)%c%s);\n");
	 sprintf(xbuf,fmt,maxiv-1,argv[3],(sflg == 1 ? ' ' : '&'),base);
	 addbuf(5,xbuf);

	 break;
       case 'f':			/* function */
       case 'p':			/* procedure */
	 procflg = (swc == 'p');
	 delim = 0;
	 dont_declare = ignore_out = 0;

	 for(i = 4;i < argc;i++){	/* is there a * in the arg list? If so,
					   probably return a value */
	    if(*argv[i] == '*' || *argv[i] == '@' || *argv[i] == '%') {
	       char *aptr = argv[i];
	       delim = i;
	       for(;*aptr != '\0';aptr++) {
		  if(*aptr == '@') dont_declare = 1;
		  else if(*aptr == '%') ignore_out = 1;
	       }
	    }
	 }
	 /* are there input arguments ? */
	 input = (argc > 3 && (!delim || delim > 4));
	 endin = argc;
	 if (input) input =4;
	 
	 output = (delim && (argc > delim + 1)); /* are there output values? */
	 if(output) {
	    endin = delim;
	    output = delim + 1;
	    otype = *argv[output];
	 } else {
	    otype = 'v';
	 }

	 if(otype == 'v') {
	    ignore_out = 1;
	 } else if((otype != 'i') && (otype != 'p') && (otype != 'f') 
	    		   && (otype != 's')) {
	    fprintf(stderr,"Illegal return type %c\n", otype);
	    goto error;
	 }
/*
 * generate arguments
 */
	 ino = 0;
	 nfarg = niarg = nparg = nsarg = 0;
	 if(input){
	    for(i=input; i<endin; i++) {
	       switch(c = *argv[i]){
		case 'f':		/* float */
		  type[ino] = 'f';
		  sprintf(clin[ino++],fargs,nfarg++);
		  break;
		case 'i':		/* integer */ 
		  type[ino] = 'i';
		  sprintf(clin[ino++],iargs,niarg++);
		  break;
		case 'p':		/* pointer */
		  type[ino] = 'p';
		  sprintf(clin[ino++],pargs,nparg++);
		  break;
		case 's':		/* string */
		  type[ino] = 's';
		  sprintf(clin[ino++],sargs,nsarg++);
		  break;
		case 'v':		/* void */
		  if(input != endin - 1) {
		     fprintf(stderr,"You can only have one void input!\n");
		     goto error;
		  }
		  break;
		default:
		  fprintf(stderr,"Illegal function argument type :%c\n",c);
		  goto error;
	       }
	    } 
	 }

	 if(strcmp(argv[2],argv[3])){
	    fprintf(elfp,"Mirella name: %s is different from C name: %s ",
		    argv[2],argv[3]);
	    nnamdif++;
	    nwarn++;
	    fprintf(elfp,"\n");
	 }
	 maxifun++ ;
	 if(procflg) nproc++;
	 else nfun++;
/*
 * input is probably OK. write it to Mirella init file
 */
	 addbuf6(argv[2],procflg ? CIF_PROCEDURE : CIF_FUNCTION,
		 dynamic ? CIF_DYNAMIC : 0,0,0,NULL);
/*
 * declare external function
 */
	 if(!dont_declare) {
	    if(dynamic) {
	       sprintf(xbuf,"static void (*p_%s)() = NULL;\n",argv[3]);
	    } else {
	       if(otype == 'p'){ 
		  sprintf(xbuf,"extern Void *%s();\n",argv[3]);
	       } else if(otype == 's'){ 
		  sprintf(xbuf,"extern char *%s();\n",argv[3]);
	       } else if(otype == 'f'){
		  sprintf(xbuf,"extern double %s();\n",argv[3]);
	       } else if(otype == 'i'){
		  sprintf(xbuf,"extern normal %s();\n",argv[3]);
	       } else {			/* if not p, s, f, or i must be void */
		  sprintf(xbuf,"extern void %s();\n",argv[3]);
	       }
	    }
	    addbuf(2,xbuf);
	 }
	 if(!procflg) {
	    sprintf(xbuf,"extern void m_%s() ;\n",argv[3]);
	    addbuf(2,xbuf);
	 }
/*
 * declare Mirella function and initialize pointers
 */
	 if(dynamic) {
	    sprintf(xbuf,"     (void (*)())&p_%s,\n",argv[3]);
	    addbuf(4,xbuf);

	    sprintf(xbuf,"     (void (*)())\"%s\",\n",argv[3]);
	    addbuf(4,xbuf);
	    maxifun++;
	    if(!procflg) {
	       sprintf(xbuf,"     m_%s ,\n",argv[3]);
	       addbuf(4,xbuf);
	       maxifun++;
	    }
	 } else {
	    if(procflg) {
	       if(otype == 'v') {
		  sprintf(xbuf,"     %s,\n",argv[3]);
	       } else {
		  sprintf(xbuf,"     (void (*)())%s,\n",argv[3]);
	       }
	    } else {
	       sprintf(xbuf,"     m_%s ,\n",argv[3]);
	    }
	    addbuf(4,xbuf);
	 }
/*
 * generate stack manipulating code
 */
	 if(!procflg) {
	    if(dynamic) {
	       sprintf(xbuf,"\nvoid m_%s(ptr)\n",argv[3]);
	       addbuf(8,xbuf);
	       if(otype == 'f'){
		  strcpy(xbuf,"float");
	       } else if(otype == 'i') {
		  strcpy(xbuf,"int");
	       } else if(otype == 'p'){
		  strcpy(xbuf,"Void *");
	       } else if(otype == 's'){
		  strcpy(xbuf,"char *");
	       } else if(otype == 'v') {
		  strcpy(xbuf,"void");
	       } else {
		  fprintf(stderr,"Unknown return type: %c\n",otype);
	       }
	       strcat(xbuf," (*ptr)();\n{");
	    } else {
	       sprintf(xbuf,"\nvoid m_%s() \n{",argv[3]);
	    }
	    addbuf(8,xbuf);
	    for(i = ino - 1;i >= 0;i--){
	       sprintf(xbuf,"%s",clin[i]);  /* pops for arguments */
	       addbuf(8,xbuf);
	    }
	    sprintf(xbuf,"\n");
	    addbuf(8,xbuf);

	    if(ignore_out || otype == 'v') {
	       sprintf(xbuf,"\n    ");	/* no return */
	    } else if(otype == 'f'){
	       sprintf(xbuf,"\n    fpush(");
	    }else if(otype == 'i' || otype == 'p'){
	       sprintf(xbuf,"\n    push(");
	    }else if(otype == 's'){
	       sprintf(xbuf,"\n    cspush(");
	    }
	    addbuf(8,xbuf);

	    if(dynamic) {
	       sprintf(xbuf,"(*ptr)(");
	    } else {
	       sprintf(xbuf,"%s(", argv[3]);
	    }
	    addbuf(8,xbuf);

	    nfarg = niarg = nparg = nsarg = 0;
	    for(i = 0;i < ino;i++){
	       switch (type[i]) {
		case 'f':
		  sprintf(xbuf," farg%d",nfarg++);
		  break;
		case 'i':
		  sprintf(xbuf," iarg%d",niarg++);
		  break;
		case 'p':
		  sprintf(xbuf," parg%d",nparg++);
		  break;
		case 's':
		  sprintf(xbuf," sarg%d",nsarg++);
		  break;
		default:
		  fprintf(stderr,"Unknown argument type: %c\n",type[i]);
		  break;
	       }
	       addbuf(8,xbuf);
	       if(i<ino-1){
		  sprintf(xbuf,",");
		  addbuf(8,xbuf);
	       }
	    }
	    if(output && !ignore_out) {
	       sprintf(xbuf,"));\n}\n");
	       addbuf(8,xbuf);
	    } else {
	       sprintf(xbuf,") ;\n}\n"); /* void */
	       addbuf(8,xbuf);
	    }
	 }
	 break;
       case 'I':			/* initialisation functions */
	 if(argv[1][1] == '1' || argv[1][1] == '2' || argv[1][1] == '3') {
	    if(argc < 3) {		/* argv[0] is not used */
	       fprintf(stderr,"you must specify a function name with %s\n",
		       argv[1]);
	    } else {
	       sprintf(xbuf,"extern void %s();\n",argv[2]);
	       addbuf(2,xbuf);
	       sprintf(xbuf,"   *init%c = %s;\n",argv[1][1],argv[2]);
	       addbuf(7,xbuf);
	    }
	 } else {
	    fprintf(stderr,"Unknown directive: %s\n",argv[1]);
	 }
	 break;
       default:
	 printf("Illegal identifier %c\n",*argv[1]);
	 goto error;
      }				/* end switch on installation types */
      continue;
/*
 * Error handling code; error is at the level of the outermost while
 */
    error:
      fprintf(stderr,
  "FATAL ERROR: I will proceed in order to catch others, but you must redo\n");
      fprintf(elfp,
  "FATAL ERROR: I will proceed in order to catch others, but you must redo\n");
      showline(infile,linecount);
      exit_status = 1;
   }
   
   if(nestflg != nestflg0){
      fprintf(stderr,"Control-line error: missing #endif\n");
      nestflg= 0;
   }

   fclose(ifp);
   depth--;
}

/*****************************************************************************/
/*
 * 
 */
static int
read_line(buf,fil)
char *buf;
FILE *fil;
{
   char *save = savbuf;
   int i,j;
   int linecount = 0;
   
   for(i = 0;i < N_LINE;i++) {
      if(fgets(buf,120,fil) == NULL) {
	 if(i > 0) {
	    fprintf(stderr,"Can't read continuation line\n");
	 }
	 return(-1);
      }
      linecount++;
      strcpy(save,buf);			/* save input line for archiving */
      if((j = strip_c_comments(buf,120,fil)) > 0) {
	 linecount += j;
	 for(j = 0;isspace(buf[j]);j++) continue;
	 if(buf[j] == '\0') buf[0] = '\0';
	 break;				/* don't look for continuation */
      }
      if(i == 0) {
	 for(j = 0;isspace(buf[j]);j++) continue;
	 if(buf[j] == '\0') {
	    buf[0] = '\0';
	    break;
	 }
      }

      j = strlen(buf);
      if(buf[j - 2] == '\\') {		/* a continuation line */
	 buf += (j - 2);		/* overwrite the \ */
	 save += (j - 2);		/* overwrite the \ */
	 continue;
      }
      break;				/* the line is all read */
   }
   if(i == N_LINE) {
      fprintf(stderr,"Too many continued lines (%d max)\n",N_LINE);
      return(-1);
   }
   
   return(linecount);
}

/*
 * Remove C-style comments, but don't worry about quotes.
 */
static int
strip_c_comments(buf,n,ifp)
char *buf;				/* read buffer */
int n;					/* size of buffer */
FILE *ifp;				/* read from this FILE */
{
   int i,j,k;
   int in_c_comment = 0;		/* are we in a comment? */
   int nread = 0;			/* number of lines we have read */

   do {
      for(i = 0;buf[i] != '\0';i++) {
	 if(in_c_comment ||
	    (buf[i] == '/' && buf[i + 1] == '*')) { /* start C comment */
	    in_c_comment = 1;
	    for(j = i;buf[j] != '\0';j++) {
	       if(buf[j] == '*' && buf[j + 1] == '/') { /* end C comment */
		  in_c_comment = 0;
		  j += 2;
		  for(k = i;(buf[k++] = buf[j++]) != '\0';) {
		     ;
		  }
	       }
	    }
	 }
      }
   } while(in_c_comment && fgets(buf,n,ifp) != NULL && ++nread);

   return(nread);
}

static void
print_warnings(file)
char *file;
{
   if(nwarn) {
      if((expect_cm < 0 && nnamdif > 0) || expect_cm != nnamdif) {
	 fprintf(stderr,"\n%d C & M name differences in file %s",nnamdif,file);
      } else {
	 nwarn -= nnamdif;
	 nnamdif = 0;
      }
      if((expect_typeless < 0 && nnone > 0) || expect_typeless != nnone) {
	 fprintf(stderr,"\n%d typeless vbl entries in file %s",nnone,file);
      } else {
	 nwarn -= nnone;
	 nnone = 0;
      }
      if((expect_no_size < 0 && narsiz > 0) || expect_no_size != narsiz) {
	 fprintf(stderr,"\n%d missing array sizes in file %s",narsiz,file);
      } else {
	 nwarn -= narsiz;
	 narsiz = 0;
      }
      if(nwarn) {
	 fprintf(stderr,"\n%d other warnings or nonfatal errors in file %s.  ",
		nwarn-nnamdif-nnone-narsiz,file);
	 fprintf(stderr,"See minstall.log\n");
      }
   }
}

/************************************************************/

static void
usage()
{
    printf("\n%s\n%s",
	   "  Usage: minstall [-ei123fkquv] [-Aname=name] [-Dname[=val]]",
	   "		      [-o outfile] [-s skel] [ script1 script2 ...]");
    printf("\n     the -i flag opens mirelic.c and does internal commands;");
    printf("\n     -e, -1, -2, -3 specify type of external linkage");
    printf("\n     -k kills old entries in mirelec.c");
    printf("\n     -o outfile specifies the name of the output file");
    printf("\n                             (default: mirel[ei123]c.c)");
    printf("\n     -q suppresses some output");
    printf("\n     -s skeleton specifies the skeleton file");
    printf("\n                             (default: mirel[ei]c.skl)");
    printf("\n     -u prints this message");
    printf("\n     -w enables warnings about undefined symbols");
    printf("\n     the -A flag aliases a symbol (e.g. -AM_TERMINAL=vt100)\n");
    printf("\n     the -D flag defines a symbol (e.g. -DBUGGY=0)\n");
    printf(
    "\n  a script file is just a file with minstall commands, one per line.");
    printf(
    "\n  An initial \\  signals a comment, a line with just <cr> terminates.");
    printf("\n");
}

/************************************************************/
/*
 * sets argv,argc and interpolates nulls in buf; finds
 * comments if any, and expands cname if repeat of fname;
 * also interprets any # control lines and recognise C code destined for
 * fragment 0
 */
static char ecname[40];

static void
mparse(buf)
char *buf;
{
   char *ptr;
   char c, *cp;
   
   ctrlflg = commentflg = 0;
   if(*buf == '#'){			/* control line; FIRST char is # */
      ctrlflg = 1;

      if((cp = parse_cpp_line(buf,&skipflg,&nestflg)) != NULL) {
	 read_include(cp);
      }
      return;
   }
/*
 * otherwise parse the line
 */
   argc = 1;
   ptr = buf;
   while(*ptr){
      while(isspace(*ptr++)) continue;	/* skip leading whitespace */
      ptr--;				/* at first nonwhite */
      if(*ptr == '\\'){			/* rest of line is comment */
	 if(ptr == buf){		/* whole line is comment */
	    commentflg = 1;
	    strcpy(line,ptr);
	    *buf = '\0';  /* null input line */
	 }else{            /* rest of line is comment */
	    commentflg = 2; 
	    strcpy(line,ptr);      /* save it */
	    *ptr = '\0';  /* terminate input line */
	 }
	 continue;
      }else if(*ptr != '\0'){
	 if(argc == 3 && (cp = strrchr(ptr,'.')) != NULL && isspace(*(cp+1))
	    && !ctrlflg){    
	    /* does cname have a terminal '.' in it ? If so, expand it */
	    strncpy(ecname,ptr,(cp-ptr));     /* copy prefix */
	    strcpy(ecname+(cp-ptr),argv[2]);  /* copy fname */
	    argv[argc++] = ecname;       /* and point to expanded name */
	 }else argv[argc++] = ptr;
	 while((c = *ptr++) != '\0' && !isspace(c)) continue;
	 *(ptr-1) = '\0';    /* replace first white by null */
	 if(!c) ptr--;       /* end */
      }
   } 
   return;
}

/************************************************************/
/*
 * Process a file, looking for #... lines
 */
static void
read_include(file)
char *file;
{
   char name[150];

   if(file[-1] == '<') {		/* a system file */
      sprintf(name,"%s%s",usr_include,file);
   } else {
      strcpy(name,file);
   }

   process_input_file(name);
}

/************************************************************/
/*
 * is a string the name of a member of a structure? Return 1 if it's
 * a pointer, 2 if it's a struct
 */
static int
isstruct(s)
char *s;
{
    char *cp;

    strcpy(sname,s);
    if(((cp = strchr(sname,'-')) != NULL && *(cp + 1) == '>')) {
       *cp = '\0';			/* terminate after name or pointer */
       return(1);
    } else if((cp = strchr(sname,'.')) != NULL) {
       *cp = '\0';
       return(2);
    } else {
       return(0);
    }
}

/************************************************************/
/*
 * returns pointer to string containing "char", "u_char", "short",
 * "normal","float" according as c = 'c', 'u', 's', 'i', 'f', 
 * or 'n' if none of these. see declarations for tcat, tname.
 * A suitable number of *'s are prepended, depending upon the
 * degree of indirection appropriate to the object being declared
 */  
static char *
tindex(c,ptr_type)
int c;
int ptr_type;
{
   char cc;
   int i;
   static char tcat[] = "cusifpn"; /* abbrev for char, u_char, short, int,
				      float, ptr, and null (externally
				      defined in mapp.h) */
   static char *tname[] = {
      "char ", "u_char ", "short ","normal ","float ","Void *"," "};
   static char type[20];
   
   for(i = 0;(cc = tcat[i]) != '\0';i++) {
      if(c == cc) {
	 int j;
	 char *tptr;
	 strcpy(type,tname[i]);
	 tptr = type + strlen(type);
	 for(j = 0;j < ptr_type;j++) *tptr++ = '*';
	 *tptr = '\0';
	 return(type);
      }
   }
   return(NULL);
}

/************************************************************/

static FILE *
tryopen(name,mode,bin)
char *name, *mode;
int bin;      /* if 1, binary open for DSI */
{
   FILE *fp;
   char m[2];				/* mode may not be writeable */
   
   /* see if file exists; if not and wish append, change to write */
   m[0] = *mode; m[1] = '\0';
   if(access(name,0) && m[0] == 'a') m[0] = 'w'; 
#ifdef DSI
   if(bin)
     fp = fopenb(name,m);
   else
#endif
     fp = fopen(name,m);		/* try this jeg880829 */
   if(!fp){
      fprintf(stderr,"Cannot open %s in requested mode, %s\n",name,m);
      exit(1);
   }
   return(fp);
}

/************************************************************/
/*
 * finds a substring s2 in the string s1; returns pointer 
 * if successful, nc is pointer to next char after.
 * not used here, but neat; put in tools
 */
char *
findstr(s1,s2,pnc)
register char *s1;
char *s2;
char **pnc;
{
   int len = strlen(s2)-1;
   register int sb = *s2;		/* first character */
   register int c;
   register int i;
   char *s3 = s2 + 1;			/* pointer to 2nd char */
   
   for(;;) {
      if((c = *s1++) == '\0') {
	 break;				/* end of s1 */
      } else if(c != sb) {
	 continue;
      }
      for(i=0;i<len;i++){
	 if(s1[i] != s3[i]) continue;
      }
      /* got it !! */
      *pnc = s1 + len ;
      return(s1-1) ;
   }
   
   *pnc = 0;
   return(NULL);			/* not there */
}

/************************************************************/

static void
takeitapart()
{
   int i;
   static char bxmark[] = "/*b?.x*/";  /* begin fragment markers in cfile */
   static char exmark[] = "/*e?.x*/";  /* end fragment markers in cfile */
   char tailstring[80];
   char *start,*ostart,*cp,*cp1;
   char *eof;
   int tail = 0;
   static char setup_func[] = "m?setup(nvar,nfunc";
			/* pattern for setup function's declaration */
   static char set_next_cif[] = "   *next_cif = next_cif?;\n";
   			/* pattern for setting next_cif() */
   static char set_init0[] = "   *init0 = m?setup0;\n";
   			/* pattern for setting init0() */
   static char decl_init0[] = "m?setup0()\n";
   			/* pattern for declaring init0() */
   static char decl_next_cif[] = "next_cif?()\n";
			/* pattern for declaring next_cif() */
/*
 * allocate memory for fragments
 */
   *tailstring = '\0';    
   for(i = 0;i < NFRAG;i++) {		/* i==NFRAG-1 is end of file only */
      if((bxp[i] = malloc(falloc[i])) == NULL) {
	 fprintf(stderr,"Cannot allocate %d bytes of memory",falloc[i]);
	 exit(1);
      }
      tail = strlen(tailstring);	/* marker from last buffer */
      strcpy(bxp[i],tailstring);	/* move into this one */
      start = bxp[i] + tail;  /* last of last buffer may have been moved in*/
      
      exmark[3] = bxmark[3] = '0' + i ;  /* ascii 'i' */
      
      while((eof = fgets(start,80,cfile)) != NULL){ 
	 /*for DSI, get both <cr> and <lf>*/
	 /* note that there is always a term nul in buffer which is written
	    over with each new read */ 
	 ostart = start;		/* pointer for last(this) read */
	 start += strlen(ostart);
	 if(strncmp(setup_func,ostart,sizeof(setup_func) - 1) == 0 ||
	    strcmp(set_next_cif,ostart) == 0 ||
	    strcmp(set_init0,ostart) == 0 ||
	    strcmp(decl_next_cif,ostart) == 0 ||
	    strcmp(decl_init0,ostart) == 0) {
	    *strrchr(ostart,'?') = iec;
	    continue;
	 } else if(*ostart != '/') {
	    continue;
	 }
	 /* have comment. see whether it is marker */
	 if(!strncmp(bxmark,ostart,8)){  /* have beginning of frag text*/
	    fxp[i] = start;        /* ptr to frag proper, after marker */
	 }else if(!strncmp(exmark,ostart,8)){  /* have end */
	    xp[i] = ostart;   /* set current pointer to beginning of line
				 with end-marker */
	    strcpy(tailstring,ostart);  /* put end marker in tbuffer */
	    
	    *ostart = '\0';           /* terminate current buffer before
					 end marker */
	    break;
	 }
      }
      
      if(eof == NULL){
	 if(i != NFRAG - 1) { 
	    fprintf(stderr,"eof or error in takeitapart loop with i=%d. Dead.\n",i);
	    exit(1);
	 } else {
	    xp[i] = fxp[i] = start;  /* buffer NFRAG - 1 is never written to */
	 }
      }
   }
/*
 * Find the maximum values of n_?args from fragment 0, and erase it.
 */
   xp[0] = fxp[0];
   xp[0][0] = '\0';
/*
 * Read the current values of maxifun and maxiv from fragment 1,
 * and erase it.
 */
   xp[1] = fxp[1];			/* Fix up buffer 1; special case */
   cp = strchr(xp[1],'#');
   cp1 = strchr(xp[1] + 10,'#');
   *cp1 = '\0';
   maxifun = atoi(cp + 15);
   maxiv = atoi(cp1 + 15);
   xp[1][0] = '\0';
}
    
/************************************************************/
/*
 * like fwrite but writes in chunks if bigger than 16K (DSI limit)
 */
#define BUFSIZE 16384
           
static int
bfwrite(str,size,len,fp)
FILE *fp;
int size,len;
char *str;
{              
   int nbuf, tail, i, c, nw=0;
   
   nbuf = (size*len)/BUFSIZE;
   tail = size*len - nbuf*BUFSIZE;
   for(i=0;i<nbuf;i++){
      c = fwrite(str + i*BUFSIZE,1,BUFSIZE,fp);
      nw += c;
      if(c == 0) return(0);		/* error */
   }
   if(tail){
      c = fwrite(str + nbuf*BUFSIZE,1,tail,fp);
      nw += c;
      if(c == 0) return(0);		/* error */
   }
   return(nw);
}        

/************************************************************/

static void
putback(fname)
char *fname;
{
   char *def_mirelic = "#define MIRELIC 1\n";
   FILE *fil;
   int i;
   
   fil = tryopen(fname,"w",1);		/* binary bcz of DSI bug */
   if(iflg == -1) {
      bfwrite(def_mirelic,1,strlen(def_mirelic),fil);
   }
   
   for(i = 0;i < NFRAG;i++){
      flen[i] = xp[i] - bxp[i];
      bfwrite(bxp[i],1,flen[i],fil);
   }
   fclose(fil);
}

/************************************************************/

static void
killold()
{
   int i;
   
   for(i=1;i < NFRAG - 1;i++){
      xp[i] = fxp[i];   /* set current pointers to beginning of frags */
   }
   maxifun = 0;
   maxiv = 0;    /* reset counters */
}

/************************************************************/
/*
 * copies buf to *frag n (at xp[n]) and bumps xp[n] 
 * to new end; does cr lf stuff for DSI (ugh)
 */
static void
addbuf(n,buf)
int n;
char *buf;
{                        
    register char *cp;
    register int c;
    
    cp = xp[n];
    while((c = *buf++) != '\0'){
        if(c != '\n') *cp++ = c;
        else{
#ifdef DSI
            *cp++ = '\r';
#endif
            *cp++ = '\n';
        }
    }
    xp[n] = cp;  /* cp is loc of next char */
    if((xp[n] - bxp[n]) > falloc[n] -100){
        fprintf(stderr,"\nOut of memory for fragment %d, allocation %d\n",
            n,falloc[n]);
        fprintf(stderr,"\nChange allocation in minstall.c and recompile. ");
        fprintf(stderr,"\nWhat are you DOING ??");
        exit(-1);
    }
}


/************************************************************/
/*
 * Add a struct initialisation to fragment 6
 */
static void
addbuf6(name,type,flag,d1,d2,size)
char *name;				/* name of C-word */
int type,flag;				/* type and any flags */
int d1,d2;				/* dimensions */
char *size;				/* expr giving size of entity */
{
   char buf[200];

   if(size == NULL) size = "0";
   sprintf(buf,"   { \"\\%03o%s\", %d, %d, %d, %d, %s },\n",
	   (int)strlen(name),name,type,flag,d1,d2,size);
   addbuf(6,buf);
}

/************************************************************/
/*
 * write to error log
 */
static void
showline(infile,line_num)
char *infile;
int line_num;
{
   fprintf(elfp,"File %s, Line %3d: %s\n",infile,line_num,savbuf);
   fprintf(stderr,"Line %3d: %s\n",line_num,savbuf);
}

/************************************************************/

static void
fcopy(fsrc,fdest)
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
