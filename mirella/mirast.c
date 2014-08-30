#include "mirella.h"

/* Code for minimal AST-type support for Mirella; asts can only be
 * delivered when the system is waiting for terminal input.
 * The asts are checked and delivered by get1char(),
 * and the mechanism ONLY works for systems
 * with a working q_key() facility. get1char() has been modified
 * (DOS 930906) to check for time to deliver an ast if no input is present.
 */


/********************** EDIT_WAITFUN() AND ASSOCIATED CODE ****************/

/* the first part of this file is general stuff for get1char() to execute
 * and code to manipulate the associated dispatch stack.
 */
 
#define NWFUNC 16
static CALLBACK_FUNC_PTR callback_fns[NWFUNC];
static int ncallback_fns = 0;

/*
 * This is called while waiting for input; it just round-robins through
 * the callback stack, executing the functions in the array which
 * return 0, and returning the (int) value of the first nonzero
 * return, which will generally be an event of some sort which is to
 * generate a character for get1char(); the low order byte is the
 * character. One could in principle send three bytes this way,
 * reserving the top byte in a 4-byte int for some nonzero value if
 * one wishes to send three nuls. Think about it.
 */
int
call_callback_fns()
{
   int i;
   int ret;
    
   for(i = 0;i < ncallback_fns;i++){
      if((ret = (*callback_fns[i])()) != 0) {
	 return(ret);
      }
   }
   return(0);
}

/*****************************************************************************/
/*
 * pushes the func f on the callback stack
 */
void
push_callback(f)
CALLBACK_FUNC_PTR f;
{
    int i;

    for(i=0;i<ncallback_fns;i++){
        if(callback_fns[i] == f){
            mprintf("\nFp %x already on callback stack",f);
            return;
        }
    }
    if(ncallback_fns >= NWFUNC) erret("\nWait fun stack full");
    callback_fns[ncallback_fns++] = f;
}


/*****************************************************************************/
/*
 * pops the callback stack; returns -1 if empty
 */
int
pop_callback()
{
   if(ncallback_fns == 0){
      mprintf("%s","\nWaitfun stack empty");
      return(-1);
   } else {
      ncallback_fns--;
      return(0);
   }
}

/*****************************************************************************/
/* 
 * removes f from the wait stack if it is there; if not, message but
 * continue and returns -1 
 */
int
kill_callback(f)
CALLBACK_FUNC_PTR f;
{
   int i;
   
   for(i=0;i<ncallback_fns;i++){
      if(callback_fns[i] == f) break;
   }
   if(i == ncallback_fns){    /* not there */
      mprintf("\nCannot find %d on callback stack");
      return(-1);
   } else {                     /* gotit */
      int i0 = i;
      for(i=i0+1;i<ncallback_fns;i++){
	 callback_fns[i-1] = callback_fns[i];
      }
      ncallback_fns--;
      return(0);
   }
}

/* returns 0 if f is not on the wait stack, the index + 1 if it is */

int
is_callback(f)
CALLBACK_FUNC_PTR f;
{
   int i;
   for(i=0;i<ncallback_fns;i++){
      if(callback_fns[i] == f) break;
   }
   if(i == ncallback_fns){		/* not there */
      return (0);
   } else {				/* gotit */
      return(i+1);   
   }
}

/********************** EDIT_OUTFUN() AND ASSOCIATED CODE ****************/

/* the second part of this file is general stuff for flushit() to execute
 * on the output buffer.
 */
 
#define NOFUNC 16
static CALLBACK_FUNC_PTR edit_outfar[NOFUNC];
static int nedit_outfun = 0;

/* 
 * this is the outfunction; it just round-robins through the outfunction
 * stack, executing all the functions in the array. If anything returns
 * a nonzero value (error) it stops and returns that value.
 */
 
int
edit_outfun(s)
char *s;
{
    int i;
    int ret;
    
    if(nedit_outfun == 0) return (0);
    for(i=0;i<nedit_outfun;i++){
        ret = (*edit_outfar[i])(s);
        if(ret) return (ret);
    }
    return (0);
}

/* pushes the func f on the outfun stack */

