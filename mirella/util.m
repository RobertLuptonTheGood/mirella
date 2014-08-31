\ VERSION 88/08/31  Mirella 5.00
\ this file contains the basic forth utilities; must be loaded first
\ 040709 added octal. `ff' is a forth word (!?) which takes a stack entry,
\ so the byte/word stuff in ~lines 100 will not work. Where is it?

: n+ SizeofN + ;
: n- SizeofN - ;
: n* SizeofN * ;
: n/ SizeofN / ;

: nceil ( i.i -- ni.i where ni is the nearest multiple of SizeofN >= i )
   dup n/ swap SizeofN mod if 1+ then n* ;

: decimal 10 base ! ;
: hex 16 base ! ;
: octal 8 base ! ;
: binary 2 base ! ;
: forth83 ;
: [ 0 state ! ; immediate
: ] 1 state ! ;
: >body n+ ;
: cword word 1+ ;       \ address of cstring in word buffer

decimal

32 constant bl
-1 constant true
0 constant false
SizeofN constant /n                     \ what's the mnemonic for /n ?
SizeofN constant /token
SizeofN constant #align

hex create estr 69646504 , 00000074 ,
\ edit string
: rawedit estr spush system ;
decimal

: 1- 1 - ;
: 2dup over over ;
: 2drop drop drop ;
: 2@ ( addr -- n1 n2 )   dup /n + @ swap @  ;
: 2! ( n1 n2 addr -- )   swap over ! /n + ! ;
: -rot rot rot ;
: >= ( n1 n2 -- f )   < 0=  ;
: u<= ( u1 u2 -- f ) 2dup u< -rot = or ;
: u>  ( u1 u2 -- f ) u<= 0= ;
: u>= ( u1 u2 -- f ) u<  0= ;
: <>  ( n1 n2 -- f ) = 0= ;
: (s [compile] ( ; immediate
: definitions context @ current ! ;
: pad here 256 + ; \ changed from 100 121227
: space  bl emit ;
: spaces 0 ?do space loop ;
: 0cr 13 emit ;
: printf ( args .... format -- ) sprintf ct ;

: aligned ( addr -- aligned-addr )
  [ #align 1-     ] literal +
  [ #align negate ] literal and
;
: align ( -- )    here here aligned swap - allot  ;

: erase ( addr count -- ) 0 fill ;
: blank ( addr count -- ) bl fill ;
: off   ( addr -- ) 0 swap ! ;
: <= ( n1 n2 -- f ) > 0= ;
: between (s n min max -- f )
   >r over <= swap r> <= and ;

: within (s n1 min max+1 -- f )
   1- between
;
: na1+ n+ ;
: na+ n* + ;
: na- n* - ;
: ta+ n* + ;

\ in token@, if contents of addr is a token primitive, pushes the corresp.
\       cfa; else just returns the contents of addr (does an @)
: token@ ( addr -- cfa )
  @  ( cfa | prim )
  dup 1 300 between  if  origin swap na+ @  then
;

\ in token! if cfa is a primitive, stores the token at addr, else stores
\        the cfa at addr (is just !)
: token! ( cfa addr -- )
  over   ( 
  @ 1 300 between  ( cfa addr f )
  if swap @ swap  then
  !
;

: token, ( cfa --- )
    here  /token allot  token!  
;

\ Words to follow dictionary links
/n constant /link
: link@ @ ;
: link! ! ; 

: >name ( cfa -- nfa )
  begin  1- dup  c@  until      \ Skip over the pad characters
  begin  1- dup  c@  bl < until \ Look for length byte and hope word
                                \ is less than 32 char long !!
;
: n>link ( nfa -- lfa ) 1 - /link - ;  ( skip flag byte and link byte )
: >link  ( cfa -- lfa ) >name n>link ;
: name> ( nfa -- cfa )  count + aligned ;
: l>name ( lfa -- nfa ) na1+ 1+ ;
hex
: wbsplit ( w -- b.low b.high )
  dup ff and
  swap -8 shift  ff and
;
: bwjoin ( b.low b.high -- w )
  8 shift or
;
: lbsplit ( l -- b.low b.mlow b.mhigh b.high )
  dup ff and
  swap -8 shift  dup ff and
  swap -8 shift  dup ff and
  swap -8 shift
;
: lwsplit ( l -- w.low w.high )  dup 0ffff and swap -10 shift ;
: wljoin  ( w.low w.high -- l )  10 shift or ;
decimal
: << shift ;
: >> negate shift ;
: body> /n - ;
: name>str   ( nfa -- str ) ;
: name>flags ( nfa -- address-of-flags-byte  ) 1- ;
: immediate? ( cfa -- f )  >name name>flags c@ ;
: >user#  ( cfa -- user# )    >body @  ;
: >user @ up@ + ;
: 'user# ( --name  user# )
    '  ( cfa-of-user-variable )
    >user#
;
: .id   ( nfa -- ) name>str count type space ;
decimal
: word-type ( cfa -- word-type )
  @ dup 300 u<
  if drop -1 ( code word )  then
;
: ualloc ( size -- user-number )
  #user @  swap #user +!
;
: nuser ( -- ) ( Input stream: name )
  /n ualloc user
;

: c.id  ( cfa -- ) >name .id ;
: crash ( -- ) ( unitialized execution vector routine )
  r@ /token - token@         ( use the return stack to see who called us )
  dup ['] execute =
  if   'word count type space   else   c.id   then
  ." <--deferred word not initialized " abort
;

: defer ( -- )
  create
  305 here body> !   \ 305 is the primitive number for deferred words
  here /n ualloc ,
  >user ['] crash  swap !
;
defer defxx

: >is ( cfa -- data-address )
  dup word-type  ( cfa code-field-word )
  dup ['] #user word-type  = swap      ( user variable? )
  dup ['] defxx word-type  = swap      ( deferred? )
  dup ['] forth word-type  = swap      ( vocabulary )
  drop or or
  if  >body >user  else  >body then
;

\ XXX needs to be state smart
: (is ( cfa cfa-of-deferred-word -- )
  >is !
;
: is ( val -- ) ( Input stream: name )
  state @
  if   [compile] ['] compile (is
  else ' (is
  then
; immediate

: oldbye bye ;
warning @ 0 warning ! defer bye warning !
' oldbye is bye

\ Following definitions are implementation-independent

decimal
: bounds ( addr len -- endaddr startaddr )
  over + swap
;

\ see the dump words in mirella.m
\ : dump1 ( addr count )
\   cr
\   ?dup
\ if   bounds   do i @ u. /n +loop
\  else drop
\  then
\ ;
\ : .. ( addr -- addr' ) dup @ . na1+ ;
\ : _dump ( addr count -- )
\   begin
\      dup  while
\      over @ .
\      swap na1+ swap
\      1 -
\   repeat
\   2drop
\ ;

: do-defined  ( cfa [ -1 | 0 | 1 ] -- ?? )
  state @
  if   0> if   execute   ( if immediate )
       else    token,    ( if not immediate )
       then
  else
      drop execute
  then
;
: do-literal ( l -- ?? )
  state @ if compile (lit , then
;
: isprint ( n -- f ) ( is n a printable ascii character ? )
  bl 127 within
;
: ascii  (s -- n )
    bl word 1+ c@
    do-literal
; immediate

: control  (s -- n )
  bl word 1+ c@  bl 1- and
  do-literal
; immediate

: "copy ( from to --- ) over c@ 2+ cmove ;  \ forth strings 
: fst@ ssp@ ;      \ --- addr of fstr at toss, chng 88/03/12 
: st@ ssp@ 1+ ;    \ --- addr of cstring at toss, chng 88/03/12

: "  (s -- str-addr ) ( Input stream: string )
  ascii " word 1+ spush ;
: ""   (s -- str-addr ) ( Input stream: string )
  bl word 1+ spush ;       \ note that spush takes, pushes C address

: tuck ( n1 n2 -- n2 n1 n2 ) swap over ;
: nip  ( n1 n2 -- n2 ) swap drop ;

: move ( from to len -- )   \ nondestructive
  -rot  2dup u< if  rot cmove>   else  rot  cmove then ;

: place     (s str-addr len to -- )
   2dup c!
   2dup +  1+ 0 swap c!  ( null byte after string )
   1+ swap move
;
: ",   (s addr len -- )
  tuck here place ( len ) 2+ allot align   ( leave room for null )
;
: ,"   (s -- )    ascii " word count  ",  ;
: skipstr ( -- addr len )
  r> r> count     ( return-addr addr len )
  2dup + 1+ aligned  ( return-addr addr len new-ip )  ( acct for null )
  >r rot >r
;
: ("s  (s -- str-addr )   skipstr drop 1- ;
: c("s (s -- cstr-addr)    skipstr drop ;      \ 88/03/12
: ("   (s -- addr len )   skipstr ;

\ NB!!! this word is seriously broken. It crashes the definition
\ process for strings much over 100 chars long; fine for
\ fewer, but I cannot find any buffer which it might be
\ overflowing. FIXME!!!!! 130511, but evidence MUCH earlier
\ crashes both from keyboard and file, so nothing to do with
\ loading per se. Complains of trying to execute a null token,
\ and completely crashes forth.

: ["]  (s Compile-time: --str ) (s Run-time: -- cstr-addr )
  compile c("s    ,"   \ 88/03/12
;   immediate

: f["]  (s Compile-time: --str ) (s Run-time: -- fstr-addr )
  compile ("s    ,"   \ 88/03/12
;   immediate

: [""] (s Compile-time: --namex ) (s Run-time: -- cstr-addr )
  compile c("s  bl word count ",   \ 88/03/12
;   immediate

: f[""] (s Compile-time: --namex ) (s Run-time: -- fstr-addr )
  compile ("s  bl word count ",   \ 88/03/12
;   immediate

: ". ( str -- ) count type ;   \ forth strings

: ldrop drop ;
: l0= 0= ;
: mu/mod ( l n -- rem quot )
  /mod
;
: ldup dup ;

: noop ;

defer error-output ( -- )
' noop is error-output  \ XXX Should select standard error

defer restore-output ( -- )
' noop is restore-output  \ XXX Should reselect standard output

: (where
 infilename c@
 if ."  File: " infilename count type space
    state @
    if   ." Compiling "
    else ." Latest word was "
    then
    last @ .id  cr
  then
;

defer where
' (where is where

: ?stack
   sp@  sp0 @  swap   u<
   if   error-output ." Stack Underflow " where restore-output
        sp0 @ sp!  abort
   then
   sp@  sp0 @ 400 -  u<   abort" Stack Overflow "
;
: (.s        (s -- )
  depth 0 ?do  depth i - 1- pick .  loop
;
: .s         (s -- )
  ?stack  depth  if (.s else ." Stack Empty " then
;

: fdepth fsp0 @ fsp@ - 4/ ;
: f.s fdepth dup if 0 do fdepth i - 1- fpick f. loop 
                 else ." Floating Stack Empty" then ;

: tdepth tsp0 @ tsp@ - n/ ;
: t.s tdepth dup if 0 do tsp0 @ i 1+ na-  @ . loop 
                 else ." Temporary Stack Empty" then ;

nuser csp
: !csp   (s -- )   sp@ csp !   ;
: ?csp   (s -- )   sp@ csp @ <>
  if error-output ." Stack Changed " where restore-output abort then
;

: \t32 ; immediate
: \t16 [compile] \ ; immediate
: \itc [compile] \ ; immediate
: \dtc [compile] \ ; immediate
: \c ; immediate

: ? @ . ;

\ Alias makes a new word which behaves exactly like an existing
\ word.  This works whether the new word is encountered during
\ compilation or interpretation, and does the right thing even
\ if the old word is immediate. If the desired word doesn't exist
\ print a message and make an alias to noop

: alias  ( -- )     ( input stream:  new-name old-word )
  create
    bl word find    ( cfa -1 | cfa 1  |  str false )
    dup if
       , , immediate
    else
      drop 1+
      false if
         ["] \nalias: can't find %s" printf
      else
         cr ["] alias: can't find " ct ct
      then
      ['] noop -1 , , immediate
    then
  does>  2@   ( cfa -1 | cfa 1 )
   do-defined
;

hex
: s->l ( w -- l )
  dup  00008000 and
  if   ffff0000 or
  else 0000ffff and
  then
;

( address manipulation )
alias la1+ n+ 
alias ca1+ 1+
alias wa1+ 2+
alias sa1+ 2+
alias ca+  +
: sa+  2* + ;
alias wa+ sa+
alias la+ na+
: sa- 2* - ;
alias na1- n-   
alias sa1- 2- 
alias s+! w+!

: _load 1 - "load ;
\ nb _load takes cstring address; "load fstring address

: c, ( c -- )
  here 1 allot w! 
;

2 constant /w
: w, ( w -- )
  here /w allot w!
;
alias s, w,
alias s@ w@
alias s! w!
alias <w@  w@     ( addr -- signed.w )
\ w@ now (86/11/17) returns a signed short; kernel mod 

hex

: uw@  w@ ffff and ;   \ unsigned word 

: n->w ( l -- w ) ffff and ;

decimal
5 constant #vocs    \ Must agree with NVOCS in forth.h
8 constant #threads

: voc-link, ( -- )  ( links this vocabulary to the chain )
  here   voc-link token@  token,   voc-link token!
;
: vocabulary ( --namex )
  create 
    #threads 0 do  0 ,  loop
    voc-link,          \ voc-link
  does> context !
;

\ The also/only vocabulary search order scheme
context dup @ swap na1+ !   ( make forth also )
\ The root vocabulary is created in intern.msc
root definitions
: also   (s -- )
   context dup na1+ #vocs 2- /n * cmove>
;
: >threads >body ;
: only
   ['] root >threads     ( thread address for root vocabulary )
   context #vocs 1- /n * ( root-thread-addr  context size-2 )
   2dup erase            ( root-thread-addr  context size-2 )
   +                     ( root-thread-addr addr-of-last-entry-in-search-list )
   !
   root
;
\
\ Seal restricts the dictionary to words in the current dictionary.
\ It is not clear if this is correct according to the forth '83 standard;
\ there seal is said to `delete all occurrences of ONLY from the search order',
\ but ONLY isn't defined. Maybe it means root and forth?
\
: seal   (s -- )
   context @   context #vocs /n * erase   context !   ;
