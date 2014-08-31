/*
 * these routines do emacs-style editing of a line of text.
 */
/* try moving this to get rid of _POSIX_SOURCE trouble; it was before tty.h*/
#ifdef unix 
#include <sys/ioctl.h>
#endif

#include "tty.h"
#include "mirella.h"
#include "charset.h"
#include "edit.h"

/* try this 
#ifdef unix 
#include <sys/ioctl.h>
#endif
*/

#define ADD_CHAR(X)			/* add a character */		\
{									\
   if(overwrite) {							\
      *lptr++ = (*scrnput)((X) & '\177');   /* overwriting is simple */	\
   } else {								\
      if(insert_char(X,line,&lptr,HSIZE) == END) { /* insert character */\
         FPUTCHAR(X);							\
      } else {					/* middle of line */	\
	 (*scrnprt)(lptr - 1);			/* and rewrite it */	\
	 UPDATE_CURSOR;							\
      }									\
   }									\
}

#define DELETE_CHAR		/* delete a character under cursor */	\
del_character(lptr);							\
if(m_term.del_char[0] != '\0') { /* that's easy */			\
   PRINT(m_term.del_char);						\
} else {			/* Can't delete just one character */	\
   delete_to_eol(lptr);							\
   (*scrnprt)(lptr);		/* redraw end of line*/			\
   UPDATE_CURSOR;							\
}

#define DEL_CHAR(X)		/* delete character under cursor */	\
{				/* X == 1, -1 if CTRL_H, CTRL_D */	\
   undo_buff[uind%USIZE] = *lptr; /* save deleted character */		\
   if(X < 0) {			/* was CTRL_D */			\
      undo_buff[uind%USIZE] |= '\200';					\
   }									\
   uind++;								\
   DELETE_CHAR;								\
}

#define PRINT(S)		/* process padding and print string */	\
   {									\
     char *ptr = S;							\
     if(isdigit(*ptr)) {						\
        delay(atof(ptr));						\
        while(isdigit(*ptr)) ptr++;					\
        if(*ptr == '.') {						\
	   ptr++;							\
           while(isdigit(*ptr)) ptr++;					\
        }								\
        if(*ptr == '*') ptr++;						\
     }									\
     (*scrnprt)(ptr);							\
   }

#define UPDATE_CURSOR							\
   if(mv_cursor(plength + lptr - line, 0) < 0) {			\
      if(*lptr != '\0') {						\
         (*scrnput)('\r');						\
         (*scrnprt)(prompt_string);					\
         nprintf(line,lptr - line);					\
      }									\
   }

#define UPDATE_CURSOR_NP					        \
   if(mv_cursor(lptr - line, 0) < 0) {           			\
      if(*lptr != '\0') {						\
         (*scrnput)('\r');						\
         (*scrnprt)(prompt_string);					\
         nprintf(line,lptr - line);					\
      }									\
   }

#define YANK_CHAR		/* yank back a character */		\
{									\
   char yank_c;								\
									\
   if(uind <= 0) {							\
      (*scrnput)(CTRL_G);						\
      break;								\
   }									\
   yank_c = undo_buff[--uind%USIZE];					\
   ADD_CHAR(yank_c);							\
   if(yank_c & '\200') {	/* character came from under cursor */	\
      (*scrnput)('\b'); lptr--;						\
   }									\
}
#define END 0				/* pointer at end of line */
#define MIDDLE 1			/* pointer not at end */
#define NKILL 5				/* number of lines in ^K kill ring */
#define NOWT 0				/* just get character from get1char */
#define FPUTCHAR(C)			/* putchar + fflush */		\
  (*scrnput)((C) & '\177');						\
  (void)(*scrnflsh)(stdout);
#define UNBOUND (-1)			/* key is not bound */
#define USIZE 160			/* number of characters to save */

static char save_buffer[NKILL][HSIZE + 1], /* Buffer for ^K/^Y */
	    undo_buff[USIZE];		/* buffer for deleted characters */
FTERMINAL m_fterm;
char *pmfterm = (char *)(&m_fterm);     /* for Mirella. thanks, rhl */
normal sizfterm = sizeof(FTERMINAL);       /* "" */
FTERMINAL *mftarray[16];
int m_nfterm = 1;			/* number of fterminals;
					   only one (ansi) initially */
extern FTERMINAL ansiterm;
extern char pstring[];			/* the (forth) prompt string */
char pstring2[40] = "";			/* secondary string */
static char prompt_string[40];		/* the (C) prompt string */
int last_char;				/* last char that was typed */
extern int verbose;			/* should I be chatty? */
static int hflg = 0;                    /* is the current cmd from history? */
static int insert_char(),
	   kill_ind = NKILL,		/* index into save_buffer kill ring */
	   kill_ring_ctr = NKILL,	/* where is ESC-y in kill ring? */
	   list_edit_tree(),
	   refresh_editor = 0,		/* do I need to refresh the editor? */
	   overwrite = 0,
	   uind = 0;			/* index to undo_buff */
extern int interrupt;
int plength;				/* num. of printing chars in prompt */
extern TERMINAL m_term;			/* describe my terminal */
char *pmterm = (char *)(&m_term);       /* for Mirella */
normal sizterm  = sizeof(TERMINAL);	/*  "" */
normal termconssize = TERM_SIZE;	/* length of control strings, 12 */
static void delete_to_eol(),		/* delete to end of line */
  	    delay(),			/* delay for x msec */
	    del_character(),
	    nprintf();

/*****************************************************************/
/*
 * Set the terminal type, for the editor. This is nothing to do with graphics.
 */
#define NOMOVE 0			/* Can't get ch or cm to move cursor */
#define CH 1				/* got "ch" */
#define CM 2				/* got "cm" */

TERMINAL m_term;			/* defined in edit.h */
int curs_prop,				/* How to move the terminal */
    nlines;


/***************************************************************************/

void
init_ed()
{
   char *ptr;

   if((ptr = get_def_value("term")) == NULL || *ptr == '\0'){
      if((ptr = getenv("TERM")) == NULL || *ptr == '\0'){
	 ptr = "vt100";
	 (*scrnprt)(
		 "\nI cannot find [MIR]TERM in the environment. Setting to: ");
	 (*scrnprt)(ptr);
      }
   }
   mftarray[0] = &ansiterm;
   setterminal(ptr);
   read_hist();
   flushit();
}

/**********************************************************************/

int
newterminal(p)
FTERMINAL *p;
{
    int len;
    if(p == NULL || (len = strlen(p->fname)) > 40){
        printf("\nNEWTERMINAL:Illegal name");
        return(-1);
    }
    mftarray[m_nfterm++] = p;
    return(0);
}

