/*VERSION 88/08/31: Mirella 5.00                                     */

/*********************** MIRELLIN.C **********************************/

/* this file contains the Mirella inner interpreter. Be careful.     */
/* recent history:
    88/05/20,21: Added primitives for local variables; still tentative.
    88/08/31:    Added 'roll', as adv. in manual (and required by F83!).
    03/12/10:    Fixed abort so to zero stack contents as well as pointers
*/    

#ifdef VMS
#include mirkern
#include mprims
#include dotokens
#include mireldec
#else
#include "mirkern.h"
#include "mprims.h"
#include "dotokens.h"
#include "mireldec.h"
#endif

#define k_binop(operator)  (tos = *sp++ operator tos)
#define k_unop(operator)   (tos = operator tos)
#define k_bincmp(operator) tos = ((*sp++ operator tos)?-1:0)
#define k_uncmp(operator)  tos = ((tos operator 0)?-1:0)
#define k_push(xx) *(--sp) = tos; tos = (normal)(xx)
#define k_pop      tos; tos = *sp++
#define k_comma    *dp_4th++ = k_pop
#define k_fbinop(operator)     (ftos = *fsp++ operator ftos)
#define k_funop(operator)  (ftos = operator ftos)
#define k_fbincmp(op) *(--sp)=tos; tos = ((*fsp++ op ftos)?-1:0);	\
  ftos = *fsp++;
#define k_funcmp(op) *(--sp)=tos; tos = ((ftos op 0.)?-1:0); ftos = *fsp++;
#define k_fpush(xx) *(--fsp) = ftos; ftos = (float)(xx)
#define k_fpop     ftos; ftos = *fsp++
#define k_fcomma   *dp_4th++ = *(normal *)(&ftos) ; ftos = *fsp++
#define k_branch   ip += *(normal *)ip
#define k_next     continue
#define k_fwd_mrk  k_push ( dp_4th ); *dp_4th++ = 1;
#define k_fwd_rslv {							\
   start = (normal *)k_pop; *start = dp_4th - start;			\
}
#define k_bck_mrk  k_push ( dp_4th );
#define k_bck_rslv {							\
   start=(normal *)k_pop; scr=start - dp_4th; *dp_4th++ = scr;		\
}
#define k_create(cf)   qt_create(canon(blword()), (cf));
#define k_lcreate(cf)  l_create(canon(blword()), (cf));

