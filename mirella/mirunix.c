/******************* MIRUNIX.C() ***************************************/
/* 
 * System interface code for Berkeley and various POSIX Unices Bk4.2, 
 * Sun, Vax, Convex, Linux, OSX
 */

#include <signal.h>
#include "mirella.h"

extern normal interrupt;		/* handle ^C interrupts */

#define NOWT 0

/*************** GET1CHAR(), Q-KEY(), KEY_AVAIL() *************************/
/* Get1char is the basic raw (cbreak in unix) nonechoing character receiver.
The mode is initialized with a CTRL_A; CTRL_A is returned if successful. 
An argument of 0 (NOWT) returns a raw character as it is struck on the 
keyboard. An argument of EOF (returns EOF) terminates the raw mode and 
returns the keyboard to normal edited input. q_key() returns 1 if there is 
a keystroke waiting, 0 if not; key_avail returns the number of keystrokes
in the typeahead buffer; these are not functionally implemented in all
systems, and always return 0 in that case.  */

#if defined(WIN32) && defined(__GNUC__)
#define POSIX_TTY
#endif

#if defined(POSIX_TTY)   
    /* defined by Linux and other POSIX systems */
#   include <termios.h>
#else
#   ifdef SYS_V
#       include <termio.h>
#   else
#       include <sgtty.h>
#   endif
#endif

#define REDRAW(FD)			/* Redraw graphics; SM */
#define CLOSE()				/* Close graphics device; SM */

#define GET1CHAR			/* we have a get1char() */

/*#define G1CDEBUG*/                    /* intermediate debugging output */