/**********************************************************************/
/*
 * sets terminal to name==str if poss; if str==NULL, resets to last state
 */
void 
setterminal(str)
char *str;
{
   int i;
   static int firstime = 1;
   static char oldterm[40];
   static char term_type[40];
   char temp[100];
   
   if(str == NULL) {
      if(firstime) erret("\nSETTERMINAL: No old terminal");
      setterminal(oldterm);
      res_scr();
      return;
   }
   
   if(set_term_type(str,0) == 0) {	/* got it */
      if(!firstime){
	 if(m_fterm.t_close != NULL) {
	    (*m_fterm.t_close)();
	 }
      }
      m_fterm = ansiterm; 
      scrnprt = m_fterm.fscrnprt;
      scrnput = m_fterm.fscrnput;
      scrnflsh = m_fterm.fscrnflsh;
#ifdef DOS32
      dosansiflg = 1;
#else
      dosansiflg = 0;
#endif                                    
   } else {				/* look through other drivers */
      for(i = 1;i < m_nfterm;i++){
	 if(!strcmp(mftarray[i]->fname,str)){  /* got one */
	    if(m_fterm.t_close != NULL) {
	       (*m_fterm.t_close)();
	    }
	    m_fterm = *mftarray[i];
	    scrnprt = m_fterm.fscrnprt;
	    scrnput = m_fterm.fscrnput;
	    scrnflsh = m_fterm.fscrnflsh;
#ifdef DOS32
	    if(m_fterm.ansiflg) dosansiflg = 1;
	    else dosansiflg = 0;
#else
	    dosansiflg = 0;
#endif      
	    break;
	 }
      }
      if(i == m_nfterm){		/* not known to system */
	 sprintf(temp,"\nUnknown term type %s",str);
	 (*scrnprt)(temp);
	 erret((char *)NULL);
      }
   }
/*
 * if get here, success. clear screen and go on
 */
   strcpy(term_type,str);
   strcpy(oldterm,str);
   m_terminal = term_type;
   (*m_fterm.t_init)();			/*  init new terminal */
   clear_scr();
   flushit();
   firstime = 0;
}

int
set_term_type(term_type,nl)
char *term_type;
int nl;				/* number of lines */
{
   char *baud_rate,
   	buff[TERM_SIZE],
   	*cap_file;
   static char esc_chars[] = "\004bdfghuvy<>"; /* recognise ESC-char */
   static int first = 1;		/* Is this the first call? */
   static TTY *tty;
#if defined(TIOCGWINSZ) && !defined(_IBMR2)
   static int fildes = -1;
   struct winsize twinsiz;
#endif

/*
 * Map things like word-operators
 */
   if(first) {
      char str[4],
      	   *ec;				/* pointer to esc_chars */
      
      first = 0;
      for(ec = esc_chars;*ec != '\0';ec++) {
	 sprintf(str,"^[%c",*ec);
	 (void)define_key(str,*ec | '\200');
      }
      (void)define_key("^[q",CTRL_Q);
      (void)define_key("^[s",CTRL_S);
   }

   if(term_type == NULL) return(-1);

   if(!strcmp(term_type,"dumb")) {	/* worst case terminal */
      m_term.forw_curs[0] = '\0';
      curs_prop = NOMOVE;
      m_term.back_curs[0] = '\0';
      m_term.clr_scr[0] = '\0';
      m_term.del_char[0] = '\0';
      m_term.del_line[0] = '\0';
      m_term.put_curs[0] = '\0';
      m_term.res_curs[0] = '\0';
      m_term.sav_curs[0] = '\0';
      m_term.ncol = 80;
      m_term.nlin = 0;
      nlines = (nl != 0) ? abs(nl) : 20;
      return(0);
   }

   strcpy(buff,term_type);		/* else get_def_value will overwrite */
   term_type = buff;			/* the terminal type */
   if((cap_file = getenv("TERMCAP")) == NULL) {    /* try the environment */
      if((cap_file = get_def_value("termcap")) == NULL) { /* try .mirella */
         cap_file = "/etc/termcap";
      }
   }
   if((tty = ttyopen(cap_file,1,&term_type,(int (*)())NULL)) == NULL) {
      return(-1);
   }
   tty_index_caps(tty,tty->t_capcode, tty->t_capindex);
/*
 * Now get the capabilities that we need
 */
   (void)ttygets(tty,"ch",m_term.forw_curs,TERM_SIZE);	/* look for "ch" */
   if(!strcmp(m_term.forw_curs,"disabled")) {
      m_term.forw_curs[0] = '\0';
   } else if(m_term.forw_curs[0] == '\0') {		/* settle for "cm" */
      (void)ttygets(tty,"cm",m_term.forw_curs,TERM_SIZE);
      curs_prop = CM;
   } else {
      curs_prop = CH;
   }
   if(nl < 0 || m_term.forw_curs[0] == '\0') {
      curs_prop = NOMOVE;
   }

   (void)ttygets(tty,"dc",m_term.del_char,TERM_SIZE);
   (void)ttygets(tty,"ce",m_term.del_line,TERM_SIZE);
   m_term.ncol = ttygeti(tty,"co") ;
   m_term.nlin = ttygeti(tty,"li") ;
   m_term.pad = ttygeti(tty,"pc") ;
   if((baud_rate = get_def_value("ttybaud")) != NULL) {
      m_term.baud = atoi(baud_rate);
   } else {
      m_term.baud = DEF_BAUDRATE;
   }
#if defined(TIOCGWINSZ) && !defined(_IBMR2)
   twinsiz.ws_col = 0;
   twinsiz.ws_row = 0;
   if((fildes = open("/dev/tty",2)) >= 0) {
      (void)ioctl(fildes,TIOCGWINSZ,(char *)&twinsiz);
   }
   m_term.ncol = (twinsiz.ws_col == 0) ? m_term.ncol : twinsiz.ws_col;
   m_term.nlin = (twinsiz.ws_row == 0) ? m_term.nlin : twinsiz.ws_row;
#endif
   nlines = (nl != 0) ? abs(nl) : (m_term.nlin > 0) ? m_term.nlin - 1 : 20;
   if(curs_prop == CM) {
      m_term.dlin = nlines;
   }
/*
 * Initialise the terminal
 */
   (void)ttygets(tty,"is",buff,TERM_SIZE);
   PRINT(buff);
   (void)mv_cursor(plength + 1,m_term.dlin);
/*
 * Get the key init sequences, bind the arrow keys
 */
   (void)ttygets(tty,"le",m_term.back_curs,TERM_SIZE);
   (void)ttygets(tty,"cl",m_term.clr_scr,TERM_SIZE);
   (void)ttygets(tty,"cm",m_term.put_curs,TERM_SIZE);
   (void)ttygets(tty,"ke",m_term.unset_keys,TERM_SIZE);
   (void)ttygets(tty,"ks",m_term.set_keys,TERM_SIZE);
   (void)ttygets(tty,"rc",m_term.res_curs,TERM_SIZE);
   (void)ttygets(tty,"sc",m_term.sav_curs,TERM_SIZE);
   PRINT(m_term.set_keys);
   (void)ttygets(tty,"kl",buff,TERM_SIZE); (void)define_key(buff,CTRL_B);
   (void)ttygets(tty,"kr",buff,TERM_SIZE); (void)define_key(buff,CTRL_F);
   (void)ttygets(tty,"kd",buff,TERM_SIZE); (void)define_key(buff,CTRL_N);
   (void)ttygets(tty,"ku",buff,TERM_SIZE); (void)define_key(buff,CTRL_P);
#if 0
   (void)ttygets(tty,"k1",buff,TERM_SIZE); (void)set_pf_key(buff,1);
   (void)ttygets(tty,"k2",buff,TERM_SIZE); (void)set_pf_key(buff,2);
   (void)ttygets(tty,"k3",buff,TERM_SIZE); (void)set_pf_key(buff,3);
   (void)ttygets(tty,"k4",buff,TERM_SIZE); (void)set_pf_key(buff,4);
#endif

   return(0);
}

