/*
 * screen-handling package for Mirella. Few systems supported as yet
 */
#ifdef VMS
#  include mirella
#  include edit
#else
#  include "mirella.h"
#  include "edit.h"
#endif
#include "scrbuf.h"
/*
 * these quantities are for the res_scrn buffer
 */
       char *scrptr;			/* current pointer in screen buffer */
       char *scrorig;			/* base of circular screen buffer */
static char *scrparr[MAXSCRNL];		/* array of line origin pointers */
static char *endptr;			/* pointer to end of current line */
static int scrlindex;			/* current line
					   (in buffer, NOT on screen) */
static int nlbuf = 0 ;			/* number of complete lines in buffer;
					   last line is also restored */
/*
 * exported to Mirella
 */
normal x_scur, y_scur;			/* cursor coordinates */
char *helpscrptr;			/* pointer to last helpscreen image */
normal dosansiflg = 1;			/* flg for dos systems using ANSI.SYS*/

/*************************************************************************/

void 
reset_scr()
{
    scrptr = scrorig;
    endptr = scrorig;
    scrlindex = 0;
    nlbuf = 0;
    putcur(1,1);
    unreadc(CTRL_L);
}

void
init_scr()   
{   
   register int i;

#ifdef DOS32
   init_dos32();
#endif

   setfont(0);				/* set up char gen pointers */
/*
 * allocate saved_screen buffer
 */
   scrorig = m_sbrk(2*MAXSCRNL*(MAXSCRLL+2));
   if(scrorig == (char *)(-1)){
      scrprintf("init_scr: Cannot allocate screen buffer space\n");
      exit(1);
   }    
   helpscrptr = scrorig + MAXSCRNL*(MAXSCRLL+2);  /* 2nd page is helpscreen*/
/*
 * initialize saved_screen buffer
 */
   nlbuf = 0;
   scrptr = scrorig;
   scrlindex = 0;
   endptr = scrorig + MAXSCRLL;
   filln(MAXSCRNL*(MAXSCRLL+2),scrorig,' ');
   for(i=0;i<MAXSCRNL;i++) {
      scrparr[i] = scrorig + i*(MAXSCRLL);  /* used to say MAXSCRLL+1 ?? */
      scrparr[i][0] = '\0';
   }
}

/*NB!!!!!!!!!! jeg060305
 * There is still something terribly wrong. The length of the lines is
 * still not correct when the buffer is restored.
 */

/************************ WSCRBUF() *********************************/
/*
 * writes a string to the screen buffer
 */ 

void
wscrbuf(buf)
register char *buf;
{
   register char c;
   int n;
   int columns = m_fterm.t_ncols ;

   if(scrptr == NULL) {			/* not yet initialised */
      return;
   }
   
   while((c = *buf++) != '\0'){
    again:        
      if(c < ' '){     /* control character; check it out */
	 switch(c){
	  case '\n':
	    scrlindex++;
	    if(scrlindex >= MAXSCRNL) scrlindex = 0;  /* wrap in buffer*/
	    if(nlbuf < m_fterm.t_nrows-1) nlbuf++; /* NB: nlbuf is # of 
						      current line*/
	    scrptr = scrparr[scrlindex];
	    filln(MAXSCRLL,scrptr,' ');   /* change fill char ??? */
	    scrptr[columns] = '\0';
	    endptr = scrptr + columns;
	    break;
	  case '\r':
	    scrptr = scrparr[scrlindex];   /* go back to beginning */
	    break;
	  case '\b':
	    if(scrptr > scrparr[scrlindex]) scrptr--;
	    break;
	  case '\t':
	    n = 4 - ((scrptr - scrparr[scrlindex])%4) ;
	    while(n--) *scrptr++ = ' ' ;
	    break;
	  default:
	    break;
	 }
      } else {
	 *scrptr++ = c;
	 if(scrptr >= endptr){
	    c = '\n';      /* wrap on screen */
	    goto again;    /* cheat; insert newline and go around again */
	 }
      }
   }
}
 
/*********************** RES_SCR() ***********************************/
/*
 * restores screen if possible
 * jeg060303
 * Several little things fixed, which kept this from working unless
 * there were exactly 80 columns. Still need `screen' to wipe buffer
 * to get rid of nonsense over screen size changes, but this is a 
 * minor annoyance.
 */
void
res_scr()
{
   int i;
   int iidx;
   int curline = m_fterm.t_nrows - 1;
   int istart = scrlindex - curline;
   
   clear_phy_scr();    
   
   if(istart < 0) istart += MAXSCRNL;
   for(i=0;i <= curline;i++) {
      iidx = i + istart;
      if(iidx >= MAXSCRNL) iidx -= MAXSCRNL;
#ifdef DOS32        
      if(dosansiflg) dos_scrline(scrparr[iidx],i);
      else (*scrnprt)(scrparr[iidx]);
#else
      (*scrnprt)(scrparr[iidx]);
#endif
   }
   putcur(1,curline + 1);
   unreadc(CTRL_L);
}

/******************************* END, MIRSCRN.C ************************************************/
