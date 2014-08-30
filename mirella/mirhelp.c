/*VERSION 90/04/21, Mirella 5.40                                       */

/* 1207: FIX THIS MESS -- I think get rid of it all and use less() */

/***************************** MIRHELP.C ********************************/

/* recent history 
 * 88/05/23-28 changed code to support abbreviations, added 'alternate'
    names, added option in 'help' with no argument to display all
    category lists. 
 * 88/07/09 fixed get_file() to search home directory as well as curr and mir.
    fixed code to deal with new dishelp,but supporting code is still not
    written, so be careful.
    Still to do is install search facility, and to integrate
    into screen handler in non-PC systems
 * 88/07/20 finished new dishelp code    
 * 88/07/31 fixed module to compile either as mirella functions or as 
    standalone module (old mhelp.c)
 * 88/08/30 added code to save last helpscreen for 'refreshme' display
 * 88/10/26 fixed __TURBOC__, DSI confusion in cpp control
 * 88/10/27 fixed ? \r, \n confusion in writepage(). 
 * 89/02/02 fixed VMS trouble with tell() not returning the true char
 *  offset into a file. Makeindex() now produces true char offsets for
 *  the index file, which is what dishelp() requires, independent of the
 *  operating system (does not use tell() at all). 
 * 89/02/03 fixed writepage() so that it deals with real tabs in the helpfile
 *  correctly (at some expense in speed).
 * 89/02/04 added search code in dishelp().
 * 89/11/18: Removed size restriction on dishelp, browse, by allocating
 *            enough space in get_file() for line counter and pointer arrays
 * 90/04/20: Several changes to accomodate arbitrary terminal types, and
 *           to implement direct screen writes under DOS, and to use the
 *           terminal description in m_fterm.
 */

/* the macro  STANDALONE  must be #defined, typically in the compiler
command line, to produce a standalone help program; in its absence, this
module compiles to code to be linked with Mirella */

#include "mirella.h"
#include "edit.h"      
#include "scrbuf.h"

#define CR 13
#define NL 10

    /* max number of categories currently 100 */
#define NCAT 100

/* a help file entry consists of text in any format, but with the name
of the entry as the first whitespace-delimited word.  The entry is
closed with a line with a backslash as the first character, followed by
a one-word category identifier.  No other line may begin with a
backslash.  The category line may be followed by any number of lines of
the form name * or name +.  These "alias" lines establish an index entry
for "name" which evokes the same text as the previous full entry, the
difference being that the '*' entries are not normally displayed by the
category lister; the '+' entries are equivalent in every way to the 'main'
entries .  */

/* the index structure arrays are kept in a set of .inx files and are created 
by the Mirella procedure makeindex() from a set of help documentation files; 
each is a collection of 32-byte entries of the form: */

struct index_en {
    char wname[16];         /* word, length+first 15 chars, null terminated*/ 
    char filename[12];      /* c string; filename (base only) */
    unsigned short foffset; /* offset into file where entry is to be found */
    u_char category; /* a category index, table in help.cat */
    u_char hkind;    /* entry type; 0 for category, 1 for normal entry
                                2 for alias */
};

/* the category table is now incorporated in help.inx */

struct cat_en {
    char indname[14];      /* c string, name of category */
    short catdex;          /* index at generation, before sorting */
};

static int findex P_ARGS(( char *, Void *, int, int, int, int ));
static struct index_en *ind0;
static int cattable[NCAT];
static int nindex;    /* number of index entries */
static int ncat;      /* number of categories */
static void writepage();

/* exported functions in this module:  */
extern void makeindex(), 
            m_ghelp(),
            m_ghelpall(), 
            m_fixhelp(),
            addhelpfile(),
            browse();    /* all Mirella procedures */
            
extern void blanklin();
static void strnull();
extern char *get_file();		/* should be in tools */
extern char *strsrch(),*rstrsrch();	/* ditto */

/* static functions in this module */
static void tascii();
static int dishelp();
static int _iscat(),_selcat(),_allcat();
static int catcomp P_ARGS(( Const Void *, Const Void *));
static int inxcomp P_ARGS(( Const Void *, Const Void * ));
static int prslist P_ARGS(( Void *, int, int, int, int (*)() ));

static char *_nextword();
static int tabcpy();

extern FILE *mcfopenb(), *mcfopen();
extern int hasext();
extern char *mdir();
/* below useful for standalone mode */
extern void findmdir();
extern void erret();

#ifdef STANDALONE
/* ansi screen code. DO SOMETHING about this */
extern void get_scur(),put_scur(),home_scur(),sav_scur(),res_scur(),
    erase_line(),clear_scr(),num_keypd(),app_keypd();

extern normal x_scur,y_scur;
#else
static int save_last_page();
#endif


/******************** M_GHELP() (standalone MAIN() ) ***********************/
/*#define EDEBUG*/

static int catp;         /* used by comparison function _iscat() */
static int allflg = 0;   /* flag to list all primaries and aliases;
                            see m_ghelpall() */
static char onebuf[20];  /* buffer for name if there is a sole entry in
                            a category submitted to prslist() */
static char findbuf[20]; /* buffer for name actually found from possible
                            abbreviation by findex() */

#ifdef STANDALONE        /* compile as a main program */

#define cprint printf
#define scrprintf printf
#define mprintf printf
#define m_gets gets
#define mrgets gets