/* resets the screen size when there is a SIGWINCH; returns a string
 * of the form *(cols,lines) 
 */
/* NB!!!! this F...s completely the saved screen buffer, not surprisingly.
 * Write a little code to fix it??? It also seems simply not to work. Some
 * work would be useful, but it reaches lots of places low in Mirella. It
 * would be really keen if this worked properly. Nasty stuff to do if you are
 * not on the last line, etc.....
 */ 
char *
screensize()
{
    static char sstring[32];
#if defined(TIOCGWINSZ) && !defined(_IBMR2)
    char lines[32];
    char columns[32];
    static int fildes = -1;
    struct winsize twinsiz;

    twinsiz.ws_col = 0;
    twinsiz.ws_row = 0;
    if((fildes = open("/dev/tty",2)) >= 0) {
       (void)ioctl(fildes,TIOCGWINSZ,(char *)&twinsiz);
    }
    m_term.ncol = (twinsiz.ws_col == 0) ? m_term.ncol : twinsiz.ws_col;
    m_term.nlin = (twinsiz.ws_row == 0) ? m_term.nlin : twinsiz.ws_row;
    m_fterm.t_ncols = m_term.ncol;
    m_fterm.t_nrows  = m_term.nlin;
    /* why do we have these two terminal descriptors?? Rationalize?? */
    sprintf(sstring,"*(%d,%d)",m_term.ncol,m_term.nlin);
    sprintf(columns,"%d",m_term.ncol);
    sprintf(lines,"%d",m_term.nlin);
    setenv("COLUMNS",columns,1);
    setenv("LINES",lines,1);
#endif
    return sstring;
}


/*
 * Move a cursor to a particular place on the screen. Ideally, the capability
 * "ch" (move within line) is used. Failing that, we use "cm", and go to the
 * `default line', m_term.dlin. Note that, by definition, "ch" strings won't
 * access this line information, so this routine needn't know if we use it.
 *
 * See Unix manual(5) for discussion of termcap.
 * (this routine fakes their definition of the "ch"/"cm" capability)
 */
int
mv_cursor(i,j)
int i,j;				/* desired column and row */
{
   char *cptr;				/* pointer to forw_curs string */

   if(curs_prop == CM) {
      if(j == 0) j = m_term.dlin;
   } else if(curs_prop == CH) {
      ;
   } else {
      return(-1);
   }
   cptr = m_term.forw_curs;

   if(isdigit(*cptr)) {
      delay(atof(cptr));
      while(isdigit(*cptr)) cptr++;	/* skip delay */
      if(*cptr == '.') {
	 cptr++;
	 while(isdigit(*cptr)) cptr++;
      }
      if(*cptr == '*') cptr++;
   }

   while(*cptr != '\0') {
      if(*cptr == '%') {
	 switch (*++cptr) {
	  case '%':
	    putchar('%');
	    break;
	  case '.':
	    printf("%c",j);
	    j = i;		/* use i next */
	    break;
	  case '+':
	    j += *++cptr;
	    printf("%c",j);
	    j = i;		/* use i next */
	    break;
	  case '>':
	    if(j > *++cptr) {
	       j += *++cptr;
	    } else {
	       cptr++;
	    }
	    break;
	  case '2':
	    printf("%2d",j);
	    j = i;		/* use i next */
	    break;
	  case '3':
	    printf("%3d",j);
	    j = i;		/* use i next */
	    break;
	  case 'd':
	    printf("%d",j);
	    j = i;		/* use i next */
	    break;
	  case 'i':
	    j++; i++;
	    break;
	  case 'r':
	    { int t; t = j; j = i; i = t;}
	    break;
	  default:
	    fprintf(stderr,"Unknown code in mv_cursor %c\n",*cptr);
	    break;
	 }
      } else {
	 putchar(*cptr);
      }
      cptr++;
   }
   return(0);
}

/*******************************************************************/
/*
 * Edit a line of text, either history or part of a macro
 */