: previous   (s -- )
   context dup na1+ swap #vocs 2- /n * cmove
   context #vocs 2- /n * + off   ;
: vswap (  --   )  ( swaps two top contexts )
    context @ context na1+ @ dup if
    context ! context na1+ ! else
    drop drop then ;  
 
\ following words are for sealing off modular code:
\ module> aarg
\ private code
\ public>
\ public code which can refer to private
\ end>
\ code after end> cannot access private code, but 
\ public code is in vocab previous to module>
\ leaves user in vocabulary previous to module>; executing
\ aarg does an also aarg definitions; aarg is a valid vocabulary
: module> ( --namex )
    create here
    #threads 0 do  0 ,  loop
    voc-link,          \ voc-link
    also dup context ! current !
  does> also dup context ! current !
;

: public>  ( -- )
    vswap definitions
;

: end>
    vswap previous
;

: forth   forth ;
: definitions   definitions   ;

: v>body ( voc-link-addr -- pfa ) #threads /n * -  ;
: v>threads ( voc-link-addr -- threads ) v>body ;
: find-voc ( voc-ptr-addr -- cfa )
  body>
;
: order   (s -- )
   ." context: " context
   #vocs 0
   do   dup @ ?dup
      if   find-voc >name .id then
   na1+
   loop drop
   4 spaces ." current: " current @ find-voc >name .id flushit
