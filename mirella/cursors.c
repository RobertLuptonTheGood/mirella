/*
 * Various cursor-related utilities
 */
#include "mirella.h"
#include "images.h"

extern int interrupt;			/* have we seen ^C? */

/*****************************************************************************/
/*
 * Take some action based on the initial value of cursor_key (it is
 * an external to allow Mirella words to modify it). This function returns
 * cursor_key which will usually have been set to -1. If it is positive,
 * the device cursor code should exit. If cursor_key on entry is -1,
 * the routine just returns it; this is handy sometimes to make the
 * routine ignore input.
 */
normal cursor_key;

int
do_cursor_key()
{
   int c;
   char word[20];
   static int step = 1;			/* cursor step size */

   c = cursor_key;			/* preserve initial value */
   cursor_key = -1;
   if( c == -1 ) return (-1);

   /* mprintf("\nkey=%d",c); */
   if(c & SPECIAL) {			/* a special key */
      c &= SPECIAL;
      if(c == BUTTON1) {
	 cinterp("lbcursor");
      } else if(c == BUTTON2) {
	 cinterp("cbcursor");
      } else if(c == BUTTON3) {
	 cinterp("rbcursor");
      } else if(c == KEYPAD_CEN) {
	 step = (step > 1) ? 1 : 7;
      /* mprintf("\nkbcen, step=%d",step); */
      } else {
	 fprintf(stderr,"Unknown special key: 0%o\n",c);
      }
   } else {
      switch (c) {
       case N_ARROW:
         /*mprintf("\ncursorup, x,y=%d %d",c_xs,c_ys); */             
	 c_ys += step;
         /* mprintf("\ncursorup, x,y=%d %d",c_xs,c_ys); */
	 break;
       case NE_ARROW:
	 c_xs += step; c_ys += step;
         /* mprintf("\ncursorul, x,y=%d %d",c_xs,c_ys); */
	 break;
       case E_ARROW:
	 c_xs += step;
	 break;
       case SE_ARROW:
	 c_xs += step; c_ys -= step;
	 break;
       case S_ARROW:
	 c_ys -= step;
	 break;
       case SW_ARROW:
	 c_xs -= step; c_ys -= step;
	 break;
       case W_ARROW:
	 c_xs -= step;
	 break;
       case NW_ARROW:
	 c_xs -= step; c_ys += step;
	 break;
       case N_ARROW | CONTROL:
	 dcenter(0,1);
	 break;
       case E_ARROW | CONTROL:
	 dcenter(1,0);
	 break;
       case S_ARROW | CONTROL:
	 dcenter(0,-1);
	 break;
       case W_ARROW | CONTROL:
	 dcenter(-1,0);
	 break;
       case CTRL_C:
       case CTRL_D:
	 c = CTRL_M;
	 interrupt = 1;
	 break;
       case CTRL_L:
         mtv();
         break;              
       case 'f':				/* faster */
	 step++;
mprintf("\nstep=%d",step);	 
	 break;
       case 's':				/* slower */
	 step--;
	 if(step <= 0) step = 1;
mprintf("\nstep=%d",step);	 
	 break;
       case 'e' | META:
	 cinterp("editcursor");
	 break;
       case 'q':
         return(0);
         break;	 
       default:
	 if(isdigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
	    sprintf(word,"%ccursor",c);
	    cinterp(word);
	 } else if(iscntrl(c)) {
	    return(0);
	 }
	 break;
      }
   }
   if(c_xs >= 0 && c_xs < xsize && c_ys >= 0 && c_ys < ysize) {
      c_val = pic[c_ys][c_xs];
   } else {
      c_val = -1;
   }
   return(cursor_key);
}

/***************************** QQ() ***********************************/
/*
 * prints cursor values, checks and updates display window
 */
 
/* ??????/ */

#if 0

void
qq()
{
   int x = curs->_xs;
   int y =  curs->_ys;
   short val;

   image_to_buf(&x,&y);
   if(get_value(&x,&y,&val) == 0) {
      mdprintf("\n%5d %5d %5d",x,y,val);
   } else {
      mdprintf("\n%5d %5d %5s",x,y,"");
   }
}

#endif

void 
qq()
{
    mprintf("\n%d %d %d", c_xs, c_ys, c_val);
}

/* like qq but no newline */
void 
qql()
{
    mprintf("\r%d %d %d", c_xs, c_ys, c_val);
}

