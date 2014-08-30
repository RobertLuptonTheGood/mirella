/*
 * these routines handle the history system, which allows any of the
 * previous commands to be repeated, by specifying ^number, ^string
 * or ^^ by analogy to the Unix csh history. The number of commands
 * remembered is given by the .sm history variable -- if this is zero
 * there is no limit to the number of commands, and if it is absent 80
 * commands are remembered. When reading macros onto the history list
 * there is no limit to the history size.
 *
 * The history list is maintained by a doubly-linked list, each of whose
 * nodes represents a line of history. The two ends of the list are
 * "first" and "last", and a free list of currently unused nodes is
 * maintained as "free_list".
 */
#include "mirella.h"
#include "charset.h"
#include "edit.h"
#include "mireldec.h"			/* yes, we do need it again */

#define FPUTCHAR(C)			/* putchar + fflush */		\
  putchar((C) & '\177');						\
  (void)fflush(stdout);

static char buffer[HSIZE + 1];		/* buffer for unread()'ing */
static char *bptr = buffer;		/* pointer to buffer */
static EDIT *first = NULL;		/* oldest history node */
static EDIT *free_list = NULL;		/* deleted history nodes */
static EDIT *find_lev();		/* return a node given its number */
static EDIT *hptr = NULL;		/* look for a command */
static EDIT *last = NULL;		/* youngest history node */
static int new_hist();			/* get a fresh history node */
static int num_hist = 0;		/* number of nodes allocated */
static int hlev = 1;			/* current history level */
int remember_history_line = 0;		/* remember the last line retrieved? */
extern int interrupt;			/* respond to a ^C */
extern TERMINAL m_term;			/* describe my terminal */
extern int verbose;			/* respond to a request for chatter */

/*
 * Edit the application buffer; appends \n at end, returns length incl newline
 */
