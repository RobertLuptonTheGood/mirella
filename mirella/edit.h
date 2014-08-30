/************************** EDIT.H ********************************/
/*
 * Mirella can incorporate R. H. Lupton's command-line editor with
 * history.
 *
 * The terminal type in use should be in the environment variable
 * MIRTERM.
 *
 * This file provides the information that the history editor needs
 * to know about terminals. To enter a new type, simply add a line
 * to the initialization of types in main.c
 *
 * All that Mirella knows about the terminal is that it assumes that
 * \b and \r will move the cursor back one space and to the start
 * of the line.
 * It also hopes that TERMINAL allows it to delete to end of line,
 * but mostly things will work anyway. If desperate, you could define
 * the delete to eol string as something like "      \b\b\b\b\b\b".
 * If there is no "delete character" all is well, but Wolf has to
 * redraw the line after a history ^ substitution.
 * If there is no "cursor forward" string, things look bad. Try setting
 * overwrite mode with ^T, and buying a new terminal.
 * We also use other attributes, though the editor does not use them
 * directly; back_curs,put_curs,num_kpd,app_kpd.
 */
#define HSIZE 250              /*82*/ /* length of line for history/editor */
#define TERM_SIZE 12

#define MARK '\001'			/* the programme mark */
#define HISTORY_MARK '\002'		/* the history pointer  */
#define SEARCH_MARK '\004'		/* the search got here */


typedef struct edit {
   struct edit *next;		/* next command */
   struct edit *prev;		/* previous command */
   char line[HSIZE + 1];		/* command text */
   char mark;				/* a mark in the list */
   int num;				/* command number */
} EDIT;

typedef struct {
   int baud,			/* baud rate of terminal */
       dlin,			/* `destination line' if "ch" unavailable */
       ncol,			/* number of columns */
       nlin,			/* and number of lines */
       pad;			/* character for padding */
    char name[28];           /* name of term */
    char del_char[TERM_SIZE];       /* delete previous character */
    char del_line[TERM_SIZE];       /* delete to the end of the current line */
    char forw_curs[TERM_SIZE];      /* move cursor forward */
    char back_curs[TERM_SIZE];      /* move cursor backward */
    char put_curs[TERM_SIZE];       /* direct cursor addr */
    char num_kpd[TERM_SIZE];        /* put keypad in numeric mode */
    char app_kpd[TERM_SIZE];        /* put keypad in 'appl' mode */
    char query_curs[TERM_SIZE];     /* query cursor position */
    char sav_curs[TERM_SIZE];       /* save cursor position internally */
    char res_curs[TERM_SIZE];       /* restore cursor after last command */
    char set_keys[TERM_SIZE];	    /* put terminal in "keyboard transmit" */
    char unset_keys[TERM_SIZE];	    /* take it out of "keyboard transmit" */
    char clr_scr[TERM_SIZE];        /* clear screen */
} TERMINAL;


/* terminal dispatch table from edit.h */

typedef struct {
    char fname[28];            /* name of terminal */
    int  (*fdel_char)();       /* function to delete previous char */
    int  (*fdel_line)();       /* delete to the end of the current line */
    int  (*fforw_curs)();      /* move cursor forward */
    int  (*fback_curs)();      /* move cursor backward */
    int  (*fput_curs)();       /* direct cursor addr */
    int  (*fnum_kpd)();        /* put keypad in numeric mode */
    int  (*fapp_kpd)();        /* put keypad in 'appl' mode */
    int  (*fquery_curs)();     /* query cursor position */
    int  (*fsav_curs)();       /* save cursor position internally */
    int  (*fres_curs)();       /* restore cursor after last command */
    int  (*fclr_scr)();        /* clear screen */
    int  (*fscrnprt)();        /* writes a string to the screen at cur curs*/
    int  (*fscrnput)();        /* writes a char "              "           */
    int  (*fscrnflsh)();       /* flushes the buffers to the screen if appl */
    int  t_ncols;              /* how many cols */
    int  t_nrows;              /* how many rows */
    int  t_curx;               /* saved x coor */
    int  t_cury;               /* saved y coor */
    int  t_kpstate;            /* saved keypad state */
    int  ansiflg;              /* flag for functions implemented as ESC seq*/
    int  stdoflg;              /* flag for OK to write on stdout */    
    void (*t_init)();          /* term init routine if any */ 
    void (*t_close)();         /* term close routine if any */ 
} FTERMINAL;

extern FTERMINAL m_fterm;
extern FTERMINAL *mftarray[];
extern int m_nfterm;

extern int dosansiflg;

extern TERMINAL m_term;
/*
 * stuff from mirscrn.c
 */
extern int last_char;			/* the last char that was typed */
extern normal m_scrnl;			/* number of lines on screen */
extern normal x_scur, y_scur;		/* cursor coordinates */
extern char *helpscrptr;		/* pointer to permanent (last) 
					   helpscreen image*/