int
edit_line(line,pflg)
char *line;				/* line to be edited */
int pflg;                               /* do you want a prompt?; in use,
                                            this is 0 for noncommand use,
                                            1 for commands; history is 
                                            suppressed if it is 0  */
{
   char c,
   	*lptr;				/* pointer to line line */
   int i;
   int xold = x_scur;
   int yold = y_scur;
   static int cur_position = HSIZE;	/* previous position of cursor */

   if(!pflg) hflg = 0;
   
   if(!hflg) {				/* not from history list*/
        if(!pflg && get_scur() >= 0){   /* no prompt and there is a 
                                            get-cur-pos fn */
            plength = x_scur-1;  
            x_scur = xold;
            y_scur = yold;		/* reset cursor coordinates */
        } else {
	   (*scrnput)('\n');		/* don't erase Mirella output */
	}
   }

   {
      char *ps;				/* pointer to prompt string */
      
      if(pflg) {
	 ps = pstring;			/* use primary prompt */
      } else {
	 if(*pstring2 == '\0') {
	    ps = NULL;
	 } else {
	    ps = pstring2;		/* use secondary prompt */
	 }
      }
      if(ps == NULL) {
	 plength = 0;
	 prompt_string[0] = '\0';
      } else {
	 for(i = 1,plength = 0;ps[i] != '\0';i++) {
	    if(ps[i] != '\n') {
	       prompt_string[plength++] = ps[i];
	    }
	 }
	 prompt_string[plength++] = ' ';
	 prompt_string[plength] = '\0';

	 (*scrnput)('\r');
	 delete_to_eol((char *)NULL);	/* don't know line */
	 if(hflg || ps == pstring2) {
	    (*scrnprt)(prompt_string);
	 } else {
	    (void)eprompt();
	 }
      }
   }
   hflg = 0;
   
   (*scrnprt)(line);

   if(pflg) {				/* try to keep cursor in same column */
      i = strlen(line);
      if(i > 0) {
	 if(cur_position > i) {		/* don't go beyond end of line */
	    cur_position = i;
	 }
	 lptr = line + cur_position;	/* put cursor in old column */
      } else {
	 lptr = line;
      }
      UPDATE_CURSOR;
   } else {
      lptr = line;

/* debug?? This is just WRONG. Why does it work???? */
      i = strlen(line);
      cur_position = i;
      UPDATE_CURSOR_NP;

/* ??? */

   }
   flushit();

   (void)get1char(CTRL_A);		/* initialise rawmode */
   app_keypd();

   for(;;) {
      if(interrupt) {
	 (void)get1char(EOF);
	 return(CTRL_C);
      }
      if(refresh_editor) {
	 refresh_editor = 0;
	 reset_tib();			/* Do we need this? RHL */
	 unreadc(CTRL_L);
	 continue;
      }
      switch(i = readc()) {
       case UNBOUND :
	 (*scrnput)(CTRL_G);
	 break;
       case CTRL_A :			/* beginning of line */
	 (*scrnput)('\r');
	 (*scrnprt)(prompt_string);
	 lptr = line;
	 break;
       case CTRL_B :			/* back a character */
	 if(lptr > line) {
	    (*scrnput)('\b');
	    lptr--;
	 }
	 break;
       case CTRL_D :			/* delete character under cursor */
	 DELETE_CHAR;
	 break;
       case CTRL_E :			/* go to end of line */
	 (*scrnprt)(lptr);
	 lptr += strlen(lptr);
	 break;
       case CTRL_F :			/* forward a character */
	 if(lptr - line < HSIZE - 1) {
	    if(*lptr == '\0') *lptr = ' ';
	    (*scrnput)(*lptr++);
	 }
	 break;
       case CTRL_H :			/* delete character before cursor */
	 if(lptr > line) {
	    lptr--;			/* back one character */
	    (*scrnput)('\b');
	    DELETE_CHAR;
	 }
	 break;
       case CTRL_I :			/* tab */
         ADD_CHAR(' ');
         while((lptr - line)%8 != 0) ADD_CHAR(' ');
         break;
       case CTRL_K :			/* erase to end of line */
	 (void)strcpy(save_buffer[(++kill_ind)%NKILL],lptr);
	 delete_to_eol(lptr);
	 erase_str(lptr,HSIZE - (lptr - line));
	 kill_ring_ctr = kill_ind;
	 break;
       case CTRL_L :			/* re-write line */
	 (*scrnput)('\r');
	 (*scrnprt)(prompt_string);
	 (*scrnprt)(line);
	 delete_to_eol((char *)NULL);	/* don't know how much to delete */
	 UPDATE_CURSOR;
	 break;
       case CTRL_M :			/* accept line command */
       case CTRL_N :			/* next command */
       case CTRL_P :			/* previous command */
       case CTRL_R :			/* search reverse */
       case CTRL_S :			/* search forward */
       case CTRL_V :			/* forward many lines */
       case ('<' | '\200') :		/* first line */
       case ('>' | '\200') :		/* last line */
       case (CTRL_D | '\200') :		/* forget this line */
       case ('g' | '\200') :		/* go to line */
	 if(uind > 0 && !isspace((undo_buff[(uind - 1)%USIZE]) & '\177')) {
	    if(undo_buff[(uind - 1)%USIZE] & '\200') {
	      undo_buff[(uind++)%USIZE] = (' ' | '\200');
	   } else {
	      undo_buff[(uind++)%USIZE] = ' ';
	   }
	 }
	 
	 if(lptr > line) {		/* only set if line non-null */
	    cur_position = (*lptr == '\0') ? HSIZE : lptr - line;
	 }
         hflg = (i != CTRL_M);		/* flag to suppress newline in prompt*/

	 (*scrnprt)(lptr);		/* go to end of line */
 	 (void)get1char(EOF);
         num_keypd();
	 return(i);
       case CTRL_O :			/* insert line above */
	 (void)unreadc(CTRL_M);
	 (void)unreadc(CTRL_E);
	 (void)unreadc(CTRL_P);
	 break;
       case CTRL_Q :			/* Literal next */
	 i = get1char(0) & '\177';
	 ADD_CHAR(i);
	 break;
       case CTRL_T :			/* toggle overwrite mode */
	 overwrite = (overwrite == 1) ? 0 : 1;
	 break;
       case CTRL_U :				/* erase to start of line */
	 while(lptr-- > line) {
	    del_character(lptr);		/* back one character */
	 }
	 lptr++;
	 unreadc(CTRL_L);
	 break;
       case CTRL_X :			/* finish editing */
         (void)get1char(EOF);		/* turn off raw mode */
         cur_position = HSIZE;
	 return(i);
       case CTRL_Y :			/* Restore save buffer */
	 for(i = 0;(c = save_buffer[kill_ind%NKILL][i]) != '\0';i++) {
	    ADD_CHAR(c);
	 }
         break;
       case CTRL_Z :			/* Attach to parent process */
         (void)get1char(EOF);		/* turn off raw mode */
	 reset_kbd();
	 (void)fflush(stdout);
	 if(suspend() != 0) {		/* try to attach to shell */
	    (*scrnput)('\r');
	    (*(m_fterm.fdel_line))();
	    (*scrnprt)("Out of raw mode for a moment...\r");
	    for(i = 0;i < 400000;i++) ;
	 }
	 (void)get1char(CTRL_A);	/* back into raw mode */
	 set_kbd();
	 (void)unreadc(CTRL_L);		/* redraw line */
         break;
       case GS:				/* ^] for now */
         (*scrnput)('\n');
	 hdl_quit(0);
         break;
       case ('b' | '\200'):
	 if(lptr > line) {
	    (*scrnput)('\b'); lptr--;
	 }
	 while(lptr > line && isspace(*lptr)) {
	    (*scrnput)('\b'); lptr--;
	 }
	 while(lptr > line && !isspace(*lptr)) {
	    (*scrnput)('\b'); lptr--;
	 }
	 if(lptr > line) (*scrnput)(*lptr++);	/* went too far */
	 break;
       case ('d' | '\200'):		/* delete a word forwards */
	 while(*lptr != '\0' && *lptr != '\0' && !isspace(*lptr)) {
	    DEL_CHAR(-1);
	 }
	 while(*lptr != '\0' && isspace(*lptr)) {
	    DEL_CHAR(-1);
	 }
	 break;
       case ('f' | '\200'):		/* forward a word */
	 while(*lptr != '\0' && !isspace(*lptr)) {
	    (*scrnput)(*lptr++);
	 }
	 while(*lptr != '\0' && isspace(*lptr)) {
	    (*scrnput)(*lptr++);
	 }
	 break;
       case ('h' | '\200'):		/* delete a word backwards */
	 if(lptr > line) {
	    (*scrnput)('\b'); lptr--;
	    DEL_CHAR(1);
	 }
	 while(lptr > line && isspace(*(lptr - 1))) {
	    (*scrnput)('\b'); lptr--;
	    DEL_CHAR(1);
	 }
	 while(lptr > line && *(lptr - 1) != '\0' && !isspace(*(lptr - 1))) {
	    (*scrnput)('\b'); lptr--;
	    DEL_CHAR(1);
	 }
	 break;
       case ('u' | '\200'):		/* yank back a word */
	 if(uind <= 0) (*scrnput)(CTRL_G);
	 while(uind > 0 && isspace((undo_buff[(uind - 1)%USIZE]) & '\177')){
	    YANK_CHAR;
	 }
	 while(uind > 0 &&
	       ((c = (undo_buff[(uind - 1)%USIZE] & '\177')),c != '\0') &&
							       !isspace(c)) {
	    YANK_CHAR;
	 }
	 break;
       case ('v' | '\200'):		/* back a number of lines */
	 (void)get1char(EOF);
	 cur_position = (*lptr == '\0') ? HSIZE : lptr - line;
	 return(i);
       case ('y' | '\200'):		/* step back through kill ring */
	 kill_ring_ctr += (NKILL - 1);	/* go back one */
	 for(i = 0;(c = save_buffer[kill_ring_ctr%NKILL][i]) != '\0' &&
	     lptr - line < HSIZE - 1;i++) {
	    ADD_CHAR(c);
	 }
         break;
       case ('0' | '\200'):			/* may be keyboard macro */
       case ('1' | '\200'): case ('2' | '\200'): case ('3' | '\200'):
       case ('4' | '\200'): case ('5' | '\200'): case ('6' | '\200'):
       case ('7' | '\200'): case ('8' | '\200'): case ('9' | '\200'):
	 {
	    char buff[15];
	    sprintf(buff,"emacro%c",(i & '\177'));
	    (void)cinterp(buff);
            if(pflg) reset_tib();	/* do we need this? RHL */
            unreadc(CTRL_L);
	 }
	 break;
       default :
	 if(iscntrl(i)) {			/* invalid key */
	    (*scrnput)(CTRL_G);
	    break;
	 }

	 ADD_CHAR(i);
	 break;
      }
      (void)fflush(stdout);
   }
}