int
edit_hist(cmd_line,pflg)
char *cmd_line;				/* application buffer */
int pflg;                    /* do you want a prompt? passed on to edit_line */
{
   char *current,			/* line currently being edited */
   	*ptr;
   int i;
   EDIT *hptr0;				/* the original position of hptr */
   EDIT *temp;

   if(new_hist(0) < 0) return(0);

   current = last->line;
   erase_str(current,HSIZE);

   if(remember_history_line) {
      for(hptr = last;hptr != NULL;hptr = hptr->prev) {
	 if(hptr->mark & HISTORY_MARK) {
	    hptr->mark &= ~HISTORY_MARK;
	    break;
	 }
      }
      if(hptr == NULL) hptr = last;
   } else {
      hptr = last;
   }
   hptr0 = hptr;

   for(;;) {
      switch(i = edit_line(current,pflg)) {
       case EOF :      /* we have gotten a (vms) cancel. Just start over. */
         return(edit_hist(cmd_line,pflg));
       case CTRL_C:
/*debug?  erret((char *)NULL);  */
/*	 interrupt = 0;  */
	 break;
       case CTRL_D | '\200':		/* forget this command */
	 erase_str(current,HSIZE);
	 if(hptr != last) {
	    if(hptr->next != NULL) {	/* cut node out of list */
	       hptr->next->prev = hptr->prev;
	    }
	    if(hptr->prev != NULL) {
	       hptr->prev->next = hptr->next;
	    }
	    if(hptr == first) first = first->next;

	    temp = hptr->next;
	    hptr->prev = free_list;	/* delete this node */
	    free_list = hptr;
	    hptr = temp;
	    (void)strcpy(current,hptr->line);
	 }
	 break;
       case ('g' | '\200') :		/* go to line n */
	 if(*(ptr = get_edit_str("which command?  ")) == '\0') {
	    break;
	 }
	 if((temp = find_lev(atoi(ptr))) == NULL) {
	    putchar(CTRL_G);		/* invalid history, e.g. deleted */
	 } else {
	    hptr = temp;
	    erase_str(current,HSIZE);
	    (void)strcpy(current,hptr->line);
	 }
	 break;
       case CTRL_M :			/* accept current command */
         (*scrnput)(' ');
         flushit(); 

	 for(i = strlen(current) - 1;i >= 0 && current[i] != '\0';i--) {
	    if(!isspace(current[i])) break;
	    current[i] = '\0';
	 }
	 if(i < 0) {			/* null command */
            cmd_line[0] = '\n';
            cmd_line[1] = '\0';
	    delete_last_history();	/* don't remember null commands */
            return(1);
	 }
	 
	 if(last->prev != NULL && strcmp(last->prev->line,current) == 0) {
	    delete_last_history();	/* same as last command */
	 } else {
	    last->num = hlev++;
	 }

	 if(hptr != hptr0) {
	    hptr->mark |= HISTORY_MARK;
	 }
         i = 0;
         while((*cmd_line++ = current[i++]) != '\0') continue;
         *cmd_line = '\0';
         *(cmd_line - 1) = '\n';
         return(i);			/* # char incl newline */
       case CTRL_N :			/* next command */
	 erase_str(current,HSIZE);
	 if(hptr->next != NULL) {
	    hptr = hptr->next;
	    (void)strcpy(current,hptr->line);
	 }
	 break;
       case CTRL_P:			/* previous command */
	 if(hptr->prev == NULL) {
	    putchar(CTRL_G);
	 } else {
	    erase_str(current,HSIZE);
	    hptr = hptr->prev;
	    (void)strcpy(current,hptr->line);
	 }
	 break;
       case CTRL_R:			/* reverse search */
       case CTRL_S:			/* forward search */
	 if(*(ptr = get_edit_str("string: ")) == '\0') {
	    *ptr = CTRL_S;		/* repeat search */
	 }
	 erase_str(current,HSIZE);
	 if((temp = find_str(ptr,(i == CTRL_R) ? 1 : -1,first,last)) == NULL) {
	    putchar(CTRL_G);
	 } else {
	    hptr = temp;
	    (void)strcpy(current,hptr->line);
	 }
	 break;
       case '<' | '\200':		/* first command */
	 erase_str(current,HSIZE);
	 hptr = first;
	 (void)strcpy(current,hptr->line);
	 break;
       case '>' | '\200':		/* last command */
	 erase_str(current,HSIZE);
	 if(last != NULL && last->prev != NULL) {
	    hptr = last->prev;
	 }
	 (void)strcpy(current,hptr->line);
	 break;
       default:
	 putchar(CTRL_G);
	 break;
      }
      (void)fflush(stdout);
   }
}

/******************************************************************/
/*
 * read a character from stdin, and store it in history buffer 
 * use get1char, so character retrieval is immediate. (Inverse is unreadc())
 *
 * The default history character is ^, and it is used in the comments
 */
#define HISTORY_CHAR ('^' | '\200')	/* character to retrieve history */

