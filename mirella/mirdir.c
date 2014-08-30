/*VERSION 89/12/20 Mirella 5.30 */
/****************** DIRECTORY SEARCH CODE FOR MIRELLA *******************/
/* 
 * 89/04/28:    Added directory stack code
 * 89/07/15:    Added backup code
 * 89/10        Modified for PL386  (DOS32 definitions)
 * 89/11/16:    Fixed trouble with DOS matches when directory is
 *                  supplied, in gdopen();
 * 89/12/10:    Debugging fixes to writeascii(), fixed so can create
 *                  ansi text files from stream files in restore, and
 *                  made big buffer size variable.
 * 89/12/16:    Made filename on tape canonically of unix form; and made
 *                  showdirs show in lowercase; converts DOS fnames to unix
 *                  ones. Fixed(?) VMS backstream problem.    
 * 90/05/05:    Fixed isasciibuf so tabs, bs, and ff are counted ascii chars
 * TODO:Make sure
 *      everything works when crossing bigblock boundaries. 
 *      Write unixtovms, and incorporate in dsrestore().
 *      Make tree descender.
 */

/* this module contains much system-dependent code */

#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif

#if defined(_POSIX_SOURCE)
#  include <sys/types.h>		/* definition of mkdir */
#  include <sys/stat.h>
#endif

static char *sspat[10];     /* pointers to substrings in pattern */
static u_char lsspat[10];   /* lengths of substrings in pattern */
static int mirmode();
static int mwildmat();
static int nsspat=0;        /* number of substrings in pattern */ 
static char *strfind();
static char stpat[64];      /* static copy of pattern */

#ifdef VMS
#define matcase   toupper
#endif
#ifdef unix
#define matcase(x)  (x)
#endif
#ifdef DOS32
#define matcase   toupper  
#endif
/* watch this!!! toupper is a macro in bk unix and is defined only on 
the lower-case ascii letters !!! */

int m_direrror = 0;    /* flag for directory function failure */

/************************ MATSTR() ****************************************/

static char *   /* converts s to matching case for opsys */
matstr(s)
register char *s;
{
    char *org = s;
    register int c;
    while((c = *s) != '\0'){
        *s++ = matcase(c);
    }
    return(org);
}

/************************ FILENAME CONVERSION UTILITIES ********************/
/* The mirella canonical filename system is a compromise. the directory
separator is '/', but devices in VMS and DOS still retain their ':'. */


/************************ FSTRTOLOWER() *************************************/
/* makes lower-case and converts DOS backslashes to slash; it should be
called 'dostounix' but it also harmlessly converts VMS filenames to
lower case, leaving the VMS diacriticals, so we call it this. Note
that there is a general-purpose strtolower function in mircintf.c */

static char scrfspec[80];

char *
fstrtolower(s)     
register char *s;
{
    register char *d = scrfspec;
    register char c;

    while((c = *s++) != '\0'){
        if(c == '\\') *d++ = '/';
        else *d++ = tolower(c);
    }
    *d++ = '\0';
    return(scrfspec);
}        

/************************ VMSTOUNIX() *************************************/
#ifdef VMS

/* converts VMS-type filespecs to 'unix' ones--actually to the Mirella
canonical form, in which explicit devices (as in VMS and DOS) are followed
by a ':'.  We do this for convenience of backup and restore to the same
machine and filesystem; the devices must be bypassed anyhow if going to
another machine, especially one with a different opsys.  If s is a directory
name ending in a  ']', result is a unix directory PREFIX ending
in '/'; root is NOT placed at the beginning, so the translation is only
relative to the highest-level directory in the pathname.  Relative
pathnames are correctly 'translated' but are usually (or often)
meaningless--thus [-]filename is xlated to ../filename, but is only
correct if the directory tree structure is preserved from one system to
the other.  The unix name comes out lowercase.  */

static char *
vmstounix(si)
char *si;
{
    char fspec[80];                 /* VMS name with logicals translated */
    register char *s = fspec; 
    register char *d = scrfspec;    /* output 'unix' filename */
    register char c;
    char *orig = s;
    int indir = 0;
    char *cp;
    char *trnlog();
    
    /* look for ':'; translate prefix if poss */
    strcpy(scrfspec,si);
    cp = rindex(scrfspec,':');
    if(cp){
        *cp = '\0';              /* kill the ':' */
        strcpy(fspec,trnlog(scrfspec));  /*try to xlate it and move to fspec*/
        strcat(fspec,cp+1);              /* fspec now has translated name */
    }else{                       /* no ':' */
        strcpy(fspec,si);        /* just copy it */
    }
        
    while( c = *s++){
        switch(c){
        case ':':
            *d++ = ':';     /* just copy it: dev:[usr]-> dev:/usr/ */
            break;
        case '[':            
            if(d > scrfspec) *d++ = '/';  /* leave out '/' if first char */
            indir = 1;
            break;
        case ']':
            *d++ = '/';
            indir = 0;
            break;
        case '.':
            if(indir) *d++ = '/';   /* dir separator */
            else *d++ = c;          /* extension separator */    
            break;
        case '-':
            if(!indir) goto errout;
            *d++ = '.';
            *d++ = '.';
            break;
        default:
            *d++ = tolower(c);
            break;
        }
    }
    *d++ = '\0';
    if(indir) scrprintf("\nVMSTOUNIX: Error in directory name in %s",orig);
    return scrfspec;
errout:
    *d++ = '\0';
    scrprintf("\nVMSTOUNIX: Error in directory name in %s",orig);
    return scrfspec;
}

#endif  /*VMS*/

/************************* FCANON() ****************************************/
/*
 * converts filenames to unix form; if dos, does a fstrtolower;
 * if vms, a vmstounix; if unix, nothing
 */
char *
fcanon(s)
char *s;
{
#ifdef VMS
    return vmstounix(s);
#endif
#ifdef DOS32
    return fstrtolower(s);
#endif
#ifdef unix
    return s;
#endif
}        

/************* DIRECTORY UTILITIES *****************************/
/* moved here from mircintf.c 0309 */

static char dirstring[100]; 
/* 
 * XXXXXXXXXXXXXX  check; these were moved to mirdir.c in doswin
 * version--do we need to do this here??
 */
 
void
m_getcwd()
{
    m_direrror=0;
    if(!getcwd(dirstring,100)){
        m_direrror = 1;
        dirstring[0] = '\0';
        error("Error in getting cwd");
    }
    cspush(fcanon(dirstring));
    
}

void m_chdir()
{
    char *dir = cspop;

    m_direrror = 0;
    if(chdir(dir) == -1){
        scrprintf("\nCannot change directory to %s",dir);
        m_direrror = 2;
    }
}

void m_mkdir()
{
    char *dir = cspop;

    m_direrror = 0;
    if(mkdir(dir,0777) == -1){
        scrprintf("\nCannot make directory %s",dir);
        m_direrror = 3;
    }
}

void m_rmdir()
{
    char *dir = cspop;

    m_direrror = 0;
    if(rmdir(dir) == -1){
        scrprintf("\nCannot remove directory",dir);
        m_direrror = 4;
    }
}
       
/************************* WILDMATCH() *************************************/

/*#define DEBUG*/

/*
 * does str match pat (with embedded '*' and '?') ? if pat == NULL, uses last
 * pattern supplied. (pat must be compiled, so it is more efficient to keep
 * track) The pattern strings are acted on by matcase(); the operating system
 * typically returns filenames either case-sensitively (unix) or in all caps
 * (DOS,VMS)
 */
int
wildmatch(str,pat)
char *str, *pat;                          
{
    register char *spat;
    register int c;
    register char *uc;
    char *osspat;
    int i;
    char *sp,*osp;
    
    if(pat && *pat){     /* compile pattern; if pat is 0 or points to a null
                                string, use existing pattern */
        spat = pat;
        uc = stpat;
        /* copy to stpat, operating with matcase() */
        while( c = *spat++,(*uc++ = matcase(c)) ) continue;
        spat = stpat;
        nsspat = 0;
        osspat = spat;
        do{
            if((c = *spat) == '*'|| c == '\0'){
                sspat[nsspat] = osspat;
                lsspat[nsspat] = spat - osspat;
                osspat = spat+1;    /* origin of next substring */
                *spat = '\0';
                nsspat++;
            }
            spat++;
        }while( c );
        /* at this point, have replaced all *'s with nulls, and have
           pointers to all substrings, contented or not. If a beginning
           *, have a null first substring; if a terminating *, have a 
           null final substring. */
    }
#ifdef DEBUG
    scrprintf("\n%d substrings in %s:",nsspat,pat); 
    for(i=0;i<nsspat;i++) printf("\n%4d:%s: len=%d",i,sspat[i],lsspat[i]);
    scrprintf("\n"); fflush(stdout);
#endif        
    if(nsspat == 0){
        scrprintf("\nWILDMATCH: No Pattern");
        return(0);
    }
    /* parse the string */
    osp = str;
    for(i=0;i<nsspat;i++){
        sp = strfind(sspat[i],osp);  /* returns ptr to next char after 
                                        first occurrence of sspat[i] in osp */
#ifdef DEBUG
        if(sp) scrprintf("\n%2d:sp=%d:%s",i,sp,sp);
#endif
        if(!sp) return(0);  /* no match */
        /* if first substring is contented, match must be with beginning
        of str; if not, fail */
        if(i==0 && *sspat[0] && sp != str+lsspat[0]){
            return(0);
        }
        osp = sp;
    }
    /* have matched all substrings; if either last pattern char was
    a '*' (last substring null) or pointer points to a null (no more
    chars in string) we are done and have a match; not if not */
    if(*sspat[nsspat-1] == '\0' || *sp == '\0') return (1);
    else return (0);
}