/*****************************************************************************/

void
need_refresh()
{
   refresh_editor = 1;		/* We need to refresh the editor */
}

/*****************************************************************************/
/*
 * Set or unset the application keyboard
 */
void
reset_kbd()
{
   PRINT(m_term.unset_keys);
}

void
set_kbd()
{
   PRINT(m_term.set_keys);
}

/******************************************************/
/*
 * delete a character from the position ptr in string
 */
static void
del_character(ptr)
char *ptr;
{
   while(*ptr != '\0') {
      *ptr = *(ptr + 1);
      ptr++;
   }
}

/******************************************************/
/*
 * prompt for and return a string
 */
char *
get_edit_str(prompt)
char *prompt;
{
   static char str[60];
   int c,i;

   (*scrnput)('\r');
   (*scrnprt)(prompt);
   delete_to_eol((char *)NULL);
   (void)fflush(stdout);

   (void)get1char(CTRL_A);
   for(i = 0;i < 60;i++) {
      if((c = get1char(0)) == EOF || (str[i] = c) == '\n' || c == '\r') {
	 break;
      } else if(str[i] == DEL || str[i] == CTRL_H) {
	 if(i == 0) {
	    break;			/* stop trying to get string */
	 } else {
	    i -= 2;
	    (void)(*scrnprt)("\b \b");
	    (void)fflush(stdout);
	 }
      } else {
	 (void)FPUTCHAR(str[i]);
      }
   }
   (void)get1char(EOF);
   str[i] = '\0';
   return(str);
}

/*******************************************************/
/*
 * Erase to the end of line. If we can't do this properly, then overwrite
 * and backspace. If we don't know how many to overwrite, guess.
 */
static void
delete_to_eol(s)
char *s;				/* rest of line */
{
   int i,
       nchar;				/* number of chars to overwrite */
   
   if(m_term.del_line[0] != '\0') {
      PRINT(m_term.del_line);
   } else {
      nchar = (s == NULL) ? 10 : strlen(s);
      for(i = 0;i < nchar;i++) (*scrnput)(' ');
      for(i = 0;i < nchar;i++) (*scrnput)('\b');
   }
}

/******************************************************/
/*
 * delay for x msec
 */
static void
delay(x)
float x;
{
   char padchar;
   float msec_per_char;
   int npadchars;
   
   if(x > 0 && m_term.baud > 0) {
      padchar = m_term.pad & '\177';
      msec_per_char = (8*1000.)/m_term.baud;
      npadchars = x/msec_per_char + 0.5;
      
      for(;npadchars > 0;npadchars--)
	write(1,&padchar,1);
   }
}

/*******************************************************/
/*
 * fill a string with nulls
 */
void
erase_str(string,size)
char *string;
int size;
{
   register char *send = string + size;

   while(string < send) *string++ = '\0';
}

/**********************************************************/
/*
 * insert a character into a string
 */
static int
insert_char(c,string,pptr,size)
int c;					/* character to be inserted */
char *string,				/* string to be inserted into */
     **pptr;				/* position before which to insert */
