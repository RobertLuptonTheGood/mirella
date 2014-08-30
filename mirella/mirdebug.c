/* VER 92/06/29 */
/******************** MIRDEBUG.C ****************************************/
/* Various simple debugging tools for Mirella C Code */
/* extensively revised jeg9207 */
/* again (added to) jeg9411 */

#include "mirella.h"


/* memchk() tests a region of memory for changes; on the first
invocation, it sets up a buffer and loads it with the contents of the
region from start to start+siz; on subsequent invocations, it ignores
its last two arguments and checks the original with the copy, reporting
any differences. Point is simply an index to tell the user which ivocation
is active; if there is no mismatch, no output, unless point is negative,
in which case ther is an OK message with each successful check. Point=0
resets memchk; other args must be present.
*/

#define SETUPDEBUG

#ifdef SETUPDEBUG
#define mprintf printf
#endif

void memchk(point,start,siz)
int point;
char *start;
int siz;
{
    static int firstime = 1;
    static char *sstart;
    static int ssiz;
    static char *chkptr = (char *)NULL;
    register char *cp;
    register char *cp1;
    register int i;
    int diff=0;

    if(firstime && point != 0) erret("\nMEMCHK: not initialized");
    if(point == 0){   /* reinitialize */
        firstime=1;
        if(chkptr != (char *)NULL){ /* with ANSI free(), could just call it */
            free((void *)chkptr);
        }
    }
    fflush(stdout);    
    if(firstime == 1){   /* allocate check buffer and set statics */
        if(!start || !siz) return;   /* just free buffer */
        chkptr = (char *)malloc(siz);
        if(!chkptr){
            scrprintf("\nMEMCHK:cannot allocate buffer");
            exit(-1);
        }else{
            bufcpy(chkptr,start,siz);
            sstart = start;
            ssiz = siz;
            mprintf("\nMEMCHK:chk buf at %xh(%u), chk reg %d by at %x(%d)",
                (unsigned int)chkptr,(unsigned int)chkptr,siz,
                (unsigned int)sstart,(unsigned int)sstart);
        }
        flushit();
    }
    /* if later, just compare */
    cp1 = sstart;
    cp = chkptr;
    for(i=0;i<ssiz;i++){
        if(*cp1++ != *cp++){
            diff++;
            if(diff < 5)mprintf(
                "\n/MEMCHK:diff at %8x : old=%2x new=%2x, chk,r,s = %x %x %d",
                (unsigned int)(sstart+i),(unsigned int)chkptr[i],
                (unsigned int)sstart[i],(unsigned int)chkptr,
                (unsigned int)sstart,ssiz);
            flushit();                
        }
    }
    if(diff > 0){
        if(firstime){
            mprintf("\nMEMCHK:Setup screwed up");
            flushit();
            exit(-1);
        }
        mprintf("\nMEMCHK:loc %d: %d mismatches\n",point,diff);
        flushit();
    }else{
        if(firstime){
            mprintf("\nMEMCHK:SETUP OK"); 
            flushit();    
        }else{
            if(point < 0){
                mprintf("\nMEMCHK:loc %d: %d compared:OK",point,ssiz);
                flushit();
            }
        }
    }
    firstime = 0;
    flushit();
}


/* this array is identical to faprt, but prints in both decimal and
hex, and is subject to the mprintf redefinition */

mirella void
dbprt(ia,n,titl)  /* prints integer array with title and numbers elements */
int *ia ;
int n ;
char *titl ;
{
    int i ;
    
    mprintf("\ninteger array %s of length %d\n",titl,n) ;
    for ( i=0 ; i < n ; i++ ) {
        if( i % 8 == 0) mprintf("\n %5d", i ) ;
        mprintf( "%8d " , ia[i]) ;
    }
    mprintf ( "\n" ) ;
    for ( i=0 ; i < n ; i++ ) {
        if( i % 8 == 0) mprintf("\n %5d", i ) ;
        mprintf( "%8x " , ia[i]) ;
    }
    pauseerret() ;
}

/* FIXME!!! -- newest compilers do NOT have VARARGS */
/*********************** DEBUG() ******************************************/
/* This routine provides printf()-like functionality which can be turned
 * on and off by the global m_dbmask. The bits in the first argument are
 * anded with m_dbmask, and if the and is nonzero, the print statement
 * is executed. The output also goes to a file, debug.log, which is 
 * appended to on each occurence.
 */
 
static char debug_string[256];
int m_dbmask;

#ifdef VARARGS
/* va_arg version..should work on all 'new' compilers..uses vsprintf() */

#include <varargs.h>

void 
DEBUG(va_alist)
va_dcl
{
    va_list ap;
    int mask;
    char *fmt;
    int j; 
    FILE *fp;
   
    if(!(mask & m_dbmask)) return;
    va_start(ap);
    mask = va_arg(ap,int);
    fmt = va_arg(ap,char*);
    vsprintf(debug_string,fmt,ap);
    va_end(ap);

    cprint(debug_string);

    fp = fopen("debug.log","a");
    if(!fp){
        mprintf("\nCannot open debugging log file");
        erret(0);
    }
    fprintf(fp,debug_string);
    fclose(fp);
    clearn(256,debug_string);
    return;
}

#else

/* old version for systems without varargs facility or vsprintf;
assumes arguments are stacked contiguously on stack, in the direction SDIR */

#define SDIR +
/* direction of stack growth; one or the other of + or - will work
for most machines unless the compiler is clever enough to pass the first
few arguments of function calls in registers, or has 24-bit pointers, or
something else weird. */

void
DEBUG(mask,str,args)   /* this version is nice if it works, but portable it
                        is emphatically not */
int mask;                        
char *str;                        
normal args;
{
    normal *aptr = (&args);
    FILE *fp;

    if(!(mask & m_dbmask)) return;
    sprintf(debug_string,str,
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
    
    cprint(debug_string); /* goes through 'emit()' and flushes */
    
    fp = fopen("debug.log","a");
    if(!fp){
        mprintf("\nCannot open debugging log file");
        erret(0);
    }
    fprintf(fp,"%s",debug_string);
    fclose(fp);
    clearn(256,debug_string);
    return;
}

#endif

/* misc junk functions */
       
/*debug*/
void CFILE()
{
    FILE *fpx;
    
    scrprintf("\nCfile--");
    fflush(stdout);     
    fpx = fopen("c:/mirella/minit.x","r");   
    if(!fpx){                                
        scrprintf("\nNOCANOPEN");
        exit(-1);               
    }else{                    
        fclose(fpx);          
        scrprintf("closed");     
        fflush(stdout);       
    }
}
/*enddebug*/                     
                   