/******************** STRFIND() ******************************************/
/*
 * finds the first occurrence of subs in s, rets ptr to first char following
 */
static char
*strfind(subs,s)
char *subs;
register char *s;
{
    register int c,cp1;        
    char *ps,*pss;
    int cs;
    
    cp1 = *subs++ ;         /* subs now points to second char */
    if(!cp1) return(s);     /* null substring matches first char */
    while((c = *s++) != '\0'){
        if(c == cp1){   /* first match */
            pss = subs;
            ps = s;
            while(  (cs = *pss) != '\0'  &&
		  ( (c = (*ps)) == cs || cs == '?') ){
	       /* match or wildcard and not end of pattern */
	       ps++; pss++;                
            }
            if(cs == '\0') return(ps);  /* success !; otherwise */
        }
        /* try again at next firstmatch */
    }
    return(0);  /* fail */
}

/************************ DSCAN(), NEXTFILE() ******************************/
static char scanstr[160];
static char *scanptr[40];
static int nscan=0;
static int scindex=0;

/***************  STRUCT DIR_T DEFINITION *********************/
struct dir_t{
    char dir_path[64];      /* basename */
    normal dir_ftime;       /* modification time of current file in DOS binary
                             format:   yy-1980  mm    dd    hh    mm    ss
                               bits:    25-31 21-24 16-20 11-15  5-10  0-4  */
    normal dir_fmode;       /* file mode: unix conventions, extended for
                                dos attributes: 0200000:hidden, 0400000:sys,
                                01000000: arch.bit.set */
    normal dir_size;        /* file size in bytes */
    char   dir_att[12];     /* attribute string, a la ls-l: drwxrwxsha */
    char   dir_tstr[20];    /* date string, in form yy/mm/dd  hh:mm:ss */
    char   dir_dir[48];     /* directory--prefix; cat of _dir and _path is
                                complete pathname */
    normal dir_ftype;       /* file 'type': 0 for binary (default); 1 for
                                stream_lf, 2 for var-len vms text (ansi) 
                                files, 3 for fixed-record size text files
                                4 for var-len binary (ansi?) */
    normal dir_reclen;      /* record length for type 3 (or 0, but irrelevant)*/
};


/************************** DSCAN() ***************************************/
extern int diropen();
extern struct dir_t *dirread();
static void fileatt(), filetime();
                          
/*
 * parses the string s into component scan patterns, goes
 * through them trying to open them, and returns 0 at first
 * success, -1 if no success or empty string
 */
int
dscan(s)
char *s;
{
    strcpy(scanstr,s);
    nscan = parse(scanstr,scanptr);
    if(nscan <= 0) return(-1);
    for(scindex=0; scindex< nscan; scindex++){
        if(!diropen(scanptr[scindex])){
            scindex++;
            return(0);
        }
    }
    return(-1);
}

/*********************** NEXTFILE() ****************************************/

struct dir_t *
nextfile()
{
    struct dir_t *ret;

    ret = dirread();
    if(ret) return(ret); /* next filename */
    /* if get here, the current pattern is exhausted; try opening the
    next, etc, until have a successful open, and then recursively
    call this function */
    for( ;scindex<nscan;scindex++){
        if(!diropen(scanptr[scindex])){
            scindex++;
            return nextfile();
        }
    }
    /* no more entries; failure */
    nscan = 0;
    scindex=0;
    return(0);
}

/************************** FILETIME() *************************************/

static void
filetime(file)
struct dir_t *file;
{
    char *str = file->dir_tstr;
    int ftime = file->dir_ftime;
    sprintf(str,"%02d/%02d/%02d  %02d:%02d:%02d ",((ftime>>25)&127) + 1980,
                    ((ftime>>21)&15),((ftime>>16)&31),((ftime>>11)&31),
                    ((ftime>>5)&63),(ftime&31)*2 );
}

/************************* FILEATT() ***************************************/

static char attpat[12] = "---------- ";

static void  
fileatt(file)
struct dir_t *file;
{
    char *extptr();
    int mode = file->dir_fmode;
    char *attstr = file->dir_att;
    
    strcpy(attstr,attpat);
    if(mode&01000000)   attstr[9] = 'a';
    if(mode&0200000)    attstr[8] = 'h';
    if(mode&0400000)    attstr[7] = 's';
    if(mode&040000)     attstr[0] = 'd';
    if(mode&0100)       attstr[3] = 'x';
    if(mode&0200)       attstr[2] = 'w';
    if(mode&0400)       attstr[1] = 'r';            
    if(mode&01)         attstr[6] = 'x';
    if(mode&02)         attstr[5] = 'w';
    if(mode&04)         attstr[4] = 'r';
    attstr[10] = file->dir_ftype + '0' ;
}


/*********************** DIROPEN(), DIRREAD() ******************************/

/* essentially all the system dependence is in these functions */
static char dirs[64];           /* the directory */
static struct dir_t path;       /* the returned full pathname and stat info */
static struct dir_t nfile = {"",0,0,0,"",""};  
static char pattern[64];        /* the search pattern, stripped of dir */   


/************** DSI and PHARLAP *******************/
#ifdef DOS32

/*#define DDEBUG*/

static int
gdopen(file,sp,mod) 
char *file;
struct dir_t *sp;
int mod;           /* the dos mode flag for SFIRST() */
{
    char *bp;
    char fcpy[64];
    char *pos;
    char *SFIRST(),*SNEXT();
    int init=1;
    char tstrng[16];

    sp->dir_path[0] = '\0';    
    bp = baseptr(file);
    if(*bp == 0) return(-1);  /* no pattern */
    strcpy(dirs,file);
    dirs[bp-file] = '\0';   /* terminate so dirs contains directory */
    strcpy(fcpy,file);
    strcpy(pattern,bp);     /* SFIRST, SNEXT return only basename,
                                so to match need basename of file */
    do{
        if(init){ 
            pos = SFIRST(fcpy,mod); 
            init = 0; 
        }else pos = SNEXT();
        if(!pos) return(-1);     /* no match */
        strcpy(tstrng,pos+30);
    }while(!wildmatch(matstr(tstrng),pattern));  
        /* DOS WILDCARD TROUBLE; try again */

    strcpy(sp->dir_path,pos+30);   /* fill out 'nextfile' structure */
    strcpy(sp->dir_dir,dirs);
    sp->dir_ftime = makelong(pos + 22);
    sp->dir_fmode = mirmode(*(pos + 21),sp->dir_path);
    sp->dir_size = makelong(pos + 26);
    sp->dir_ftype = 0;
    sp->dir_reclen = 0;
    filetime(sp);
    fileatt(sp);
    return(0);
}        


int getstat(name,sp)  /* fills out the directory structure *sp for the file
                            name; searches for all directory entries; rets
                            -1 if not found, 0 if successful; name is full
                            pathname */
char *name;
struct dir_t *sp;
{
    return gdopen(name,sp,0x16);
}


char 
*isdir(dirname)     /* rets "dirname/" if directory, 0 otherwise */
char *dirname;
{
    struct dir_t stat;
    static char fdir[64];

    getstat(dirname,&stat);
    if(stat.dir_fmode & 040000 && !(stat.dir_fmode & 030000)){
        strcpy(fdir,dirname);
        strcat(fdir,"/");
        return(fdir) ;
    }
    else return (0);
}


int diropen(file)       /* opens the pattern 'file'; in the DOS scheme, the
                        function SDIR() which does this actually returns 
                        the first matching filename. Thus to make the Mirella 
                        scheme transparent, this is saved for the first 
                        invocation of dirread() in the static string nfile */

char *file;
{
    return gdopen(file,&nfile,0);
}


struct dir_t *
dirread()    /* returns 'next' filename; actually, it was generated
                on the last call to dirread or diropen, and this
                function merely parrots it, and collects the 
                NEXT one. This could have been done less clumsily. The code
                has been fixed to circumvent DOS's crazy handling of 
                wildcards */
{
    char *pos;
    char *SNEXT();
    char tstrng[16];

    if(!(nfile.dir_path[0])){
        nfile.dir_path[0] = '\0';
        return(0);   /* no file from last pass */
    }
    path=nfile;

#ifdef DDEBUG
    scrprintf("\nftime=%d, fmode=%d size=%d",path.dir_ftime,
         path.dir_fmode,path.dir_size);
    scrprintf("\n%s %s %s",path.dir_path,path.dir_tstr,path.dir_att);
#endif                

    do{
        pos = SNEXT(); 
        if(!pos){
            nfile.dir_path[0] = '\0';
            return(&path);     /* no match */
        }
        strcpy(tstrng,pos+30);
    }while(!wildmatch(matstr(tstrng),pattern));  
        /* DOS WILDCARD TROUBLE; try again */

    strcpy(nfile.dir_path,pos+30);
    strcpy(tstrng,nfile.dir_path);
    strcpy(nfile.dir_dir,dirs);
    nfile.dir_ftime = makelong(pos + 22);
    nfile.dir_fmode = mirmode(*(pos + 21),nfile.dir_path);
    nfile.dir_size = makelong(pos + 26);
    nfile.dir_ftype = 0;
    nfile.dir_reclen = 0;
    filetime(&nfile);
    fileatt(&nfile);
    return(&path);
}

