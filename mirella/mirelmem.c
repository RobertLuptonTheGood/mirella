/*VERSION 90/05/28: Mirella 5.50                                         */
/************************ MIRELMEM.C *************************************/

/* Mirella memory allocation and initialization code */

/* recent history:
 * 88/08/20: Added code to deal with crazy CONVEX virtual-addressing
 *  scheme, which starts at 0x8000000 (usually).
 * 88/08/31: Changed (with some trepidation) the memory-inititialization
 * scheme which Mirella uses. The dictionary (user area, actually) now
 * starts on the first 64K boundary above the C code and data area;
 * the user is warned if that has moved since the last dictionary
 * image is made when he tries to load the image. Scrapped m_malloc
 * code.
 * 89/05/20: got rid of some 'erret()'s in the free code; system no
 * longer sends the user back to the OI if she tries to free nonallocated
 * memory.
 * 89/06/10: changed memalloc so 0 pointer does same thing as ptr to 0
 * 89/08/13: Fixed so can use sbrk() mechanism or malloc() mechanism
 * as appropriate.
 * 90/05/28: Incorporated changes in command-line processing so if a 
 * dict image is read with a larger origin than default, it uses the image
 * origin.    
 * 06/04/02: beginning of stuff for 3d and 4d matrices; does not
 * reconstruct them from reread dictionary properly, but who cares???

The Mirella memory manager is an interface to the library malloc() to
be used within Mirella and supporting C code.  The memory allocated is
described by an array of structures in the MEM_AREA just following the
user area, which are of the form

 struct memarea{
    char   *mpointer;         memstruct ptr; CONTENTS is pointer to origin
    normal mlength;           #bytes
    short  melsize;           element size in bytes
    short  msiz1;             # of largest elements in matrix (rows for 2d)
    short  msiz2;             # of next ""
    short  msiz3;             # of next ""
    char mname[16];           name truncated to 15 char + null
 }; 

This structure describes the pointer topology uniquely for 
4-d matrices or simpler.

The origin of the array is in the user variable V_MORIG (Mirella mp0) 
and the size of the structure (32 bytes) in the user variable V_MSSIZE
(Mirella mssize). The allocation is done via the C function memchan = 
memalloc(name,size) or the Mirella procedure name size _memalloc(), which
pushes memchan. In both cases memchan is the pointer to the memorigin
pointer in the assigned structure member, which is coincident with
the pointer to the structure; this is useful. The double indirection is
necessary in the case of Mirella code to avoid compiling volatile 
C addresses into the dictionary; in C code, the user is, of course, at
liberty to do the indirection once into a variable. There are two 
free mechanisms, in analogy to the two close mechanisms for the Mirella
file system. The memory can be freed by name, memfree(name) (C) 
name _memfree or memfree name  (Mirella) or by CHANNEL address chfree(memchan)
(C) or memchan chfree (Mirella). The chfree() function will take either a
channel NUMBER or a channel ADDRESS; the function decides which by
checking range. The limit on the name length is 14 characters,
as usual. In Mirella, the channels in use are displayed by showalloc. At
the moment, 40 channels can be open at any one time; this can be changed
by changing MEM_AREA in mirella.h and recompiling the world (actually only
mirella.c and this module).

WARNING WARNING WARNING  This code has NOT been initialized at the
time applinit() is executed in the startup procedure. DO NOT,
THEREFORE, use the memalloc() functions in applinit(). 

There is a function matrix(), which allocates matrices via malloc()
and sets up pointer arrays, which can be used in applinit(); in general,
use matralloc(), which uses the memalloc() mechanism.

There is also a function tryalloc(), which simply is malloc with an
erret() return with message in case of failure.
*/

#define mem_t  struct memarea		/* before #include "mirdecl.h" */

#ifdef VMS
#include mirkern
#include mireldec
#else
#include "mirkern.h"
#include "mireldec.h"
#endif

/* memory management system variables, defined for DSI-DOS */
#ifdef DSI32
char *heaphigh = (char *)0x2024;
char *heaplow  = (char *)0x2020;
char *heapnext = (char *)0x371f;
#else
char *heaphigh = 0;
char *heaplow  = 0;
char *heapnext = 0;
#endif