void push_outfun(f)
CALLBACK_FUNC_PTR f;
{
    int i;
    for(i=0;i<nedit_outfun;i++){
        if(edit_outfar[i] == f){
            mprintf("\nFp %x already on outfun stack",f);
            return;
        }
    }
    if(nedit_outfun >= NOFUNC) erret("\nOutfun stack full");
    edit_outfar[nedit_outfun++] = f;
}

/* pops the outfun stack; returns -1 if empty */

int
pop_outfun()
{
    if(nedit_outfun == 0){
        mprintf("%s","\nOutfun stack empty");
        return (-1);
    }else{
        nedit_outfun--;
        return (0);
    }
}

/* 
 * removes f from the out stack if it is there; if not, message but
 * continue and returns -1 
 */
    
int
kill_outfun(f)
CALLBACK_FUNC_PTR f;
{
    int i,i0;
    for(i=0;i<nedit_outfun;i++){
        if(edit_outfar[i] == f) break;
    }
    if(i == nedit_outfun){    /* not there */
        mprintf("\nCannot find %x on outfun stack",f);
        return (-1);
    }else{                     /* gotit */
        i0 = i;
        for(i=i0+1;i<nedit_outfun;i++){
            edit_outfar[i-1] = edit_outfar[i];
        }
        nedit_outfun--;
        return  (0);
    }
}

/* returns 0 if f is not on the out stack, the index + 1 if it is */

int
is_outfun(f)
CALLBACK_FUNC_PTR f;
{
   int i;
   for(i=0;i<nedit_outfun;i++){
      if(edit_outfar[i] == f) break;
   }
   if(i == nedit_outfun){		/* not there */
      return (0);
   } else {				/* gotit */
      return(i+1);
   }
}

#if defined(DOS32)

/* the rest of the stuff is for timer and 'ast' functions, which might
 * be queued by hardware interrupts, and are a specific kind of wait
 * function. The asts are described by an array of structures which
 * are kept sorted for delivery; one member is the clock time (as read
 * from the Mirella timer gtime() at which the ast is to be executed, or
 * zero if it is to be executed ASAP
 */
 
#define NAST_Q 16			/* depth of ast queue */
#define MAST_TYPE 0
#define CAST_TYPE 1

normal nast_q;       /* number of asts pending */
normal nextast_time; /* time for next ast, or zero for non_timer asts */

struct ast_entry_t{
    normal ast_time;    /* time as reported by gtime() for trigger */
    normal ast_type;    /* 0 for forth words, 1 for C functions. */
    normal *ast_cfa;    /* cfa of word or pointer to function 
                         * to be executed. NB!!!! There is 
                         * nothing to keep the hapless user from screwing
                         * with the dictionary while waiting for an AST.
                         * caveat emptor. It might be better to keep the
                         * string and interpret it, but the baggage is
                         * pretty terrible.
                         */
    char *ast_arg;      /* optional argument to be passed to ast. If a
                         * C function, it is passed to the function; it can
                         * be any normal-sized quantity (be careful of casts)
                         * but is typically the
                         * address of an argument list. For forth asts this
                         * quantity is pushed onto the parameter stack. NB!!!
                         * ast-words MUST!!!!!! pop the stack even if they
                         * 'need' no arguments.
                         */
}ast_stack[NAST_Q];

int sizast = sizeof(struct ast_entry_t);
char *past_stack = (char *)ast_stack;

extern void ast_clr(),          /* clears ast flag AND i/o flag */
            ast_enable(),       /* arg 0, disables asts, 1, enables */
            mast_queue(),       /* queues asts specified by cfa */
            cast_queue(),       /* queues asts specified by fn ptr */
            ast_preamble(),     /* init code for ASTs which do screen I/O */
            ast_postable(),     /* cleanup code for ASTs which do screen I/O */
            ast_deliver(),      /* delivers the top AST */
            ;

extern int  ast_flg(),          /* 1 if ast has occurred since last ast_clr()*/
            ast_is_enabled(),   /* 1 if enabled, 0 otherwise */
            ast_check(),        /* checks time and delivers top AST if time */
            ;

static int ast_enflg = 1;
static int ast_set = 0;

/* The philosophy of the ast queue is that checking and delivery should be very
 * efficient but queueing a new one need not be. Therefore for simplicity
 * the structures are kept on a physical stack, which is assumed always
 * to be sorted so that the top of the stack is always the next one
 * to be executed.
 */