void
dirclose()   /* this is a null function here, since we do not open anything*/
{}

long 
makelong(decl)  /* converts a dec/intel long (given its address as a
                        byte string) into a long for the DSI coprocessor */
char *decl;
{
    char ret[4];
    int i;
#ifdef IBMORDER    
    for(i=0;i<4;i++) ret[i] = decl[3-i];
#else
    for(i=0;i<4;i++) ret[i] = decl[i];
#endif
    return *(long *)ret;
}

static int
mirmode(dosm,name)  /* returns extended unix mode from dos
                                    mode and filename */
int dosm;
char *name;
{
    char *cp = extptr(name);    
    int ret = 0;
    
    if(cp && (!strcmp(cp,"exe") || !strcmp(cp,"bat") || !strcmp(cp,"com")
        || !strcmp(cp,"sh")) ) ret |= 0101 ;    /* executable */
    if(!(dosm&0x01))           ret |= 0200 ;    /* writeable  */        
                               ret |= 0404 ;    /* readable   */ 
    if(dosm&0x10)              ret |= 040000;   /* directory  */
    else                       ret |= 0100000;  /* regular file */
    if(dosm&0x02)              ret |= 0200000;  /* hidden */
    if(dosm&0x04)              ret |= 0400000;  /* system */
    if(dosm&0x20)              ret |= 01000000; /* archive bit set */
    return ret;
}

#endif

/*************** BKUNIX ************/
#if defined(unix)

#if !defined(_POSIX_SOURCE)
#  undef u_short
#  undef u_char
#  undef u_long
#endif

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

static struct stat statbuf;

static char *dot = ".";
static DIR *dp = (DIR *)(0); 

int
diropen(file)
char *file;
{
    char *bp;
    char direc[64];
    
    bp = baseptr(file);
    if(*bp == 0) return(-1);   /* no pattern */
    strcpy(pattern,bp);    /* save the search pattern */
    strcpy(dirs,file);
    dirs[bp-file] = '\0'; /* terminate so dirs contains 
                                        directory prefix */
    /* make directory name */
    strcpy(direc,dirs);
    if(direc[0] == '\0') strcpy(direc,dot);   /* current */
    else(direc[strlen(direc) - 1] = '\0');    /* kill '/'  */
    dp = opendir(direc);
    if(!dp) return(-1);
    return (0);
}


/*#define DDEBUG*/


/*
 * fills the dir structure *sp for the filename name;
 * rets 0 if success, -1 if fail
 */
int
getstat(name,sp)
char *name;
struct dir_t *sp;
{
    int ret;
    int ftime;
    struct tm *tptr;
    
    ret = stat(name,&statbuf);
    if(ret == -1) return(-1);
    sp->dir_size = statbuf.st_size;
    sp->dir_fmode = mirmode(statbuf.st_mode,name); /* live with system dep. */
    ftime=statbuf.st_mtime;                
    tptr = localtime((time_t *)&ftime);
    sp->dir_ftime = (tptr->tm_sec/2 + ((tptr->tm_min)<<5)
        + ((tptr->tm_hour)<<11) + ((tptr->tm_mday)<<16)
        + (((tptr->tm_mon)+1)<<21) + (((tptr->tm_year)-80)<<25));
    sp->dir_ftype = 0;
    sp->dir_reclen = 0;    
    filetime(sp);
    fileatt(sp);
    return(0);
}

/*
 * rets "dirname/" if directory, NULL otherwise
 */
char *
isdir(dirname)
char *dirname;
{
    struct dir_t stat;
    static char fdir[64];

    getstat(dirname,&stat);
    if(stat.dir_fmode & 040000 && !(stat.dir_fmode & 030000)){
        strcpy(fdir,dirname);
        strcat(fdir,"/");
        return(fdir) ;
    }
    else return (NULL);
}


struct dir_t *
dirread()
{
    struct dirent *dirp;
    int ret;
    char pathname[64];

    if(dp == 0) return(0);    /* no open directory */
    while(1){
        dirp = readdir(dp);
        if(dirp){
            ret = wildmatch(dirp->d_name,pattern);
            if(ret){   /* success*/
                strcpy(pathname,dirs);
                strcat(pathname,dirp->d_name);
                strcpy(path.dir_path,dirp->d_name);
                getstat(pathname,&path);
                strcpy(path.dir_dir,dirs);
#ifdef DDEBUG
                scrprintf("\nftime=%d, fmode=%d size=%d year=%d",path.dir_ftime,
                    path.dir_fmode,path.dir_size,tptr->tm_year);
                scrprintf("\n%s %s %s",path.dir_path,path.dir_tstr,
                    path.dir_att);
#endif                
                return(&path) ;
            }
        }else{  /* end of directory */
            closedir(dp);
            dp = 0;
            return(0);
        }
    }
}

void              /* if do not exhaust nextfile() list, should do this */
dirclose()
{
    if(dp){
        closedir(dp);
        dp = 0;
    }
}

static int
mirmode(umode,name)  /* returns mirella mode from unix mode */
int umode;
char *name;
{
    return umode & 0177777;
}

#endif

/******************* VMS *****************/

#ifdef VMS
/****************/
/*#define DDEBUG*/
/****************/

#include stat
#include time

static struct stat statbuf;
static char *dirbuf = 0;
static int dirbuflen;
static int dirnent;
static char *dirhere;
static int nextent;            


int diropen(file)
char *file;
{
    char *bp,*cp, *cp1, *cp2;
    int len;
    char curdir[64];
    char newdir[64];
    char dirscr[64];
    char *trnlog();
    int n;
    int ddes;

    if(dirbuf) free(dirbuf);      /* get rid of old one */    
    dirbuf = 0;                   /* if we exit unsuccessfully*/
    bp = baseptr(file);
    if(*bp == 0){
        return(-1);
    }   /* no pattern */
    strcpy(pattern,bp);    /* save the search pattern */
    strcpy(dirs,file);
    dirs[bp-file] = '\0';   /* terminate so dirs contains 
                                        directory prefix*/
    strcpy(dirscr,dirs);    /* make another copy */
    cp =dirscr + strlen(dirscr) -1;  /* last character */
    if( *cp == ':') *cp = '\0';      /* kill trailing colon if any */

    /* new code using trnlog() */
    if(*dirscr){    /* not cwd */
        strcpy(newdir,trnlog(dirscr));
    }else getcwd(newdir,64);   /* cwd */

#ifdef DDEBUG
    scrprintf("\nnewdir=%s",newdir);
#endif

    /* have newdir. Must now decide what directory file to open */
    /* first look for a '.' following a '[', followed by a ']'; we are
        in a subdirectory at least one level down from the main */
    if( (cp=rindex(newdir,'[')) && (cp1 = rindex(cp,'.')) 
            && (cp2 = rindex(cp1,']')) ){
        *cp1 = ']';
        strcpy(cp2,".dir");
#ifdef DDEBUG
        scrprintf("\nAttempting to open %s",newdir);
#endif
        stat(newdir,&statbuf);
        ddes = open(newdir,O_RDONLY,0677);
        if(ddes == -1) return(-1);
    /* if there is no '.', probably in a main directory; interpose a '000000'*/
    }else if(cp){   /* there is a '[' */
        if(!(cp2 = rindex(cp,']'))) return(-1);   /* no matching ']' */
        strcpy(cp2,".dir");
        strcpy(dirscr,cp+1);  /* save the tail */        
        strcpy(cp+1,"000000]");
        strcat(newdir,dirscr);
        stat(newdir,&statbuf);
        ddes = open(newdir,O_RDONLY,0667);
#ifdef DDEBUG
        scrprintf("\nAttempted to open %s, des=%d",newdir,ddes);
#endif
        if(ddes == -1) return(-1);
    }else{   /* error in arg */
        scrprintf("\nDIROPEN:I cannot cope with the directory %s",newdir);
        return (-1);
    }
    /* if get here, have opened directory successfully and have a ddes. */
    dirbuflen = statbuf.st_size;   /* size of directory file */
    dirbuf = (char *)malloc(dirbuflen+512);
    if(!dirbuf){
        scrprintf("\nDIROPEN:Cannot allocate directory buffer");
        return(-1);
    }
    cp = dirbuf;
/*debug  scrprintf("\ndirbuf = %d",dirbuf);*/
    dirnent = 0;
    do{
        n = read(ddes,cp,100);
        if(n){
            dirnent++;
            *(cp + 4 + *(cp+3)) = 0;
            cp = cp + *(cp+3) + 5;
        }
    }while(n);               
    dirhere = dirbuf+3;
    nextent = 0;
    close(ddes);

    /* have read directory file; VMS nicely sorts it for you, so you are
    done. If ptr is the address of the count field of a name, the next
    count field is ptr + *ptr + 5; the first one is at dirbuf + 3  */

    return(0);
}