int
readc()
{
   char word[40],                       /* as in ^word for history request */
	*wptr;				/* pointer to word */
   EDIT *command = NULL;		/* the command of interest */
   int c,                               /* next character */
       i,
       len;                             /* length of word */

   if(bptr > buffer) return((int)*--bptr); /* first try the buffer */
/*
 * there are no characters waiting on the buffer, so read one from stdin.
 * look for a string ^c
 */
   if((c = map_key()) != HISTORY_CHAR) { /* not history */
      return(c);
   } else {						 /* probably history */
/*
 * we now know that we are dealing with a history symbol one of
 * ^^, ^$, ^nnn, or ^string so look at second character
 */
      FPUTCHAR(last_char);
      if((c = map_key()) == HISTORY_CHAR || c == '$') {
	 printf("\b%s",m_term.del_char); (void)fflush(stdout); /* delete ^ */
	 if((command = last->prev) == NULL) {
	    len = -1;
	 } else {
	    len = strlen(command->line);	/* length of command */
	    while(isspace(command->line[--len])) ;
	    len++;
	 }

	 if(m_term.del_char[0] == '\0')	/* terminal is dumb, so redraw line */
	   (void)unreadc(CTRL_L);

	 if(c == HISTORY_CHAR) {
	    while(--len >= 0) {
	       (void)unreadc(command->line[len]);
	    }
	 } else {
	    while(--len >= 0 && !isspace(i = command->line[len])) {
	       (void)unreadc(i);
	    }
	    printf("%s",m_term.del_char); (void)fflush(stdout); /* delete ^ */
	 }
	 return(readc());
      }
/*
 * Not one of special cases about last line, so find which line is wanted
 */
      wptr = word;
      *wptr++ = '^';			/* match at start of line */
      (void)unreadc(c);			/* push first character */
      do {
	 if((c = readc()) == DEL || c == CTRL_H) {
	    printf("\b%s",m_term.del_char); /* delete previous character */
	    (void)fflush(stdout);
	    if(wptr > word) {
	       wptr--;			/* delete previous */
	    } else {
	       return(readc());
	    }
	 } else {
	    if(c != '\0' && !isspace(c)) { FPUTCHAR(c); }
	    *wptr++ = c;
	    }
      } while(c != '\0' && !isspace(c)) ;
      c = *--wptr;			/* save last character*/
      *wptr = '\0';
/*
 * we have the string from ^string in word[], but it may be an integer
 */
      len = strlen(word);
      for(i = 1;i < len;i++) {		/* skip leading `^' */
	 if(i == 1 && word[i] == '-') continue;
	 if(!isdigit(word[i])) break;	/* string not just digits */
      }

      if(i == len) {			/* string was an integer */
	 command = find_lev(atoi(&word[1]));
      } else {				/* a string */
	 command = find_str(word,1,first,last);
      }
/*
 * we now know that the required command was "command" (NULL if not found)
 * delete characters of ^string
 */
      for(i = strlen(word);i > 0;i--) printf("\b%s",m_term.del_char);

      if(m_term.del_char[0] == '\0')	/* terminal is dumb, so redraw line */
	 (void)unreadc(CTRL_L);

      if(command == NULL) {
	 putchar(CTRL_G);		/* ring bell */

	 (void)unreadc(CTRL_H);
	 (void)unreadc(c);		/* something for readc() to read */
      } else {
	 (void)unreadc(c);	     /* char which terminated history symbol */
         len = strlen(command->line);
         while(--len >= 0) (void)unreadc(command->line[len]);
      }
      return(readc());
   }
}

/***************************************************/
/*
 * Return a character to the history buffer
 */
void
unreadc(c)
int c;
{
   if(bptr - buffer >= HSIZE) {
      fprintf(stderr,"Attempt to push too many characters in unreadc\n");
      return;
   } else {
      *bptr++ = c;
   }
}

/***************************************************/
/*
 * Define a macro from the history buffer
 */