char *c_endmem;   /* mirella variable heaporigin; harmless echo of endmem */
char *c_endprog;  /* mirella variable endprogram; harmless echo of 'try' */

struct memarea{
    char  *mpointer;    /* memstruct pointer; memory ptr is contents of this */
    normal mlength;     /* bytes */
    short  melsize;     /* size in bytes of elements if array */
    short  msiz1;       /* # of largest elements in matrix (rows for 2d) */
    short  msiz2;       /* # of next "" */
    short  msiz3;       /* # of next "" */
    char   mname[16];   /* name truncated to 15 chars + null */
}; 

normal sz_memarea = sizeof(struct memarea);
normal nmchan = MEM_AREA*sizeof(normal)/sizeof(struct memarea);

static mem_t *memorig ;
static mem_t *find_memchan();

extern normal *m_oldorigin; /* dictionary image origin */
extern int verbose;


/*#define DEBUG*/
/*#define MALDEBUG*/

#define MQUANTUM  0x10000		/* 64 K */

#ifdef MIRSBRK				/* use sbrk */
/*********************** INIT_MALLOC ************************************/
/*
 * these routines make use of brk and sbrk to allocate the
 * dictionary space, and will work on some systems; for
 * systems in which malloc() and sbrk() cannot get along, use
 * the code in the '#else' section below.
 *
 * If at all possible use the other code! Many mallocs are subtly unhappy
 * with you if you use sbrk() (e.g. sun o/s 4.1.2)
 */
#define MOFFSET   1024
/* a little headroom */

static char *hiwater = 0;

char *
init_malloc(needed)
normal needed;
{
    char *hw, *hwm;
    normal nalloc;
    char *usorigin ;
    
    hw = sbrk(0);
    hiwater = (hw > hiwater ? hw : hiwater );    
    hw = hiwater;
    usorigin = (char *)((((unsigned)hw+MOFFSET)/MQUANTUM + 1) * MQUANTUM);  
        /* next 64K bdy */

    /* if reading dictionary image and image has bigger origin, use it */
    if((char *)m_oldorigin > usorigin) usorigin = (char *)m_oldorigin;        

    nalloc = needed + usorigin - hw + 1024;
    hwm = (char *)sbrk(nalloc);
    if(usorigin - hw - MOFFSET < 2048) 
        printf("\nMemory headroom only %d bytes", usorigin-hw-MOFFSET);
    if(hw == (char *)(-1) || hwm == (char *)(-1)){
        printf("\nCannot allocate memory for dictionary");
        exit(1);
    }
    if(verbose)
      printf("\nC code hw: %d, new origin: %d, allocated: %d, new hw: %d",
	     (int)hw,(int)usorigin,nalloc,(int)sbrk(0) );
    return usorigin;
}

Void *
m_sbrk(size)
int size;
{
    return (Void *)sbrk(size);
}

#else   /*use malloc and pray*/

/******************* DSI 68020 ROUTINES *********************************/
/* processor IS DSI68 -- punt */
/* these routines make no use of sbrk at all, but assume that malloc
allocates large chunks of memory from close to the high-water mark....
they might work fine for VMS as well, but will not work for at least
Berkeley unix. They do not work for non-obvious reasons for pharlap */

#define MOFFSET 0x800			/* overhead for malloc's perfidies */
#define FUDGE 0x1000			/* ditto for the end */
#define NTRY 20				/* n of attempts to alloc dictionary */

static char *hiwater = NULL;

