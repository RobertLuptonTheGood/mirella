/*
 * this file contains the c interface initialization, the string stack
 * manipulation code words, and misc. C->Mirella functions
 */
#ifdef VMS
#include mirkern
#include mprims
#include dotokens
#include mireldec
#include mircintf
#else
#include "mirkern.h"
#include "mprims.h"
#include "dotokens.h"
#include "mireldec.h"
#include "mircintf.h"
#endif

#if defined(_POSIX_SOURCE)
#  include <sys/types.h>		/* definition of mkdir */
#  include <sys/stat.h>
#endif

extern char home_dir[];     /* from mirellio.c */
extern normal m_termflg;    /*   "             */
extern int (*scrnprt)();    /*   "             */

/*********************** INIT_CIF() ***************************************/

void
init_cif(next_cif,vparr,fparr)
MIRCINTF *(*next_cif)();
normal *vparr;
void (*fparr[])();
{
    char *fword;
    int immediate;
    int next_prim = 1;
    int next_uvar = 0;
    int findex = 0;
    int vindex = 0;
    int i;
    MIRCINTF *cif;			/* a C-interface definition */
    normal savelink;
    normal curentry;

    curentry = V_CURRENT;               /* save current */

    while((cif = (*next_cif)()) != NULL) {
       fword = cif->fname;
       immediate = 0;

       switch (cif->type) {
	case CIF_FUNCTION:
	case CIF_PROCEDURE:
	   if(cif->flags & CIF_DYNAMIC) {
	      qt_create(fword,
			cif->type == CIF_FUNCTION ? DOCODE_FPTR : DOCODE_PPTR);
	      *dp_4th++ = (normal)fparr[findex++]; /* ptr to ptr to func() */
	      *dp_4th++ = (normal)fparr[findex++]; /* name of function */
	      if(cif->type == CIF_FUNCTION) {
		 *dp_4th++ = (normal)fparr[findex++]; /* ptr to m_func() */
	      }
	   } else {
	      qt_create(fword,DOCODE);
	      *dp_4th++ = (normal)fparr[findex++];
	   }
	   break;
	 case CIF_VARIABLE:		/* c variable or array */
	   qt_create( fword , DOCVAR );
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_CONSTANT:		/* c constant */
	   qt_create( fword, DOCCON );
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_FCONSTANT:		/* float c constant */
	   qt_create( fword, DOFCCON );
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_STRING:		/* c string */
	   qt_create( fword , DOSCCON );
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_OFFSET:		/* structure offset */
	   qt_create( fword, DOOFFSET );
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_NARRAY:		/* normal array */
	   qt_create( fword , DONCAR );
	   if(cif->d1 < 0)
	     printf("\nWARNING: Missing array size for %s",fword +1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_SARRAY:		/* short array */
	   qt_create( fword , DOSCAR );
	   if(cif->d1 < 0)
	     printf("\nWARNING: Missing array size for %s",fword +1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_CARRAY:		/* char array..NOT A STRING */
	   qt_create( fword , DOCCAR );
	   if(cif->d1 < 0)
	     printf("\nWARNING: Missing array size for %s",fword +1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_STRUCARRAY:		/* array of structs */
	   qt_create( fword , DOSTRUCCAR );
	   if(cif->d1 < 0)
	     printf("\nWARNING: Missing array size for %s",fword +1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = cif->size;
	   *dp_4th++ = (normal)&vparr[vindex++];
	   break;
	 case CIF_NMATRIX:		/* normal matrix */
	   qt_create( fword , DONCMAT );
	   if(cif->d1 < 0 || cif->d2 < 0) 
	     printf("\nWARNING: Missing array size(s) for %s",fword +1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = cif->d2;
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;
	 case CIF_SMATRIX:		/* short matrix */
	   qt_create( fword , DOSCMAT );
	   if(cif->d1 < 0 || cif->d2 < 0) 
	     printf("\nWARNING: Missing array size(s) for %s",fword + 1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = cif->d2;
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;            
	 case CIF_CMATRIX:		/* char matrix */
	   qt_create( fword , DOCCMAT );
	   if(cif->d1 < 0 || cif->d2 < 0) 
	     printf("\nWARNING: Missing array size(s) for %s",fword +1);
	   *dp_4th++ = cif->d1;
	   *dp_4th++ = cif->d2;
	   *dp_4th++ = (normal)vparr[vindex++];
	   break;            
	 case CIF_VOCAB:		/* new vocabulary */
	   qt_create( fword , DOVOC );
	   V_CURRENT = (normal)dp_4th;
	   for(i=0;i<NTHREADS;i++) *dp_4th++ = 0;   /* no definitions yet for
						       any thread */
	   savelink = V_VOC_LINK;
	   V_VOC_LINK = (normal)dp_4th;
	   *dp_4th++ = savelink;	/* establish links to voc chain */
	   break;
	 case CIF_FORTH:		/* old vocabulary */
	   V_CURRENT = curentry;
	   break;
	 case CIF_IMMEDIATE:		/* immediate primitive */
	   immediate = 1;
	   /* fall thru */
	 case CIF_PRIMITIVE:		/* primitive */
	   qt_create( fword, next_prim );
	   origin[next_prim] = (normal)(dp_4th -1);
	   if(immediate) makeimmediate();
	   next_prim++;
	   break;
	 case CIF_USER:			/* user variable */
	   qt_create( fword, DOUSER );
	   *dp_4th++ = next_uvar;
	   next_uvar += sizeof(normal);
	   break;
	 default:
	   scrprintf(
		  "\nUnknown cmd while initing the dict or c interface: '%c',",
		     cif->type);
	   scrprintf(" (ascii %d)\n",cif->type);
	   exit(1);
        }
    }
    V_CURRENT = curentry;		/* restore current */
}

/********************** STRING STACK WORDS *********************************/
/*
The string stack is a circular stack of SSNSTRING 256-byte strings; 
each consists of a count byte and up to 255 bytes of string. The pointer 
is an address which points to the first char (2nd byte) in the nth string. 
080607: This needs to be looked at seriously. With modern machines, the
memory usage could be much bigger, and in particular the lengths could
be much bigger--but the length byte in Forth is a BYTE, and that is a 
problem.

This had the depth of the string stack compiled in, which was B A D.
fixed. It is now defined in mirkern.h, SSNSTRING, which may be assumed
to be a power of 2. It is currently 256

*/

/*********************** ALLOC_SS() **************************************/
/*
 * allocates space for string stack
 */
void
alloc_ss()
{
   char *ssorig;
    
   ssorig = (Void *)malloc(SSSIZE);
   if(!ssorig) erret("\nCannot allocate string stack space");
   str_stk = ssorig + 12;  /*  why ???  */
   printf("\nSSIZE=%d  SSNSTRING=%d str_stk=%u\n",SSSIZE,SSNSTRING,(unsigned int)str_stk);
}

/************************ SSINIT() ***************************************/

static char *sspar[SSNSTRING+16];
static normal ssindex;

void ssinit()
{
    int i;

    ssindex = 0;    
    xssp_4th = str_stk;
    for(i=0;i<SSNSTRING;i++){
        sspar[i] = str_stk + 256*i;
        *sspar[i] = '\0';
        *(sspar[i] + 1) = '\0';
    }
}

/*************************** SPUSH(), FSPUSH() *****************************/

/*
 * takes cstr address, copies to sstack, returns cstr address on stack
 * (stk ptr +1)--note that strings pushed on stack are proper forth strings,
 * so the forth string vesions of the stack words still all work
 */
char *
spush(str)
register char *str;
{
    int count;
    register char *cp;
    
    xssp_4th = sspar[(++ssindex & (SSNSTRING-1))];
    cp  = xssp_4th + 1;    /* add 1 for Cstrings */
    while((*cp++ = *str++) != '\0') continue; /* copy string onto stack */
    count = cp - xssp_4th - 2;    
    *xssp_4th = count;
    if(count > 255){
        scrprintf("\nWARNING: string stack overflow,count=%d", count);
        *xssp_4th = 255;
        *(xssp_4th + 255) = '\0';
    }
    return (xssp_4th + 1) ;  /* return the  Cstring address */
}

/*
 * pushes a (not necessarily terminated) forth string onto the sstack.
 * the copy there is terminated
 */
char *
fspush(fstr)
register char *fstr;
{
    register int count = *fstr++;
    register char *cp;
    
    xssp_4th = sspar[(++ssindex & (SSNSTRING-1))];
    cp  = xssp_4th;
    *cp++ = count;
    while(count--) *cp++ = *fstr++; /* copy string onto stack */
    *cp = '\0';                     /* terminate */
    return (xssp_4th + 1) ;         /* return the Cstring address */
}    

/*************************** CSPUSH() *****************************/
/*
 * pushes address of C string onto parameter stack; this should be a macro
 */
void
cspush(str)
char *str;
{
    *--xsp_4th = (normal)str;
}


/***********************  SDOWN *************************************/
/*
 * returns pointer to cstring below current top; resets s_stack pointer
 */
char *
sdown()
{
    return ((xssp_4th = sspar[(--ssindex & (SSNSTRING-1))]) + 1);
}

/*********************** SUP() **************************************/
/*
 * returns pointer to cstring above current top; resets s_stack pointer
 */
char *
sup()
{ 
    return ((xssp_4th = sspar[(++ssindex & (SSNSTRING-1))]) + 1); 
}

/********************** CINTERP(), CEXEC()********************************/

normal ci_cfa = 0;  /* global which holds cfa of last word invoked with
                        cinterp */
char ci_lastword[40] = {0,0};  /* last word executed */

/*
 * executes one forth word from C, name cstr. slow, but handy. Returns 1
 * if successful, 0 if cannot find word; if execs same * word repeatedly,
 * searches only once; uses saved cfa on subsequent calls
 */
int
cinterp(cstr)
char *cstr;
{
    int len = strlen(cstr);
    int fit;
    token_t tp;
    extern int verbose;

    if(len == ci_lastword[0] && !strcmp(cstr,ci_lastword+1) && ci_cfa){
        return cexec(ci_cfa);
    }   /* if word was same as last time, execute it and return */
    ci_lastword[0] = len;
    strcpy(ci_lastword+1,cstr);  /* make forth string */
    tp = (normal)canon(ci_lastword);
    if((fit = find((char **)&tp)) == 0) {
        if(verbose > -1) scrprintf("\nCannot find Mirella word %s",cstr);
        ci_cfa = 0;        
        *ci_lastword = *(ci_lastword+1) = 0;
        return 0;
    }
    ci_cfa = (normal)tp;
    return cexec(tp);    /* execute it */
}

int
cexec(tp)    /* if you have the cfa, use this */
token_t tp;
{
    token_t cbuf[2];

    if( *(token_t *)tp < MAXPRIM)
        tp = *(token_t *)tp;       /* if prim, use prim # instead of cfa */
    cbuf[0] = tp;                  /* set up comp. buffer */
    cbuf[1] = FINISHED;
    in_intp(cbuf);                 /* do it */
    return 1;
}

/******************* INTERP_TOKARR(), INTERP_CSTR() ************************/

/*
 * these routines interpret and execute the tokens in a c string or token
 * array, one by one; the second one treats a string; it is parse()ed
 * and hence destroyed. The first takes an array of tokens and a count.
 * One cannot make colon definitions with this routine, since STATE is
 * not handled correctly, but literals (including strings in a hokey way)
 * are done correctly; I think one can EXECUTE almost anything, but words
 * which take input from the input stream are NOT handled correctly, which
 * forces the rather special treatment of '"'. NOTICE that in the use of
 * this code in the COMMAND-LINE interpreter in op_nxt_f in mirellio.c,
 * the special treatment of quotes by most operating systems makes the usage
 * particularly arcane; in the Percent Bourne shell under DOS, for instance,
 * if dir is a variable containing a directory name, one must say
 * mirella -s '\"' $dir'\"'_chdir to change the directory upon invocation of
 * Mirella. It might be well to adopt a non-special char and terminator
 * for this usage, but it would not be portable.
 */


void
interp_tokarr(nitem,lptr)
int nitem;
char **lptr;
{
    int i;
    int stringlit = 0;
    char strlit[256];
    char *cp;
    
    for(i=0;i<nitem;i++){
        if(*(lptr[i]) == '"' && strlen(lptr[i]) == 1){  /* next token is first
                                                          word of string lit */
            stringlit = 1;
            continue;
        }
        if(!stringlit){     /* forth word */
            (void)cinterp(lptr[i]);
        }else{              /* handle string literal */
            if(stringlit == 1) *strlit= '\0';  /* init */
            strcat(strlit,lptr[i]);
            stringlit++;
            if((cp = strchr(strlit,'"')) != 0){
                *cp = '\0';
                cspush(spush(strlit));  /* push onto sstack and addr to stk*/
                stringlit = 0;
            }
        }
    }
}

void 
interp_cstr(str)
u_char *str;
{
    char *lptr[20];
    int nitem;
    
    nitem = parse((char *)str,lptr);
    interp_tokarr(nitem,lptr);
}


/***** VARIOUS SUPPORT CODE FOR INTERNAL FUNCTIONS AND VARIABLES *******/

/* following #defines from mirella.h */

#define  fpop       (double)(*xfsp_4th++)
#define  pop        (*xsp_4th++)
#define  push(xx)   (*(--xsp_4th)) = (normal)( ( xx ) )
#define  fpush(xxx) (*(--xfsp_4th)) = (float)( ( xxx ) )
#define  cppop      (char *)(*xsp_4th++)
#define  cspop      ((char *)(*xsp_4th++))

static char *parg0;
static normal iarg1,iarg2;
mirella int system_code = 0;		/* return code of the system() call */

void sys()  /* Mirella procedure */
{
   char com[MAXSTRING + 1];
   parg0 = cspop; 
   strncpy(com,parg0,MAXSTRING); com[MAXSTRING] = '\0';
   reset_kbd();
   if((system_code = system(com)) == -1) {
      perror("system() call");
   }
   set_kbd();
}

static int
icomp(i,j)
Const Void *i,*j;
{
	return (*(int *)i - *(int *)j);
}


static int
ricomp(i,j)
Const Void *i,*j;
{
	return (*(int *)j - *(int *)i);
}

static int
fcomp(a,b)
Const Void *a,*b;
{
   if(*(float *)a > *(float *)b) return (1);
   if(*(float *)a < *(float *)b) return (-1);
   return (0);
}

static int
rfcomp(a,b)
Const Void *a,*b;
{
   if(*(float *)b > *(float *)a) return (1);
   if(*(float *)b < *(float *)a) return (-1);
   return (0);
}

static int
scomp(s,q)
Const Void *s, *q;
{
   return(strcmp(*(char **)s,*(char **)q));
}

static int
rscomp(s,q)
Const Void *s, *q;
{
   return(strcmp(*(char **)q,*(char **)s));
}

void iqsort() 
{
    iarg2 = pop;   
    iarg1 = pop;   
    parg0 = cppop; 
    qsort( parg0, iarg1, iarg2, icomp);
}

void r_iqsort() 
{
    iarg2 = pop;   
    iarg1 = pop;   
    parg0 = cppop; 
    qsort( parg0, iarg1, iarg2, ricomp);
}

void fqsort() 
{
    iarg2 = pop;   
    iarg1 = pop;   
    parg0 = cppop; 
    qsort( parg0, iarg1, iarg2, fcomp);
}

void r_fqsort() 
{
    iarg2 = pop;   
    iarg1 = pop;   
    parg0 = cppop; 
    qsort( parg0, iarg1, iarg2, rfcomp);
}

void sqsort() 
{
    iarg2 = pop;   
    iarg1 = pop;   
    parg0 = cppop; 
    qsort( parg0, iarg1, iarg2, scomp);
}

void r_sqsort() 
{
    iarg2 = pop;   
    iarg1 = pop;   
    parg0 = cppop; 
    qsort( parg0, iarg1, iarg2, rscomp);
}

/* the following is an interface to qsort for arbitrary entities
*  through Mirella, which can be used for anything which can be represented
*  by an array of pointers.
*
*  base number cfa-of-compare mqsort    
*/


static normal ccfa;  /* cfa of compare function, takes two pointers on
                        stack */

int
_mcompare(p1,p2)     /* p1,p2 must be pointers to something */
Const Void *p1, *p2;
{
    push(p1);
    push(p2);
    cexec(ccfa);
    return pop;
}

void mqsort()
{
    ccfa = (normal)cppop;  /* cfa of comparison word */
    iarg1 = pop;    /* number of elements */
    parg0 = cppop;  /* base address of array */
    qsort( parg0, iarg1, sizeof(char *), _mcompare);
}

/******************** INPUT CHECKING CODE() ****************************/

void
irchk(n,n1,n2,msg) /* range check n1 <= n < n2; if not erret(msg) */
int n,n1,n2;
char *msg;
{
    if(n < n1 || n >= n2) erret(msg);
}

void
frchk(f,f1,f2,msg)  /* ditto for floats */
double f, f1, f2;
char *msg;
{
    if(f<f1 || f>f2) erret(msg);
}


/******************** MIRELLA STRING PROCEDURES **************************/
/*************** FORTH KERNEL FUNCTIONS **********************************/
/*
 * ( addr n -- ) emit()s the first n char at addr
 */
void
m_type()
{
    register int n = pop;
    register char *str = (char *)pop;
    
    if(n>0) { while(n--) emit(*str++); }
}   

void dash_trlng()    /* (addr n --- addr n')
                        adjusts count n to remove trailing blanks from a 
                        forth string whose first char is at addr */
{
    int count = pop;
    char *addr = (char *)pop;
    register int n = count;
    register char *cp = addr + count;
    
    n++;
    while(--n && *(--cp) == ' ') continue;
    push(addr);
    push(n);
}
    
void m_dfdotr()  /* ( flt wid dec -- ) prints the float flt in a field of
                    width wid, dec places to rt of decimal */
{
    int decimals = pop;
    int wid = pop;
    float fscr = fpop;
    dfdotr(fscr,decimals,wid);
}

/* returns the pointer to the character FOLLOWING the substring su in
 * the string s. If the substring is empty, returns s; if the substring
 * is not found, returns (char *)0.
 */
 
char * 
strrstr(s,su)
char *s;
char *su;
{
    int l = strlen(su);
    char *cp;
    
    if(l == 0) return s;
    cp = strstr(s,su);
    if ( cp == (char *)NULL){
        return cp;
    }else{
        return cp + l;
    }
}
    
        

/*************** C LIBRARY STRING FUNCTIONS ******************************/ 
/* why in the name of the wee man did I procedureize these??? */

void m_index()
{
    int chr = pop;
    char *str = cspop;
    cspush(strchr(str,chr));
}

void m_rindex()
{
    int chr = pop;
    char *str = cspop;
    cspush(strrchr(str,chr));
}

void m_strcpy()   /* note that Mirella arguments are reversed */
{
    char *dstr = cspop;
    char *sstr = cspop;
    strcpy(dstr,sstr);
}

void m_strcat()  /* note that Mirella arguments are NOT reversed */
{
    char *sstr = cspop;
    char *dstr = cspop;
    strcat(dstr,sstr);
}

void m_strncat()   /* Mirella string args are NOT reversed */
{
    int n = pop;
    char *sstr = cspop;
    char *dstr = cspop;
    strncat(dstr,sstr,n);
}

void m_strncpy()    /* Mirella string arguments are reversed */
{
    int n = pop;
    char *dstr = cspop;
    char *sstr = cspop;
    strncpy(dstr,sstr,n);
}


void m_memcpy()    /* Mirella string arguments are reversed */
{
    int n = pop;
    char *dstr = cspop;
    char *sstr = cspop;
    bufcpy(dstr,sstr,n);
}

void m_strcmp()
{
    char *s2 = cspop;
    char *s1 = cspop;
    push(strcmp(s1,s2));
}

void m_strncmp()
{
    int n = pop;
    char *s2 = cspop;
    char *s1 = cspop;
    push(strncmp(s1,s2,n));
}

void m_strlen()
{
    char *s = cspop;
    push(strlen(s));
}

/* new functions added jeg0312 */

void m_strchr()
{
    int chr = pop;
    char *str = cspop;
    cspush(strchr(str,chr));
}

void m_strrchr()
{
    int chr = pop;
    char *str = cspop;
    cspush(strrchr(str,chr));
}

/* finds the first instance of str2 contained in  str1 & returns pointer,
 * NULL if not found */
void m_strstr()
{
    char * str2 = cspop;
    char * str1 = cspop;
    cspush(strstr(str1,str2));
}

/* like above, but ignores case */
void m_strcasestr()
{
    char * str2 = cspop;
    char * str1 = cspop;
    char *strcasestr();
    cspush(strcasestr(str1,str2));
}

/* finds the first occurrence of any char of str2 within str1 and returns
 * the pointer (in str1) thereto, NULL if not found */
void m_strpbrk()
{
    char *str2 = cspop;
    char *str1 = cspop;
    cspush(strpbrk(str1,str2));
} 

/********** C LIBRARY STRING-TO-NUMBER CONVERSION PROCEDURES ********/

/* this one has a twist -- terminating K and M are correctly dealt with,
 * and perhaps we want to do this for C as well.
 */
  
int matoi(s)
char * s;
{
    int fac = 1;
    int len = strlen(s);
    char c;
    
    if((c = s[len-1]) == 'K' || c == 'M'){
        fac = 1024;
        if( c=='M') fac *= 1024;
        s[len-1] = '\0';
    }      
    return (atoi(s)*fac);
}

void m_atoi()
{
    char *s = cspop;
    push(matoi(s));
}

void m_atof()
{
    char *s = cspop;
    double atof();
    fpush(atof(s));
}

/************* PARSING FUNCTIONS: NEXTWORD(), PARSE() *******************/

char mdelimar[200];

/*
 * finds pointer to next word in string, entries separated by whitespace
 * or commas; first invocation takes string address; subsequent ones
 * take NULL; returns NULL at end of string. Destroys string, so make a
 * copy if that upsets you. Continues to return NULL at end of string
 */
char *
nextword(cpp)
char *cpp; 
{
    char *word,c;
    static int ip=0;
    static char *ptr= 0;
    if(cpp){
        ptr = cpp;
        ip = 0;
    }
    else if(!ptr) return 0;  /* do not complain if not inited; just ret 0*/
    /* skip to first nonwhite/comma char */
    while(isspace(c = (*ptr++)) || c == ',' ) continue;
    word = --ptr;           /* first nonwhite char (or term null if none) */
    if(!(*word)) return (ptr = (char *)0);  /* out of string */
    /* find first foll white/comma  or null */
    while((c = *(ptr++)) != '\0' && !isspace(c) && c != ',') continue;
    *(--ptr) = '\0';       /* make it null if not already */
    if(ip<200) mdelimar[ip++]=c;
    if(c) ptr++;           /* if c is not term null, adv ptr to next char */
    return  word;
} 

void
m_nextword()
{
    char *s = cspop;

    push(nextword(s));
}

/* 
 * Parses str, putting pointers to the successive tokens in it in argv, 
 * which is terminated with a null pointer (is this what we want to do??)
 * The routine returns the count. IT IS ASSUMED THAT ARGV IS LARGE ENOUGH.
 * NOTE THAT THE INPUT STRING IS DESTROYED.
 */
int parse(str,argv)   
    char *str;
    char **argv;
{
    int argc = 0;
    while((*argv++ = nextword(argc++ ? 0 : str)) != NULL) ;
    argc--;
    return argc;
}

void m_parse()
{
    char **argv = (char **)pop;
    char *str = cspop;
    push(parse(str,argv));
}

/************ SEQUENCE WRITING STUFF ********************************/

static char seqstr[100];

void
sequence(n,wid)
int n;      /* number to output if writing to terminal */
int wid;    /* field width: 9 or smaller */
{
   static char fmt[] = "\r%5d";
   if(!m_termflg) return;
   if(wid > 9 || wid < 0) wid = 9;    
   fmt[2] = wid + '0';
   sprintf(seqstr,fmt,n);
   (*scrnprt)(seqstr);
}

void seq1(n,wid)    /* use subsequent to sequence for second and succeeding
                        numbers on same line */
int n;      /* number to output if writing to terminal */
int wid;    /* field width; 9 or less */
{
   static char fmt[] = "  %5d";
   if(!m_termflg) return;
   if(wid > 9 || wid < 0) wid = 9;
   fmt[3] = wid + '0';
   sprintf(seqstr,fmt,n);
   (*scrnprt)(seqstr);
}

/***** NUMBER TO STRING CONVERSION PROCEDURES ***********************/

static char numbuff[64];

void m_itoa()   /* pushes address of ascii string for int on tos */
{
    int n = pop;
    sprintf(numbuff,"%d",n);
    cspush(numbuff);
}


char * 
zitoa(n,fld)
int n;
int fld;
{
    char buf[40];
    int len;
    char *cps = buf;

    if(fld > 63){
        scrprintf("\nZITOA: Ascii representation too long");
        fld = 63;
    }
    sprintf(buf,"%d",n);
    len = strlen(buf);
    if(len > fld){
        cps = buf + len-fld ;   /* truncate string at most significant end */
        strcpy(buf,cps);
    }else{
        if(fld-len) ffill(numbuff,fld-len,'0');
        strncpy(numbuff+fld-len,buf,len);
        numbuff[fld] = '\0';
        len=fld;
    }
    return(numbuff);

}
    
void m_zitoa() /* pushes address of leading-zero filled string of length
                    tos with ascii value of next int on stack; if number
                    is bigger than field, truncates at most significant 
                    end; string is always of desired length */
{
    int fld = pop;
    int n = pop;
    char *cp = zitoa(n,fld);
    cspush(cp);
}


static char fmt1[] = "%                     ";

/*#define FLDEBUG*/
 
char *
fltstr(arg,wid,dec)  /* general float formatting routine */
double arg;  /* argument float */
int wid;     /* field width; if negative, 'significance'; decimal point floats
                    to give string of constant length */
int dec;     /* decimals; if zero, integer output; if negative, exponential
                    notation */
{
    int numwid;
    char *cp = numbuff;
    char *ret = numbuff;
    double scarg;
    int p10 = 0;
    int negflg = ( arg < 0.0 );

    if(wid > 64 || wid < -64){
        scrprintf("\nFLTSTR: Field too large");
        *numbuff = '\0';
        return numbuff;
    }        
    if(wid>0){
        if(dec>0){
            sprintf(fmt1+1,"%d.%df",wid,dec);
            sprintf(numbuff,fmt1,arg);
        }else if(dec == 0){
            sprintf(fmt1+1,"%dd",wid);
            sprintf(numbuff,fmt1,(int)arg);
        }else{           /* negative, exponential output */
            sprintf(fmt1+1,"%d.%de",wid,-dec);
            sprintf(numbuff,fmt1,arg);
        }
    }else if(wid < 0){   /* constant precision float */
        wid = -(wid);
        numwid = wid;
        scarg = arg;
        if(negflg){
            numwid = wid-1;   /* width of actual numeric field */
            arg = -arg;
            numbuff[0] = '-';
            cp = numbuff + 1;
        }
        if(arg >= 1.0){
            p10 = -1;
            do{
                p10++;
                scarg /= 10.0;
                if(scarg < 1.0) break;
            }while(1);
            /* p10 is the int part of the log of arg if arg is >1, 0 if neg */
            dec = numwid-p10-2;
            if(dec == 0) numwid++; /*jeg9301 C does not print decimal point */
            if(dec < 0) dec = 0;   /*jeg9301 we will let big #s overflow */
            sprintf(fmt1+1,"%d.%df",numwid,dec);
            sprintf(cp,fmt1,arg);
        }else{
            dec = numwid-1;   /* jeg9302; we will try to get rid of leading 0*/
            sprintf(fmt1+1,"%d.%df",numwid,dec);
            sprintf(cp,fmt1,arg);
            if(*cp == '0'){   /* prints 0.---- */
                if(negflg) *cp = '-';
                ret=numbuff+1;
            }else{;}                   /* prints .---- OK */
        }
#ifdef FLDEBUG
        scrprintf("\narg=%f,scarg=%f,numwid=%d,dec=%d,p10=%d\n",arg,scarg,numwid,
                    dec,p10);
#endif
    }
    return ret;
}

        
void sfdotr()  /* float wid dec --- */
{
    int dec = pop;
    int wid = pop;
    float arg = fpop;
    cspush(fltstr(arg,wid,dec));
}
    
void sdotr()   /* int wid  --- */
{
    int wid = pop;
    int arg = pop;
    sprintf(fmt1+1,"%dd",wid);
    sprintf(numbuff,fmt1,arg);
    cspush(numbuff);       
}

void sspaces()  /* str n --- */  /* cats n spaces to end of str */
{
    register int n = pop;
    char *str = cppop;
    register char *cp = str;
    while(*cp++) continue; 
    cp--;  /* cp points to terminating null */
    while(n--) *cp++ = ' ';
    *cp = '\0';
}

/*
 * cats s2 onto s1 on the string stack and rets addr
 */
char *
scat(s1,s2)
char *s1;
char *s2;
{
    char *ret;
    
    ret = spush(s1);
    strcat(ret,s2);
    return(ret);
}

/************** EOL STUFF **************************************/

char *m_eol = EOL;
int m_leol = LEOL;

/************************ DICALLOC() **************************************/
/* creates a dictionary entry of name 'name' and allots n bytes of space
in Mirella dictionary. Name is a Cstring. This is useful in C code, and is
equivalent to 'create name n allot' in forth. */

char *dicalloc(name,n)
char *name;
int n;
{
    char fname[80];
    char *ret;
    
    strcpy(fname+1,name);
    fname[0] = strlen(name);
    qt_create(fname,DOVAR);
    ret = (char *)dp_4th;
    dp_4th += (n-1)/sizeof(normal) + 1;
    return (ret);
}

/********************* MISC NUMERIC ROUTINES **********************/

/* modern Unix-like systems have an internal math routine called round()
 * which takes a double and returns a double. This causes build problems;
 * There is a macro in mirella.h which #defines mround as `round', so this
 * should be transparent. This fn shd probably be a macro itself:
 * #define round(x) (int)(x>=0? x+0.5 : x-0.5)
 * but it must be exported to the Mirella environment as a function.
 */

normal
mround(f)
double f;
{
    normal ret;
    if(f >= 0.) ret = f + 0.5;
    else ret = f - 0.5;
    return ret;
} 

double exp10(f)
double f;
{   
    return( exp(f*2.302585093) );
}

/********************* _PAUSE() **************************************/

void _pause()   /* Mirella procedure--should be a primitive */
{
    if( dopause() ) erret((char *)NULL);
}

/********************** ELAPSED TIME ROUTINES *************************/
/* must have a system-specific macro or function gtime(), which returns
the (int) reading of a continuously running  millisecond clock. See
the system-specific mirsys.c. There is no mirella-level support as yet for 
timers which deliver interrupts or for interrupt code. */

/* number of timers and 'and' mask */
#define NTIMER 32
#define NTMM1 31

static unsigned normal t_start;
static unsigned oritimer[NTIMER];   /* can have up to 32 separate timers */
static int tpointer = 0;

#define TLIM 0x3fffffff  /* maximum 2^n-1 for reliable timer reading. 3
                            instead of 7 because some systems do not have
                            millisecond clocks and truncation causes conversion
                            difficulties. The wraparound time is about 300
                            hours and there is no glitch at wraparound */


int starttimer()    
/* 
 * starts a new timer whose index is returned and
 * must be used in all future references to that timer 
 * waits for return until gtime() has changed, for synchronization 
 * puposes; on DOS machines, the timer granularity is 1/18 sec, 
 * which is too gross for many things, but if we synch we can at 
 * least be clever.*/
{
    int ntime;
    int otime = gtime();
    while((ntime = gtime()) == otime){;}
    oritimer[tpointer&NTMM1] = ntime;
    return (tpointer++) ;
}

void waituntil(next,tp)   /* wait until the time is next milliseconds for
                                timer tp */
normal next;
normal tp;
{
    unsigned normal time,start;

    start = oritimer[tp&NTMM1];
    do{
        _pause();
        time = gtime();
    }while(((time - start) & TLIM) < next);
}                

normal ntimer(tp)   /* returns the value of timer tp */
int tp;
{
    return( (gtime() - oritimer[tp&NTMM1]) & TLIM );
}

void waitfor(t)  /* wait until t ms has passed. uses none of the
                    numbered timers */
normal t;            
{
    unsigned normal start,time;

    start = gtime() ;
    do{
        _pause();
        time = gtime();
    }while(((time - start) & TLIM) < t);
}

void startwatch()  /* start a special unnumbered timer whose reading is
                      thereafter returned by stopwatch() */
{
    t_start = gtime();
}

normal stopwatch()   /* startwatch() resets it; returns ms since reset */
{
    unsigned int time;
    
    time = gtime();
    return ((time - t_start) & TLIM);
}

int
_fgets(buf,n,fp)
char *buf;
int n;
FILE *fp;
{
    register int c;
    register char *cp = buf;
    
    if(n<=0) return 0;
    while(--n){
        *cp++ = (c = fgetc(fp));
        if(c == '\n' || c == EOF)break;
    }
    *cp++ = '\0';
    return( c == EOF ? 0 : (int)buf);
}


/************************ MORE MATH FUNCTIONS ******************************/

double asinh(y)
double y;
{
    if(fabs(y) > 1.e-3){
        return log(y + sqrt(1. + y*y));
    }else{
        return y*(1.-.1666666*y*y);
    }
}

double acosh(y)   /* positive branch */
double y;
{
    if(y>1.001){
        return log(y + sqrt(y*y - 1.));
    }else{
        return sqrt(2*(y - 1.));
    }
}

double atanh(y)
double y;
{
    if(fabs(y) > 1.e-3){
        return log((1. + y)/(1. - y));        
    }else{
        return y*(1. + 0.3333333*y*y);
    }
}

/*
 * character classification and modification functions--these have to
 * be functions, because the definitions are macros which one cannot
 * minstall; they also return forth booleans. Should make them
 * mirella procedures, but I do not think it matters.
 */
 
int m_toupper(int c)
{
    return toupper(c);
}

int m_tolower(int c)
{
    return tolower(c);
}


int m_isspace(int c)
{
    return isspace(c) ? -1 : 0;
}

int m_isalpha(int c)
{ 
    return isalpha(c) ? -1 : 0;
}

int m_isdigit(int c)
{
    return isdigit(c) ? -1 : 0;
}

int m_isalnum(int c)
{
    return isalnum(c) ? -1 : 0;
}

int m_islower(int c)
{
    return islower(c) ? -1 : 0;
}

int m_isupper(int c)
{
    return isupper(c) ? -1 : 0;
}

int m_iscntrl(int c)
{
    return iscntrl(c) ? -1 : 0;
}

char *strtoupper(char *str)  /* capitalizes a string in place */
{
    char *cp = str;
    char c;
    while((c = *cp) != '\0'){
        *cp = toupper(c);
        cp++;
    }
    return str;
}

char *strtolower(char *str)
{
    char *cp = str;
    char c;        
    while ((c = *cp) != '\0'){
        *cp = tolower(c);
        cp++;
    }
    return str;
}

char *strtocamelcase(char *str)
{
    char *cp = str;
    char *cp1;
    char *str1;
    char c;

    while(isspace(*cp)) cp++ ;   /* skip leading blanks */
    str1 = cp;
    c = toupper(*cp);
    *cp = c; /* capitalize first letter */

#if 1
    while((c = *cp) != 0){
        if(isspace(c)){
            /* if blank, capitalize next character */
            *(cp+1) = toupper(*(cp+1));
            /* if last letter before blank is upper case, replace ' ' by '_'*/
            if(isupper(*(cp - 1))){
                *cp = '_';
            /* otherwise just remove blank */
            }else{
                cp1 = cp + 1;
                while (*(cp1)){
                    *(cp1-1) = *cp1;
                    cp1++;
                }
                *(cp1-1) = '\0';
            }
        }
        cp++;
    }
#endif
    return str1;
}
                
           
/*********************** END **********************************************/