#if 0
int
define_history_macro(name,l1,l2)
char name[];				/* name of macro */
int l1,l2;				/* use history commands l1-l2 */
{
   char *macro,				/* text of macro */
	*mptr;				/* pointer to macro */
   EDIT *hl1,*hl2;			/* the two history nodes */
   EDIT *hptr;				/* node that traverse history */
   int i,
       len,				/* length of macro */
       nchar;				/* number of characters in macro */

   if(first == NULL) {
      if(verbose > 1) {
	 printf("There are no lines on the history list\n");
      }
      return(-1);
   }

   if(l2 < l1) {			/* switch l2 and l1 */
      i = l1;
      l1 = l2;
      l2 = i;
   }
/*
 * Check that lines are on history list
 */
   if((hl1 = find_lev(l1)) == NULL) {
      if(verbose > 1) {
	 fprintf(stderr,"Line %d is not on history list\n",l1);
      }
      hl1 = first;
   }
   if((hl2 = find_lev(l2)) == NULL) {
      if(verbose > 1) {
	 fprintf(stderr,"Line %d is not on history list\n",l2);
      }
      hl2 = last;
   }
/*
 * We must convert lines into a linear array
 */
   nchar = 0;
   for(hptr = hl1;;hptr = hptr->next) {	/* count num. of characters */
      nchar += strlen(hptr->line) + 1;	/* allow for \n or \0 */
      if(hptr == hl2) break;
   }
   if(nchar <= 0) {
      (void)define(name,"",0);
      return(0);
   }
   if((macro = malloc((unsigned)nchar)) == NULL) {
      fprintf(stderr,"Can't malloc space in define_history_macro\n");
      return(-1);
   }
/*
 * We have the space, so copy lines onto mptr
 */
   mptr = macro;
   for(hptr = hl1;;hptr = hptr->next) {
      (void)strcpy(mptr,hptr->line);
      len = strlen(hptr->line);
      mptr += len;
      if(len > 0) {
	 *mptr++ = '\n';
      }
      if(hptr == hl2) break;
   }
   if(mptr > macro) {
      *(mptr - 1) = '\0';
   }
   (void)define(name,macro,0);
   free(macro);
   return(0);
}
#endif

/***************************************************/

void
history_list(dir)
int dir;				/* direction to list in */
{
   char buff[90];
   EDIT *hptr;
   int lev;

   for(lev = 1,hptr = first;hptr != NULL;hptr = hptr->next,lev++) {
      hptr->num = lev;
   }
   lev--;
	 
   (void)more((char *)NULL);			/* initialise more */
   if(dir >= 0) {
      for(hptr = last;hptr != NULL;hptr = hptr->prev,lev--) {
	 (void)sprintf(buff,"%3d  %s\n",hptr->num,hptr->line);
	 if(more(buff) < 0) break;
      }
   } else {
      for(lev = 1,hptr = first;hptr != NULL;hptr = hptr->next,lev++) {
	 (void)sprintf(buff,"%3d  %s\n",hptr->num,hptr->line);
	 if(more(buff) < 0) break;
      }
   }
}

/***********************************************************/
/*
 * write history to a file
 */
void
write_hist()
{
   char *outfile;			/* file for saving history */
   FILE *fil;				/* stream for write */
   EDIT *hptr;

   if((outfile = get_def_value("history_file")) == NULL) {
      return;
   }

#ifdef vms			/* must delete old version of saved history */
   delete(outfile);
#endif
   if((fil = fopen(outfile,"w")) == NULL) {
      fprintf(stderr,"Can't open file %s for saving history\n",outfile);
      return;
   }

   for(hptr = first;hptr != NULL;hptr = hptr->next) {
      fprintf(fil,"%s\n",hptr->line);
   }

   (void)fclose(fil);
   return;
}

/***********************************************************/
/*
 * read history from a file
 */
void
read_hist()
{
   char *outfile;			/* file for saving history */
   FILE *fil;				/* stream for read */

   if((outfile = get_def_value("history_file")) == NULL) {
      return;
   }
   if((fil = fopen(outfile,"r")) == NULL) {
      return;
   }

   while(new_hist(0) == 0 && fgets(last->line,HSIZE,fil) != NULL) {
      last->line[strlen(last->line) - 1] = '\0';
      last->num = hlev++;
   }
   delete_last_history();		/* we created one too many */

   (void)fclose(fil);
   return;
}

/****************************************************/
/*
 * Delete an entry from the history list
 */