int
in_intp(ip)
register token_t *ip;
{
    register normal     *sp = xsp_4th;
    register token_t    *token;
    register normal     tos  = *sp++;
    register token_t    **rp;            /* = xrp_4th; in old impl */
             float      *fsp = xfsp_4th;
             float      ftos = *fsp++;
    register normal     scr;
    register char       *ascr;
    register char       *ascr1;
             float      fscr;
             long       lscr;
             normal     scr2;
             normal     *start;
             normal     scr3;

    /* now return stack is an automatic array */
             token_t    *irtn_stk[RSSIZE];   

  /* init return stack pointer at each entry into the inner interpreter */
  rp = &irtn_stk[RSSIZE - sizeof(normal)];    

  while(1) {
    token = *(token_t **)ip++;

doprim:

    switch ((token_t)token) {
    case 0:
        error("Tried to execute a null token\n");
        goto abort;
    case NOT:    k_unop (~);     continue;
    case AND:    k_binop (&);    continue;
    case OR:     k_binop (|);    continue;
    case XOR:    k_binop (^);    continue;
    case PLUS:   k_binop (+);    continue;
    case MINUS:  k_binop (-);    continue;
    case TIMES:  k_binop (*);    continue;
    case DIVIDE: k_binop (/);    continue;
    case MOD:    k_binop (%);    continue;
    case SHIFT:  if (tos < 0) { tos = -tos; k_binop(>>);} else k_binop(<<); continue;
    case DUP:    *(--sp) = tos;  continue;
    case DROP:   tos = *sp++;  continue;
    case SWAP:   scr = *sp;    *sp = tos;      tos = scr;      continue;
    case OVER:   k_push (sp[1]); continue;
    case ROT:    scr = tos; tos = sp[1]; sp[1] = *sp; *sp = scr; continue;
    case PICK:   tos = *(sp + tos); continue;
    case Q_DUP:  if (tos) { *(--sp) = tos; }    continue;
    case TO_R:   *(normal *)--rp = k_pop;     continue;
    case R_FROM:       k_push ( *(normal *)rp++ );    continue;
    case R_FETCH:      k_push ( *(normal *)rp );      continue;

    /* We don't have to account for the tos in a register, because */
    /* k_push has already pushed tos onto the stack before */
    /* ps_top - sp  is computed */
    case DEPTH:        k_push ( ps_top - sp) ; continue;
    case LESS:         k_bincmp (<);     continue;
    case EQUAL:        k_bincmp (==);    continue;
    case GREATER:      k_bincmp (>);     continue;
    case Z_LESS:    k_uncmp (<);      continue;
    case Z_EQUAL:   k_uncmp (==);     continue;
    case Z_GREAT:   k_uncmp (>);      continue;
    case U_LESS:    tos = ((unsigned)*(sp++) < (unsigned)tos) ? -1:0; continue;
    case ONE_PLUS:     tos++;        continue;
    case TWO_PLUS:     tos += 2;     continue;
    case TWO_MINUS:    tos -= 2;     continue;
    case U_M_TIMES:    tos = (unsigned) * sp++ * (unsigned) tos;    continue;
#ifndef LOGCSHIFT
    case TWO_DIVIDE:   tos >>= 1;    continue;     /* Should be signed */
#else
    case TWO_DIVIDE:   tos /= 2;     continue;
#endif
    case IMAX: scr = *sp++; if (tos < scr) { tos = scr; }    continue;
    case IMIN: scr = *sp++; if (tos > scr) { tos = scr; }    continue;
    case ABS:           if (tos < 0)   { tos = -tos; }   continue;
    case NEGATE:  k_unop (-);      continue;
    case FETCH:   tos = *(normal *)tos;  continue;
    case C_FETCH: tos = *(char *)tos;  continue;
    case W_FETCH: tos = *(short *)tos; continue; /* this used to be u_short */
    case STORE:      *(normal *)  tos = *sp++; tos = *sp++;    continue;
    case C_STORE:    *(char *)  tos = *sp++; tos = *sp++;    continue;
    case W_STORE:    *(short *)   tos = *sp++; tos = *sp++;    continue;
    case PLUS_STORE: *(normal *) tos += *sp++; tos = *sp++;    continue;
    case IFETCH:     tos = *(int *)tos;  continue;
    case ISTORE:     *(int *)tos = *sp++; tos = *sp++;    continue;

    case CMOVE:
        ascr = (char *)*sp++;
        ascr1 = (char *)*sp++;
        cmove(ascr1, ascr, tos);
        tos = *sp++;
        continue;

    case CMOVE_UP:
        ascr = (char *)*sp++;
        ascr1 = (char *)*sp++;
        cmove_up(ascr1, ascr, tos);
        tos = *sp++;
        continue;

    case FILL: 
        scr = *sp++;
        ascr = (char *)*sp++;
        ffill(ascr, scr, tos);
        tos = *sp++;
        continue;

    case COUNT: 
        *(--sp) = (normal)(tos + 1);
        ascr = (char *) tos;
        tos = (normal)(*ascr);
        continue;

    case PAREN:      word(')'); continue;
    case BACKSLASH: if (V_DELIMITER != '\n') { word('\n'); }     continue;
    case I:         k_push ( ((normal *)rp)[0] + ((normal *)rp)[1] ); continue;
    case J:         k_push ( ((normal *)rp)[3] + ((normal *)rp)[4] ); continue;
    case BRANCH:    k_branch;      continue;

    case Q_BRANCH:
        if (tos == 0) { k_branch; } else { ip++; }
        tos = *sp++;
        continue;

    case UNNEST:
    case EXIT:         ip = *rp++;     continue;

    case EXECUTE:      token = (token_t *)k_pop;
execute:
        if ( *token < MAXPRIM )
            token = (token_t *)*token;
        goto doprim;

    case KEY:          k_push ( key() ); continue;
    case EMIT:         emit (tos);     tos = *sp++;    continue;
    case CR:           emit ('\n');    continue;
    
    case DOT_PAREN:
        ascr = word(')');
        scr = *ascr++;
        while(scr-- > 0) emit(*ascr++);
        continue;

    case ALLOT: 
        dp_4th = (normal *)((char *)dp_4th + tos);
        tos = *sp++;
        if (dp_4th > dict_end)
            error( "Out of dictionary space\n" );
        continue;

    case FIND:  k_push ( find((char **)sp) );    continue;
    case VFIND: tos = vfind((char **)sp, (vocab_t *)tos); continue;

    case ABORT:
abort:
        /* initialize the stack pointers */
        ssinit();
        sp = ps_top + 1;  fsp = fs_top + 1;  
        tos = 0; ftos = 0.;
        xtsp_4th = ts_top ; *xtsp_4th = 0;
    /* fall through */
    case QUIT:         
        /*
         * Restore the local copies of the virtual machine registers to the
         * external copies and exit to the outer interpreter.
         */
        *(--sp) = tos;   xsp_4th = sp;    /* xrp_4th = rp;    */
        *(--fsp) = ftos; xfsp_4th = fsp;  return(1);

    case FINISHED:
        /*
         * Restore the local copies of the virtual machine registers to the
         * external copies and exit to the outer interpreter.
         */
        *(--sp) = tos;    xsp_4th = sp;   /*  xrp_4th = rp;   */
        *(--fsp) = ftos;  xfsp_4th = fsp;  return(0);

    case HERE:          k_push ( dp_4th );         continue;
    case TIB:           k_push ( &tibbuf_4th[0] );     continue;
    case DOT:           ndot( tos );  tos = *sp++; continue;
    case U_DOT:         udot( tos ); tos = *sp++; continue;
    case WORD:          tos = (normal) word(tos); continue;
    case COMMA:         k_comma;           continue;
    case DOT_QUOTE:     compile(P_DOT_QT); comma_string( word('"') ); continue;

    case COLON:     k_create (DOCOLON);   hide();   
            V_STATE = (normal)COMPILING;  V_NLVAR = 0;    continue;

    case SEMICOLON: compile(UNNEST); reveal(); 
            V_STATE = (normal)INTERPRETING;   continue;

    case ABT_QTE:       compile(P_ABT_QT); comma_string(word('"')); continue;
    case COMPILE:       compile (P_COMPILE);     continue;
    case CONSTANT:      k_create (DOCON); k_comma;      continue;
    case VARIABLE:      k_create (DOVAR); *dp_4th++ = 0;  continue;
    case CREATE:        k_create (DOVAR); continue;
    case QT_CREATE:     ascr = (char *)k_pop; qt_create (ascr, DOVAR); continue;
    case USER_SIZE:     k_push (MAXUSER); continue;
    case USER:          k_create (DOUSER); k_comma; continue;
    case IMMEDIATE:     makeimmediate ();       continue;
    case DOES:          compile(P_DOES);    continue;
    case IF:            compile(Q_BRANCH); k_fwd_mrk;    continue;

    case ELSE:  compile(BRANCH);
        k_fwd_mrk;
        scr = *sp;   *sp = tos;   tos = scr;  /* swap */
        k_fwd_rslv;
    continue;

    case THEN:
    case FWD_RSLV:  k_fwd_rslv;   continue;
    case FWD_MRK:   k_fwd_mrk;        continue;
    case LITERAL:   compile (P_LIT);    k_comma;  continue;
    case DO:        compile(P_DO);    k_fwd_mrk;     k_bck_mrk;  continue;
    case Q_DO:      compile(P_Q_DO);    k_fwd_mrk;     k_bck_mrk;    continue;
    case LOOP:      compile(P_LOOP);  k_bck_rslv; k_fwd_rslv; continue;
    case PLUS_LOOP: compile(P_PL_LOOP); k_bck_rslv; k_fwd_rslv;  continue;

    case Q_LEAVE: scr = k_pop;  if (!scr) { continue; }
             /* else fall through */
    case LEAVE:     rp += 2;    /* Throw away the loop indices */
            ip = *(token_t **)rp++; /* Go to location after (do */
            k_branch;       /* Get the offset there */
            continue;

    case BEGIN:
    case BCK_MRK:      k_bck_mrk;      continue;
    case BCK_RSLV: k_bck_rslv;         continue;
    case WHILE:   compile(Q_BRANCH); k_fwd_mrk;      continue;

    case REPEAT:  scr = *sp; *sp = tos; tos = scr;   /* swap */
        compile(BRANCH); k_bck_rslv; /* Branch back to begin */
        k_fwd_rslv;            /* Complete while */
        continue;

    case UNTIL:       compile(Q_BRANCH); k_bck_rslv;  continue;

    case M_TICK:  { char *temp;  temp = canon(blword());
            if ( find(&temp) ) { k_push ( temp );}
            else           { where(); goto abort;}
        }
        continue;

    case BR_TICK:     compile (P_TICK);
    case BR_COMPILE: scompile(canon(blword()));     continue;

    case P_Q_DO:
        scr = *sp++;
        if ( scr == tos ) { tos = *sp++; k_branch; continue; }
        *(token_t **)--rp = ip++;       /* Address of offset to end */
        *(normal *)--rp =   scr ;
        *(normal *)--rp = tos - scr ;
        tos = *sp++;
        continue;

    case P_DO: 
        scr = *sp++;
        *(token_t **)--rp = ip++;       /* Address of offset to end */
        *(normal *)--rp =   scr ;
        *(normal *)--rp = tos - scr ;
        tos = *sp++;
        continue;

    case P_LOOP: 
        if (++*(normal *)rp < 0) {
            k_branch;
            continue;
        }
        /* Loop terminates; clean up return stack and skip k_branch offset */
        rp= (token_t **)((char *)rp + 2 * sizeof(normal) + sizeof(token_t *));
        ++ip; continue;

    case P_PL_LOOP: 
        /* The test should really be for carry set */
        if ((*(normal *)rp += tos) < 0) {
            tos = *sp++; k_branch; continue;
        }
        /* Loop terminates; clean up return stack and skip k_branch offset */
        tos = *sp++;
        rp= (token_t **)((char *)rp + 2 * sizeof(normal) + sizeof(token_t *));
        ++ip; continue;

/*
 * This is subtle.  The rule is that whenever a token appears on the stack,
 * it is in the form of the absolute address of the code field of its header.
 * When a token is stored in a definition, it is either the switch index
 * for a primitive or the code-field-address.  (' has to decide which it
 * is, and if it is the switch index, convert it to the appropriate cfa.
 * A table of cfa's indexed by the switch index appears as the first thing
 * in the dictionary, at origin.  This table is constructed by the code
 * that initializes the dictionary.
 */
    case P_TICK:    token = *(token_t **)ip++;
            if ( (token_t)token < MAXPRIM && (token_t)token > 0 )
                token = (token_t *)origin[(normal)token];
            k_push ( token ); continue;

    case P_LIT:    k_push ( *ip++ );    continue;

    case P_DOES:
        *name_from(&((dic_en_t *)V_LAST) -> name) = (token_t)ip;
        ip = *rp++;       continue;

    case P_DOT_QT:
        print_string((char *)ip);
        ip = (token_t *)aligned( (char *)ip + *(char *)ip + 1 );
        continue;

    case P_ABT_QT:
        if (tos != 0) {
            print_string((char *)ip);
            tos = *sp++;
            goto abort;
        }
        tos = *sp++;
        ip = (token_t *)aligned( (char *)ip + *(char *)ip + 1 );
        continue;

    case P_COMPILE:       compile(*ip++);    continue;
    case BYE:        m_bye(1); continue;
    case LOSE: error("Undefined word encountered\n");  goto abort;

    /* Don't need to modify sp to account for the top of stack being */
    /* in a register because k_push has already put tos on the stack */
    /* before the argument ( sp ) is evaluated */

    case SPFETCH:    k_push ( sp );         continue;
    case SPSTORE:    scr = k_pop;  sp = (normal *)scr + 1;  continue;
    case RPFETCH:    k_push ( rp );         continue;
    case RPSTORE:    rp = (token_t **)k_pop;    continue;
    case UPFETCH:    k_push ( up_4th );         continue;
    case UPSTORE:    up_4th = (normal *)k_pop;      continue;
    case TICK_WORD:  k_push ( &wordbuf[0] );    continue;
    case DIV_MOD: scr = *sp; *sp = scr%tos; tos = scr/tos; continue;
    case DIGIT:      if ( (scr = digit ( tos, *sp )) > 0 ) {
            *sp = scr; tos = -1;
        } else
        tos = 0;
        continue;

    case NMBR_Q: tos = number ( (char *) tos, --sp) ? -1 : 0 ; continue;
    case HIDE:       hide();              continue;
    case REVEAL:     reveal();            continue;
    case QT_LOAD:    ascr = (char *)k_pop; load(ascr);  continue;
    case LOAD:       load(blword());  continue;
    case SEMI_S:     semi_s();  continue;
    case CANON:      canon((char *)tos);    continue; 

/* all tokens above this point are Bradley originals. below, be careful */ 

/*  floating: */
    case FPLUS:     k_fbinop (+);    continue;
    case FMINUS:    k_fbinop (-);    continue;
    case FTIMES:    k_fbinop (*);    continue;
    case FDIVIDE:   k_fbinop (/);    continue;
    case FDUP:   *(--fsp) = ftos;  continue;
    case FDROP:  ftos = *fsp++;  continue;
    case FSWAP:  fscr = *fsp; *fsp = ftos; ftos = fscr; continue;
    case FOVER:  k_fpush (fsp[1]); continue;
    case FROT:   fscr = ftos; ftos = fsp[1]; fsp[1] = *fsp; *fsp = fscr; continue;
    case FPICK:  *(--fsp) = ftos; ftos = fsp[tos]; tos = *sp++; continue;
    case FIX:    k_push(ftos) ; ftos = *fsp++; continue;
    case FLOAT:  k_fpush(tos) ; tos = *sp++ ; continue ;
    case FLESS:     k_fbincmp (<);     continue;
    case FEQUAL:    k_fbincmp (==);    continue;
    case FGREATER:  k_fbincmp (>);     continue;
    case FABS:      if(ftos < 0.) ftos = -ftos ; continue ;
    case FNEGATE:   ftos = -ftos ; continue ;
    case FMIN:      fscr = *fsp++; if(ftos > fscr) ftos = fscr ; continue ;
    case FMAX:      fscr = *fsp++; if(ftos < fscr) ftos = fscr ; continue ;
    case FCOMMA:    k_fcomma ; continue ;
    case FPLUSSTORE: *(float *)tos += ftos; ftos = *fsp++; tos = *sp++;
	continue; 
    case FSTORE:    *(float *)tos  = ftos; ftos = *fsp++; tos = *sp++;
	continue;
    case FFETCH:    *--fsp = ftos; ftos = *(float *)tos; tos = *sp++ ;
	continue;

/* 32-bit utils */
    case FOUR_PLUS:     tos += sizeof(normal); continue;
    case FOUR_MINUS:    tos -= sizeof(normal); continue;
    case FOUR_TIMES:    tos <<= (sizeof(normal) == 4 ? 2 : 4); continue;
#ifndef LOGCSHIFT
    case FOUR_DIVIDE:   tos >>= (sizeof(normal) == 4 ? 2 : 4); continue;
#else
    case FOUR_DIVIDE:   tos /= sizeof(normal);  continue;
#endif
    case TWO_TIMES:     tos <<= 1; continue;
    case STAR_SLASH:    scr = k_pop; lscr = (long)tos * (long)(*sp++);
                        tos = lscr/scr ; continue;

/* temporary stack and interstack */

    case FTOP: fscr = k_fpop; k_push(*(normal *)(&fscr)); continue;
    case PTOF: scr2 = k_pop; k_fpush(*(float *)(&scr2)); continue;
    case FTOT: fscr = k_fpop; *(--xtsp_4th) = *(normal *)(&fscr); continue;
    case PTOT: *(--xtsp_4th) = k_pop; continue;
    case TTOF: k_fpush(*(float *)xtsp_4th); xtsp_4th++; continue;
    case TTOP: k_push(*xtsp_4th); xtsp_4th++; continue;

/*   odds and ends I thought of too late */
    case FCONSTANT: k_create (DOFCON);    k_fcomma;     continue;
    case FDOT:      fscr = k_fpop; fdot(fscr); continue;  
    case FSPFETCH:  k_push((fsp-1)); continue;
    case TSPFETCH:  k_push(xtsp_4th); continue;
    case SPUSH:     tos = (normal)spush((char *)tos); continue;
    case SDOWN:     k_push(sdown()); continue;
    case SUP:       k_push(sup());   continue ;
    case SSP_STORE: xssp_4th = (Void *)k_pop; continue ;
    case SSP_FETCH: k_push(xssp_4th); continue ;
    case H_ADDR:    k_push( (normal)&dp_4th ) ; continue;
    case P_FLIT:    k_fpush ( *(float *)ip++ ); continue ;
    case W_PLUS_ST: *(short *)tos += *sp++; tos = *sp++; continue;
    case DFETCH:    tos = *(normal*)(*(normal *)tos); continue; /* @@ */
    case PUTC:      mfputc(tos&255); tos = *sp++; continue;
    case GETC:      k_push( mfgetc()&255 ); continue;
/* local variables */
    case LVAR:      k_lcreate(DOVAR); *dp_4th++ = 0; continue;
    case P_LVAR:    ip += 2;
	continue;			/* skip the P_LVAR, the DOVAR, and the 
					   parameter field */    
    case QT_LCREATE: ascr = (char *)k_pop; l_create (ascr, DOVAR); continue;
    /* two primitives which should have been in from the beginning */
    case ROLL:   scr=tos; tos = *(sp+tos); 
                    while(scr--) *(sp+scr+1) = *(sp+scr); sp++; continue;
    case FROLL:  scr=k_pop; scr--; fscr = ftos; ftos = *(fsp+scr);
                    while(scr--) *(fsp+scr+1) = *(fsp+scr); *fsp = fscr; continue;

    /* this is the end of the primitives list; if the token is not a primitive,
    it is an address pointing to the cfa of a forth word; the contents of that
    address is either one of the many DO-tokens defined below, which head
    one of the variety of types of Mirella words, or is itself an address 
    if the word pointed to was defined by a k_create..does> pair. */
    
    default: 

    switch (*token++) {
       case DOCOLON: *(--rp) = ip; ip = token; continue;
       case DOCON:   k_push ( *(normal *) (token) );  continue;
       case DOVAR:   k_push ( token ); continue;
       case DOUSER:  k_push ( *(normal *) token + (normal) up_4th ); continue;
       case DODEFER: token = *(token_t **)((u_char *)up_4th +*(normal *)token);
             goto execute;
       case DOVOC:   V_CONTEXT = (normal)token;   continue;
    /*  for DOCODE and DOCODE_[FP]PTR
     * Restore the local copies of the virtual stack pointers to the
     * external copies so functions can manipulate the stacks, then
     * set up internal copies again. Functions must not touch return stack.
     */
       case DOCODE:
            *(--sp) = tos; xsp_4th = sp; *(--fsp) = ftos; xfsp_4th = fsp;  
            (*(void (*)())(*token))(); 
            sp = xsp_4th; tos = *sp++; fsp = xfsp_4th; ftos = *fsp++; continue;
       case DOCODE_PPTR:
            *(--sp) = tos; xsp_4th = sp; *(--fsp) = ftos; xfsp_4th = fsp;  
            (**(void (**)())(*token))(); 
            sp = xsp_4th; tos = *sp++; fsp = xfsp_4th; ftos = *fsp++; continue;
       case DOCODE_FPTR:
            *(--sp) = tos; xsp_4th = sp; *(--fsp) = ftos; xfsp_4th = fsp;  
            (*(void (*)())(*((normal *)token + 2))) (*(void (**)())(*token)); 
            sp = xsp_4th; tos = *sp++; fsp = xfsp_4th; ftos = *fsp++; continue;

       case DOFCON:  k_fpush ( *(float *) (token) ); continue;

    /* following are for dealing with C constants; normals, floats,
    and strings, which are copied onto string stack. in all these cases,
    token is, as always, the address of the k_next entry (the pfa)
    in the dictionary; in initializing 
    the C interface, the address of a C quantity  (*token) is placed in 
    the dictionary  following one of these (DOCCON, etc). DOCCON pushes 
    the value at the stored address. DOFCCON does likewise, but interprets
    the pointer as one to float; DOSCCON "pushes" a string. */
    
       case DOCCON:  k_push ( *(normal *)(*(normal *)(token)) ); continue ;
       case DOFCCON: k_fpush ( *(float *)(*(normal *)(token)) ); continue ;
       case DOSCCON: k_push (*(normal *)(token)); continue ;
       /* new cstring code 88/03/12; just pushes C address of string */
          
       case DOCVAR:  k_push ( *(normal *) (token) );  continue;
        /* DOCVAR does the same thing as DOCON, but for the sake of the
        decompiler we use a separate token */   
        
    /* The following code is for the handling of arrays and matrices,
    in their several varieties. Generally the dictionary entry
    for an array consists of the array token DONAR...in the cfa,
    followed by the size of the array (int), followed by the array
    entries; for a matrix, the cfa is followed by the number of rows
    and then the number of columns and then the array of pointers and
    then the entries. In the Carray implementations, of
    course, the array storage is separate from the dictionary entry,
    and the pfa +1 or 2 contains a pointer to a pointer to the data; This 
    supposes that the C entity is a pointer which by allocation or
    assignment points to the base of the matrix pointer array. in the memarray
    implementations, the indirection is the same depth, and the
    pfa + 1 or 2 contains a pointer to the memalloc entry which
    contains a pointer to the data.  */
    
            /* Mirella dictionary arrays */
        case DONAR: scr = k_pop; k_push((normal *)token + 1 + scr); continue ;
        case DOSAR: scr = k_pop; k_push((short int *)token + 2 + scr); continue ;
        case DOCAR: scr = k_pop; k_push((char *)token + 4 + scr); continue ;     
            /* C arrays */
        case DONCAR: scr = k_pop; k_push( *((normal**)token + 1) + scr); continue ;
        case DOSCAR: scr = k_pop; k_push( *((short int**)token + 1) + scr);
	 continue;
        case DOCCAR: scr = k_pop; k_push( *((char **)token + 1) + scr); continue;
            /* allocated memory arrays */
        case DONMAR: scr = k_pop; k_push(*(*((normal ***)token + 1)) + scr); continue;
        case DOSMAR: scr = k_pop; k_push(*(*((short int ***)token+1)) + scr); continue; 
        case DOCMAR: scr = k_pop; k_push(*(*((char ***)token + 1)) + scr); continue;
            /* arrays passed as minstalled pointers */
        case DONCPAR: scr = k_pop; 
            k_push(*(*(*((normal ****)token + 1))) + scr); continue; 
        case DOSCPAR: scr = k_pop; 
            k_push(*(*(*((short int ****)token + 1))) + scr); continue; 
        case DOCCPAR: scr = k_pop; 
            k_push(*(*(*((char ****)token + 1))) + scr); continue; 
            /* Mirella dictionary matrices; these all do the same thing, but for
            bookeeping and later range checking, I will use separate tokens */
        case DONMAT: scr = k_pop; scr2 = k_pop;
            k_push(*((normal **)token + 2 + scr2) + scr); continue ;
        case DOSMAT: scr = k_pop; scr2 = k_pop;
            k_push(*((short int **)token + 2 + scr2) + scr); continue;
        case DOCMAT: scr = k_pop; scr2 = k_pop;
            k_push(*((u_char **)token + 2 + scr2) + scr); continue;
            /* C matrices: ditto */
        case DONCMAT: scr = k_pop; scr2 = k_pop;
            /* this generates the same code as DONMMAT, and supposes that
            the Minstalled C entity is a POINTER, whose ADDRESS is therefore
            passed to the Clinks; it points to the base of the matrix
            pointer array */
            k_push(*(*(*((normal ****)token + 2)) + scr2) + scr); continue;
                /* NOT
                k_push(*(*((normal ***)token + 2) + scr2) + scr); continue;
                    as would be the case if the C entity were the array 
                    address itself.
                */
        case DOSCMAT: scr = k_pop; scr2 = k_pop;
            k_push(*(*(*((short int ****)token + 2)) + scr2) + scr); continue;
        case DOCCMAT: scr = k_pop; scr2 = k_pop;
            k_push(*(*(*((u_char ****)token + 2)) + scr2) + scr); continue;
            /* see comments above */
            
            /* allocated memory matrices-- ditto */
        case DONMMAT: scr = k_pop; scr2 = k_pop;
            k_push(*(*(*((normal ****)token + 2)) + scr2) + scr); continue;
        case DOSMMAT: scr = k_pop; scr2 = k_pop;
            k_push(*(*(*((short int ****)token + 2)) + scr2) + scr); continue;
        case DOCMMAT: scr = k_pop; scr2 = k_pop;
            k_push(*(*(*((u_char ****)token + 2)) + scr2) + scr); continue;
            
            /* structures in forth */
        case DOOFFSET:
            tos += *(normal *)token ;  continue ;
        case DOSTRUCAR:
            scr = k_pop;   /* index */
            k_push((char *)((normal *)token + 2) 
                + (*((normal *)token + 1))*scr) ; 
            continue ;
            /* pfa is number, k_next is size, k_next data */
        case DOSTRUCCAR:
            scr = k_pop;   /* index */
            k_push(*(char **)(*((normal ***)token + 2)) 
                + (*((normal *)token + 1))*scr);
            continue ;
            /* pfa is number, k_next size, k_next ptr to ptr to data */
        case DOSTRUCMAR:      /* structure arrays installed as pointers */
            scr = k_pop;
            k_push(*(char **)(*(char ***)(*((normal ****)token + 2)))
                + (*((normal *)token + 1))*scr);
            continue;                
            /* pfa is number, k_next size, k_next ptr to ptr to data */
        /* 3D matrices in dictionary -- extension of 2D ones above */
        case DON3MAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;            
            k_push(*(*((normal ***)token + 3 + scr3) + scr2) + scr);
            continue;
        case DOS3MAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*((short int ***)token + 3 + scr3) + scr2) + scr);
            continue;
        case DOC3MAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*((char ***)token + 3 + scr3) + scr2) + scr);
            continue;
        /* 3D matrices in C space */
        case DON3CMAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            /* this generates the same code as DON3MMAT, and supposes that
            the Minstalled C entity is a POINTER, whose ADDRESS is therefore
            passed to the Clinks; it points to the base of the matrix
            pointer array */
            k_push(*(*(*(*((normal *****)token + 3)) + scr3) + scr2) + scr);
            continue;
        case DOS3CMAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*(*(*((short int *****)token + 3)) + scr3) + scr2) + scr);
            continue;
        case DOC3CMAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*(*(*((char *****)token + 3)) + scr3) + scr2) + scr);
            continue;
        /* 3D matrices in the heap */
        case DON3MMAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*(*(*((normal *****)token + 3)) + scr3) + scr2) + scr);
            continue;
        case DOS3MMAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*(*(*((short int *****)token + 3)) + scr3) + scr2) + scr);
            continue;
        case DOC3MMAT: scr = k_pop; scr2 = k_pop; scr3 = k_pop;
            k_push(*(*(*(*((char *****)token + 3)) + scr3) + scr2) + scr);
            continue;
        /* let's make these work for now; do 4-d ones later */
        default:        /* DOES> word */
            /* Push parameter field address */
            k_push ( token );
            *(--rp) = ip;
            ip = (token_t *)*(--token);
            continue;
    }       /* End of default switch */
    }       /* End of top level switch */
  }         /* End of while (1) */
}