/*********************** ISDIR() *****************************************/
/*#define ISDDEBUG*/
/* this routine returns a pointer to a partially decoded directory name
if dirname is a directory, 0 if not...will eventually do a full decode */

char *
isdir(dirname)
char *dirname;
{
    char *cp, *cp1, *cp2;
    char newdir[64];
    char dirscr[64];
    char *trnlog();
    static fdirname[48];

    strcpy(dirscr,dirname);
    *fdirname = 0;
    cp =dirscr + strlen(dirscr) -1;  /* last character */
    if( *cp == ':') *cp = '\0';      /* kill trailing colon if any */
    strcpy(newdir,trnlog(dirscr));
#ifdef ISDDEBUG
    scrprintf("\nnewdir=%s",newdir);
#endif

    /* have newdir. Must now decide what directory file to query */
    /* first look for a '.' following a '[', followed by a ']'; we are
        in a subdirectory at least one level down from the main */
    if( (cp=rindex(newdir,'[')) && (cp1 = rindex(cp,'.')) 
            && (cp2 = rindex(cp1,']')) ){
        *cp1 = ']';
        strcpy(cp2,".dir");
#ifdef ISDDEBUG
        scrprintf("\nQuerying %s",newdir);
#endif
        if(stat(newdir,&statbuf) == -1) return (0);
        if(statbuf.st_mode & 040000 && !(statbuf.st_mode & 030000)){
            strcpy(fdirname,newdir);
            return(fdirname) ;
        }
        else return (0);
    /* if there is no '.', probably in a main directory; interpose a '000000'*/
    }else if(cp){   /* there is a '[' */
        if(!(cp2 = rindex(cp,']'))) return(-1);   /* no matching ']' */
        strcpy(cp2,".dir");
        strcpy(dirscr,cp+1);  /* save the tail */        
        strcpy(cp+1,"000000]");
        strcat(newdir,dirscr);
#ifdef ISDDEBUG
        scrprintf("\nQuerying %s",newdir);
#endif
        if(stat(newdir,&statbuf) == -1) return (0);
        if(statbuf.st_mode & 040000 && !(statbuf.st_mode & 030000)){
            strcpy(fdirname,newdir);
            return(fdirname) ;
        }
        else return (0);
    }else{   /* error in arg */
#ifdef ISDDEBUG
        scrprintf("\nISDIR:I do not THINK %s is a valid directory",dirname);
#endif
        return (0);
    }
}    

#if 0   /* superseded ?? by new RMS fn in mirvms.c */

/************************* GETSTAT() ***********************************/

int getstat(name,sp)   /* fills the dir structure *sp for the filename name;
                            rets 0 if success, -1 if fail */
char *name;
struct dir_t *sp;
{
    int ret;
    int ftime;
    struct tm * tptr;

    ret = stat(name,&statbuf);
    if(ret == -1) return(-1);
    sp->dir_size = statbuf.st_size;
    sp->dir_fmode = mirmode(statbuf.st_mode,name); /* live with system dep. */
    ftime=statbuf.st_mtime;                
    tptr = localtime(&ftime);
    sp->dir_ftime = (tptr->tm_sec/2 + ((tptr->tm_min)<<5)
        + ((tptr->tm_hour)<<11) + ((tptr->tm_mday)<<16)
        + (((tptr->tm_mon)+1)<<21) + (((tptr->tm_year)-80)<<25));
    XXXXXXXXXXXXXXXXXXXXX  need ftype and reclen if ever use again XXXXXXXX
    filetime(sp);
    fileatt(sp);
    return(0);
}

#endif


static char *
dir_getent()
{
    char *cp;
    if(nextent >= dirnent) return 0;
    cp = dirhere + 1;
    dirhere = dirhere + *dirhere + 5;   
    nextent++;
    return cp;
}


struct dir_t *
dirread()
{
    char *name;
    int len;
    int ret;
    int nread;
    int ftime;
    struct tm *tptr;
    char pathname[64];
    
    if(dirbuf == 0) return(0);
    while(1){
        name = dir_getent();
        if(name){
            ret = wildmatch(name,pattern);
            if(ret){   /* success*/
                strcpy(pathname,dirs);
                strcat(pathname,name);
                strcpy(path.dir_path,name);                
                getstat(pathname,&path);
                strcpy(path.dir_dir,dirs);      /* changed 89/12/11 */
#ifdef DDEBUG
                scrprintf("\nftime=%d, fmode=%d size=%d year=%d",path.dir_ftime,
                    path.dir_fmode,path.dir_size,tptr->tm_year);
                scrprintf("\n%s %s %s",path.dir_path,path.dir_tstr,path.dir_att);
#endif                
                return(&path) ;
            }
        }else{  /* end of directory */
#ifdef DDEBUG
            scrprintf("\nEnd of directory");
#endif        
            free(dirbuf);
            dirbuf = 0;
            return(0);
        }
    }
}

void
dirclose()  /* if you for some reason do not exhaust the newfile() list,
                you MUST do this to free the memory allocated by diropen() */
{
    if(dirbuf){
        free(dirbuf);
        dirbuf = 0;
    }
}                


static int 
mirmode(vmode,name)   /* rets mirella mode from vmsmode */
int vmode;
char *name;
{
    char *cp, *extptr();
    int mode = vmode & 0177777 & ~0100 ;
    if((cp = extptr(name)) && (!strcmp(cp,"exe") || !strcmp(cp,"com"))){ 
        mode |= 0100 ;
        if(vmode&0100)  mode |= 0100;
    }
    return mode;
}

#endif

/************************* DIRECTORY STACK CODE *****************************/

static char *cdbuf = 0;
static char *dirsp[8];
static char *dirp[8];
static int dptr=0;
extern char home_dir[];   /* from mirellio.c */

#define NDIR 8
#define NDM1 7

#define QCD(n)                                                      \
    er = chdir(dirp[(n)&NDM1]);                                     \
    if(er){                                                         \
        scrprintf("\nCannot change directory to %s",dirp[(n)&NDM1]);   \
        for(i=(n);i< dptr-1;i++) dirp[i&NDM1]=dirp[(i+1)&NDM1];     \
        dptr--;                                                     \
        return;                                                     \
    }                                                   

/*jeg0312*/

static void
initds()
{
    char *getcwd();
    int i;
    
    if(!cdbuf){   /* first time: allocate directory stack and init */
        cdbuf = (char *)malloc(1024);
        if(!cdbuf) erret("\n_PUSHD:Cannot allocate directory stack");
        for(i=0;i<NDIR;i++) dirsp[i] = dirp[i] = cdbuf + i*128;
        getcwd(dirp[0],127);
        dptr++;
    }
}

void 
_pushd()
{
    char *s = cspop;
    int er,c;
    void swapd();
    char *getcwd();
    
    initds();
    if((c=s[0]) == 0 || c == '\r' || c == '\n' ){
        swapd();
        return;
    }
    er = chdir(s);    
    if(er){
        scrprintf("\nCannot change directory to %s",s);
        return;
    }
    getcwd(dirp[(dptr++)&NDM1],127);
    return;
}

void swapd()
{
    int er,i;
    char *tem;
        
    if(dptr < 2) erret("\nSWAPD:directory stack empty");
    QCD(dptr-2);
    if(er){  /* delete offending entry */
        dirp[(dptr-2)&NDM1] = dirp[(dptr-1)&NDM1];
        dptr--;
    }
    tem = dirp[(dptr-2)&NDM1];
    dirp[(dptr-2)&NDM1] = dirp[(dptr-1)&NDM1];
    dirp[(dptr-1)&NDM1] = tem;
}

void rolld()
{
    int n = pop;
    int er;
    char *tem;
    int i;
    
    if(dptr < n+1) erret("\nROLLDIR:directory stack empty");
    QCD(dptr-n-1);
    tem = dirp[(dptr-n-1)&NDM1];
    for(i=n;i>0;i--) dirp[(dptr-i-1)&NDM1] = dirp[(dptr-i)&NDM1];
    dirp[(dptr-1)&NDM1] = tem;
}
   
void popd()
{
    int er,i;

    if(dptr < 2) erret("\nPOPD:directory stack empty");
    QCD(dptr-2);
    dptr--;
}

void cleardirs()  /* goes home, clears directory stack */
{
    int i;
    extern char home_dir[];
    
    dptr = 1;
    for(i=0;i<NDIR;i++) dirsp[i] = dirp[i] = cdbuf + i*128;
    chdir(home_dir);
    getcwd(dirp[0],127);
}

void showdirs()
{
    int ndir = (dptr > 8  ? 8 : dptr) ;    
    int i;
    
    initds();  /* jeg0312 */
    mprintf("\n");
    /* make some def and fix this--jeg0312 */
#ifdef unix
    for(i=0;i<ndir;i++) mprintf("%s ",(dirp[(dptr-i-1)&NDM1]) );    
#else
    for(i=0;i<ndir;i++) mprintf("%s ",fstrtolower(dirp[(dptr-i-1)&NDM1]) );
#endif
}