int
get1char(c)
int c;					/* what shall I do ? */
{
   void setsigint();
   char i;
   int old_interrupt;			/* value of interrupt before read */
   static int fildes = -1,
              first = 1,		/* is this first entry? */
	      is_tty;			/* is stdin attached to a tty? */
#if defined(POSIX_TTY) || defined(SYS_V)
#  if defined(POSIX_TTY)
   struct termios arg;
   static struct termios save_arg;
#  else
   struct termio arg;
   static struct termio save_arg;
#  endif
#else
   struct sgttyb arg;			/* state of terminal */
   static struct sgttyb save_arg;	/* initial state of terminal */
   static struct sgttyb save_mod_arg;   /* modified initial state--^C */
   struct ltchars ltarg;		/* interrupt chars for terminal */
   static struct ltchars save_ltarg;	/* saved value of ltarg */
#endif

   /********* IF c=1, INITIALIZE GET1CHAR ************************/
   if(c == CTRL_A) {                    /* set up for 1-character mode */
#ifdef G1CDEBUG
      printf("\nget1char got a ^A \n");
#endif
      if(first) {                       /* save initial state--only ONCE */
      
	 first = 0;

	 is_tty = isatty(0);
	 if((fildes = open("/dev/tty",2)) < 0) {
	    printf("Can't open /dev/tty in get1char\n");
	    return(-1);
	 }
#if defined(POSIX_TTY) || defined(SYS_V)
#  if defined(POSIX_TTY)
         (void)tcgetattr(fildes,&save_arg);
#  else
         (void)ioctl(fildes,TCGETA,(char *)&save_arg);
#  endif
#else
	 (void)ioctl(fildes,TIOCGLTC,(char *)&save_ltarg);
         (void)ioctl(fildes,TIOCGETP,(char *)&save_arg);
#endif
	 (void)close(fildes);
      }

      if(!is_tty) {			/* stdin isn't attached to a tty */
	 fildes = 0;			/* read stdin */
	 return(CTRL_A);
      }

      if((fildes = open("/dev/tty",2)) < 0) {
	 printf("Can't open /dev/tty in get1char\n");
	 return(-1);
      }
      arg = save_arg;				/* get initial state */
#if defined(POSIX_TTY) || defined(SYS_V)
      arg.c_lflag &= ~(ICANON | ECHO);  /*add ISIG if want to NOT process ^C*/
      /*arg.c_lflag &= ~(ICANON | ECHO | ISIG);*/  /*do NOT process ^C*/
      /* arg.c_lflag |= ISIG;*/  /*  is this what is missing ?? no */
      arg.c_iflag &= ~(INLCR | ICRNL);
      arg.c_cc[VMIN] = 1;
      arg.c_cc[VTIME] = 0;
      /* arg.c_cc[VINTR] = 4; */  /* set ^D for interrupt; crashes like ^C */
#if defined(VDSUSP)			/* ^Y on solaris */
      arg.c_cc[VDSUSP] = -1;
#endif
#  if defined(POSIX_TTY)
      arg.c_cc[VSUSP] = -1;
      (void)tcsetattr(fildes,TCSADRAIN,&arg);
#  else
      (void)ioctl(fildes,TCSETA,(char *)&arg);
#  endif
#else
      ltarg = save_ltarg;
      ltarg.t_suspc = -1;		/* default: ^Z */
      ltarg.t_dsuspc = -1;		/*          ^Y */
      ltarg.t_flushc = -1;		/*          ^O */
      ltarg.t_lnextc = -1;		/*          ^V */
      (void)ioctl(fildes,TIOCSLTC,(char *)&ltarg);
      arg.sg_flags |= CBREAK;
      arg.sg_flags &= ~(ECHO | CRMOD);
      (void)ioctl(fildes,TIOCSETN,(char *)&arg); /* set new mode */
#endif
      setsigint();
      return(CTRL_A);
    
   /*************** REVERT IF c>32 ***************************************/
   
   } else if(c == EOF) {			/* revert to initial state */
#ifdef G1CDEBUG   
      printf(" get1char got an EOF\n");
#endif 
      if(fildes <= 0){
         return(EOF);
      }
#if defined(POSIX_TTY) || defined(SYS_V)
#  if defined(POSIX_TTY)
      (void)tcsetattr(fildes,TCSADRAIN,&save_arg);
#  else
      (void)ioctl(fildes,TCSETA,(char *)&save_arg);
#  endif
#else
      (void)ioctl(fildes,TIOCSETN,(char *)&save_arg);	/* reset terminal */
      (void)ioctl(fildes,TIOCSLTC,(char *)&save_ltarg);
#endif
      (void)close(fildes);
      fildes = -1;
      /* reinstall the signal handlers ?? */
#ifdef SIGNALS 
#ifdef SIGACTION
        sigmirella.sa_handler=(void (*)P_ARGS((int)))hdl_sig;
        sigaction(SIGINT,&sigmirella,(struct sigaction *)NULL);
#else
        signal(SIGINT,(void (*)P_ARGS((int)))hdl_sig);
#endif                
#ifdef G1CDEBUG           
        printf("Resetting signal handler\n");
#endif
#endif  /* SIGNALS */
      setsigint();
      return(EOF);

   /*********** JUST GET A CHARACTER (c=0) *************************/

   } else {		
      if(fildes < 0){
         return(EOF);
      }
/*
 * give the device a chance 
 */
#if defined(macOSX)
      REDRAW(0);			/* appears to have trouble  mixing
					 * select() and /dev/tty 
					 */
#else
      REDRAW(fildes);
#endif

      old_interrupt = interrupt;
      setsigint();
      while(read(fildes,&i,1) < 0) {	/* EOF or ^C */
      setsigint();

#if defined(EINTR)
	 if(errno == EINTR) continue;
#endif
	 if(interrupt == old_interrupt) { /* not ^C; must be EOF */
	    (void)get1char(EOF);
	    CLOSE();
	    exit(0);
	 }
	 /* if we are returning from an interrupt while in this routine,
	    just do an erret() to clean up */
         if(interrupt !=0) erret(NULL);	 
         /*debug interrupt = old_interrupt;  This is crazy ???? */
      }
      i &= 0177;	   /* kill the 8th bit */
      if(i == CTRL_C){  /* this should never happen if ISIG is set; if not,
                         * do something appropriate 
                         */
          /* hdl_sig(SIGINT); */
          erret("^C");
      }
      return (i);
   }
}

/*
 * return 1 if a key is available
 */
int
key_avail()
{
   fd_set read_fds;

   FD_ZERO(&read_fds);
   FD_SET(1,&read_fds);
   select(2,&read_fds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *)NULL);
   return(FD_ISSET(1,&read_fds) ? 1 : 0);
}

/* in this implementation, q_key() and key_avail() have identical
 * functionality; q_key is *SUPPOSED* to return the character waiting
 * without disturbing the next read; I suppose we need a resident
 * buffer?? (why can't we use the OS buffer?)
 */

int q_key()
{
    return ( key_avail() ? 1 : 0);   /* can we get the character ?? */
}

/************************ DOPAUSE() *********************************/
/* detects ^C */