;
: vocs   (s -- )
   voc-link token@
   begin   dup v>body body> >name .id
      token@ dup origin u<=
   until
   drop
   flushit
;
: ?missing   (s f -- )
  if   error-output 'word count type ."  ?" restore-output abort  then ;
: dp! ( addr -- ) here - allot ;
nuser fence
: trim   (s faddr voc-addr -- )
   #threads 0
   do   2dup     link@       ( faddr thread  faddr link )
      begin
        2dup u<=               ( faddr thread  faddr link   f )
      while      link@       ( faddr thread  faddr link' )
      repeat
      nip over link!              ( faddr thread )
   /link +   loop
   2drop
;
\ It is a bad idea to do a forget that will result in the forgetting of
\ vocabularies that are presently in the search order.
: (forget   (s cfa -- )
   >link
   dup fence @ u< abort" below fence"  ( addr )
   \ first adjust lastfile
   lastfile begin @ 2dup             ( lfa lastfile? )
   > until                           ( down chain until lastfile<lfa )   
   lastfile !                        ( lfa on stack )
   \ now forget any vocabularies defined since the word to forget

   dup voc-link
   begin   link@ 2dup  u>= until      ( addr voc-link-addr )
   dup voc-link link! nip             ( addr voc-link-addr )

  \ now, for all remaining vocabularies, forget words defined
  \ since the word to forget )
   begin
     dup origin u>=  ( any more vocabularies? )
   while
     2dup v>threads ( addr voc-link-addr addr voc-threads-addr )
     trim           ( addr voc-link-addr )
     link@          ( addr new-voc-link-addr )
   repeat
   drop   dp!
;
: _forget ( name.st -- )	\ NAME is a forth string
   canonical
   current @ vfind 0= ?missing
   (forget
;
: forget ( i:name.st -- )
   bl word _forget ;

\ : reload ( i:name.st --- )
\	bl cword spush
\	st@ ascii . index 0= if
\		 ["] .m" scat
\		 st@ strlen st@ 1- c!
\	then
\	st@ 1- dup canonical current @ vfind 0= if
\		drop "load
\	else
\		(forget "load
\	then
\	drop
\ ;

\ words to manipulate iteration index

: seti ( newi --- ) r> r> r> 3 roll over - rot drop swap >r >r >r ;

: inci ( deltai --- ) r> r> rot + >r >r ;


only forth also definitions

: ?comp   state @  0= abort" Compilation Only " ;
: ?exec   state @     abort" Execution Only " ;
: ?pairs  - abort" Conditionals not paired " ;

nuser hld
: hold   (s char -- )   -1 hld +!   hld @ c!   ;
: <#     (s -- )     pad  hld  !  ;
: #>     (s l# -- addr len )    ldrop  hld  @  pad  over  -  ;
: sign   (s l# n1 -- l# )  0< if  ascii -  hold  then  ;
: #      (s n1 l# -- n1 l# ) ( for upper case hex output, change 39 to 7 )
  base @ mu/mod           ( n1 nrem l# )
  swap                  ( n1 l# nrem )
  9 over < if  39 + then  ( n1 l# nrem' )
  48  +  hold
;
: #s     (s -- )     begin  #  ldup l0=  until  ;

\ (u.) and u.r are broken. Replace with C code.

\ : (u.)  (s u -- a len )      <# #s #>   ;
\ : u.r   (s u len -- )    >r  dup dot_push  (u.)   r> over - spaces   type   ;
: (.)   (s n -- a len )   dup abs  <# #s   swap sign   #>   ;
\ : .r    (s n l -- )    >r  dup dot_push  (.)   r> over - spaces   type   ;
: st.   ssp@ 1+ ct ;

here fence !








