\  decompiler: version 88/05/20
\ Based on the F83 decompiler by Perry and Laxen
\ Heavily modified by Mitch Bradley:
\   Added structured decompilation of conditionals
\   Made largely machine independent
\   Print the name of the definer for child words instead of the
\     definer's DOES> clause.
\   "Smart" decompilation of literals.
\ And later by J. Gunn to work with Mirella; added are floating literals,
\ and all the C interface stuff; some of the niceties of the Bradley version
\ had to be removed, like reporting the values of variables; since variables
\ can be either fixed or floating in Mirella, the decompiler has no way of
\ knowing how to interpret them. Deals with local variables correctly.

\ A Forth decompiler is a utility program that translates
\ executable forth code back into source code.  Normally this is
\ impossible, since traditional compilers produce more object
\ code than source, but in Forth it is quite easy.  The decompiler
\ is almost one to one.
\ It was written with modifiability in mind, so if you add your
\ own special compiling words, it will be easy to change the
\ decompiler to include them.  This code is implementation
\ dependent, and will not necessarily work on other Forth system.
\ Many machine dependencies have been isolated into a separate
\ file "decompiler.m.f".
\ To invoke the decompiler, use the word decompile <name> where <name> is the
\ name of a Forth word.  Alternatively,  (decompile will decompile the word
\ whose cfa is on the stack.

module> decompil
decimal
alias l@ @ ;
alias l. . ;

\ Machine/implementation-dependent definitions
decimal

/n constant /branch

: +str    ( ip -- ip' addr len )
  count 2dup + aligned  -rot
;

: ta1+ ( -- ) /token + ;

alias >data >is

\ >target depends on the way that branches are compiled
: >target ( ip-of-branch-instruction -- target )
  ta1+ dup @ na+
;

\ Since ;code is not implemented yet, (does always introduces a
\ does> clause.
: does-ip?   (s ip -- ip' f )
   ta1+ true
;
\ Given an ip, scan backwards until you find the cfa.  This assumes
\ that the ip is within a colon definition, and it is not absolutely
\ guaranteed to work, but in practice it nearly always does.
\ This is dependent on the alignment requirements of the machine.
: find-cfa ( ip -- cfa)
   begin
      #align - dup token@    ( ip' token )
      ['] does-ip? token@ =  ( look for the cfa of a : defn )
   until  ( ip)
;

: ccode ;
: primitive ;
: cconstant ;
: cstring ;
: fcconstant ;
: fconstant ;
: cvariable ;
: narray ;
: sarray ;
: carray ;
: ncarray ;
: scarray ;
: ccarray ;
: nmarray ;
: smarray ;
: cmarray ;
: nmatrix ;
: smatrix ;
: cmatrix ;
: ncmatrix ;
: scmatrix ;
: ccmatrix ;
: nmmatrix ;
: smmatrix ;
: cmmatrix ;
: offset ;
: strucarray ;
: struccarray ;

: definer ( cfa-of-child -- cfa-of-defining-word )
  @       ( code-field )
  dup 300 u<  if  drop ['] primitive  exit  then
  dup 334 u<
  if
    case
      301 of ['] :          endof
      302 of ['] constant   endof
      303 of ['] variable   endof
      304 of ['] user       endof
      305 of ['] defer      endof
      306 of ['] ccode      endof
      307 of ['] vocabulary endof
      308 of ['] fconstant  endof    \ these are all the C interface.
      309 of ['] cconstant  endof    \
      310 of ['] fcconstant endof    \
      311 of ['] cstring    endof    \
      312 of ['] cvariable  endof    \ does same as constant, added for decom.
      313 of ['] narray     endof
      314 of ['] sarray     endof
      315 of ['] carray     endof
      316 of ['] ncarray    endof
      317 of ['] scarray    endof
      318 of ['] ccarray    endof
      319 of ['] nmarray    endof
      320 of ['] smarray    endof
      321 of ['] cmarray    endof
      322 of ['] nmatrix    endof
      323 of ['] smatrix    endof
      324 of ['] cmatrix    endof
      325 of ['] ncmatrix   endof
      326 of ['] scmatrix   endof
      327 of ['] ccmatrix   endof
      328 of ['] nmmatrix   endof
      329 of ['] smmatrix   endof
      330 of ['] cmmatrix   endof
      331 of ['] offset     endof
      332 of ['] strucarray endof
      333 of ['] struccarray endof
      
    endcase
  else
      find-cfa
  then
;

\ We define these so that the decompiler can find them, but they aren't used
: (;code ;
: (;uses ;
: (llit ;

\ end machine-dependent section; still very cforth-dependent


\ Like ." but goes to a new line if needed.
: cr." ( -- ) ?cr skipstr type ;
: .."  ( -- ) compile cr." ," ; immediate

\ Positional case defining word                       28AUG83HHL
\ Subscripts start FROM 0
: out   ( # apf -- ) \ report out of range error
   cr  ." subscript out of range on "  dup body>
   c.id  ."    max is " ?   ."    tried " .  quit   ;
: map  ( # apf -- a ) \ convert subscript # to address a
   2dup @  u< if   na1+ swap na+   else   out  then   ;
: maptoken  ( # apf -- a ) \ convert subscript # to address a
   2dup @  u< if   na1+ swap /token * +   else   out  then   ;

: case:   (s n --  ) \ define positional case defining word
   constant  hide    ]
   does>   ( #subscript -- ) \ executes #'th word
     maptoken  token@  execute   ;

: tassociative:
   constant
   does>         (s n -- index )
      dup @              ( n pfa cnt )
      dup >r -rot na1+        ( cnt n table-addr )
      r> 0                    ( cnt n table-addr cnt 0 )
      do   2dup token@ =          ( cnt n pfa' bool )
         if 2drop drop   i 0 0   leave   then
            ( clear stack and return index that matched )
         ta1+
      loop   2drop
;

defer (decompile

\ Breaks is a list of places in a colon definition where control
\ is transferred without there being a branch nearby.
\ Each entry has two items: the address and a number which indicates
\ what kind of branch target it is (either a begin, for backward branches,
\ a then, for forward branches, or an exit.

: breaks pad ;
variable end-breaks

: add-break ( break-address break-type -- )
  end-breaks @ 2!  /n 2*  end-breaks +!
;
variable break-type  variable break-addr   variable where-break
: next-break ( -- break-address break-type )
  -1 break-addr !   ( prime stack)
  end-breaks @  breaks
  ?do  i  2@ over   break-addr @ u<
       if  break-type !  break-addr !  i where-break ! else 2drop then
  /n 2* +loop
  break-addr @  -1  <>  if -1 -1 where-break @ 2! then
;

: forward-branch? ( ip-of-branch-token -- f )
  dup >target u<
;

\ Bare-if? checks to see if the target address on the stack was
\ produced by an IF with no ELSE.  This is used to decide whether
\ to put a THEN at that target address.  If the conditional branch
\ to this target is part of an IF ELSE THEN, the target address
\ for the THEN is found from the ELSE.  If the conditional branch
\ to this target was produced by a WHILE, there is no THEN.
: bare-if? ( ip-of-branch-target -- f )
  /branch - /token - dup token@  ( ip' possible-branch-cfa )
  dup ['] branch  =    \ unconditional branch means else or repeat
  if  drop drop false exit then  ( ip' cfa )
  ['] ?branch =        \ cond. forw. branch is for an IF THEN with null body
  if   forward-branch?
  else drop true
  then
;

\ While? decides if the conditional branch at the current ip is
\ for a WHILE as opposed to an IF.  It finds out by looking at the
\ target for the conditional branch;  if there is a backward branch
\ just before the target, it is a WHILE.
: while? ( ip-of-?branch -- f )
  >target
  /branch - /token - dup token@  ( ip' possible-branch-cfa )
  ['] branch =           \ looking for the uncond. branch from the REPEAT
  if  forward-branch? 0= \ if the branch is forward, it's an IF .. ELSE
  else drop false
  then
;
: indent ( -- )
  #out @ lmargin @ > if cr then
  lmargin @ #out @ - spaces
;
: +indent ( -- ) indent 6 lmargin +! ;
: -indent ( -- ) -6 lmargin +! indent ;
: <indent ( -- ) -6 lmargin +! +indent ;

: .begin ( -- ) +indent .." begin " ;
: .then  ( -- ) -indent .." then  " ;

\ Extent holds the largest known extent of the current word, as determined
\ by branch targets seen so far.  This is used to decide if an exit should
\ terminate the decompilation, or whether it is "protected" by a conditional.
variable extent  extent off
: umax ( u1 u2 -- umax ) 2dup u< if swap then drop ;   \ u< ???
: +extent ( possible-new-extent -- )
  extent @ umax extent !
;
: +branch ( ip-of-branch -- next-ip )  ta1+ /branch + ;
: scan-branch ( ip-of-?branch -- ip' )
  dup dup forward-branch?
  if    >target dup +extent   ( branch-target-address)
        dup bare-if? ( ip flag ) \ flag is true if this is an IF branch
        if   ['] .then add-break
        else drop
        then
  else >target ['] .begin add-break
  then
  +branch
;

: scan-unnest ( ip -- ip' | 0 )
  dup extent @ u>= if drop 0 else ta1+ then
;

: .(;code    (s ip -- ip' )
   does-ip?
   if    .." does> "
   else  0 lmargin ! indent .." ;code " drop 0
   then
;
: scan-(;code ( ip -- ip' | 0 )
  does-ip?  0=  if  scan-unnest  then
;
: .branch ( ip -- ip' )
  dup forward-branch?
  if   <indent .." else  "
  else -indent .." repeat "
  then
  +branch
;
: .?branch ( ip -- ip' )
  dup forward-branch?
  if  dup while?
      if   <indent .." while "
      else +indent .." if    "
      then
  else -indent .." until " then
  +branch
;

: .do    ( ip -- ip' )  +indent .." do    " +branch ;
: .?do   ( ip -- ip' )  +indent .." ?do   " +branch ;
: .loop  ( ip -- ip' )  -indent .." loop  " +branch ;
: .+loop ( ip -- ip' )  -indent .." +loop " +branch ;

\ Guess what kind of constant n is
: classify-literal ( n -- )
  dup isprint
  if   .." ascii " dup emit .."  ( " . .." ) "
  else . then
;

\ local variable stuff
create lname 10 allot      12 constant Ldelem    variable colbase

" l00" 1- lname 5 cmove

: localname ( -- addr of name in localdic) localdic @ nlvar @ Ldelem * + ;
: nlcompile ( ip -- ip )
    p>t nlvar @ dup 
    10 / ascii 0 + lname 2 + c! 
    10 mod ascii 0 + lname 3 + c!
    localname dup                   \ base of this element 
    8 + t@ swap !                   \ store cfa
    lname swap   
    \ ( debug) ." (name addr " dup . ." )"
    5 cmove              \ place name
    ?cr
    lname count type space          \ print name    
    nlvar ++
    t>p
;

: l.id ( cfa -- ) \ prints name of local variable
    dup n- @ ['] (lvar @ = if   \ is it a local var ? 
        n+ p>t
\       ( debug) ." (lcfa: " t@ . ." ) "
        nlvar @ 0 ?do
            localdic @ Ldelem i * + 
            dup 8 + @ t@ = if space count type space else drop then
        loop
        tdrop
    else c.id
    then
;

: cl.id dup colbase @ swap u< if  ( local) l.id  else ( normal) c.id then ;
\ u< ???

: lcr cr lmargin @ spaces ;   \ newline with proper indentation

\ first check for word being immediate so that it may be preceded
\ by [compile] if necessary
: check-[compile] ( cfa -- cfa )
  dup dup colbase @ u< if immediate? if .." [compile] " then else drop then 
;  \ u< ????

: .word         (s ip -- ip' )  dup token@ check-[compile] ?cr cl.id   ta1+   ;
: skip-word     (s ip -- ip' )  ta1+ ;
: .inline       (s ip -- ip' )  ta1+ dup @  classify-literal  na1+   ;
: skip-inline   (s ip -- ip' )  ta1+ na1+ ;
: .llit         (s ip -- ip' )  ta1+ dup l@ l. la1+  ;
: skip-llit     (s ip -- ip' )  ta1+ la1+ ;
: skip-branch   (s ip -- ip' )  +branch ;
: .compile      (s ip -- ip' )  ." compile " ta1+ .word   ;
: skip-(compile (s ip -- ip' )  ta1+ ta1+ ;
: skip-string   (s ip -- ip' )  ta1+ +str 2drop ;
: .[']          (s ip -- ip' )  ta1+  .." ['] " dup token@ c.id  ta1+ ;
: .[p']         (s ip -- ip' )  ta1+  .." [p'] " dup token@ na1- c.id ta1+ ;
: .[cp']        (s ip -- ip' )  ta1+  .." [cp'] " dup token@ na1- c.id ta1+ ;
: .[m']         (s ip -- ip' )  ta1+  .." [m'] " dup token@ c.id ta1+ ;
: .[a']         (s ip -- ip' )  ta1+  .." [a'] " dup token@ c.id ta1+ ;
: skip-token    (s ip -- ip' )  ta1+ ta1+ ;
: .finish       (s ip -- ip' )  .word   drop   0  ;
: .string       (s ip -- ip' )  +str type ascii " emit space ;
: .(."string    (s ip -- ip' )  lcr ta1+ .." ." ascii " emit space .string ;
: .flit         (s ip -- ip' )  ta1+ dup f@ f. la1+ ;
: skip-flit     (s ip -- ip' )  ta1+ la1+ ;
: skip-lvar     (s ip -- ip' )  ta1+ ta1+ la1+ ;  
: .lvar         (s ip -- ip' )  ta1+ ta1+ .." lvar " nlcompile la1+ ;
: .["]string    (s ip -- ip' )  lcr .." [" ascii " emit ." ] " ta1+ .string ;
: .f["]string   (s ip -- ip' )  lcr .." f[" ascii " emit ." ] " ta1+ .string ;

\ Use this version of .branch if the structured conditional code is not used
\ : .branch     (s ip -- ip' )  .word   dup <w@ .   /branch +   ;

: .unnest     (s ip -- ip' )
   dup extent @ u>=
   if   0 lmargin ! indent .." ; " cr  drop   0
   else .." exit " ta1+
   then
;


\ classify each word in a definition
24 tassociative: execution-class  ( token -- index )
   (  0 ) [compile]  (lit            (  1 ) [compile]  ?branch
   (  2 ) [compile]  branch          (  3 ) [compile]  (loop
   (  4 ) [compile]  (+loop          (  5 ) [compile]  (do
   (  6 ) [compile]  (compile        (  7 ) [compile]  (."
   (  8 ) [compile]  (abort"         (  9 ) [compile]  (;code
   ( 10 ) [compile]  unnest          ( 11 ) [compile]  c("s
   ( 12 ) [compile]  (?do            ( 13 ) [compile]  (;uses
   ( 14 ) [compile]  exit            ( 15 ) [compile]  (llit
   ( 16 ) [compile]  ('              ( 17 ) [compile]  (flit
   ( 18 ) [compile]  (lvar           ( 19 ) [compile]  ("s
   ( 20 ) [compile]  (p'             ( 21 ) [compile]  (cp'
   ( 22 ) [compile]  (m'             ( 23 ) [compile]  (a' 

\ Print a word which has been classified by  execution-class
25 case: .execution-class  ( ip index -- ip' )
   (  0 )     .inline                (  1 )     .?branch
   (  2 )     .branch                (  3 )     .loop
   (  4 )     .+loop                 (  6 )     .do
   (  6 )     .compile               (  7 )     .(."string
   (  8 )     .string                (  9 )     .(;code 
   ( 10 )     .unnest                ( 11 )     .["]string
   ( 12 )     .?do                   ( 13 )     .finish
   ( 14 )     .unnest                ( 15 )     .llit
   ( 16 )     .[']                   ( 17 )     .flit
   ( 18 )     .lvar                  ( 19 )     .f["]string
   ( 20 )     .[p']                  ( 21 )     .[cp'] 
   ( 22 )     .[m']                  ( 23 )     .[a']
  ( default ) .word
;

\ Determine the control structure implications of a word
\ which has been classified by  execution-class
25 case: do-scan
   (  0 )     skip-inline            (  1 )     scan-branch
   (  2 )     scan-branch            (  3 )     skip-branch
   (  4 )     skip-branch            (  6 )     skip-branch
   (  6 )     skip-(compile          (  7 )     skip-string
   (  8 )     skip-string            (  9 )     scan-(;code
   ( 10 )     scan-unnest            ( 11 )     skip-string
   ( 12 )     skip-branch            ( 13 )     scan-unnest
   ( 14 )     scan-unnest            ( 15 )     skip-llit
   ( 16 )     skip-token             ( 17 )     skip-flit
   ( 18 )     skip-lvar              ( 19 )     skip-string
   ( 20 )     skip-token             ( 21 )     skip-token
   ( 22 )     skip-token             ( 23 )     skip-token
  ( default ) skip-word
;

\ Scan the parameter field of a colon definition and determine the
\ places where control is transferred.
: scan-pfa   (s cfa -- )
   dup ta1+ colbase ! 
   dup extent ! 
   breaks end-breaks !
   >body
   begin
      pause
      dup token@ execution-class do-scan
      dup 0= 
   until   drop
;

\ Decompile the parameter field of colon definition
: .pfa   (s cfa -- )
  dup scan-pfa next-break 3 lmargin ! indent
   >body
   begin
      pause
      ?cr   break-addr @ over =
         if    break-type @ execute  next-break
         else  dup token@ execution-class .execution-class
         then
      dup 0= 
   until   
   drop
;
: .immediate   (s cfa -- )
   immediate? if   .." immediate"   then   ;

: .definer (s cfa definer-cfa -- cfa )
  cl.id dup cl.id ;

\ Display category of word
: .:          (s cfa definer -- )  .definer space space  .pfa   ;
: .constant   (s cfa definer -- )  over >body ?   .definer drop ;
: .fconstant  (s cfa definer -- )  over >body f?  .definer drop ;
: .vocabulary (s cfa definer -- )  .definer drop ;
: .ccode      (s cfa definer -- )  
	.definer ."  ( Pointer(hex) = " >body base @ swap hex ? base ! .."  )" ;
: .variable   (s cfa definer -- )
     .definer   ."   ( Pfa = " >body . .."  )" ;
: .cconstant  (s cfa definer -- )  over >body @ ? .definer drop ;
: .fcconstant (s cfa definer -- )  over >body @ f? .definer drop ;
: .cvariable  (s cfa definer -- )  
      .definer   ."   ( Address = " >body ? .."  )" ;
: .cstring    (s cfa definer -- )  
      .definer >body @ cr .." Value: " ct ;
: .user       (s cfa definer -- ) 
   over >body ?   .definer   .."  value = "   >data  ?
;
: .defer      (s cfa definer -- )
  .definer  .." is " cr  >data token@ (decompile
;
: .array      (s cfa definer -- ) over >body ?  .definer drop ;
: .matrix     (s cfa definer -- ) 
    over >body dup @ swap na1+ @ . .  .definer drop ;

\ Decompile a word whose type is not one of those listed in
\ definition-class.  These include does> and ;code words which
\ are not explicitly recognized in definition-class.
: .other   (s cfa definer -- )
    .definer   >body @ ."    ( Parameter field: " . ." ) "
;


\ Classify a word based on its cfa
: cf, ( --name ) ( find the next word and compile its code field value )
  ' token,
;
30 tassociative: definition-class
   ( 0 )   cf,  :          ( 1 )   cf,  constant
   ( 2 )   cf,  variable   ( 3 )   cf,  user
   ( 4 )   cf,  defer      ( 5 )   cf,  ccode
   ( 6 )   cf,  vocabulary ( 7 )   cf,  fconstant 
   ( 8 )   cf,  cconstant  ( 9 )   cf,  fcconstant
   ( 10 )  cf,  cstring    ( 11 )  cf,  cvariable
   ( 12 )  cf,  narray     ( 13 )  cf,  nmatrix
   ( 14 )  cf,  sarray     ( 15 )  cf,  smatrix
   ( 16 )  cf,  carray     ( 17 )  cf,  cmatrix   
   ( 18 )  cf,  ncarray    ( 19 )  cf,  ncmatrix
   ( 20 )  cf,  scarray    ( 21 )  cf,  scmatrix
   ( 22 )  cf,  ccarray    ( 23 )  cf,  ccmatrix  
   ( 24 )  cf,  nmarray    ( 25 )  cf,  nmmatrix
   ( 26 )  cf,  smarray    ( 27 )  cf,  smmatrix
   ( 28 )  cf,  cmarray    ( 29 )  cf,  cmmatrix        
 
31 case:   .definition-class
   ( 0 )   .:              ( 1 )   .constant
   ( 2 )   .variable       ( 3 )   .user
   ( 4 )   .defer          ( 5 )   .ccode
   ( 6 )   .vocabulary     ( 7 )   .fconstant
   ( 8 )   .cconstant      ( 9 )   .fcconstant
   ( 10 )  .cstring        ( 11 )  .cvariable
   ( 12 )  .array          ( 13 )  .matrix 
   ( 14 )  .array          ( 15 )  .matrix 
   ( 16 )  .array          ( 17 )  .matrix 
   ( 18 )  .array          ( 19 )  .matrix 
   ( 20 )  .array          ( 21 )  .matrix 
   ( 22 )  .array          ( 23 )  .matrix 
   ( 24 )  .array          ( 25 )  .matrix 
   ( 26 )  .array          ( 27 )  .matrix 
   ( 28 )  .array          ( 29 )  .matrix 
   ( default )  .other
;

\ top level of the decompiler DECOMPILE
: ((decompile   (s cfa -- )
   here ta1+ colbase !   \ get colbase out of harm's way
   0 nlvar !
   cr dup dup definer dup   definition-class .definition-class
   .immediate
   flushit
;
' ((decompile  is (decompile 

public>

: decompile   (s -- )
   '   (decompile     ;

end>