/************************* BACKUP CODE *************************************/

struct b_head_t{
    char b_hbanner[16];    /* "MIRELLABACKUP" */
    char b_fname[16];      /* basename+ext */
    char b_dirname[48];    /* directory PREFIX--incl term '/' in unix */
    char b_length[16];     /* length as an ASCII decimal number */
    char b_ftime[16];      /* decimal number in ASCII */
    char b_ftype[8];       /* 0 for binary files, 1 for ascii files */
    char b_reclen[8];      /* record length for funny files */
    char b_eols[8];        /* null for binary files, EOL for ascii files */ 
    char b_sys[8];         /* opsys writing tape */ 
    char b_back_stream[8]; /* back_stream flag */
    char b_dummy[368];
};

struct b_tail_t{
    char b_tbanner[16];     /* "MIRCHECKBLOCK" */
    char b_checksum[16];    /* ASCII decimal integer--0 for now */
    char b_crc[16];         /* ASCII decimal integer */
    char b_tdummy[464];     
};

/* it is presumed that the tapedrive is open; we will not use Mirella 
functions for the files being read */

int b_bigblk = 64;          /* number of READLEN length records in BBUFLEN */
static int BBUFLEN = 1048576;
#define READLEN 16384
/* readlen is a legal read size on all Mirella systems; we read a bufferful
and then write to tape 16KB at a time */

/************************ ISASCIIBUF() ************************************/
/* this routine decides whether a buffer cotaining the image of a file
is likely to be an ascii or a binary file, based on its content (It CAN
be fooled). The idea is to take about 4000 samples of the file as a bytestream,
and look at the statistics of the data. If more than a few have the eighth
bit set, or most are nonprintable, the decision is made binary. The return code
is 0 for binary, 1 for ascii; if ascii, eol is the EOL for the system, null
if binary.
The backup tapes have the EOL from the system; since work has to be done
on restore anyhow, translation is reserved for then */

/*#define ISASDEBUG*/

int
isasciibuf(buf,len,eol,it)
char *buf;
int len;
char *eol;      /* this is RETURNED if it=0. CAREFUL */
int it;         /* file type; used only as flag (0) to check for newline
                    fraction in stream files */
{
    int space;
    register char *cend = buf+len;
    register char *cp=buf;
    register int n=0;
    register int c;
    register int nlc=0;   /* histog index of last nonnull char */
    int n8 = 0;
    int nlf = 0;
    int ncr = 0;
    int nctl = 0;
    int ntab = 0;
    int nbs = 0;
    int nff = 0;
    int nalnum = 0;
    int nnul =0;
    int med;
    int histog[256];
    int i;
    int sum,n2;
    float ninv;
    float flf,fcr,falnum;
    
    for(i=0;i<256;i++) histog[i]=0;
    space = len/4096;
    while(cp < cend){
        histog[c = (u_char)(*cp)]++;
        if(c) nlc = n;
        cp += (((rand()&4095)*space)>>11) + 1;
        n++;
    }
    for(i=1;i<32;i++) nctl += histog[i];
    for(i=128;i<256;i++) n8 += histog[i];
    for(i=' '-1;i<='z';i++) nalnum += histog[i];
    nnul = histog[0];
    ntab = histog['\t'];
    nbs = histog['\b'];
    nff = histog['\f'];
    ncr = histog['\r'];
    nlf = histog['\n'];
    nctl -= (ncr + nlf + ntab + nbs + nff);   /* all control chars except 
                                                cr,lf,ht,bs,ff */
    n2 = nlc/2;  /* half the number counted until there are no more nonulls */
    sum = 0;
    for(i=255;i>=0;i--){
        sum += histog[i];
        if(sum > n2) break;
    }
    med = i;
    ninv = 1./(float)n;
    flf = nlf*ninv;
    fcr = ncr*ninv;
    falnum = (float)nalnum/(float)(n-ncr-nlf-nnul);
    
#ifdef ISASDEBUG
    scrprintf(
"\nA?: n=%d md=%d fan=%5.3f fcl=%5.3f f8=%5.3f fcr=%5.3f flf=%5.3f fnul=%5.3f\n"
        ,n,med,falnum,(float)nctl*ninv,(float)n8*ninv,fcr,flf,(float)nnul*ninv);
    scrprintf("\nlen=%d, nlc=%d",len, nlc);
#endif
    
    /* the criteria here are arguable; I allow 1/30 eight-bit set, 1/20
    control chars other than newline sets, demand that the median be
    alphanumeric and that at least 2/3 the nonnewline chars be 
    alphanumeric to be counted ascii */
             
    if( 30*n8 > n || 20*nctl > n || med < ' ' || med > 'z' || falnum < .667 ){ 
        *eol = '\0';
        return(0); /*binary*/
    }
    /* if get here, ascii. */
    strcpy(eol,EOL);
    if(it==0 && nlf*150 < n && ncr*150 < n){
        scrprintf("\nAbnormally low newline fraction:lf=%5.3f,cr=%5.3f",flf,fcr);
    }
    return(1);
}

void m_isasciibuf()  /* mirella procedure for checking buffer; does not
                        return eol or give warning if newline fraction is
                        abnormal */
{
    char beol[5];
    int len = pop;
    char *str = cspop;
    push(isasciibuf(str,len,beol,1));
}
    
/******************** CRCCHK() ********************************************/
/* this routine calculates a checksum on a char array treating it as
an integer array and also a 'crc'--actually the XOR product of the
array, again treating it as an integer array--it is ASSUMED that either
the length is divisible by 4 or that the array is terminated by enough
nulls that no garbage is included in the integer length nibuf 

89/12/10: Trouble: checksum is a problem treated this way on machines with
different byte orders. Until something clever is worked out, we will set
the checksum to zero; the 'crc' only needs to be fixed up at the end; we will
always write a dec/intel byteorder crc--first byte first, no ?? .

*/

void
crcchk(ibuf,nibuf,pcksum,pcrc)
register normal *ibuf;     /* integer array */
register normal nibuf;     /* length of same */
normal *pcksum,*pcrc;      /* pointers to the cksum and crc */
{
    register int c;
    register int crc = *pcrc;
    register int checksum = *pcksum;
    while(nibuf--){
        checksum += (c = *ibuf++);
        crc ^= c;
    }
    *pcksum = 0  /*checksum*/ ;
    *pcrc = crc;
}

static int 
stdcrc(crc)
int crc;
{
#ifdef IBMORDER    /* swap the bytes */
    u_char *str = (u_char *)(&crc);
    u_char scr;
    
    scr = str[0];
    str[0] = str[3];
    str[3] = scr;
    scr = str[1];
    str[1] = str[2];
    str[2] = scr;
#endif
    return (crc);
}

    
    
normal b_backstream = 0;   /* flag for backup type; if 0, backup image on
                             tape is a strict binary image of the file, and
                             the onus of translation is on restore(); if 1,
                             all file images look like unix stream files, and
                             the restoration is trivial, but VMS backup is
                             glacial */
                           
#ifdef VMS
#define VMSFLG 1
#else
#define VMSFLG 0
#endif

/************************ DSBACKUP() **************************************/

/*#define BKDEBUG*/

static struct b_head_t **bpptr = 0;
static struct b_head_t *bptr = 0;
static char * asptr = 0;

void freeback()
{
    if(bpptr){
        chfree((Void *)bpptr);
        bpptr = 0;
        bptr = 0;
    }
}    


void
dsbackup(sptr)
struct dir_t *sptr;
{
    char fname[64];
    int i;
    struct b_tail_t *btptr;
    char *buffer,*buf0;
    int len = sptr->dir_size + 512;         /* bytes to write */
    int rperbig = b_bigblk;
    normal *ibuf;
    register int nibuf;
    int nread;
    int nwrite;
    int tread ;
    int fdes;
    int ntail;
    char *bufend;
    normal checksum;
    normal crc;
    int endflg = 0;
    int bovfl;
    int bbufno=0;
    int tbufno = 0;
    int isasciibuf();
    char eol[8];
    int ftype = sptr->dir_ftype;
    int imty;                   /* the file type of the tape image */

    BBUFLEN = READLEN * rperbig ;          /* 64 read buffers per bigbuffer*/
    /* allocate backup buffer if necessary */
    if(bpptr == 0 || *bpptr == 0){    
      bpptr = (struct b_head_t **)memalloc("backup",BBUFLEN + 3*READLEN + 512);
      if(!bpptr) erret("\ncannot allocate backup buffer (1 MB)");
      bptr = *bpptr;
    }

    clearn(512,(char *)bptr);
    strcpy(bptr->b_hbanner,"MIRELLABACKUP");
    strcpy(fname,sptr->dir_dir);
    strcat(fname,baseptr(sptr->dir_path));   /* the baseptr should not be
                                                necessary. Fix it */
    strcpy(bptr->b_fname,fcanon(baseptr(fname)) );

