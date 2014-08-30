#ifdef VMS
#include mirella
#include edit
#else
#include "mirella.h"
#include "edit.h"
#endif

extern normal x_scur;
extern normal y_scur;

static int
adel_char()
{
    char *p = m_term.del_char;
    if(*p == '\0') return(-1);
    printf(p);
    return 0;
}

static int
adel_line()
{
    char *p = m_term.del_line;
    if(*p == '\0') return(-1);
    printf(p);
    return 0;
}

static int
aforw_curs(n)
int n;
{
    char *p = m_term.forw_curs;
    if(*p == '\0') return(-1);
    printf(p,n);
    return 0;
}

static int
aback_curs(n)
int n;
{
    char *p = m_term.back_curs;
    if(*p == '\0') return(-1);
    printf(p,n);
    return 0;
}

static int
aput_curs(x,y)  /* puts cursor at x,y */
int x,y;
{
#if 1
    mv_cursor(x-1,y-1);
#else
    char *p = m_term.put_curs;
    
    if(*p == '\0') return(-1);
    printf(p,y,x);
    fflush(stdout);
#endif
    return(0);
}


static int
anum_kpd()
{
    char *p = m_term.num_kpd;
    if(*p == '\0') return(-1);
    printf(p);
    return 0;
}

static int
aapp_kpd()
{
    char *p = m_term.app_kpd;
    if(*p == '\0') return(-1);
    printf(p);
    return 0;
}

static int
aquery_curs()    /* gets cursor position, puts in x_scur, y_scur */
{
    char *cp1,*cp2,*cp3;
    char buf[80];
    register int i;
    int c;
    int atoi();
    int err = 1;
    char *p = m_term.query_curs;
    
    if(!(*p)){   /*nocando*/
        x_scur = 1;
        y_scur = 24;
        return -1;
    }
    printf(p);
    fflush(stdout);
    get1char(1);    /* need to set mode for unix; works ok for VMS anyway */
    for(i=0;i<12;i++){
        c = get1char(0);
        buf[i] = c;
#ifdef DOS32
        if(c == '\n' || c == 0 || c == '\r') break;
        /* &^%$#*&^ ANSI.SYS puts a newline on the string */
#else
        if(c == 'R') break; /* normal termination */
#endif        
    }
    err &= ((cp1 = strchr(buf,'[')) != NULL) ? 1 : 0;
    err &= ((cp2 = strchr(buf,';')) != NULL) ? 1 : 0;
    err &= ((cp3 = strchr(buf,'R')) != NULL) ? 1 : 0;
    if(!err) erret(" get_scur: failed" ) ;
    *cp1 = *cp2 = *cp3 = '\0';
    y_scur = atoi(cp1+1);
    x_scur = atoi(cp2+1);
    get1char(EOF);
    return 0;
}

static int
asav_curs()  /* saves cursor position internally */
{
    char *p = m_term.sav_curs;
    if(!(*p)) return -1;
    printf(p);
    fflush(stdout);
    return 0;
}

static int
ares_curs() /* restores it */
{
    char *p = m_term.res_curs;
    if(!(*p)) return -1;
    printf(p);
    fflush(stdout);
    return 0;
}

static int
aclr_scr() /* clears screen */
{
    char *p = m_term.clr_scr;
    if(!(*p)) return -1;
    printf(p);
    aput_curs(1,1);
    fflush(stdout);
    return 0;
}

extern int stdprt(), stdput(), stdflsh();
static void donot() {;}

FTERMINAL ansiterm = 
{
    "ANSITERMINAL",
    adel_char,
    adel_line,
    aforw_curs,
    aback_curs,
    aput_curs,
    anum_kpd,
    aapp_kpd,
    aquery_curs,
    asav_curs,
    ares_curs,
    aclr_scr,
    stdprt,
    stdput,
    stdflsh,
    80,
    24,
    1,
    1,
    -1,
    1,
    1,
    donot,
    donot
};      
    

/************** GLOBAL SCREEN FUNCTIONS *********************************/


int get_scur()    /* gets cursor position, puts in x_scur, y_scur */
{
    return (*(m_fterm.fquery_curs))();
}


int put_scur()  /* puts cursor at x_scur, y_scur */
{
    return (*(m_fterm.fput_curs))(x_scur,y_scur);
}

int putcur(x,y) /* puts cursor at x,y */
int x,y;
{
    return (*(m_fterm.fput_curs))(x,y);
}

int home_scur()
{
    return (*(m_fterm.fput_curs))(1,1);
}

int sav_scur()  /* saves cursor position internally */
{
    return (*(m_fterm.fsav_curs))();
}

int res_scur() /* restores it */
{
    return (*(m_fterm.fres_curs))();    
}

int erase_line() /* erases from cursor to end of line */
{
    return (*(m_fterm.fdel_line))();    
}

int forw_curs(n)
int n;
{
    return (*(m_fterm.fforw_curs))(n);
}    
    
int del_char()
{
    return (*(m_fterm.fdel_char))();    
}
  

int clear_phy_scr() /* clears physical screen only, cursor at top left */
{
    return (*(m_fterm.fclr_scr))();
}

int clear_scr() /* clears screen, cursor at top left; inits save buffer */
{
    return(clear_phy_scr());
}

int num_keypd()  /* sets keypad to numeric mode */
{
    return (*(m_fterm.fnum_kpd))();
}

int app_keypd()  /* sets keypad to 'application' mode */
{
    return (*(m_fterm.fapp_kpd))();
}