/******************* MAST_QUEUE(), CAST_QUEUE() **************************/

/* 
 * this is the basic ast-stack queueing routine; it has variants for
 * the various kinds of ast.
 */

static int asts_are_ready = 0;  /* 
                                 * flag for the ast function ast_check()
                                 * being on the wait stack.
                                 */
                                
 
static void
gast_queue(time,type,cfa,arg)
normal time;
normal type;
normal *cfa;
char *arg;
{
    int i;
    int aindex = nast_q;
    
    /* 
     * find out whether the system knows about asts and inform it
     * if necessary
     */
     
    if( asts_are_ready == 0 ){
        push_callback(ast_check);
        asts_are_ready = 1;
    }        
    
    /* find where in the queue this ast goes */

    if(nast_q >= NAST_Q) erret("\nERROR: AST QUEUE FULL");
    /* 
     * note that we go all the way through the stack; if there are
     * several asts with the same time, the ones queued earlier will
     * be delivered earlier
     */
    for(i=nast_q-1;i>= 0;i--){
        if(time >= ast_stack[i].ast_time){
            aindex = i;
        }
    }

    /* move higher entries up if appropriate */
    for(i=nast_q-1;i>=aindex;i--){
        ast_stack[i+1] = ast_stack[i];
    }

    /* insert this one */
    ast_stack[aindex].ast_time = time;
    ast_stack[aindex].ast_type = type;
    ast_stack[aindex].ast_cfa  = cfa;
    ast_stack[aindex].ast_arg  = arg;

    /* and increment the counter and make sure the next time is set properly */
    nextast_time = ast_stack[nast_q].ast_time;
    nast_q++;
}

/* C version */
void cast_queue(time,faddr,arg)
normal time;
void (*faddr)();
char *arg;
{
    gast_queue(time,1,(normal *)faddr,arg);
}

/* Mirella version */
void mast_queue(time,cfa,arg)
normal time;
normal *cfa;
char *arg;
{
    gast_queue(time,0,cfa,arg);
}

/***************** AST_DELIVER(), AST_CHECK() *************************/

/*
 * This routine delivers the next ast and pops the ast_stack
 */
 
void ast_deliver()
{
    struct ast_entry_t *sptr;
    normal type;

    if(nast_q == 0) erret("\nAST stack empty");
    nast_q--;
    sptr = ast_stack + nast_q;
    type = ast_stack[nast_q].ast_type;
    if(type == 0){
        push(sptr->ast_arg);
        cexec(sptr->ast_cfa);
    }else if(type == 1){
        (*(CALLBACK_FUNC_PTR)(sptr->ast_cfa))(sptr->ast_arg);
    }
    ast_set = 1;
}

/* 
 * This routine checks the time of the next ast and delivers it if
 * appropriate
 */
 
int
ast_check()
{
   if(nast_q && ast_enflg && gtime() >= nextast_time) ast_deliver();
   return 0;
}

/******************** AST STATUS ROUTINES() *****************************/

/* 
 * returns 1 if ast has been delivered since last ast_clr() 
 */
 
int
ast_flg()
{
   return ast_set;
}

/*
 * clears ast flag AND ast termio flag
 */
 
void ast_clr()
{
    ast_set = 0;
}

/*
 * returns 1 if asts are enabled, 0 otherwise
 */
 
int
ast_is_enabled()
{
   return ast_enflg;
}

/*
 * enables asts if arg is nonzero, disables if zero
 */
 
void ast_enable(flg)
int flg;
{
    ast_enflg = (flg ? 1 : 0);
}

/****************** AST_PREAMBLE(), _POSTAMBLE() ***********************/
/* this pair should be used to head and end all ASTs which do terminal
 * i/o; the rules are that any such ast expects to be in character mode
 * at the beginning (after the preamble), and leaves the system in character 
 * mode at the end (before the postamble). The editor detects the ast_ioflg
 * and takes appropriate action.
 */
 
static int oldgmode=0;

void
ast_preamble()
{
    oldgmode = grafmode;
    gmode(0);
    emit('\n');
    need_refresh();
}

void ast_postamble()
{
    emit('\n');    
    gmode(oldgmode);
}

#endif