    /* FIXME!!! this way of getting the directory is deadly: relative
        directory names are not translated. probably should GO to the
        indicated directory, getcwd, and come back. Slow. */
    if(*(sptr->dir_dir)){    /* has a directory */
    /* the directory entries on the tape are always in unix form */
        strcpy(bptr->b_dirname,fcanon(sptr->dir_dir));
    }else{         /* current directory */ 
        getcwd(sptr->dir_dir,48);    
        strcpy(bptr->b_dirname,fcanon(sptr->dir_dir));
    }
    sprintf(bptr->b_length,"%ld", (long)sptr->dir_size); 
    sprintf(bptr->b_ftime,"%ld", (long)sptr->dir_ftime);
    sprintf(bptr->b_back_stream,"%ld", (long)b_backstream);
    sprintf(bptr->b_reclen,"%ld", (long)sptr->dir_reclen);
    strcpy(bptr->b_sys,M_OPSYS);

    /* header is done. Now read the file */
    buf0 = (char *)bptr;    
    bufend = buf0 + BBUFLEN;
    buffer = buf0 + 512; /* first read buffer; skip past header */
    if(b_backstream){     /* makes verbatim stream copies of ascii files*/
        fdes = open(fname,O_RDONLY);   /* in VMS, opens as native type */
    }else{               /* default: makes image copies, warts and all */
        fdes = openblk(fname,O_RDONLY);
    }
    if(fdes == -1){     
        scrprintf("\nCannot open %s",fname);
        chfree((Void *)bpptr);
        erret((char *)NULL);
    }        
    mprintf("\n%-44.44s ",fcanon(fname));
    
    if(sptr->dir_size == 0){
        mprintf("  ZERO-LENGTH FILE: NOT COPIED");
        goto out;
    }

    tread = 512;    /* pretend have 512 bytes already to allow for header */
    checksum = 0;
    crc = 0;
    tbufno = 0;
    do{
        /* this will break if the file has fixed-length disk records 
            bigger than READLEN */
        if(b_backstream){            
            nread = read(fdes,buffer,READLEN);    
        }else{
            nread = readblk(fdes,buffer,READLEN);
        }            
        if(nread == -1){ 
            scrprintf("\nDISK READ ERROR");
            goto out;
        }
        tread += nread;     /* this runs */
        buffer += nread;    /* this is reset every write */
        endflg = ((tread >= len || nread == 0) ? 512 : 0); /* end of file? */

        if(endflg || buffer > bufend){     /* time to write ?? */

            /* VMS sometimes reads one block beyond the eof; if this
            happens, the checkblock is not placed properly; fix it */
            if(endflg && tread > len) buffer -= tread-len;

            /* decide how much we have to write, and clean up the
            buffer tail */
            bovfl =((ntail = buffer + endflg - buf0 - BBUFLEN) > 0);
            if(endflg || !bovfl) clearn(READLEN,buffer);
            nwrite = (buffer + endflg - buf0 -1)/READLEN + 1;
            if(nwrite > rperbig && !endflg) nwrite = rperbig;  
                /* this can happen with VMS record files; if we ARE
                at the end, we will just write it all. */

            /* if this is first buf, decide whether ascii or binary and 
            fill in header */

            if(bbufno == 0){    
                if(isasciibuf(buf0+512,buffer-buf0-512,eol,ftype)
                    || (VMSFLG && (ftype == 2 || ftype == 3))){   
                            /* ascii image */
                    mprintf(" ASCII:");
                    if(!VMSFLG || b_backstream || (VMSFLG && ftype == 0)){
                        /* output is ascii stream file, which may be converted
                        to ansi text file on restore; force EOL to be output
                        even if isasciibuf screwed up, which it does often
                        on little ansi files */
                        
                        strcpy(eol,EOL);
                        sprintf(bptr->b_ftype,"0");
                        strcpy(bptr->b_eols,eol);
                        imty=0;
                    }else{
                        /* weird text file--copy verbatim */
                        sprintf(bptr->b_ftype,"%d",ftype);
                        *(bptr->b_eols) = '\0';
                        imty=ftype;
                    }
                }else{  /* this ASSUMES that all type 2 and 3 files are 
                            'caught', ie classified as ascii */
                    mprintf("BINARY:");
                    sprintf(bptr->b_ftype,"1");
                    *(bptr->b_eols) = '\0';
                    imty=1;
                    if(VMSFLG && ftype == 4){   /*variable-length record bin*/
                        if(b_backstream){
                            scrprintf(
"\nERROR: variable-record-length binary file; cannot do with b_backstream.");
                            goto out;
                        }else{
                            sprintf(bptr->b_ftype,"4");
                            *(bptr->b_eols) = '\0';
                            imty=4;
                        }
                    }
                }
            }
            
            /* now update checksum and crc; note that checksum and crc
            include the header block but NOT (natch) the checkblock  */
            
            ibuf = (normal *)buf0;
            nibuf = nwrite*READLEN/sizeof(normal);
            if(!endflg || !b_backstream || bbufno){    
                /* if b_backstream and last block and first block,
                    calculate this below--cannot do twice */ 
	       crcchk(ibuf,nibuf,&checksum,&crc);
            }

#ifdef BKDEBUG
            scrprintf("\nWriting bigbuf %d, %d tape buffers, chk,crc=%d %d",
                bbufno,nwrite,checksum,crc);
#endif                
            if(endflg){ /* place check block on 512-byte bdy */
                if(b_backstream){
                    /* if in backstream mode, file length should be reported
                    as READ, not the native file size; if the file is too
                    big, (more than one bblock) you do not know that number
                    until after you have written the header, and so are up
                    the creek. ---so */

                    /* put real read length in header-- */

                    sprintf(bptr->b_length,"%d",tread-512); 
                    /* and warn joe if header has already been written */
                    if(bbufno){
                        mprintf(
"\nWARNING: backstream backup not reliable for files larger than %d bytes",
                            BBUFLEN);
                        mprintf("\nfile length may be in error on tape image");
                    }else{
                        crcchk(ibuf,nibuf,&checksum,&crc);
                    }
                    
                }

                buffer = buf0 + ((buffer-buf0 -1)/512 +1)*512;
                btptr = (struct b_tail_t *)buffer;
                strcpy(btptr->b_tbanner,"MIRCHECKBLOCK");
                sprintf(btptr->b_checksum,"%ld", (long)checksum);
                sprintf(btptr->b_crc,"%ld", (long)stdcrc(crc));
#ifdef BKDEBUG 
                scrprintf("\nWriting last(%d)dbuffer checksum=%d, crc=%d",
                        bbufno,checksum,stdcrc(crc));
                scrprintf("\nbuffer,buf0,btptr,tread=%d %d %d %d ",
                        buffer,buf0,btptr,tread);
#endif          
            }   
            /* write the buffer */
            for(i=0;i<nwrite;i++){
                mwrite(buf0 + i*READLEN,READLEN);
                tbufno++;
            }
            bbufno++;
            if(nread == 0 || tread >= len) break;
            if(ntail > 0){
                /* move tail to head*/
                bufcpy(buf0,buf0+BBUFLEN,ntail);
                buffer = buf0 + ntail;
            }else{
                buffer = buf0;
            }
        }
        pause(out);
    }while(1);
    mprintf("%1d %7d bytes %3d blocks",
        imty,len-512,tbufno);
out:
    if(b_backstream){
        close(fdes);
    }else{
        closeblk(fdes);
    }
    /* NO EOF NOW. MUST HAVE ONE IN EXEC ROUTINE */
    pauseerret();
}        

/************************ DSRESTORE() ***********************************/
/* BUGS: The blk functions in VMS still do not extend and truncate files
    properly */
    
/* restore option flags */

int b_askflg = 0;       /* enquire about overwriting */
int b_forceflg = 0;     /* force overwrite-- default is NOT to overwrite
                                later version */
int b_listflg = 0;      /* list contents of backup tape */
int b_stoansi = 1;      /* if VMS, convert ascii stream files to ansi
                            (type 2) text files */

/*#define RESDEBUG*/