char *
init_malloc(needed)
/* do the best we can with malloc */
normal needed;
{
#if 1
   return(malloc(needed));
#else
    char *hw, *hwm = NULL;
    int i;
    normal nalloc;
    char *usorigin ;
    
    hw = (char *)sbrk(0);
    hiwater = (hw > hiwater ? hw : hiwater );    
    hw = hiwater;
    usorigin = (char *)((((unsigned)hw+MOFFSET)/MQUANTUM + 1) * MQUANTUM);  
        /* next 64K bdy */

    /* if reading dictionary image and image has bigger origin, use it */
    if((char *)m_oldorigin > usorigin) usorigin = (char *)m_oldorigin;

    nalloc = needed + usorigin - hw + FUDGE;
    for(i = 0;i < NTRY;i++) {
       hwm = (char *)malloc(nalloc);
       if(hwm + nalloc >= usorigin + needed) {
	  break;
       }
       free((char *)hwm);
       nalloc += FUDGE;
       if(verbose) {
	  printf("\nIncreasing nalloc");
       }
    }
    if(i == NTRY) {
       printf("\nDictionary Underallocated. Increase constant FUDGE\n");
       exit(1);
    }
    
    if(hwm > usorigin){
        printf(
  "\nDSI68 malloc ruse failed; try incr. MOFFSET in mirelmem and recompile");
        printf("\nusorigin = 0x%x; origin after malloc= 0x%x\n",
	       (int)usorigin,(int)hwm);
        exit(1);
    }
    if(usorigin - hw - MOFFSET < 2048) 
        printf("\nMemory headroom only %d bytes", usorigin-hw-MOFFSET);
    if(hw == (char *)(-1) || hwm == (char *)(-1)){
        printf("\nCannot allocate memory for dictionary");
        exit(1);
    }

    if(verbose) {
       printf("\nC code hw: 0x%x, new origin: 0x%x, alloc: %d, new hw: 0x%x",
	      (int)hw,(int)usorigin,nalloc,(int)(hwm + nalloc + 4));
    }
    return usorigin;
#endif
}

/*
 * because sbrk does not work at all with malloc on the DSI68, that's why
 */
Void *
m_sbrk(size)
normal size;
{
   char *ptr = malloc(size);
   
   hiwater = (ptr + size > hiwater ? ptr + size : hiwater );
   if(ptr == NULL) return((Void *)(-1));
   return((Void *)ptr);
}
        
/*************************  END DSI68 ROUTINES *************************/    
#endif  /* MIRSBRK */


/* End of dictionary allocation stuff. Next routines are Mirella interface
to library malloc */

/************************ INIT_MEM() ************************************/

/*  NB!!!!!!!!!  this does NOT do the correct thing for matrices more
 *  complex than 2d--but it WILL fail with an error */

void init_mem() /* attempts to reopen any channels open in dictionary image */
{
    int i,j;
    char * ret;
    mem_t *mp;
    int nrows, ncols;

    memorig = (mem_t *)mp_4th;
#ifdef DEBUG
    fprintf(stderr,"\nSetting memalloc origin = %d",memorig);
    flushit();
#endif
    if(!dict_image) return;
    /* attempt to reallocate previously open channels */
    for(i=0; i<nmchan; i++){
        mp = memorig + i;
        if(mp->mname[0] != '\0'){	/* prev. open channel */
            if(!strncmp(mp->mname,"graph_",6) || mp->msiz2 != 0){
                mp->mlength = 0;
                mp->mname[0] = '\0';
                mp->mpointer = 0;
                mp->melsize = 0;
                mp->msiz1 = 0;                
                mp->msiz2 = 0;
                mp->msiz3 = 0;
                if( mp->msiz2 != 0){
                    sprintf(out_string,
                        "   Failed; cannot yet recreate 3d or 4d matrices");
                    cprint(out_string);
                }
                continue;
            }
                /* do not reopen graphs */
            sprintf(out_string,"\nAttempting to allocate %6d bytes as %s ",
            mp->mlength,mp->mname);
            cprint(out_string);
            ret = (char *)malloc(mp->mlength);
            mp->mpointer = ret;
            if(ret){
                sprintf(out_string,"  pointer %d",(int)ret);
                cprint(out_string);
                if((nrows = mp->msiz1) != 0){
                    ncols = (mp->mlength - nrows*sizeof(char *))/
                        (nrows*(mp->melsize));
                    if(ncols <= 0){
                        sprintf(out_string,"  matrix dim. error");
                        mp->mlength = 0;
                        mp->mname[0] = '\0';
                        mp->mpointer = 0;
                        mp->melsize = 0;
                        mp->msiz1 = 0;
                        mp->msiz2 = 0;
                        mp->msiz3 = 0;
                    }else for(j=0;j<nrows;j++){  
                        /* set up pointers*/ 
                        *((char **)(mp->mpointer) + j) = 
                            ((char *)(mp->mpointer) + nrows*sizeof(char *)
                             + j*ncols*(mp->melsize));
                    }
                }
            }else{
                sprintf(out_string,"   failed");
                cprint(out_string);
                mp->mlength = 0;
                mp->mname[0] = '\0';
                mp->melsize = 0;
                mp->msiz1 = 0;
                mp->msiz2 = 0;
                mp->msiz3 = 0;
            }
        }
    }
    V_NR_OUT = 1;
}