main(argc,argv) 
int argc;
char ** argv;
{
    char *str ;
    char *idx ;
    char ifname[64];
    char name[40];
    char catname[20];
    int iindex[20];    /* provision for up to 20 same-name entries */
    char *icdex[20];   /* category name table for above */
    int error;
    int c;
    unsigned short int whereis,where2;
    char buffer[20];
    char catbuf[20];
    char entbuf[20];
    char *cp, *scrname;
    int i, j, ioffs, nfind, ient, nent;
    int icat, jcat, hascat;
    int scrcat, iscr;
    int hasboth;
    int hasoent;
    int clen, elen;
    void blanklin();
    unsigned int sizinfil;

    findmdir();
    if(argc<2){
        printf("\nMust identify index file: USAGE: help indexfile entry\n");
        exit(1);
    }
    idx = argv[1];  /* index filename */                       
    strcpy(ifname,idx);    /* ifname is index filename */
    if(!hasext(ifname)) strcat(ifname,".inx");   /* index file must live in
                                                    current directory, so
                                                    you must run makeindex()
                                                    for each help file list 
                                                    for each application */

    if(argc > 2){
        str = argv[2]; 
        strcpy(name+1,str);
        name[0] = strlen(str);   
    }else{
        name[0] = name[1] = 0;
    }
    /* name is now in our own array in fstring form */

#else     /* compile as Mirella procedure g_help() */

void m_ghelp()       /* ( str indexfilename --- )
                    Mirella procedure. takes string address from stack,
                    reads index file, attempts to find entry in index, and
                    displays entry on terminal */
{
    char *str ;
    char *idx ;
    char ifname[64];
    char name[40];
    char catname[20];
    int iindex[20];    /* provision for up to 20 same-name entries */
    char *icdex[20];   /* category name table for above */
    int c;
    unsigned short int whereis;
    char buffer[20];
    char catbuf[20];
    char entbuf[20];
    char *cp, *scrname;
    int i, j, ioffs, nfind, ient, nent;
    int icat, jcat, hascat;
    int scrcat, iscr;
    int hasboth;
    int hasoent;
    int clen, elen;
    void blanklin();
    unsigned int sizinfil;

         /* input string code below changed 88/03/13 for new
            string conventions */
    idx = cspop;  /* index filename */                       
    strcpy(ifname,idx);    /* ifname is index filename */
    if(!hasext(ifname)) strcat(ifname,".inx");  
    
    str = cspop;   /* string stack address. str is a C string, note, and
                        we need a forth string for indexing */
    strcpy(name+1,str);
    name[0] = strlen(str);   /* name is now in our own array in fstring form */

#endif
    /* code below is common to procedure and standalone program */
    
    /* read index file into memory area. any exit hereafter must free ind0..
    Note that get_file() searches current dir first, then appl. home dir, then
    Mirella dir. */
#ifdef EDEBUG
    scrprintf("\nCalling get_file, filename %s\n",ifname);    
#endif
    ind0 = (struct index_en *)get_file(ifname,&sizinfil);  
#ifdef EDEBUG
    scrprintf("\nifname,ind0,sizinfil=%s %d %d\n",ifname,ind0,sizinfil);
    flushit();
#endif
    nindex = sizinfil/sizeof(struct index_en);  /* number of index entries */
    
    /* find the number of categories and construct category cross-reference
    table. We must do this each time now, since there can be many indexes */

    for(i=0;i<nindex;i++){
        if((ind0+i)->hkind != 0) break; /* plumb ran out of categories */
        jcat = (ind0+i)->category;
        if(jcat<0 || jcat >= NCAT) 
            scrprintf("\n%d not a legal category entry",jcat);
        cattable[(ind0+i)->category] = i;
#ifdef EDEBUG
        scrprintf("\ni,cattable,cat:%d %d %s",i, (ind0+i)->category,
            (ind0+i)->wname + 1);
#endif
    }
    ncat = i;
#ifdef EDEBUG
    scrprintf("\nncat = %d",ncat);
#endif

    /* code above here is never reentered--sorry about the spaghetti below--
    is isn't spaghetti unless something goes wrong, at which point we have
    to retrace our steps; the normal flow is linear and straight through */

    /* null name and come back to here if everything is screwed up */
printcat:    
    /* if name is null string, list categories and let user pick one */    
    if(name[0] == '\0'){  
        scrprintf(
"\n         Mirella Help Categories--type 'hhelp' for guide to help system\n");
        /* try just category list--should all be at beginning */
        prslist((Void *)ind0,sizeof(struct index_en),1,ncat,_iscat);
        scrprintf(
"\nCategory (<cr> to return to Mirella, * to see lists of all cats) ? ");
        m_gets(name+1); name[0] = strlen(name+1);
        if(name[1] == 0) goto out2;   /* cleanup and exit if <cr> alone */
        if(name[1] == '*'){     /* show all categories */
            for(i=0;i<ncat;i++){
                scrprintf(
                    "\n             Mirella Help Entries in Category <%s>\n",
                    ((ind0+i)->wname) + 1);
                catp = (ind0+i)->category;   /* global used by selcat */
                prslist((Void *)(ind0+i+1), sizeof(struct index_en),1,
			nindex-i-1,_selcat);
                scrprintf(
                    "\n\nHit return to continue, <esc> to return to Mirella");
                c = get1char(0);
                blanklin();
                switch(c){
                case '\n':
                case '\r':
                    break;
                case ESC:
                    goto out2;  /* leave */
                default:
                    break;
                }
            }
            /* go back and print categories and let user have a chance to
            pick one */
            name[0] = 0;
            goto printcat;
        }
        if(!strchr(name,'.')) strcat(name,".");   /* mark as category */
    }

    /* come back to here with user-entered name from query */    
parsename:
#ifdef EDEBUG
    scrprintf("\nParsing %d:%s",name[0],name+1);
#endif
    /* parse name; if it includes a '.', is in form of 'category.entry' */
    cp = strchr(name,'.');
    if(cp){
        *cp = '\0';              /* write over '.' to terminate cat field */
        strcpy(catbuf+1,name+1);
        catbuf[0] = strlen(name+1);
        strcpy(entbuf+1,cp+1);
        entbuf[0] = strlen(cp+1);
    }else{                      /* COULD still be a category name */
        strcpy(entbuf,name);
        catbuf[0] = catbuf[1] = '\0';
    }
    hasboth = (*catbuf != 0 && *entbuf != 0 ? 1 : 0 );  /* 1 if 'full' entry*/
    hasoent = (cp == name);    /* '.' is first char; forces to be entry */
#ifdef EDEBUG
    scrprintf("\ncatbuf,entbuf %d:%s  %d:%s",*catbuf,catbuf+1,*entbuf,entbuf+1);
#endif

    /* now find all occurrences of entries in entbuf and catbuf 
    (entbuf entry could be category if alone) */
    /* first look for explicit category in category list */
    hascat = 0;
    icat = -1;
    if(*catbuf){
        icat = findex(catbuf,(Void *)ind0,16,sizeof(struct index_en),0,ncat);
        if(icat == -1){
            scrprintf("\nNo such category as %s",catbuf+1);
            name[0] = '\0';
            goto printcat;   /* go back and try again. ugh. */
        }else{
            strcpy(catname,findbuf);        /* get category name */
            hascat = 1;                     /* has valid explicit category */
        }
    }
#ifdef EDEBUG
    scrprintf("hascat,icat = %d %d",hascat,icat);
#endif
    
    /* we at this point either have no explicit category or have a
    valid one. If the former, look for entry in category table; it might
    be one. */

    if(!*entbuf){
#ifdef EDEBUG
        scrprintf("\nno entbuf: copying catbuf to entbuf");
#endif
        strncpy(entbuf,catbuf,17);
    }else if(!hascat && !hasoent){   /* don't check if already have cat
                                        or explicitly want only entry */
        icat = findex(entbuf,(Void *)ind0,16,sizeof(struct index_en),0,ncat);
        if(icat >0 ) strcpy(catname,findbuf);
#ifdef EDEBUG
        scrprintf("\nlooked for %s in category list, icat = %d",entbuf+1,icat);
#endif
    }

    /* come back to here if have good category but bad entry; this code
    prints all main entries or all entries in the category  */
printentry:
    if(icat >= 0 && !hasboth){
        /* one way or another, have a valid category and no entry.
            Print entries and inquire*/
        catp = (ind0+icat)->category ;  /* global used by selcat */
        scrprintf(
            "\n             Mirella Help Entries in Category <%s>\n",
            catname);
        if(!allflg) {
	   nent = prslist((Void *)(ind0+icat+1),sizeof(struct index_en),
			  1,nindex-icat-1,_selcat);   /* only main entries */
	} else {
	   nent = prslist((Void *)(ind0+icat+1),sizeof(struct index_en),1,
			  nindex-icat-1,_allcat);   /* all, incl aliases */
	}
        /* nent is the number of entries. if nent=1, grab it for the 
        (only) entry; if bigger, inquire */
        if(nent > 1){
            scrprintf("\nEntry ?  ");
            m_gets(entbuf+1); entbuf[0] = strlen(entbuf+1);
            if(entbuf[0] == '\0') goto out2;
        }else{
            strcpy(entbuf+1,onebuf);
            entbuf[0] = strlen(entbuf+1);
        }
        hascat = 1;      /* at this point, have explicit category */
    }
    
    /* if have gotten this far, entbuf either contains a dud or a 
    (one of perhaps many) valid help entries. Find them. */
    ioffs = ncat;   /* jump over category entries */
    nfind = 0;
    ient = -1;
    for(i=0;i<20;i++){
        iindex[i] = findex(entbuf,(Void *)(ind0+ioffs),16,
            sizeof(struct index_en),0,nindex-ioffs);
        if(iindex[i] == -1) break;
        /* got one. iindex with respect to origin,ind0+offs, so fix */
        iscr = ioffs;
        ioffs += iindex[i] + 1;        
        iindex[i] += iscr;
        nfind = i+1;
        ient = iindex[i];
#ifdef EDEBUG
        scrprintf("\nfound %s at i= %d",entbuf+1,ient);
#endif
        /* now check if have a category match; unique if so */
        if(hascat && (ind0+ient)->category == (ind0+icat)->category){
            nfind = 1;
#ifdef EDEBUG
            scrprintf("\nmatching category: %d",(ind0+icat)->category);
#endif
            break;
        }   
    }
    /* check if nothing found */
    if(iindex[0] < 0){    /* nobody home; print category list and try again */
        if(hascat){
            hasboth = 0;   /* good category, entry bad, go back to category 
                            table */
            scrprintf("\nCannot find %s in this category; entries are:",
                entbuf+1);
            if(*catbuf) strncpy(entbuf,catbuf,17);                            
            goto printentry;
        }else{             /* go back to categories */
            scrprintf("\nCannot find %s; Categories are:\n",entbuf+1);
            name[0] = '\0';
            goto printcat;
        }
    }
    /* have at this point found all entries (and have at least one) */
    /* if several, list and ask for advice */
    if(nfind >1){
        scrprintf("\n%d entries found:",nfind);
        for(i=0;i<nfind;i++){
            scrcat = (ind0+iindex[i])->category;
            scrname = (ind0+iindex[i])->wname;
            for(j=0;j<ncat;j++){
                if(scrcat == (ind0+j)->category){     /* get category */
                    strncpy(buffer,(ind0+j)->wname,16); buffer[16] = '\0';
                    scrprintf("\n  %2d: %s.",i,buffer+1);
                    icdex[i] = (ind0+j)->wname;
                    break;
                }
            }
            strncpy(buffer,scrname,16); buffer[16] = '\0';
            if(j == ncat){ 
                scrprintf("\nERROR: no category %d found for %s",
                    scrcat,buffer+1);
                goto out2;
            }
            scrprintf("%s",buffer+1);
        }
        scrprintf("\nWhich do you want (number or (abbrev of) name) ?  ");
        m_gets(name+1); name[0] = strlen(name+1);
        if(name[1] == '\0') goto out2;   /* return to Mirella */
        /* see if user entered number */
        iscr = name[3];
        name[3] = '\0';
        if(isdigit(name[1])){     /* user entered number */
            i = atoi(name+1);
            /* now construct full name */
            clen = *icdex[i];                    /* length of cat name */
            strncpy(name,icdex[i],clen+1);       /* category */
            name[clen + 1] = '.';                /* separator */
            elen = *((ind0+iindex[i])->wname);   /* len of entry name */
            strncpy(name + clen + 2,(ind0 + iindex[i])->wname + 1,
                elen );                          /* entry */
            name[clen+elen+2] = '\0';            /* terminate */    
        }else name[3] = iscr;     /* user entered name */
        goto parsename;           /* go back and try again */
    }
    
    /* at this point, have one normal entry; make helpfilename */
    if(ient==-1) scrprintf("\nERROR: no entry found");
    strcpy(buffer,(ind0+ient)->filename);
    strcat(buffer,".hlp");
    
    whereis = (ind0+ient)->foffset;  /* offset into helpfile for entry */

    /* now get and display helpfile */
    if(dishelp(buffer,(unsigned long)whereis,1,'\\')== -1){
        scrprintf("\nM_GHELP:error opening/reading help file %s",buffer);
        goto out2;  /* cleanup and exit */
    }
    /* exits */
out2:
    free(ind0);
    allflg = 0;
    return;
}

/********************* DISHELP() *****************************************/
/*#define DDEBUG*/

static char contline[] =
"<*Browse: <cr> or <sp> for more, - for back, ESC for return, / for search *>";
static char endline[] =
"<*** EOF: - for back, ESC or <cr> for return, ^B for beginning ************>";
static char disline[] =
"<*** LAST HELP PAGE DISPLAYED *********************************************>";

#define STATLIN()                   \
{                                   \
    x_scur = 1;                     \
    y_scur = plen+2;                \
    put_scur();                     \
    blanklin();                     \
}
/* puts cursor on status line and blanks it */


static int
dishelp(fname,offs,bdy,delim)  /* displays help files in vaguely 
                    "more"-like format. fname is the filename, offs 
                    the initial offset, bdy a flag which, if on, restricts
                    the search and display to the region offset to the first
                    occurrence of the char delim; if off, the whole file may 
                    be searched and displayed. If bdy is on, the routine
                    skips the first line, which is supposed to be a help-file
                    identifier; if off, it does not, the supposition being that
                    the whole file is to be browsed. Returns 0 if successful,
                    -1 if there is a file error */
    char *fname;
    unsigned long offs;
    int bdy;
    char delim;
{
    char *fbody;        /* pointer to file text ret by get_file() */
    unsigned int flen;  /* length of file        ""   ""  "" */
    char **linep;       /* line pointers. try this; may be too small */
    u_char *linelen;		/* line lengths */
    int maxlines;       /* maximum line number under allocation strategy
                            used by get_file(), which allocates twice file
                            size */
#ifdef STANDALONE
    int plen = 21;
#else                                
    int plen = m_fterm.t_nrows -3 ; 
#endif
                        /* length of help page, usually 2*pstep */
    int pstep = 2*plen/3;
                        /* advance/back  by pstep lines in browse mode */

    char *orig;         /* origin of text; either fbody or fbody+offs */
    int nline;          /* # of lines in searched part of text */
    register char *cp;  /* dummy */
    register char *fend; /* end of search region */ 
    int ceol;           /* first eol char, cr or nl */
    char eol[2];        /* eol string */
    int ll,c,i;         /* dummy */
    int lindex;         /* running line index during display */
    int endflg;         /* flag for end of text */
    char buf[80];
    char temp[80];
    char srch[80];      /* search string */
    char *srchp=NULL;   /* search pointer */
    int srchlen;        /* length to search from current search pointer */
    int srchflg;        /* present screen was disp. as a result of a search*/
    int srchlin;        /* line of last succ. search */
    int srchcol;        /* col of    ""    ""        */
    int backflg;        /* backward search ? */

    clear_scr();

    /* get eol char */
    strcpy(eol,EOL); ceol = eol[0];  
    /* open and read help file */
    if((fbody = get_file(fname,&flen)) == NULL) return (-1);
    /* make line pointer array */    
    maxlines = 4*(flen/20 + 1) + 512;
    linelen = (u_char *)(fbody + 4*(flen/4 + 1) + 512);
    linep = (char **)(linelen + maxlines + 128);  
    orig = fbody;
    if(bdy) orig += offs;
    fend = fbody + flen;
    srch[0] = '\0';      /* init search string */
    srchp = orig;
#ifdef DDEBUG
    scrprintf("\nceol,fbody,flen,fend=%d %d %d %d",ceol,fbody,flen,fend);
#endif
    nline=0;
    cp = orig;
    if(!bdy)linep[nline++] = orig;  /* if bdy, want to skip ID line */
    while(cp < fend){
        c = *cp;
        if(c == ceol){
            if(nline){ 
                ll = cp - linep[nline-1];   /* linelength is NOT chkd here!!*/
                linelen[nline-1] = ll;     /* linelen is # of printing chars*/
            }
            *cp = 0;
            cp += LEOL;
            linep[nline++] = cp;
            if(nline == maxlines){
                scrprintf("\nDISHELP:Helpfile has too many lines");
                free(fbody);
                return(-1);
            }
            c = *cp;
            if(bdy && c == delim){
                fend = cp-LEOL;
                break;
            }
        }else cp++;
    }
    nline--;     /* last line counted begins at EOR or EOF; back up */

#ifdef DDEBUG    
    scrprintf("\nnline=%d",nline);    
    naprt(linep,nline,"line pointers");
    caprt(linelen,nline,"line lengths");
    if(key() != '\r') return;
#endif
    *fend = '\0';   /* just in case there is no newline at end. The funny
    line # arithmetic also assures that the last line is ok if no newline */
    /* now have array of line origins and lengths. This array can be fed to
    writepage() to output page in some system-satisfactory manner */
   
    lindex = 0;
    if(pstep >= plen -1) pstep = plen-1;  /* pages must overlap */
    
    while(1){
        backflg = 0;                       /* def. search fwd */
        endflg = (nline-lindex <= plen);   /* EOF on page ?? <= try 890202 */
        /* write the page out */
        if(endflg) writepage(linep,linelen,nline,lindex,plen,endline);
        else writepage(linep,linelen,nline,lindex,plen,contline);
        if(!srchflg){
            STATLIN();  /* get cursor out of way */
        }else{    /* return from successful search */
            x_scur = srchcol + 1;
            y_scur = srchlin - lindex + 1;
            put_scur();
        }
        get1char(1);             /* set raw mode, so can catch ^S */
        c = get1char(0);         /* raw getchar */
        get1char(EOF);           /* go back to cooked */
        switch(c){
        case ESC:
             goto getout;
        case '-':   
            lindex -= pstep;
            if(lindex < 0) lindex = 0;
            srchflg = 0;
            break;
        case CTRL_B:
            lindex = 0;
            srchflg = 0;
            break;
        case CTRL_R:  /* backward search */
            backflg = 1;
            /* fall through */            
        case '/':     /* search forward */
        case CTRL_S:  /* if you are lucky */
            STATLIN();
            sprintf(temp,"SEARCH STRING(%s): ",srch);
            (*scrnprt)(temp);
            mrgets(buf);
            if(buf[0]){   /* new search string */
                strcpy(srch,buf);
                srchflg = 0;
                srchlin = (backflg ? lindex + plen -1 : lindex);   
                   /* start at top of page (bottom if srch back) */
                if(srchlin >= nline) srchlin = nline-1;                   
                srchcol = 0;
                srchp = 0;
            }
            if(!srch[0]) break;     /* no search string */
            if(!backflg){   /* search fwd */
              for(i=srchlin;i<nline;i++){
                srchp = (srchp && i==srchlin ? srchp : linep[i]);
                srchlen = (srchp && i==srchlin ? linelen[i] -(srchp - linep[i])
                             : linelen[i]);
                if(srchlen <= 0){
                    srchp = 0;
                    continue;                            
                }
                if((srchp = strsrch(srch,srchp,srchlen)) == NULL) break;
              }
            }else{          /* search back */
              for(i=srchlin;i>=0;i--){
                srchlen=(srchp && i==srchlin ? srchp - linep[i] - strlen(srch)
                            : linelen[i]);
                if(srchlen <= 0){
                    srchp = 0;
                    continue;                            
                }
                srchp = linep[i];
                /* if last srch was successful, start there; else at line orig*/
                if((srchp = rstrsrch(srch,srchp,srchlen)) != NULL) break;
              }
            }
            if(!srchp){
                STATLIN();
                scrprintf("*** NOT FOUND ***");
            }else{   /* found it */
                srchflg = 1;
                srchlin = i;
                srchcol = srchp - linep[i];
                lindex = srchlin - plen/2;
                if(lindex < 0) lindex =0;
            }
            break;            
        case CR:
        case NL:
            if(endflg){
                goto getout;
            }
            /* fall through */
        case '+':
        case ' ':
            if(lindex + plen < nline) lindex += pstep;
            srchflg = 0;
            break;
        default:
            break;
        }
    }
getout:
#ifndef STANDALONE
    save_last_page(linep,linelen,nline,lindex,plen);
#endif
    free(fbody);

    /* put the cursor back where you found it. This, due to the cupidity
       of terminals, is not always easy. */


#ifndef STANDALONE
    res_scr();
#endif

    return 0;
}

/************************** WRITEPAGE() ************************************/

static void
writepage(lpa,lla,nline,ln,lp,cap)
char **lpa;				/* lline pointer array */
u_char *lla;			/* line length array */
int nline;				/* total number of lines */
int ln;					/* starting line # */
int lp;					/* number of lines to write */
char *cap;				/* pointer to caption line */
{
    /* we will first do this in a line-by line fashion, OK ? LATER we can get
    fancy, if we have to */
    
    char line[200];
    int i;
    int end;        /* index of last line + 1 */
    int rest=0;     /* # blank lines after end of text before caption line*/
    int len;        /* dummy; length of string */
    int tlen;       /* length of terminal line */
    int lines = 0;
  
    tlen = 79;  /* we have a goddamn term structure--why are we not using?*/

    home_scur();    /* start at top of page */
    
    end = ln + lp;
    if (end > nline){
        end = nline;
        rest = lp - (end-ln);
    }
    for(i=ln;i<end;i++){          /* compose and write page */
        len = lla[i];
        if(len) len = tabcpy(line,lpa[i],len);   /* expands tabs to spaces */
        if(tlen>len) filln(tlen-len,line+len,' ');
        line[tlen] = '\n';
        line[tlen+1] = '\0'; /* terminate line buffer */
#ifndef DOS32        
        (*scrnprt)(line);
#else
        if(dosansiflg) dos_scrline(line,lines);
        else (*scrnprt)(line);
#endif
        lines++;
    }
    filln(tlen,line,' ');
    for(i=0;i<rest;i++){
#ifndef DOS32        
        (*scrnprt)(line);
#else
        if(dosansiflg) dos_scrline(line,lines);
        else (*scrnprt)(line);
#endif
        lines++;
    }
    len = strlen(cap);
    strcpy(line,cap);
    if(tlen-len) filln(tlen-len,line+len,' ');
#ifndef DOS32        
    (*scrnprt)(line);
#else
    if(dosansiflg) dos_scrline(line,lines);
    else (*scrnprt)(line);
#endif
    lines++;
    x_scur = 1;
    y_scur = lines + 1;
    put_scur();
}

/************* TABCPY() ***************************************************/
/*
 * copies src to dest, expanding tabs as it goes; returns actual length
 */
static int 
tabcpy(dest,src,len)
register char *dest, *src;
register int len;              /* length of source string */
{
    register int c;
    char *d0 = dest;
    int loc,nblk;
    
    while(len--){
        if((c = *src++) != '\t'){
            *dest++ = c;
        }else{                /* have a tab */
            loc = dest-d0;        
            nblk = (loc/8 + 1)*8 - loc;  /* number of blanks */
            while(nblk--) *dest++ = ' ';
        }
    }
    return dest-d0;
}

/************* SAVE_LAST_PAGE(), RES_HELP() *******************************/

#ifndef STANDALONE
/* this code is used only under Mirella */

/*#define SLPDEBUG*/

static char *llptr[MAXSCRNL];
static u_char lllen[MAXSCRNL];
static int  llnline;
static int  lplen;
static int  help_sav_flg = 0;

static int
save_last_page(lpa,lla,nline,lindex,plen)
char **lpa;
u_char *lla;
int nline;
int lindex;
int plen;     /* same arguments as writepage() above */
{
    int i;
    int len;
    
    llnline = nline - lindex;
    if(llnline < 0) erret("\nsave_last_page:negative line count");
    if(llnline > plen) llnline = plen;
    lplen = plen;
#ifdef SLPDEBUG
    scrprintf("\ns_l_p:lpa %d,lla %d,nline %d,lindex %d,plen %d,helpscrptr %d",
        lpa,lla,nline,lindex,plen,helpscrptr); fflush(stdout);
#endif
    for(i=0;i<llnline;i++){
        llptr[i] = (char *)helpscrptr + i*(MAXSCRLL + 1);        
        len = lla[lindex+i];
        if(len > MAXSCRLL) len=MAXSCRLL;
        lllen[i] = len;
#ifdef SLPDEBUG
        scrprintf("\nline %d:copying %d bytes from %d to %d",i,len,lpa[lindex+i],
            llptr[i]); fflush(stdout);
#endif
        if(len)bufcpy(llptr[i],lpa[lindex+i],len);
    }
    help_sav_flg = 1;
    return 0;
}

int
res_help()    /* rewrites last help page */
{
    if(!help_sav_flg) return 0;
    writepage(llptr,lllen,llnline,0,lplen,disline);
    return 1;
}

#endif /* ifndef STANDALONE */ 
    
/************************** BLANKLIN() ***********************************/

void blanklin() /* blanks a console line, leaves cursor pos at beginning */
{
    int i;
    char buffer[82];

    for(i=0;i<81;i++)
        buffer[i] = ' ';
    buffer[0] = '\r'; 
    buffer[79] = '\r';
    buffer[80] = '\0';
    (*scrnprt)(buffer);
    (*scrnflsh)();
}


/**************************** FINDEX() ******************************/

/* findex looks for the char array name in an array of nelt elements of
size elsize, pointed to by the ptr array.  It uses the smaller of
nsigch or strlen(name) characters of significant comparison, and ignores 
the count byte (this allows abbreviations). It expects the comparison
array to begin at an offset offset into each element.  It returns the
array index if successful, -1 if not.  (It should ret a pointer). The
string in the array actually matched is placed in the static string 
findbuf */

static int 
findex(name,array,nsigch,elsize,offset,nelt)
char *name;  	  		/* ptr to name to be found */
Void *array;			/* ptr to array */
int elsize;			/* size of elements in array (typ structures) */
int nsigch;			/* # of sig chars in name */
int nelt;			/* # of elements of size elsize in array */
int offset;			/* offset into an element for start of string */
{
    register int j;
    char *cp1 = (char *)array + offset;
    int i = 0;

    if(nelt <= 0) return -1;
    j= strlen(name);
    if(nsigch>j) nsigch = j;

    while(1){
        for(j=1;j<nsigch;j++){    /* skip count byte */
            if( cp1[j] != name[j]) break;
        }
        if(j == nsigch){   /* foundit!! */
            strcpy(findbuf,cp1+1);
            return i;
        }else{
            i++;    /* go to next one */
            if(i == nelt){    /* not there */
                return ( -1);
            }
            cp1 += elsize;
        }
    }
}

/******************** STRSRCH(),RSTRSRCH() *******************************/

char *
strsrch(sf,s,len)   /* searches for a string sf in the string s
                         of length n; returns pointer to first char
                         FOLLOWING if found, 0 if not; case-insensitive */
char *sf;
register char *s;
register int len;                         
{
    register int cs = tolower(*sf);   /* first char of search str */
    register int c;
    char *cp,*cps;
    
    while(len--){
        c = *s++;
        if(tolower(c) == cs){  /* match first ? */
            cps = sf+1;
            cp = s;  /* points to first char foll match */
            while((c = tolower(*cps)) != '\0' && c == tolower(*cp)){
                cps++; 
                cp++;
            }
            if(c == 0) return(cp);   /* got it */
        }
    }
    return(0);  /* no dice */
}

char *
rstrsrch(sf,s,len)   /* searches backwards for a string sf in the string s
                         of length n; returns pointer to first char
                         FOLLOWING if found, 0 if not. Search is 
                         case-insensitive   */
char *sf;
register char *s;
register int len;                         
{
    register int cs = tolower(*sf);   /* first char of search str */
    register int c;
    char *cp,*cps;
    
    s += len;
    while(len--){
        c = *(--s);
        if(tolower(c) == cs){  /* match first ? */
            cps = sf+1;
            cp = s+1;  /* points to first char foll match */
            while((c = tolower(*cps)) != '\0' && c == tolower(*cp)){
                cps++; 
                cp++;
            }
            if(c == 0) return(cp);   /* got it */
        }
    }
    return(0);  /* no dice */
}

/*********************** GET_FILE() *******************************/
/*#define GDEBUG*/
/*
 * reads (bin) the file 'name', starting at offset 'offset'  and returns
 * ptr to memory image. uses malloc. pfsize is a pointer to a long; the 
 * filesize (from offset to the end) in bytes is placed there. Note that
 * line terminator is /r/n in some systems, since read is binary. Returns NULL 
 * on error. REMEMBER TO FREE RETURNED POINTER.
 */

char *
get_file(name,pfsize)
char *name;
unsigned int *pfsize;
{
    char *ixp;
    int ifdes;
    int error;
    int ixlen;
    char *fn;
    normal fsize;

    if((fn = mcfname(name)) == NULL || (ifdes=openb(fn,O_RDONLY)) == -1) {
       scrprintf("Cannot open %s in this, home, or Mirella dir\n",name);
       return(0);
    }
#ifdef GDEBUG
    scrprintf("\nGET_FILE:name=%s",fn); flushit();
#endif
    lseek(ifdes,0L,2);
    fsize = lseek(ifdes,0L,1);
    lseek(ifdes,0L,0);
    ixlen = fsize;
    ixp = (char *)(malloc((2*ixlen) + 4096));  /* just to be safe,
                                leaves room for line pointers at end */
    if(!ixp){
        close(ifdes);
        scrprintf("Cannot allocate memory for %s image \n",name);
        flushit();
        return (0);
    }
#ifdef GDEBUG
    addr = (long)ixp;
    scrprintf("   READING:ixlen=%d, ixp=%lx",ixlen,addr); flushit();    
#endif
    error = read(ifdes,ixp,ixlen); /* N. B. filesize limited in DSI32 system 
                                        to 32640 bytes */
#ifdef GDEBUG
    scrprintf("error,ixlen,ifdes,ixp = %d %d %d %d\n",
        error,ixlen,ifdes,ixp);
#endif
    if(error != ixlen){
#ifdef VMS   /* not a stream file ?? */
        if(error != 0){  /* got something */
            do{
                nrd = read(ifdes,ixp+error,ixlen-error);
                error += nrd;
            }while( nrd > 0 && error < ixlen);
            ixlen = error;   /* actual length of text; this is space which
                                needs to be scanned */
            goto out; /* no error checking, note; I don't know how--
                         error is not in general the same as ixlen */
        }
#endif
        scrprintf("\nRead error reading %s:req %d bytes, read %d bytes:",
            name,ixlen,error); flushit();
        close(ifdes);
        free(ixp);
        return(NULL);
    }

    *pfsize = ixlen;
    close(ifdes);
    return(ixp);
}

/********************* _SELCAT(), _ISCAT(), _ALLCAT() ***********************/

static int 
_selcat(ep)       /* true if category of *ep is catp and *ep is a normal
                            entry */
    struct index_en *ep;
{
    return (ep->category == catp && ep->hkind == 1);
}

static int 
_iscat(ep)			/* true if *ep is a category entry */
struct index_en *ep;
{
    return (ep->hkind == 0);
}

static int 
_allcat(ep)      /* true if category of *ep is catp and *ep is not a
                        category entry */
    struct index_en *ep;
{
    return (ep->category == catp && ep->hkind != 0);
}

/************************* PRSLIST() *******************/

/* prslist prints a list in 'ls'-type 4-column format from the array array
of elements of size elsize and nelt elements; the strings to be printed
start at offset bytes from the beginning of each element. Only the first
fourteen bytes of each string are printed; the strings must be null-
terminated. pgo is a pointer to a function which, when handed the pointer
to an element, returns 1 if the element is to be printed, 0 if not. */

static int 
prslist(array,elsize,offset,nelt,pgo)
Void *array;
int elsize;
int offset;
int nelt;
int (*pgo)();
{
    register int i;
    register char *elt = array;
    int nprt = 0;
    int line = 0;
    char c;
    char buffer[20];

    scrprintf("\n\r");
    get1char(1);  /* set raw mode */

    for(i=0; i<nelt; i++){
        if((*pgo)(elt)){
            strncpy(buffer,elt+offset,14);
            buffer[14] = 0;
            scrprintf("%-19s",buffer);
            nprt++;
            if(!(nprt%4)){
                scrprintf("\n\r");
                line++;
                if(!line%22){
                    scrprintf("<sp>,<cr> for more, ESC to return");
                    do{
                        c = get1char(0);    /* raw getchar */
                        if(c == ' ' || c == CR || c == NL) break;
                        if(c == ESC) goto out;
                    }while(1);
                blanklin();
                }
            }
        }
        elt += elsize;
    }
    if(nprt == 1) strcpy(onebuf,buffer);
out:
    scrprintf("\n\r");
    flushit();
    get1char(EOF);
    return (nprt);
}

#ifndef STANDALONE     /* compile below in Mirella module */

/*********** MIRELLA-SPECIFIC CODE AND INDEX CODE *******************/

/************************ M_GHELPALL() ******************************/
void m_ghelpall()    /* Mirella procedure to display all entries including
                        aliases, in a category; if the string on the ss is
                        not a category, result is identical to m_ghelp */
{
    allflg = 1;
    m_ghelp();
    allflg = 0;
}

/******************* _NEXTWORD() ********************************/
/* The next routine returns pointer to first nonwhite char in the
string *cpp.  Places null in pos of first whitespace following, and
advances *cpp to first char following terminating whitespace.  If no
following whitespace, advances *cpp to terminating null. If this routine is
used for strings on Mirella string stack, best to copy them to scratch
buffer first. This routine is slightly different from the one in 
mircintf.c, and is named with an underscore. Sometime make it the
same, and get rid of this version. */

static char *
_nextword(cpp)
    char **cpp; 
{
    char *word,*ptr,c;

    ptr = *cpp;
    while(isspace(*(ptr++))) continue; /* skip to first nonwhite char */
    word = --ptr;           /* first nonwhite char (or term null if none) */
    while((c = *(ptr++)) != '\0' && !isspace(c)) continue;
    *(--ptr) = '\0';       /* make it null if not already */
    if(c) ptr++;           /* if c is not term null, adv ptr to next char */
    *cpp = ptr;
    return  word;
} 

/************************** BROWSE() ********************************/

void
browse()   /* Mirella procedure; displays any ascii file */
{
    char *fname = cspop;
    
    dishelp(fname,0,0L,0);
}

/*********************** ADDHELPFILE() **********************************/
/* this Mirella procedure adds a helpfile to the list from which makeindex
makes the index */

void addhelpfile()  /* ( helpfilename listfilename --- ) The listfile
                    should in general have the same basename as the index
                    file made from it; listfilename need have no extension
                    here; if none, ".lis" is assumed */
{
    char *lfn = cspop;
    char *fname = cspop;
    FILE *lptr ;   /* pointer to list file, containing names of help files */
    char lfname[64];
    
    strcpy(lfname,lfn);
    if(!hasext(lfname)) strcat(lfname,".lis");

    if((lptr = fopena("lfname","a")) == NULL) {
        scrprintf("Cannot open %s\n",lfname);
        return;
    }
    fprintf(lptr,"%s\n",fname);
    fclose(lptr);
}

/*********************** MAKEINDEX() **********************************/

/* This Mirella procedure generates a help index file from the file list
in list file whose name is on the stack.  The name of the index file has
the same base as the list file, but has the extension .inx.  The index
file is written in the current directory, but the list file and help
files may be in either the current or the Mirella directory */

void makeindex()
{
    char *cp, *cp1, *cp2, *cf;
#ifdef MSDEBUG
    char * cp3;
#endif
    int c;
    int fnp;
    FILE *hfp;
    char buffer[82];
    char catbuf[20];
    char hfname[30];
    struct index_en tindex;
    struct index_en *indp;
    struct cat_en *catp;
    int i,len;
    int ncat = 0;
    int nindex = 0;
    int lindex;
    int isexcat = 0;
    int indes;    /* descriptor for index file */
    FILE *lfp ;   /* pointer to list file, containing names of help files */
    long floc,loc,oloc;
    char *rn;
    char *lfn = cspop;
    char lfname[64],ifname[64];

    strcpy(lfname,lfn);   /* get list filename */
    if(!hasext(lfname))strcat(lfname,".lis");
    strcpy(ifname,lfname);    
    cp = strrchr(ifname,'.');
    strcpy(cp,".inx");   /* produce index filename */

    /* allocate memory for category array */
    catp = (struct cat_en *)malloc(NCAT*sizeof(struct cat_en));
    if( !catp){
        scrprintf("Cannot allocate memory for category array\n");
        return;
    }
    /* create index file */
    indes = creatb(ifname,0666);
    if( indes == -1){
        scrprintf("Cannot create index file: %s\n",ifname);
        free((char *)catp);
        return;
    }
    /* open list file */
    if((lfp = mcfopen(lfname,&rn)) == NULL) {    /* try both current and Mirella
                                            directories */
        free((char *)catp);
        return;
    }
    mprintf("\nFiles from %s",rn);
    /* read helpfiles names from list */
    do{
    	hfname[0] = '\0';
        fnp=(int)fgets(hfname,30,lfp);      /* get filename */

        cf = hfname-1;
        while((c = *++cf) != '\0' && c != '\n' && c != '\r')
           continue;				/*find null or newline*/
        *cf = '\0';                                    /* kill it */
        if(isspace(c =hfname[0]) || !c ) continue;    /* skip blank lines */

        mprintf("\nOpening %s",hfname);
        if((hfp=mcfopenb(hfname,0)) == NULL) {
            continue;   /* just announce helpfiles not found; go on */
        }
        cf = (char *)strchr(hfname,'.');    /* find extension separator */
        if(cf) *cf = '\0';         /* truncate it */
        loc = 0;            /* char location in file */
        do{
            do{
                buffer[0] = '\0';  /* unix fgets keeps old buffer on EOF */
                cp = (char *)fgets(buffer,80,hfp);
                oloc = loc;   /* before current read */
                loc += strlen(buffer);  /* after current read */
                cp1= buffer;
                cp2 = _nextword(&cp1); /* 1st word; cp2 is 0 for blnk lines */
#ifdef MSDEBUG
                scrprintf("%s buf,cp1,cp2=%d %d %d\n",
                    buffer,buffer,cp1,cp2);
#endif
            }while(!*cp2 && cp); /* reads over any blank lines;
                                stops at contented line or EOF */

            floc = oloc;           /* character offset to begin of name line */
            if(!cp && !buffer[0]) break;        /* if EOF and empty line, quit*/
            strnull(tindex.wname,16);           /* clear name */
            tindex.wname[0] = strlen(cp2);      /* length byte */
            *(cp2+14) = '\0';                   /* make life easier */
            strcpy(tindex.wname+1,cp2);         /* Mirella name */
            strcpy(tindex.filename,hfname);     /* source filename, sans ext */
            tindex.hkind = 1;                   /* normal entry, probably */

            mprintf("\nword:%3d %-15s  ",tindex.wname[0],cp2);


            cp2 = _nextword(&cp1);             /* get next word */
#ifdef MSDEBUG
            cp3 = cp1;   /* debugging */
            scrprintf("\n oldcp1,cp2,cp1,nextword:%d %d %d:%s\n",
                cp3,cp1,cp2,cp2);
#endif
            if(*cp2 == '*') {   /* indirect reference (alias) */
                tindex.hkind = 2;   /* alias */
                goto wrtit;     /*  write existing structure with
                                        new name */
            }
            if(*cp2 == '+') {   /* indirect reference (alternate) */
                tindex.hkind = 1;   /* normal entry */
                goto wrtit;     /*  write existing structure with
                                        new name */
            }
            do{
                cp1 = (char *)fgets(buffer,80,hfp);
                oloc = loc;   /* before current read */
                loc += strlen(buffer);  /* after current read */
#ifdef MBDEBUG
                scrprintf(":%s: buf,cp1=%d %d \n",buffer,buffer,cp1);
#endif
            }while(*cp1 != '\\' && cp1);    /* look for line beg. with bs */
            if(!cp1 && *cp1 != '\\'){
                mprintf("\nNo category line for last entry in %s",fnp);
                break;
            }
            cp1 = buffer+1;         /* have a category line; skip backslash */
            cp2 = _nextword(&cp1) ;      /* category name */
            *(cp2+13) = '\0';            /* chop; max len with null is 14 */
            strcpy(catbuf,cp2);          /* for printing */

            isexcat = findex(cp2,(Void *)catp,14,sizeof(struct cat_en),0,ncat);
            if(isexcat == -1){           /* new category */
                if(ncat >= NCAT){
                    free((char *)catp);
                    close(indes);
                    fclose(hfp);
                    fclose(lfp);
                    mprintf("Too many categories; limit %d\n",NCAT);
                    erret((char *)NULL);
                }
                strnull(catp[ncat].indname,14);
                strcpy(catp[ncat].indname,cp2);
                isexcat = ncat;
                catp[ncat].catdex = ncat;    /* indices will be scrambled
                                                by sorting at end */
                ncat++;
            }
            tindex.foffset = floc;
            tindex.category = isexcat;
wrtit:
            mprintf("cat: %-13s ",catbuf);
            mprintf(":c=%2d k=%d n=%4d offs=%5d",isexcat,tindex.hkind,
                nindex,floc);
            write(indes,(char *)&tindex,sizeof(struct index_en));
            nindex++;
            flushit();
        }while(cp);    /* end of do reading current help file, stops at EOF */
        fclose(hfp);     /* current help file */
        pauseerret();
    }while(fnp);             /* end of do reading help files from list, stops
                                    at EOF  */
    if(lfp) fclose(lfp);        /* helpfile list */
    /* now do sorts */
#ifdef MDEBUG
    scrprintf("\nDoing sorts; ncat=%d, nindex=%d ",
        ncat, nindex );
#endif
    if(!ncat || !nindex){    /* no entries */
        free(catp);
        close(indes);
        return;
    } 

    flushit();
    qsort((Void *)catp,ncat,sizeof(struct cat_en),catcomp);
				/* catp now points to sorted category array */
    lindex = lseek(indes,0L,1);		/* length of index file */

#ifdef MDEBUG
    for(i=0;i<ncat;i++)
        scrprintf("\ncatsort=%-15s,cat=%d",(catp+i)->indname,
            (catp+i)->catdex);
    scrprintf("lindex=%d\n",lindex);
#endif
    lseek(indes,0L,0);      /* rewind */
    indp = (struct index_en *)malloc(lindex);
    if(indp == 0){
        scrprintf("Cannot allocate memory for index file\n");
        close(indes);
        free(catp);
        return;
    }
    read(indes,(char *)indp,nindex*sizeof(struct index_en));
#ifdef MDEBUG
    scrprintf("\nnindex = %d",nindex);
    for(i=0;i<nindex;i++){
        scrprintf("\nindex = %d",i);
        scrprintf("\nn=%-15s,f=%-15s,o=%5d,c=%3d,k=%2d",
            (indp+i)->wname+1,(indp+i)->filename,(indp+i)->foffset,
            (indp+i)->category,(indp+i)->hkind);
    }
#endif
    lseek(indes,0L,0);
    qsort((Void *)indp,nindex,sizeof(struct index_en),inxcomp);
#ifdef MDEBUG
    scrprintf("\nnindex = %d,now sorted",nindex);
    for(i=0;i<nindex;i++){
        scrprintf("\nindex = %d,now sorted",i);
        scrprintf("\nn=%-15s,f=%-15s,o=%5d,c=%3d,k=%2d",
            (indp+i)->wname+1,(indp+i)->filename,(indp+i)->foffset,
            (indp+i)->category,(indp+i)->hkind);
    }
#endif
    /* now write category entries */
    for(i=0; i<ncat; i++){
        len = strlen((catp+i)->indname);
        strcpy(tindex.wname+1,(catp+i)->indname);
        *(tindex.wname) = len;
        tindex.hkind = 0;  /* category */
        *(tindex.filename) = '\0';
        tindex.foffset = 0;
        tindex.category = (catp+i)->catdex;
        write(indes,(char *)&tindex,sizeof(struct index_en));
    }
    /* and rest of index */
    write(indes,(char *)indp,nindex*sizeof(struct index_en));
    close(indes);
    free(indp);
    free(catp);
    flushit();
    return;
}

static int 
catcomp(a,b)
Const Void *a, *b;
{
    return(strcmp(((struct cat_en *)a)->indname,
		  ((struct cat_en *)b)->indname));
}

static int 
inxcomp(a,b)
Const Void *a, *b;
{
    return(strcmp(((struct index_en *)a)->wname+1,
		  ((struct index_en *)b)->wname+1));
}

static void 
strnull(s,l)   /* fills the char array s with l NULLs */
register char *s;
int l;
{
    register char *end = s + l;
    while(s < end) {*s++ = '\0' ; }
}

/********************* TASCII() *********************************/
static void
tascii(in,out)  /* takes a string and expands tabs to 4 spaces; removes
                        all control chars except cr and nl */
    register char *in, *out ;
{
    register char c ;
    register int l;
    int  k; 

    l = 0;  /* printable outpur char cnt. always points to next char */
    while((c = *in++) != '\0') {
        if(isprint(c) || c == '\n'|| c == '\r'){
            *out++ = c;
            l++;
        }else if(c == '\t'){   
            k = 4*(l/4 + 1); /* stopping place. always at least one
                                            space */
            for(; l< k; l++)
                *out++ = ' ';
        }
    }
    *out = '\0' ;
}

/********************* M_FIXHELP() ********************************/

void m_fixhelp()     /* Mirella procedure; expands tabs in help files */
{
    char *name, inbuf[100], outbuf[100];
    FILE *infp, *outfp;
    int error, werror;

    name = cspop;
    if((infp = fopena(name,"r")) == NULL) {
        scrprintf("Cannot open %s",name);
        erret((char *)NULL);
    }
    if((outfp = fopena("temp.hlp","w")) == NULL) {
        scrprintf("Cannot open output file");
        fclose(infp);
        erret((char *)NULL);
    }
    do{
        error = (int)fgets(inbuf,81,infp);
        if(error == 0 && inbuf[0] == 0) break;
        tascii(inbuf,outbuf);
        werror = (int)fputs(outbuf,outfp);
        if(werror == -1){
            fclose(outfp);
            fclose(infp);
            erret("Error writing to temp.hlp; input is OK");
        }
    }while(error);
    fclose(infp); 
    fclose(outfp);
    unlink(name); 
    rename("temp.hlp",name);
}

#else     /* ifndef STANDALONE -- compile below in standalone module */

/******** STANDALONE IS DEFINED; MUST DEFINE ALL FUNCTIONS WHICH ARE ********/
/*********** IN OTHER MODULES IN MIRELLA ************************************/

char home_dir[100],temp_dir[100],mir_dir[100];  /* use here only */

/*************** FINDMDIR(), MDIR(), HDIR() ****************************/

extern char home_dir[];
extern char temp_dir[];
extern char mir_dir[];
static char nullstr[] = "";

void findmdir()
{
    int i;
    char *getenv();
    char *md;

    md = getenv("MIRELLADIR");
    if(!md || !(*md)){ 
        printf("\nFINDMDIR:Cannot find MIRELLADIR in the environment; I quit");
        erret((char *)NULL);
    }else{
        if(strlen(md)>64){
            printf("\nMIRELLADIR=%s: too long",md); flushit();
            erret((char *)NULL);
        }
        strncpy(mir_dir,md,64);
    }
    md = getenv("MIRTEMP");
    if(!md || !(*md)){ 
        printf(
      "\nFINDMDIR:Cannot find MIRTEMP in the environment; setting to null");
        md = nullstr;
    }else{
        if(strlen(md)>64){
            printf("\nFINDMDIR:tempdir=%s: too long",md); flushit();
            erret((char *)NULL);
        }
        if(md){
            strncpy(tempdir,md,64);
            strncpy(temp_dir,md,64);
        }else{
            tempdir[0] = '\0';
            temp_dir[0] = '\0';
        }
    }
    /* now attend to home directory. Note that home_dir is the name of the
    home directory with no cated separator, and is declared in mirella.c */
    if(!(getcwd(home_dir,100))){
        printf("\nFINDMDIR:Error finding home directory");
        erret((char *)NULL);
    }
    if(strlen(home_dir)>64){
        printf("\nFINDMDIR:Home directory name too long");
        erret((char *)NULL);
    }
}

char *
mdir(s)     /* returns pointer to filename in mirella dir, given base+ext */
    char *s;
{
    static char dirstring[72];

    strcpy(dirstring,mirelladir);
    strcat(dirstring,s);
#ifdef MDEBUG
    printf("\ndirstring:%s",dirstring);
#endif
    return dirstring;
}

char *
hdir(s)       /* returns pointer to filename in home dir, given base+ext */
    char *s;
{
    static char dirstring[72];

    strcpy(dirstring,homedir);
    strcat(dirstring,s);
#ifdef MDEBUG
    printf("\ndirstring:%s",dirstring);
#endif
    return dirstring;
}

/************************ EXTPTR, HASEXT() *****************************/
char *  
extptr(s)   /* rets pointer to extension if s contains a '.' followed 
                    by a char after directory separators if any; rets 0
                    if file has no extension */
char *s;
{
    char *p, *p1;
    int ret;
    
    if((p1 = strrchr(s,']')) != 0) s = p1;  /* vms path, pointer at dir sep.*/
    if((p1 = strrchr(s,'/')) != 0) s = p1;  /* unix path, pointer at 
                                                last dir sep.*/
    /* if a DOS filename has a '.', it MUST be the extension separator */
    ret = (((p = strrchr(s,'.')) != 0) && !isspace(*(p+1))) ;
    return (ret ? p+1 : 0 );
}

int hasext(s)
{
    return (extptr(s) ? 1 : 0 );
}

/********************* ERRET() *****************************************/

void erret(str)
char *str;
{
    if(str>(char *)1) printf("%s",str);
    exit(1);
}

/******************* FLUSHIT() ***************************************/

flushit() 
{ 
    fflush(stderr); 
    fflush(stdout); 
}

/******************* ANSI screen code ********************************/

int x_scur;
int y_scur;

static char getcstr[] =  { ESC, '[', '6', 'n', '\0'};
static char setcstr[] =  { ESC, '[', '%', 'd', ';', '%', 'd','f','\0'};
static char savcstr[] =  { ESC, '[', 's', '\0' };
static char rescstr[] =  { ESC, '[', 'u', '\0' };
static char erasestr[] = { ESC, '[', 'K', '\0' };
static char clearstr[] = { ESC, '[', '2', 'J', '\0' };
static char apkeystr[] = { ESC, '=', '\0' };
static char numkeystr[]= { ESC, '>', '\0' };  

void get_scur()    /* gets cursor position, puts in x_scur, y_scur */
{
    char *cp1,*cp2,*cp3;
    char buf[80];
    register int i;
    int c;
    int atoi();
    int err = 1;
    
    printf(getcstr);
    fflush(stdout);
    for(i=0;i<12;i++){
        c = get1char(0);
        buf[i] = c;
    }
    err &= ( (cp1 = strchr(buf,'[')) != 0 ? 1 : 0 ) ;
    err &= ( (cp2 = strchr(buf,';')) != 0 ? 1 : 0 ) ;
    err &= ( (cp3 = strchr(buf,'R')) != 0 ? 1 : 0 ) ;
    if(!err) erret(" get_scur: failed" ) ;
    *cp1 = *cp2 = *cp3 = '\0';
    y_scur = atoi(cp1+1);
    x_scur = atoi(cp2+1);
    return;
}

void put_scur()  /* puts cursor at x_scur, y_scur */
{
    printf(setcstr,y_scur,x_scur);  /* ANSI.SYS has order reversed */
    fflush(stdout);
}

void home_scur()
{
    x_scur = y_scur = 1;
    put_scur();
}

void sav_scur()  /* saves cursor position internally */
{
    printf(savcstr);
    fflush(stdout);
}

void res_scur() /* restores it */
{
    printf(rescstr);
    fflush(stdout);
}

void erase_line() /* erases from cursor to end of line */
{
    printf(erasestr);
    fflush(stdout);
}

void clear_scr() /* clears screen, cursor at top left */
{
    printf(clearstr);
    home_scur();
}

void num_keypd()  /* sets keypad to numeric mode */
{
    printf(numkeystr);
}

void app_keypd()  /* sets keypad to 'application' mode */
{
    printf(apkeystr);
}

/******************* GET1CHAR() *********************************/


#endif /* STANDALONE */