void
delete_history(l1,l2,this_too)
int l1,l2;				/* range of lines to delete */
int this_too;				/* forget this command too */
{
   EDIT *hl1,*hl2;			/* the two history nodes */
   int i;

   if(first == NULL) {
      if(verbose > 1) {
	 printf("There are no lines on the history list\n");
      }
      hlev = 1;
      return;
   }

   if(l2 < l1) {			/* switch l2 and l1 */
      i = l1;
      l1 = l2;
      l2 = i;
   }
/*
 * Check that lines are on history list
 */
   if((hl1 = find_lev(l1)) == NULL) {
      if(verbose > 1) {
	 fprintf(stderr,"Line %d is not on history list\n",l1);
      }
      hl1 = first;
   }
   if((hl2 = find_lev(l2)) == NULL) {
      if(verbose > 1) {
	 fprintf(stderr,"Line %d is not on history list\n",l2);
      }
      hl2 = last;
   }

   if(hl1 == first) {			/* fixup first and last */
      first = hl2->next;
   }
   if(hl2 == last) {
      last = hl1->prev;
      this_too = 0;			/* it'll go automatically */
   }

   if(hl1->prev != NULL) {		/* cut section from history */
      hl1->prev->next = hl2->next;
   }
   if(hl2->next != NULL) {
      hl2->next->prev = hl1->prev;
   }

   hl1->prev = free_list;		/* and put it onto free list */
   free_list = hl2;

   if(first == NULL) {			/* reset history counter */
      hlev = 1;
   }

   if(this_too) {
      delete_last_history();
   }
}

/****************************************************/
/*
 * Delete most recent entry from the history list
 */
void
delete_last_history()
{
   EDIT *temp;

   if(last == NULL) {
      hlev = 1;
      return;
   }
   
   temp = last;
   last = last->prev;			/* cut it off active list */
   if(last != NULL) {
      last->next = NULL;
   } else {
      first = NULL;
   }
   
   temp->prev = free_list;		/* and put it on free list instead */
   free_list = temp;
}

/*******************************************************/
/*
 * Replace last command on history list by another
 */
void
replace_last_command(string)
char *string;				/* desired new command */
{
   (void)strncpy(last->line,string,HSIZE);
}

/***************************************************************/
/*
 * find_lev returns a history node, given a history number
 */
static EDIT *
find_lev(hnum)
int hnum;				/* the history number */
{
   EDIT *hptr;

   if(hnum < 0) {			/* number relative to end of list */
      for(hptr = last;hptr != NULL && hnum < 0;hptr = hptr->prev,hnum++) {
	 ;
      }
   } else {
      for(hptr = last;hptr != NULL;hptr = hptr->prev) {
	 if(hptr->num == hnum) {
	    break;
	 }
      }
   }
   return(hptr);
}

/***************************************************************/
/*
 * get a fresh node for the next history command
 */
static int
new_hist(alloc)
int alloc;			/* if true, always make a node if needed */
{
   char *ptr;
   static int hsize = -1;		/* size of history list */

   if(hsize < 0) {
      if((ptr = get_def_value("history")) == NULL) {
	 hsize = 80;
      } else {
	 hsize = atoi(ptr);
      }
   }

   if(last == NULL) {
      if(free_list == NULL) {		/* no deleted nodes */
	 if((last = (EDIT *)malloc(sizeof(EDIT))) == NULL) {
	    fprintf(stderr,"Can't allocate storage for new history node\n");
	    return(-1);
	 }
	 num_hist++;
      } else {
	 last = free_list;
	 free_list = free_list->prev;
      }
      last->next = last->prev = NULL;
      first = last;
   } else {
      if(free_list != NULL) {		/* get one from free list */
	 last->next = free_list;
	 free_list = free_list->prev;
      } else {				/* need a new node... */
	 if(alloc || hsize <= 0 || num_hist < hsize) {
	    if((last->next = (EDIT *)malloc(sizeof(EDIT))) == NULL) {
	       fprintf(stderr,"Can't allocate storage for new history node\n");
	       return(-1);
	    }
	    num_hist++;
	 } else {			/* re-use oldest command */
	    last->next = first;
	    if(first->next != NULL) first->next->prev = NULL;
	    first = first->next;
	 }
      }
      last->next->prev = last;
      last->next->next = NULL;
      last = last->next;
   }
   last->mark = '\0';
   return(0);
}