void
dsrestore(dirflg,pat)
int dirflg;   /* dummy for now */
char *pat;
{
    /* in this prelim. version, must be IN restoration directory */
    char fname[16];
    char sname[16];
    char *patar[20];      /* pointers to words in pattern */
    int npat;         /* number of items in pattern */  
    char dirname[48];
    int i;
    struct b_tail_t *btptr;    
    char *buffer,*buf0,*buf1;
    int len ;                               /* file length */ 
    int lenp;                               /* total len with hdr & trailer*/
    int lenf;                               /* file len + 512 */
    int rperbig = b_bigblk;                 /* 64(def) tape blocks per bigblk*/
    int nread;
    int tread;
    int fdes = (-1);
    int tbuf=0;
    int ntbuf;
    int tail;
    int rbuf;
    normal checksum;
    normal crc,tcrc;
    char teol[8];
    int ftime;
    int ftype,ctype;                        /* file type on tape, target dsk*/
    int reclen;
    int exflg,skipflg;
    normal *ibuf;
    register int nibuf;
    int endflg;
    /* int backstream = 0; */
    int nw;
    int ntbinbuf;
    void vmplaceeof();
    
    BBUFLEN = rperbig * READLEN;
    /* allocate backup buffer if necessary */
    if(bpptr == 0 || *bpptr == 0){    
      bpptr = (struct b_head_t **)memalloc("backup",BBUFLEN + 3*READLEN + 512);
      if(!bpptr) erret("\ncannot allocate backup buffer (1 MB)");
      bptr = *bpptr;
    }
    
    /* parse pattern */
    npat = parse(pat,patar);
    if(npat == 0) erret("\nDSRESTORE:No pattern set");   

    buf0 = (char *)bptr;   /* big buffer, length BBUFLEN + 512 */
    asptr = (char *)bptr + BBUFLEN + READLEN + 512; 
                           /*ascii buffer for writeascii; length 2*READLEN*/
    
    do{     /* loop over files */
      pause(out);
      checksum = 0;
      crc = 0;  
      buffer = buf0;
      tbuf = 0;
      rbuf = 0;
      tread = 0;
      endflg = 0;
      do{       /* loop over records */
        nread = mread(buffer, READLEN);
        pause(out);
        if(rdeof || nread == 0){
            mprintf("\nEND OF BACKUP FILE");
            return;      /* this logic allows one file per restore command */
        }
        tread += nread;
        if(tbuf == 0){    /* first record; tend to header chores */
            if(strcmp(buffer,"MIRELLABACKUP")){
                erret("\nThis is NOT a Mirella backup tape. I quit");
            }
            strncpy(fname,bptr->b_fname,16); fname[15] = 0;
            strcpy(sname,fname);
            len = atoi(bptr->b_length);        
            strncpy(dirname,bptr->b_dirname,48); dirname[47] = 0;
            mprintf("\n%-16s  %7d  ",fname,len);
            ftype = atoi(bptr->b_ftype);
            /* backstream = atoi(bptr->b_back_stream); ???? */
            /* figure out disk filetype */

            skipflg = 0;   /* a priori, we will not skip this file; trouble
                              or disinterest may lead us to */
#ifdef VMS
            /* if input is stream ascii and req to convert to ansi is on,
                    output file is ansi var-len text file; otherwise types
                    are preserved */
            ctype = ftype;
            if(ftype == 0 && b_stoansi) ctype = 2;
#else
            switch(ftype){
            case 0:
            case 1:
                ctype = ftype;      /* stream and block binary files are
                                        verbatim */
                break;
            case 2:
            case 3:                 
                ctype = 0;          /* ansi and fix-len text to stream */
                break;
            case 4:
                ctype = -1;          /* var-len binary cannot be converted
                                        meaningfully */
                skipflg = 1;         /* so skip it in restore process */
                break;
            default:
                scrprintf("Illegal file type = %d",ftype);        
                erret((char *)NULL);
            }
#endif            
            if(ftype == 1 || ftype == 4){
                mprintf("BINARY:%d->%d ",ftype,ctype);
            }else{
                mprintf(" ASCII:%d->%d ",ftype,ctype);
                strcpy(teol,bptr->b_eols);
            }
            ftime = atoi(bptr->b_ftime);
            reclen = atoi(bptr->b_reclen);
            nfile.dir_ftime = ftime;
            filetime(&nfile);
            mprintf("%s  ",nfile.dir_tstr);   /* have printed the name,type,
                                                 size, and creation date */

            /* are we interested in file ?? */
            if(mwildmat(matstr(fname),patar,npat) && !b_listflg && !skipflg){
                /* file IS a candidate for copying. Does it exist already?*/
                exflg = 0;
                if(!getstat(fname,&path)){
                    exflg = 1;   /* file exists */
                    if(path.dir_ftime > nfile.dir_ftime){
                        skipflg = 1;
                        mprintf("LAT VER: ");
                        if(b_askflg){
                            scrprintf("\nLater version exists: Overwrite ? ");
                            if(_yesno())skipflg=0;
                        }
                        if(b_forceflg && !b_askflg) skipflg = 0;
                    }
                    if(exflg && !skipflg) unlink(fname);  /* kill the old one */
                }
            }else skipflg = 1;   /* no match or already disqualified */

            /* skip to next file if appropriate */
            if(skipflg || b_listflg){
                /* not a candidate file; press on */
                ntbuf = ((len+1024)-1)/READLEN;   /* how many more records ?*/
                for(i=0;i<ntbuf;i++) mread(buffer,READLEN);
                break;   /* go to next file */
            }
            
            /* if we get here, restore the file */
            /* do something hereabouts to check on directories */

            lenp = ((len-1)/512 +1)* 512 + 1024;   
            lenf = len + 512;
                    /* file + header and trailer block; tb on 512-byte bdy */
            ntbuf = (lenp -1)/READLEN + 1;
            fdes = -1;
#ifdef RESDEBUG
            scrprintf("\nlen,lenf,lenp,ntbuf=%d %d %d %d",len,lenf,lenp,ntbuf);
#endif           
            /* OK. Now create the file. Carefully */
#ifndef VMS
            fdes = creatblk(fname,len);         /* this is easy */
#else       /* opsys is VMS */
            fdes = creatvms(fname,(len*11)/10+512,ctype);

            /* XXXXXX--there is a problem with the file extension if the
                needed length is a little longer than the tape length;
                fix it and change this back. Fixed, but I will leave this */
            
            /* this should work--all block binary files are type 1; if
            backstream is 1, all ascii files are 0; if not, ascii files
            are properly typed; note that all files are opened for BLOCK
            i/o, and images will be placed--that is OK, I think; type 4
            files in backstream mode are illegal and should not have
            been written to the tape. */
#endif           
            if(fdes == -1){
                chfree((Void *)bpptr);
                scrprintf("\nCannot create %s",fname);
                erret((char *)NULL);  /* just throw up your hands */
            }
        }   /* end of if block for header block */

        endflg = (tread >= lenp);
#ifdef RESDEBUG
        scrprintf("\nnread,tread=%d %d, buffer= %d,endflg=%d",
            nread,tread,buffer,endflg);
#endif
        buffer += nread;
        
        if(endflg || buffer-buf0 >= BBUFLEN){
            /* big buffer is full or we are out of data; 
                    do bookkeeping and write to the file */
            /* calc ntbinbuf, which is the number of bytes read from the tape
                in the CURRENT big buffer */
            ntbinbuf = BBUFLEN;
            if(endflg) ntbinbuf = tread%BBUFLEN;
            if(ntbinbuf%READLEN)mprintf("\nDSRESTORE:Tape block length error");

            /* update checksum and crc */
            ibuf = (normal *)buf0;
            nibuf = BBUFLEN/sizeof(normal);  /* for a full buffer */
            /* if at end, skip the checkblock !! */
            if(endflg){
                nibuf = (ntbinbuf - (tread-lenp) - 512)/sizeof(normal);
            }
            if(nibuf > 0){    /* if checkblock is ONLY thing in last tblock
                                    nibuf can be zero */
                crcchk(ibuf,nibuf,&checksum,&crc); 
            }
            /* if at end, get the crc and checksum from the checkblock */
            if(endflg){
                btptr = (struct b_tail_t *)
                            (buf0 + ntbinbuf - (tread-lenp) - 512);
                if(strcmp(btptr->b_tbanner,"MIRCHECKBLOCK")){
                    mprintf("\nERROR:NO VALID CHECKBLOCK");
                }
                tcrc = atoi(btptr->b_crc);
                crc = stdcrc(crc);
                if(crc != tcrc ){  
                    mprintf("\nERROR:");
                    if(crc != tcrc) mprintf("  CRC:tape=%d, read=%d",tcrc,crc);
                }
            }       
            buf1 = (rbuf == 0 ? buf0 + 512 : buf0 );  /* start; skip header */
            tail = ntbinbuf - (buf1 - buf0);           /* how many ? */
            if(endflg) tail = tail - (tread - lenf);
            /* now for the work: write the buffer to the disk */
#ifdef RESDEBUG
            scrprintf("\nWriting %d bytes from %d",tail,buf1);                  
#endif

#ifdef VMS     
            /* VMS files are ALWAYS OK, since they are images or
            images of stream files being written as stream files.
            actually, this is not so, because of the EOF mess;
            type 2 (and 4?) files need a 0 ff ff at the very end. */
            /* this has been fixed; writeblk and closeblk now deal
            properly with the end-of-file */
        
            if(ftype == 0 && (strcmp(teol,EOL) || b_stoansi)){  
                /* str file from sys w/diff eol or req. to cvrt to ansi */
                nw=writeascii(fdes,buf1,tail,ftype,reclen,teol,
                                endflg,rbuf,b_stoansi);
                buffer = buf0 + nw;
            }else{            
                writeblk(fdes,buf1,tail);
                buffer = buf0;
            }
#else
            if(ftype==1){   /* block binary, easy */
                writeblk(fdes,buf1,tail);
                buffer = buf0;
            }else{    /* this is a MESS. We must convert from images to stream,
                        or at least deal with possibly different eols */
                if(ftype == 0 && !strcmp(teol,EOL)){   /* simple */
                    writeblk(fdes,buf1,tail);
                    buffer = buf0;
                }else{
                    nw = writeascii(fdes,buf1,tail,
                            ftype,reclen,teol,endflg,rbuf,0);
                    buffer = buf0 + nw;
                }
            }
#endif
            rbuf++;
            if(endflg){
                mprintf("COPIED");
                break;
            }
        }        
        tbuf++;
        pause(out);
      }while(1);    /* loop over tape records */
      if(fdes > 0){
          closeblk(fdes);   
          fdes = -1;
      }
    }while(1);      /* loop over files */
out:
    if(fdes > 0){
        closeblk(fdes);
        fdes = -1;
    }
    mprintf("\nDONE");
    pauseerret();
    return;
}