int size;				/* size of string */
{
   char cc,save_c;
   char *line = string,			/* UPDATE_CURSOR needs these names */
	*lptr = *pptr;

   if(strlen(string) >= size) {
      fprintf(stderr,"\rSorry, there is no room for that character%c",CTRL_G);
      (void)fflush(stderr);
      (*scrnput)('\r');
      (*scrnprt)(prompt_string);
      (*scrnput)(' ');
      (*scrnprt)(line);
      delete_to_eol((char *)NULL);	/* don't know how much to delete */
      UPDATE_CURSOR;
      return(-1);
   }
   (*pptr)++;

   cc = c & '\177';

   if(*lptr == '\0') {
      *lptr++ = cc;
      *lptr = '\0';
      return(END);
   }

   do {
      save_c = *lptr;
      *lptr++ = cc;
      cc = save_c;
   } while(cc != '\0');
   *lptr = '\0';
   
   return(MIDDLE);
}

/**********************************************/
/*
 * Print first n characters of a string
 */
static void
nprintf(str,n)
char str[];
int n;
{
   int i;
   
   for(i = 0;i < n && str[i] != '\0';i++) {
      (*scrnput)(str[i]);
   }
}

/***********************************************************/
/*
 * find_str returns a history node, given a string
 */
EDIT *
find_str(str,dir,first,last)
char *str;				/* string to find */
int dir;				/* -1: down; 1: up */
EDIT *first,*last;			/* range of nodes to search */
{
   EDIT *command;

   if(*str != CTRL_S) {			/* a real search */
      (void)match_pattern("",str,(char **)NULL); /* compile pattern */
   }

   command = (dir == 1) ? last : first;
   while(command != NULL) {
      if(command->mark & SEARCH_MARK) {
	 command->mark &= ~SEARCH_MARK;
	 if(*str == CTRL_S) {		/* repeat last search; start here */
	    command = (dir == 1) ? command->prev : command->next;
	    if(command == NULL) return(NULL);
	    break;
	 }
      }
      command = (dir == 1) ? command->prev : command->next;
   }
   if(command == NULL) command = (dir == 1) ? last : first;
      
   while(command != NULL) {
      if(match_pattern(command->line,(char *)NULL,(char **)NULL) != NULL) {
	 if(*command->line == '\0') {	/* not a real match */
	    ;
	 } else {
	    command->mark |= SEARCH_MARK;
	    break;
	 }
      }
      command = (dir == 1) ? command->prev : command->next;
   }

   return(command);
}

/************************************************/
/*
 * Now the code to handle key-bindings.
 *
 * Every ascii character has an entry in array map[] which governs the mapping
 * of key-sequences starting with that letter. This array consists of a struct
 * containing a pointer to a chain of nodes decribing multi-character mapping
 * sequences; in the event that this is NULL the other element of the array
 * gives the action to be associated with the character typed. For example, the
 * letter `a' has a struct { 'a', NULL } meaning that no multi-character
 * sequences start with an a, and that an `a' is to be interpreted as a simple
 * `a'. To map lower to upper case, use { 'a', 'A' }. The action char can be a
 * single char (\0 -- \127) or a char with the \200 bit set. These actions are
 * understood by the main edit loop, and are the final output of the
 * key-mapping process. A structure was used instead of a union as a struct
 * is no larger, when allowance is made for keeping tabs on what is stored.
 *
 * To return to multi-character strings. In this case, the pointer is not NULL,
 * and the action is ignored. The pointer points to a branched chain of nodes,
 * each of which contains four items: the action and pointer as before, and
 * also a pointer to a side-tree and a char. A typed key-sequence is compared
 * char by char as the code
 * traverses the chain, if the char in a node doesn't match the current char
 * in the input key-sequence the next node is inspected; if it does and the
 * action isn't UNBOUND, the action is returned, otherwise another character
 * is obtained and the search continues down the sub-tree.
 *
 * lint is confused by MAKE_NODE, as R->T is declared as (struct map *)
 * not (MAP *)
 */
#define MAKE_NODE(R,T,C)	/* make and initialise a node of type T */    \
   if((R->T = (MAP *)malloc(sizeof(MAP))) == NULL) {		      	      \
      fprintf(stderr,"Can't allocate a node in define_key()\n");	      \
      return(-1);							      \
   }									      \
   R->T->next = R->T->subtree = NULL;					      \
   R->T->action = UNBOUND;						      \
   R->T->c = (C);

typedef struct node {
   short action;		/* action to take */
   struct map *subtree;		/* tree of more complex mappings */
} NODE;

typedef struct map {
   short action;		/* action to take */
   struct map *next,		/* next node in search for match */
   	      *subtree;		/* tree of more complex mappings */
   char c;			/* character to look for at this node */
} MAP;
/*
 * Note that ^@, ^J --> ^M, ^[ --> UNBOUND, ^W --> (\200 | h), DEL --> ^H
 * ^ --> (\200 | ^)
 */
NODE map[128] = {
		  {'\015',NULL},{'\001',NULL},{'\002',NULL},{'\003',NULL},
		  {'\004',NULL},{'\005',NULL},{'\006',NULL},{'\007',NULL},
		  {'\010',NULL},{'\011',NULL},{'\015',NULL},{'\013',NULL},
		  {'\014',NULL},{'\015',NULL},{'\016',NULL},{'\017',NULL},
		  {'\020',NULL},{'\021',NULL},{'\022',NULL},{'\023',NULL},
		  {'\024',NULL},{'\025',NULL},{'\026',NULL},{'h'|'\200',NULL},
		  {'\030',NULL},{'\031',NULL},{'\032',NULL},{UNBOUND,NULL},
		  {'\034',NULL},{'\035',NULL},{'\036',NULL},{'\037',NULL},
		  {'\040',NULL},{'\041',NULL},{'\042',NULL},{'\043',NULL},
		  {'\044',NULL},{'\045',NULL},{'\046',NULL},{'\047',NULL},
		  {'\050',NULL},{'\051',NULL},{'\052',NULL},{'\053',NULL},
		  {'\054',NULL},{'\055',NULL},{'\056',NULL},{'\057',NULL},
		  {'\060',NULL},{'\061',NULL},{'\062',NULL},{'\063',NULL},
		  {'\064',NULL},{'\065',NULL},{'\066',NULL},{'\067',NULL},
		  {'\070',NULL},{'\071',NULL},{'\072',NULL},{'\073',NULL},
		  {'\074',NULL},{'\075',NULL},{'\076',NULL},{'\077',NULL},
		  {'\100',NULL},{'\101',NULL},{'\102',NULL},{'\103',NULL},
		  {'\104',NULL},{'\105',NULL},{'\106',NULL},{'\107',NULL},
		  {'\110',NULL},{'\111',NULL},{'\112',NULL},{'\113',NULL},
		  {'\114',NULL},{'\115',NULL},{'\116',NULL},{'\117',NULL},
		  {'\120',NULL},{'\121',NULL},{'\122',NULL},{'\123',NULL},
		  {'\124',NULL},{'\125',NULL},{'\126',NULL},{'\127',NULL},
		  {'\130',NULL},{'\131',NULL},{'\132',NULL},{'\133',NULL},
		  {'\134',NULL},{'\135',NULL},{'\336',NULL},{'\137',NULL},
		  {'\140',NULL},{'\141',NULL},{'\142',NULL},{'\143',NULL},
		  {'\144',NULL},{'\145',NULL},{'\146',NULL},{'\147',NULL},
		  {'\150',NULL},{'\151',NULL},{'\152',NULL},{'\153',NULL},
		  {'\154',NULL},{'\155',NULL},{'\156',NULL},{'\157',NULL},
		  {'\160',NULL},{'\161',NULL},{'\162',NULL},{'\163',NULL},
		  {'\164',NULL},{'\165',NULL},{'\166',NULL},{'\167',NULL},
		  {'\170',NULL},{'\171',NULL},{'\172',NULL},{'\173',NULL},
		  {'\174',NULL},{'\175',NULL},{'\176',NULL},{'\010',NULL}
	    };
