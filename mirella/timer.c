/*
 * This code adds a callback to handle timers
 *
 * A stack of timers with associated words is provided. When a timer
 * expires the corresponding word is executed. You can list the timer
 * stack, or delete a given timer.
 *
 * This works by defining a callback function for get1char, so timers
 * are only triggered when mirella is waiting for input. This is probably
 * OK
 */
/*#include <stdio.h> */
#include "mirella.h"

typedef struct timer {			/* element of a stack of timers */
   struct timer *next, *prev;		/* next and previous timer in stack */
   char *word;				/* word to execute */
   int id;				/* which one is it? */
   int duration;			/* how long was it set for? */
   int expires;				/* when does it expire? */
} TIMER;

static void disable_timers P_ARGS(( void ));
static int timer_callback P_ARGS(( void ));
static void tpop P_ARGS(( void ));
static TIMER *tpush P_ARGS(( int t, char *word ));
static void tremove P_ARGS(( TIMER *ptr ));

static TIMER *head = NULL;		/* head of timer stack */
static int mtimer = -1;			/* which mirella timer are we using? */
static int timer_id = 0;		/* number that we'll report as a
					   timer id on the stack */

/*****************************************************************************/
/*
 * Add a new timer to the timer callback stack; if there is no callback
 * function registered register it.
 *
 * The timer will execute <word> in <t>ms
 */
int
add_timer(char *word, int t)
{
   TIMER *timer = tpush(t,word);

   if(timer == NULL) {
      return(-1);
   }
   if(timer->next == NULL && timer->prev == NULL) { /* only element on stack */
      push_callback(timer_callback);
   }
   return(timer == NULL ? -1 : timer->id);
}

/*****************************************************************************/
/*
 * delete a timer on the stack
 */
int
delete_timer(int id)
{
   TIMER *ptr;

   for(ptr = head;ptr != NULL;ptr = ptr->next) {
      if(ptr->id == id) {
	 tremove(ptr);
	 free(ptr->word);
	 free((char *)ptr);

	 if(head == NULL) {		/* disable callbacks */
	    disable_timers();
	    break;
	 }
	 return(0);
      }
   }
   return(-1);
}

/*****************************************************************************/
/*
 * Here's the function used to handle callbacks
 */
static int
timer_callback(void)
{
   while(ntimer(mtimer) > head->expires) { /* time to execute it */
      cinterp(head->word);
      tpop();
      if(head == NULL) {		/* disable callbacks */
	 disable_timers();
	 break;
      }
   }
   return(0);
}

/*****************************************************************************/
/*
 * disable timer callbacks
 */
static void
disable_timers(void)
{
   kill_callback(timer_callback);
   mtimer = -1;
   timer_id = 0;
}

/*****************************************************************************/
/*
 * print active timers
 */
void
print_timers(void)
{
   TIMER *ptr;

   for(ptr = head;ptr != NULL;ptr = ptr->next) {
      printf("\n%2d %-7d %-7d %s",
	     ptr->id,ptr->duration,ptr->expires - ntimer(mtimer),ptr->word);
   }
}

/*****************************************************************************/
/*
 * push a timer onto the timer stack; the one that will expire next
 * is at the bottom
 */
static TIMER *
tpush(int t, char *word)
{
   TIMER *timer;
   TIMER *ptr;

   if((timer = (TIMER *)malloc(sizeof(TIMER))) == NULL) {
      fprintf(stderr,"Can't allocate storage for timer\n");
      return(NULL);
   }
   if(mtimer == -1) {			/* no timer is running */
      mtimer = starttimer();
   }
   timer->id = timer_id++;
   timer->duration = t;
   timer->expires = ntimer(mtimer) + t;
   if((timer->word = malloc(strlen(word) + 1)) == NULL) {
      fprintf(stderr,"Can't allocate storage for timer->word\n");
      free((char *)timer);
      return(NULL);
   }
   strcpy(timer->word,word);
/*
 * push it onto the stack at the proper place, based on how much longer
 * the timers have to run
 */
   if(head == NULL) {
      timer->next = timer->prev = NULL;
   } else {
      for(ptr = head;ptr != NULL;ptr = ptr->next) {
	 if(ptr->expires > timer->expires) {
	    ptr = ptr->prev;		/* we went one too far */
	    if(ptr == NULL) {
	       head->prev = timer;
	       timer->prev = NULL;
	       timer->next = head;
	       head = timer;
	    } else {
	       timer->next = ptr->next;
	       timer->prev = ptr;
	       ptr->next->prev = timer;
	       ptr->next = timer;
	    }
	    
	    break;
	 }
	 if(ptr->next == NULL) {
	    ptr->next = timer;
	    timer->next = NULL;
	    timer->prev = ptr;
	    break;
	 }
      }
      return(timer);
   }
   
   if(timer->prev == NULL) {		/* it's the new head */
      head = timer;
   }

   return(timer);
}

/*****************************************************************************/
/*
 * pop the bottom off the timer stack
 */
static void
tpop(void)
{
   TIMER *timer = head;

   if(head == NULL) {
      fprintf(stderr,"\nThe timer stack is empty");
      return;
   }
   head = head->next;
   if(head != NULL) {
      head->prev = NULL;
   }
   free(timer->word);
   free((char *)timer);
}

/*****************************************************************************/
/*
 * remove a timer from the stack
 */
static void
tremove(TIMER *ptr)
{
   if(ptr->prev == NULL) {		/* this is the head */
      head = ptr->next;
      if(head != NULL) {
	 head->prev = NULL;
      }
   } else {
      if(ptr->prev != NULL) {
	 ptr->prev->next = ptr->next;
      }
      if(ptr->next != NULL) {
	 ptr->next->prev = ptr->prev;
      }
   }
}   