/************************** MWILDMAT() **************************************/
/* matches str to multiple word patterns */
static int 
mwildmat(str,patar,npat)
char *str;
char **patar;
int npat;
{
    int i,ret=0;
    for(i=0;i<npat;i++){
        if(wildmatch(str,patar[i])){
            ret = 1;
            break;
        }
    }
    return ret;
}
    

#if 0     /* do not need this anymore, since now know where eof is */
/************************* VMPLACEEOF() *************************************/
/* places a 0 0xff 0xff at the end of an ansi text file */

static void vmplaceeof(buf,n)
u_char *buf;   /* origin of buffer */
int n;       /* #bytes in buffer; should be div by 512 */
{
    register char *cp = buf + ((n-1)/2 + 1)*2 ;   /* even byte; it is 
                                        ASSUMED that the prev. records add up
                                        to an even #. */
    register int i = n;
    while(i--){
        if (*(--cp)) break;
    }
    cp++;    /* points to first null in tailstring */
/*debug*/scrprintf("\nPlacing EOF at %d, buf=%d",cp,buf);
    if((cp - buf) > (n - 3)) return;   /* not enough room anyhow */
    *cp++ = 0;
    *cp++ = 0xff;
    *cp++ = 0xff;
}

#endif    
    

/************************* WRITEASCII ***************************************/
/* this awful routine writes the contents of an image buffer to a file in
stream format; converts strange VMS text files to stream files on rational
systems, and corrects for different types of eols on different rational
systems. IT ALWAYS WRITES A STREAM FILE, unless stoansi is nonzero, in
which case if ftype is zero, it writes an ansi text file */

/*#define WADEBUG*/
int
writeascii(des,buf,n,ftype,reclen,eol,endflg,runflg,stoansi)
int des;        /* descriptor of output file */
char *buf;      /* buffer */
int n;          /* # bytes */
int ftype;      /* 0=stream, 2= ansii var, 3 = fixed-length text */
int reclen;     /* record length for fixed-length file buf image */
char *eol;      /* eol in buf if stream buffer */
int endflg;     /* last buffer ? --if so write everything; if not, write
                    even # of READLEN-length blocks and shift end to head; 
                    routine will write rest next time */
int runflg;     /* 0 initializes internal write buffer; do for each file once*/
int stoansi;    /* if nonzero and ftype is 0, converts to ansi text file */
{
    register int i;
    register char *cps,*cpd,*cpq;
    char *cpe;
    register int c;
    int iodd;
    register int ceol = *eol;
    int lteol;
    int schareol;
    int nleft;
    static int oofs;
    static char *cpl;  /* pointer to line origin for type 2 output */
    int len;     
    int dostoansi = ( ftype == 0 && stoansi );

    /* there are two possible problems related to buffer boundaries. The
    first is that there may be an interpretation problem at the final
    boundary--the last char in the buffer might be the first byte of a
    short int count, or the first char of a possible 2-char EOL string,
    or a var or fixed-len record may span the boundary. This is dealt 
    with by returning the # of ambiguous bytes, and moving the partial 
    data to BPTR so that the calling program can move the origin of the 
    buffer to suit on the next call.
    
    The second problem is that the buffer presented might not be an even
    number of write buffers, here of length READLEN; that is addressed by
    keeping a static output buffer origin, which is initialized by a
    zero in the variable runflg */
    
    /* set up destination buffer pointers */
    if(runflg == 0){
        oofs = 0;
        cpl = cpd = asptr;   /* modify for s->ansi conv; see code */
    }else  cpd = asptr + oofs;
    cpe = asptr + READLEN;
    
    /* and source */
    cps = buf;
    cpq = buf+n;

#ifdef WADEBUG
    scrprintf(
"\nWRITEASCII: buf=%d n=%d ftype=%d reclen=%d endflg=%d runflg=%d stoansi=%d",
                buf,n,ftype,reclen,endflg,runflg,stoansi);
#endif

    switch(ftype){
    case 0:   /* stream file to stream file with (poss) different eol's,
                    or stream file to ansi text file */     
        lteol = strlen(eol);        /* len of eol ON TAPE */
        schareol = (lteol == 1);    /* flag for single-char eol */
        i=n;

        if(!stoansi){   /* stream to stream; this seems to work */
          do{
            if((c = (*cpd++ = *cps++)) == ceol){   /* end of line ? */
                if(cps==cpq && !schareol){  /* trouble--end of srcblk and
                                                cannot get all of EOL */
                    cps--;
                    cpd--;                  /* take back the 1st char & split*/
                    break;
                }                                                
                if(schareol || *cps == eol[1]){   /* end of line */
                    strcpy(cpd-1,EOL);
                    cpd += LEOL-1;
                    cps += lteol-1;
                }
                if(cpd >= cpe){
                    writeblk(des,asptr,READLEN);
                    nleft = cpd-cpe;
                    if(nleft) bufcpy(asptr,cpe,nleft);
                    cpd = asptr + nleft;
                }
            }
          }while(cps<cpq);
        }else{      /* convert to ansi */
          if(runflg == 0) cpd += 2;  /*at beginning, skip count bytes */
          do{
            if((c = (*cpd++ = *cps++)) == ceol){   /* end of line ? */
                if(cps==cpq && !schareol){  /* trouble--end of srcblk and
                                                cannot get all of EOL */
                    cps--;
                    cpd--;                  /* take back the 1st char & split*/
                    break;
                }                                                
                /* if get here, OK */
                if(schareol || *cps == eol[1]){     /* end of line */
                    cpd--;                          /* toss eol char */
                    len = cpd - cpl -2;             /* line length */
                    if(len&1) *cpd++ = 0;           /* even line len */
                    *cpl = len & 0xff;
                    *(cpl+1) = len >> 8;            /* place count bytes */
                    cps += lteol-1;
                }
                if(cpd >= cpe){
                    writeblk(des,asptr,READLEN);
                    nleft = cpd-cpe;
                    if(nleft) bufcpy(asptr,cpe,nleft);
                    cpl = asptr + nleft;  /* ptr to count for next line */
                    cpd = cpl + 2;
                }else{
                    cpl = cpd;
                    cpd += 2;
                }
            }
          }while(cps<cpq);
        }
        break;   
    case 2:   /* var len ascii to stream. The record always starts
                    with a length, lobyte first */
        do{
            if(cpq-cps < 2)break;     /* too near end of bigbuf to get complete 
                                    count */
            i = *(u_char *)cps + ((*(u_char *)(cps+1))<<8); 
                /* count; VMS always puts the NEXT count on an even byte ?? */
            iodd = i&1;                     /* 1 if i is odd */
            if(cps + 2 + i + iodd > cpq) break;  /* incomplete record */
            cps += 2;                       /* beginning of string */
            while(i--) *cpd++ = *cps++ ;    /* copy it */
            strcpy(cpd,EOL); cpd += LEOL;   /* add EOL */
            cps += iodd;                    /* skip null if odd char cnt */

            if(cpd >= cpe){
                writeblk(des,asptr,READLEN);
                nleft = cpd-cpe;
                if(nleft) bufcpy(asptr,cpe,nleft);
                cpd = asptr + nleft;
            }
        }while(cps < cpq);
        break;   
    case 3:    /* fixed len ascii to stream, record len reclen */
        do{
            i = reclen;
            if(cps + i > cpq) break;        /* incomplete record */
            while(i--) *cpd++ = *cps++ ;
            strcpy(cpd,EOL); cpd += LEOL;
            if(cpd >= cpe){
                writeblk(des,asptr,READLEN);
                nleft = cpd-cpe;
                if(nleft) bufcpy(asptr,cpe,nleft);
                cpd = asptr + nleft;
            }
        }while(cps < cpq);
        break;
    default:
        scrprintf("\nWRITEASCII: %d is not a filetype I support",ftype);
        return(-1);
        break;
    }
    if(endflg && cpd > asptr){     /* still something in outbuffer */
        if(dostoansi && cpd == cpl+2) cpd -= 2;  /* reset cpd before
                                                nonexistent last count */
#ifdef WADEBUG                                                
    scrprintf("\nwriting final block: asptr=%d n=%d",asptr,cpd-asptr);
#endif
        writeblk(des,asptr,cpd-asptr);
        return 0;
    }
    if(cps < cpq){   /* we did not get through all of source blk because
                         of record boundary problems */
        bufcpy((char *)bptr,cps,cpq-cps);   /* tailtohead */
        return(cpq-cps);   /* inform joe of situation */
    }
    scrprintf("\n Fell off bottom of writeascii");
    return(0);
}           


/************************* END OF MODULE MIRDIR.C ***************************/