/********************* MEMALLOC(), _MEMALLOC() ****************************/

int prefreeflg = 1;     /* if set to 1, frees existing channel and tells you.
                          if set != 1, frees but does NOT tell you (beware)
                          if cleared,  does not free */
char *
memalloc(name,size)    /* allocates heap area of size size, clears. Returns
                        a pointer to the memory array pointer, NOT to the
                        allocated memory; the allocated memory pointer is the
                        CONTENTS of the returned quantity */
    normal size;
    char * name;
{
    register char *ptr,*pend;
    mem_t *mp;

    /* see if name is already in the channel pool and free it, unless
        name is NULL or ""  */
    if( name != (char *)0 && strlen(name) != 0 && 
            (mp = find_memchan(name)) != NULL ) {  
        /* channel exists*/
        if(prefreeflg == 0){  /* do not free */
            fprintf(stderr,"\nMemory channel %s exists. Duplicating", name);
        }else{   /* free it */
            if(prefreeflg == 1){  /* and tell you */
                fprintf(stderr,"\nFreeing memory channel %s",name);     
            }
        }
        chfree((Void *)(&mp->mpointer)); 
    }
    /* whether or not, make a new one */
    if((mp = find_memchan((char *)NULL)) == NULL){
        fprintf(stderr,"Too many memory channels open");
        return 0;
    }
    /* have one now */
    ptr = (char *)malloc(size);
#ifdef MALDEBUG
    fprintf(stderr," ptr, size, name = 0x%x %d  %s",ptr,size,name);
#endif
    if(ptr == (char *)NULL){
        fprintf(stderr,"\nCannot allocate memory");
	*mp->mname = '\0';
        return 0;
    }
    if(name != NULL && *name != '\0') {
       strncpy(mp->mname,name,15);
    }
    mp->mlength = size;
    mp->mpointer = ptr;
    mp->melsize = 0;
    mp->msiz1 = 0;
    mp->msiz2 = 0;
    mp->msiz3 = 0;   /* make zero; matrix code will fill in if necessary */
    pend = ptr + size;
    while(ptr < pend) *ptr++ = '\0';
    return (char *)(&(mp->mpointer));
}

void _memalloc()   /* Mirella procedure */
{
    normal size = pop;
    char *name = cspop;
    char *ret ;

    ret = memalloc(name,size);
#ifdef MALDEBUG
    fprintf(stderr," passing name, size = %s,%d",name,size);
#endif 
    if( ret == (char *)NULL ) erret((char *)NULL);
    push(ret);
}

/*****************************************************************************/
/*
 * Return a pointer to a memory channel, NULL if none.  If name is
 * non-NULL a pointer to a pre-existing channel called name is
 * returned, otherwise a pointer to a new channel
 */
static mem_t *
find_memchan(name)
char *name;
{
   int i;
   mem_t *mp;
   
   for(i = 0;i < nmchan;i++){
      mp = &memorig[i];
      if(name == NULL) {
	 if(mp->mname[0] == '\0') {	/* an empty slot */
	    sprintf(mp->mname,"t%d",i);
	    return(mp);
	 }
      } else if(strcmp(name,mp->mname) == 0) { /* found it */
	 return(mp);
      }
   }
   return(NULL);			/* none left, or didn't find it */
}

/******************** MATRALLOC(), _MATRALLOC(), MATRIX() ********************/
/*
 * allocates for matrix of elements of size elsize. Again, returns memory
 * array pointer, whose contents are the pointer to the matrix line
 * pointer array
 */