/*
 * Map a key sequence to a code, keep the last key typed in
 * extern int variable last_char
 */
int
map_key()
{
   MAP *n;

   if((last_char = get1char(0)) == EOF) return(EOF);
   last_char &= '\177';

   if(map[last_char].action != UNBOUND) {
      return(map[last_char].action);
   } else {
      if((n = map[last_char].subtree) == NULL) {
	 return(UNBOUND);
      }
      last_char = get1char(0);
      for(;;) {
	 if(last_char == n->c) {
	    if(n->action != UNBOUND) {
	       return(n->action);
	    } else if((n = n->subtree) == NULL) {
	       return(UNBOUND);
	    } else {
	       last_char = get1char(0);
	    }
	 } else if((n = n->next) == NULL) {
	    return(UNBOUND);
	 }
      }
   }
}

/*
 * Define a string to map to an action. The string is processed for
 * escapes, namely: ^n --> control-n (n alpha only)
 *		    \e,\E,^[ --> ESC
 *		    \nnn --> octal nnn
 */
int
define_key(string,action)
char *string;			/* string to be mapped */
int action;			/* to this action */
{
   char *key,			/* ascii value of key */
        kkey[10],		/* storage for key */
        *sptr;			/* pointer to input string */
   int nkey,			/* number of chars in key sequence */
       okey;			/* octal value of *key */
   MAP *n;
   NODE *root;

   if(*string == '\0') return(0);
/*
 * Expand escape sequences
 */
   for(sptr = string, key = kkey;
       (*sptr != '\0') && (key < kkey + sizeof(kkey) - 1); ) {
      if(*sptr == '^' && (*key = *(sptr + 1)) != '\0') { /* ^c */
	 sptr += 2;				/* skip ^n */
	 if(islower(*key)) *key = toupper(*key);
	 if(*key >= '@') {
	    *key = *key - '@';
	    key++;
	 } else {
	    *(sptr + 2) = '\0';		/* isolate offender */
	    fprintf(stderr,"%s is illegal\n",sptr);
	    break;
	 }
      } else if(!strncmp(sptr,"\\E",2) || !strncmp(sptr,"\\e",2)) {/* escape */
	 sptr += 2;				/* skip \E */
	 *key++ = '\033';
      } else if(sscanf(sptr,"\\%o",&okey) == 1) {	/* \nnn */
	 sptr += 4;				/* skip \nnn */
	 *key++ = okey & '\177';
      } else {
	 *key++ = *sptr++;
      }
   }
   nkey = key - kkey;
   *key = '\0';				/* terminate */
   key = kkey;				/* and rewind */
/*
 * Now go ahead and do the mapping
 */
   if(*key == '\0' && nkey == 1) {	/* special case */
      map[0].action = action;
      return(0);
   }
   root = &map[(int)(*key++)]; nkey--;
   if(*key == '\0') {
      root->action = action;
      return(0);
   }
   if(root->subtree == NULL) {
      MAKE_NODE(root,subtree,*key);
   }
   n = root->subtree;

   for(;;) {
      if(n->c == *key) {
	 key++; nkey--;
	 if(nkey == 0) {
	    n->action = action;
	    return(0);
	 } else {
	    if(n->subtree == NULL) {
	       MAKE_NODE(n,subtree,*key);
	    }
	    n = n->subtree;
	 }
      } else {
	 if(n->next == NULL) {
	    MAKE_NODE(n,next,*key);
	 }
	 n = n->next;
      }
   }
}

/***************************************************************/
/*
 * Define a mapping from the characters typed by the user to those
 * expected by the history/macro editors.
 * The names of the possible definitions are given in the struct mappings[],
 * and in addition a single character represents itself.
 * A default mapping is defined to do nothing except map ^J to ^M and DEL to ^H
 */
static struct {
   char operation[30],		/* name to use */
	def_key;		/* the default value of that operation */
} mappings[] = {
   {"start_of_line",CTRL_A},
   {"previous_char",CTRL_B},
   {"delete_char",CTRL_D},
   {"end_of_line",CTRL_E},
   {"next_char",CTRL_F},
   {"illegal",UNBOUND},
   {"delete_previous_char",CTRL_H},
   {"tab",CTRL_I},
   {"kill_to_end",CTRL_K},
   {"refresh",CTRL_L},
   {"carriage_return",CTRL_M},
   {"next_line",CTRL_N},
   {"insert_line_above",CTRL_O},
   {"previous_line",CTRL_P},
   {"quote_next",CTRL_Q},
   {"search_reverse",CTRL_R},
   {"search_forward",CTRL_S},
   {"toggle_overwrite",CTRL_T},
   {"delete_to_start",CTRL_U},
   {"scroll_forward",CTRL_V},
   {"exit_editor",CTRL_X},
   {"yank_buffer",CTRL_Y},
   {"attach_to_shell",CTRL_Z},
   {"escape",ESC},
   {"abort",GS},
   {"history_char",'^' | '\200'},
   {"previous_word",'b' | '\200'},
   {"delete_next_word",'d' | '\200'},
   {"delete_from_history",CTRL_D | '\200'},
   {"next_word",'f' | '\200'},
   {"goto_line",'g' | '\200'},
   {"delete_previous_word",'h' | '\200'},
   {"undelete_word",'u' | '\200'},
   {"yank_previous_buffer",'y' | '\200'},
   {"scroll_back",'v' | '\200'},
   {"first_line",'<' | '\200'},
   {"last_line",'>' | '\200'},
   {"emacro0",'0' | '\200'},
   {"emacro1",'1' | '\200'},
   {"emacro2",'2' | '\200'},
   {"emacro3",'3' | '\200'},
   {"emacro4",'4' | '\200'},
   {"emacro5",'5' | '\200'},
   {"emacro6",'6' | '\200'},
   {"emacro7",'7' | '\200'},
   {"emacro8",'8' | '\200'},
   {"emacro9",'9' | '\200'},
};
#define NMAP (sizeof(mappings)/sizeof(mappings[0]))
/*
 * Set up the map[] array from a file.
 * The format is: two lines of header, followed by pairs of op_name key_name,
 * where op_name must be defined in mappings[], and key_name is the alias
 * that the user wishes to use. Control characters (^A to ^Z) may be entered as
 * ^n, octal sequences (\nnn) are also allowed.
 */