int dopause()
{
    return interrupt;
}

/*********************** BK PROCESS CONTROL ******************************/

normal shellesc()
{
   char *shell = get_def_value("shell");

   if(shell == NULL) {
      shell = "csh";
   }
   return system(shell);
}
 
/* Suspend a process, and return control to its parent */

#ifdef SIGNALS
#include <signal.h>

int
suspend()
{
   return(kill(getpid(),SIGSTOP));	/* suspend process */
}
#else
int 
suspend()     /* "suspend" same as shellescape if not using signals */
{
    return(shellesc());
}
#endif

normal
attach(pid)
int pid;
{
    scrprintf("\nThe ATTACH function is not supported in this environment");
    return (-1);
}

/******************** BK MEMORY ***************************************/

#if !defined(filln)
void filln(n,buf,chr)  /* fills the buffer buf with n copies of chr
                            --defined in DSI32 lib */
register char *buf;
register int n;
register int chr;
{
    if(n>0){
        while(n--) *buf++ = chr;
    }
}
#endif

/******************** TIMEOFDAY AND TIMER FUNCTIONS **********************/

struct mir_tm m_ltime; /* time structure (struct tm, defined in time.h */
normal m_ytime;        /* deciseconds into the current year, local or
                        * universal depending on how generated 
                        */

unsigned int 
gtime()  /* running ms clock */
{
    struct timeval times;
    gettimeofday(&times,NULL);
    return((times.tv_usec/1000 + 1000*(times.tv_sec & 0x3fffff)) & 0x3fffffff);
}

char *getltime()
{
    struct timeval times;
    long clock;
    int len;
    static char timestr[30];
    
    gettimeofday(&times,NULL);
    clock = times.tv_sec;
    m_ltime = *((struct mir_tm *)localtime(&clock));
    m_ytime = m_ltime.tm_yday*864000 + m_ltime.tm_hour*36000 +
              m_ltime.tm_min*600 + m_ltime.tm_sec*10 + times.tv_usec/100000;
    strcpy(timestr,ctime(&clock));
    len = strlen(timestr);
    timestr[len-1] = '\0';  /* kill newline */
    return timestr;
}

char * getutime()
{
    struct timeval times;
    long clock;
    int len;
    static char timestr[30];

    gettimeofday(&times,NULL);
    clock = times.tv_sec ;
    m_ltime = *((struct mir_tm *)gmtime(&clock));
    m_ytime = m_ltime.tm_yday*864000 + m_ltime.tm_hour*36000 +
              m_ltime.tm_min*600 + m_ltime.tm_sec*10 + times.tv_usec/100000;
    strcpy(timestr,asctime(gmtime(&clock)));
    len = strlen(timestr);
    timestr[len-1] = '\0';  /* kill newline */
    return timestr;
}

char *datestr()
{
    struct timeval times;
    long clock;
    static char datestr[30];

    gettimeofday(&times,NULL);
    clock = times.tv_sec ;
    m_ltime = *((struct mir_tm *)gmtime(&clock));
    sprintf(datestr,"%02ld%02ld%02ld", (long)m_ltime.tm_year%100, (long)m_ltime.tm_mon + 1,
	    (long)m_ltime.tm_mday);
    return datestr;
}

/* NB!!!! do not use m_ltime and m_ytime for this...make some new variables */

struct mir_tm m_altime;
normal m_aytime;

char *mctime(clock) /* makes ascii string and populates structure, local time
                     * from unix time seconds  
                     */
long clock;         /* unix time, seconds */
{

    int ftime();
    int len;
    static char timestr[30];
    
    m_altime = *((struct mir_tm *)localtime(&clock));
    m_aytime = m_altime.tm_yday*864000 + m_altime.tm_hour*36000 +
              m_altime.tm_min*600 + m_altime.tm_sec*10 ;
    strcpy(timestr,ctime(&clock));
    len = strlen(timestr);
    timestr[len-1] = '\0';  /* kill newline */
    return timestr;
}


void mcutime(clock) /* populates structure, universal
                      * time from unix time seconds 
                      */
long clock;         /* unix time, seconds */
{
    m_altime = *((struct mir_tm *)gmtime(&clock));
    m_aytime = m_altime.tm_yday*864000 + m_altime.tm_hour*36000 +
              m_altime.tm_min*600 + m_altime.tm_sec*10 ;
}