Void *
matralloc(name,rows,cols,elsize)
char *name;
int rows,cols;
int elsize;
{
    int needed = rows*sizeof(char *) + cols*rows*elsize;
    char *ret = (char *)memalloc(name,needed);
    int j;
    mem_t *mp;

    if((mp = (mem_t *)ret) != NULL) {      /* successful allocation */
        mp->msiz1 = rows;
        mp->melsize = elsize;
        for(j=0;j<rows;j++){  /* set up pointers; mtrx */
            *((char **)(mp->mpointer) + j) = 
              (char *)(mp->mpointer) + rows*sizeof(char *) + j*cols*elsize;
        }
    }
    return ret;
}

/* mirella procedure--this is basically useless, because there is
 * currently no way to make a corresponding dictionary entry with
 * the Forth matrix code attached; we need to write some.
 */
void _matralloc()  /* mirella procedure */
{
    char *name = cspop;
    normal size = pop;
    normal cols = pop;
    normal rows = pop;
    char *ret ;

    ret = matralloc(name,rows,cols,size);
    if(ret == NULL) erret((char *)NULL);
    push(ret);
}


Void *
mat3alloc(name,nmat,rows,cols,elsize)
char *name;
int nmat,rows,cols;
int elsize;
{
    int needed = (rows*sizeof(char *) + cols*rows*elsize)*nmat 
        + nmat*sizeof(char**);
    char *ret = (char *)memalloc(name,needed);
    int j,i;
    mem_t *mp;

    if((mp = (mem_t *)ret) != NULL) {      /* successful allocation */
        /* note mp->mpointer is ret */
        mp->msiz1 = nmat;
        mp->msiz2 = rows;
        mp->melsize = elsize;
        for(i=0;i<nmat;i++){ /* set up pointers to 2d matrices */
            *((char ***)(mp->mpointer) + i) =
                (char **)(mp->mpointer) + nmat + i*rows ;
            for(j=0;j<rows;j++){                
                *((char **)(mp->mpointer) + nmat + i*rows + j) = 
                (char *)(mp->mpointer) 
                + nmat*sizeof(char **)     /* skip over matrix ptrs */
                + rows*nmat*sizeof(char *) /* skip over row ptrs */
                + i*rows*cols*elsize       /* skip over previous mats */
                + j*cols*elsize;           /* to the jth row pointer mat i */
            }
        }
    }
    return ret;
}

/* mirella procedure--this is basically useless, because there is
 * currently no way to make a corresponding dictionary entry with
 * the Forth matrix code attached; we need to write some.
 */
void _mat3alloc()  
{
    char *name = cspop;
    normal size = pop;
    normal cols = pop;
    normal rows = pop;
    normal nmat = pop;    
    char *ret ;

    ret = mat3alloc(name,nmat,rows,cols,size);
    if(ret == NULL) erret((char *)NULL);
    push(ret);
}

/* following routine does not use memalloc() mechanism, but uses malloc()
and is suitable for permanent matrices ( and those created in the 
initialization code) and ones freed in the calling function */

char **matrix(nr,nc,elsize)  /* allocates memory and returns pointer */
    int nr;         /* rows */
    int nc;         /* columns */
    int elsize;     /* size of elements in bytes */
{
    register int i;
    char **rp;
    char *a;
    int nby = nr*(nc*elsize + sizeof(char *));

    if((rp = (char **)malloc(nby)) == NULL) {
        printf("\nMatrix:cannot allocate %d bytes of memory",nby) ;
        erret((char *)NULL);
    }
    a = (char *)rp;
    for(i=0;i<nr;i++){
        rp[i] = a + nr*(sizeof(char *)) + i*nc*elsize;
    }
    return rp;
    /* element is rp[i][j] */
}

Void ***
matrix3(nmat,rows,cols,elsize)
int nmat,rows,cols;
int elsize;
{
    int needed = (rows*sizeof(char *) + cols*rows*elsize)*nmat 
        + nmat*sizeof(char**);
    char *ret = (char *)malloc(needed);
    int j,i;

    if(ret != NULL) {      /* successful allocation */
        for(i=0;i<nmat;i++){ /* set up pointers to 2d matrices */
            *((char ***)ret + i) =
                (char **)ret + nmat + i*rows ;
            for(j=0;j<rows;j++){                
                *((char **)ret + nmat + i*rows + j) = 
                (char *)ret 
                + nmat*sizeof(char **)     /* skip over matrix ptrs */
                + rows*nmat*sizeof(char *) /* skip over row ptrs */
                + i*rows*cols*elsize       /* skip over previous mats */
                + j*cols*elsize;           /* to the jth row pointer mat i */
            }
        }
    }else{
        printf("\nMatrix3:cannot allocate %d bytes of memory",needed);
        erret((char *)NULL);
    }
    return (Void ***)ret;
}