int
read_map(file)
char file[];			/* name of file */
{
   char cchar = '#';		/* comment character */
   char key_name[10],		/* name of user-defined key */
   	line[81],		/* read a line */
	op_name[30];		/* for this operation */
   FILE *fil;			/* input file */
   int i;
   
   if((fil = fopen(file,"r")) == NULL) {
      fprintf(stderr,"Can't open %s\n",file);
      return(-1);
   }
   (void)fgets(line,81,fil);		/* skip header */
   (void)fgets(line,81,fil);

   while(fgets(line,81,fil) != NULL) {
      if(line[0] == cchar) {		/* a comment */
	 continue;
      }
      for(i = 0;isspace(line[i]);i++) ;
      if(line[i] == '\0') continue;
      
      if(sscanf(line,"%s %s",op_name,key_name) != 2) {
	 printf("error READ EDITing line \"%s\" from file %s\n",line,file);
	 continue;
      }
      if(sscanf(op_name,"\\%o",&i) == 1) { /* \nnn */
	 op_name[0] = i; op_name[1] = '\0';
      }
      if(op_name[1] == '\0') {				/* single char */
	 (void)define_key(key_name,op_name[0]);
	 continue;
      }
      for(i = 0;i < NMAP;i++) {
	 if(!strcmp(op_name,mappings[i].operation)) {	/* found it */
	    (void)define_key(key_name,mappings[i].def_key);
	    break;
	 }
      }
      if(i == NMAP) {
         printf("Unknown operator %s\n",op_name);
      }
   }
   (void)fclose(fil);
   return(0);
}

/********************************************/
/*
 * Specify a mapping from the keyboard
 */
void
define_map(op_name,key_name)
char *op_name,			/* operator desired */
     *key_name;			/* mapped to this string */
{
   int i;

   if(op_name[1] == '\0') {				/* single char */
      (void)define_key(key_name,op_name[0]);
      return;
   }
   for(i = 0;i < NMAP;i++) {
      if(!strcmp(op_name,mappings[i].operation)) {	/* found it */
	 (void)define_key(key_name,mappings[i].def_key);
	 return;
      }
   }
   printf("Unknown operator %s\n",op_name);
}

/*************************************************************/
/*
 * List all current bindings
 */
#define ADD_KEY(C)		/* add a key to kseq[] in printable form */\
  if((C) < 27) { (void)sprintf(kptr,"^%c",(C)+'A'-1); kptr += 2; }	\
  else if((C) == 27) { (void)sprintf(kptr,"^["); kptr += 2; }		\
  else if(isprint(C)) { (void)sprintf(kptr,"%c",(C)); kptr++; }		\
  else { (void)sprintf(kptr,"\\%03o",(C)); kptr += 4; }

static char buff[81],			/* buffer for more */
	    kseq[17];			/* key-sequence being listed */

void
list_map()
{
   char *kptr;				/* pointer to kseq */
   int i,j;
   NODE *root;

   (void)more((char *)NULL);		/* initialise more() */
   for(i = 0;i < sizeof(map)/sizeof(map[0]);i++) {
      kptr = kseq;
      ADD_KEY(i);
      root = &map[i];
      if(root->action == i) {
	 if(verbose == 0) {
	    continue;
	 } else if(isprint(i)) {
	    if(verbose >= 2) {
	       (void)sprintf(buff,"%s\t%c\n",kseq,i);
	       if(more(buff) < 0) break;
	    }
	    continue;
	 }
      }
      if(root->action == UNBOUND) {
	 if(root->subtree == NULL) {
	    (void)sprintf(buff,"%s\tUNBOUND\n",kseq);
	    if(more(buff) < 0) break;
	 } else {
	    if(list_edit_tree(root->subtree,kptr) < 0) break;
	 }
      } else {
	 if((map[i].action & '\200') && isdigit(map[i].action & '\177')) {
	    (void)sprintf(buff,"%s\tExecute word: emacro%c\n",kseq,
			  			     (map[i].action & '\177'));
	 } else {
	    for(j = 0;j < NMAP;j++) {
	       if(mappings[j].def_key == map[i].action) break;
	    }
	    (void)sprintf(buff,"\n%s\t%s",kseq,
				    (j==NMAP)?"UNKNOWN":mappings[j].operation);
	 }
	 if(more(buff) < 0) break;
      }
   }
}

static int
list_edit_tree(node,kptr)
MAP *node;				/* next node to consider */
char *kptr;
{
   char *init_kptr;			/* kptr on entry into function */
   int j;

   init_kptr = kptr;
   ADD_KEY(node->c);

   if(node->action != UNBOUND) {	/* this is a complete binding */
      if((node->action & '\200') && isdigit(node->action & '\177')) {
	 (void)sprintf(buff,"%s\tExecute word: emacro%c\n",kseq,
						      (node->action & '\177'));
      } else {
	 for(j = 0;j < NMAP;j++) {
	    if(mappings[j].def_key == node->action) break;
	 }
	 (void)sprintf(buff,"%s\t%s\n",kseq,
		 	(j==NMAP)?"UNKNOWN":mappings[j].operation);
      }
      if(more(buff) < 0) return(-1);
   }

   if(node->subtree != NULL) {
      if(list_edit_tree(node->subtree,kptr) < 0) {
	 return(-1);
      }
   }

   if(node->next != NULL) {
      if(list_edit_tree(node->next,init_kptr) < 0) {
	 return(-1);
      }
   }

   return(0);
}