/********************** CHFREE(), _CHFREE() ******************************/
/*
 * careful--makes use of numerical coincidence of memory
 * structure pointer and pointer to its pointer member
 * rev jeg0601
 */
int
chfree(v_mch)
Void *v_mch;
{
    mem_t *mch = (mem_t *)v_mch;

    if((unsigned)mch < nmchan){  /* probably a channel number */
        if((memorig+(unsigned)mch)->mpointer == 0){
            fprintf(stderr,"Memchannel %d not open",(unsigned)mch); 
            fflush(stderr);
            return 0;
        }else mch = (memorig+(normal)mch);
    }else if(mch < memorig || mch >= memorig+nmchan || *mch->mname == '\0'){
        fprintf(stderr,"Memchannel %ld not open",
		(long)(mch-memorig)/sizeof(mem_t));
        fflush(stderr);
        return 0;
    }
    /* got one */
    free(mch->mpointer);
    mch->mpointer = 0;
    mch->mlength = 0;
    mch->melsize = 0;
    mch->msiz1 = 0;
    mch->msiz2 = 0;
    mch->msiz3 = 0;
    *mch->mname = '\0';
    return 1;
}

void _chfree()    /* Mirella procedure, name chfree */
{
    Void *mch = (Void *)pop;

    chfree(mch);
}

/********************** MEMFREE(), _MEMFREE() ****************************/
static int
imemfree(name,v)
char *name;
int v;	/* be chatty? if 1, tells you no channel, if 2 tells you if there IS */
{
   mem_t *mp;

   if((mp = find_memchan(name)) == NULL) {
      if(v == 1) {
	 fprintf(stderr,"\nNo open memory channel named %s",name);
      }
      return 0;
   }else{
      if(v == 2) {
         fprintf(stderr,"\n Freeing existing channel named %s",name);
      }
      return chfree((Void *)(&mp->mpointer));
   }
}

int
memfree(name)
char *name;
{
   return(imemfree(name,1));
}

int
qmemfree(name)
char *name;
{
   return(imemfree(name,0));
}

void
_memfree()  /* Mirella procedure, name memfree */
{
   char *name = cspop;

   memfree(name);
}

void freeall()  /* Mirella procedure */
{
    int i;
    mem_t *mch;

    for(i=0;i<nmchan;i++){
        mch = memorig + nmchan -1 - i;
        if(mch->mname[0] != '\0'){
            free(mch->mpointer);
            mch->mpointer = 0;
            mch->mlength = 0;
            *mch->mname = '\0';
        }
    }
}

/************************* SHOWALLOC() ********************************/

void showalloc()
{
    int i;
    mem_t *mp;
    int found = 0;
#ifdef DEBUG
        fprintf(stderr,"\norigin,nmchan %d %d",memorig,nmchan);
#endif

    fprintf(stderr,
"\n ch        &ptr     pointer      length esiz siz1 siz2 siz3  name");
    for(i=0; i<nmchan; i++){
        mp = memorig+i;
        if(mp->mname[0] != '\0'){
            fprintf(stderr,
                "\n%3d %11u %11u %11u %4d %4d %4d %4d  %-15s", 
                i,(int)&mp->mpointer,(int)mp->mpointer,mp->mlength,mp->melsize,
                mp->msiz1, mp->msiz2, mp->msiz3, mp->mname);
            found++;
        }
    }
    if(!found) fprintf(stderr,"\nNo allocated heap areas");
    V_NR_OUT = 1;  /* flag for cr if not MIREDIT */
    fflush(stderr);
}
/*********************** TRYALLOC() ********************************/
/* for C code; just malloc with erret() return */

char *tryalloc(nbytes)
normal nbytes;
{
    char * ret;
    if((ret = (char *)malloc(nbytes)) == NULL) {
        printf("\nCannot allocate %d bytes",nbytes);
        erret((char *)NULL);
    }
    return ret;
}
