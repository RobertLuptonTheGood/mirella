\ Mirella definitions  VERSION 91/08/23 Mirella 5.50x 
\ History:
\ 88/04/27  new strcat def's
\ 88/05/24  new help def's, new [p'], [cp']
\ 88/06/12  vector graphics
\ 88/06/18  conditional compilation finished, with #else and nesting
\ 88/06/23  new directory code
\ 88/08/30  added 'sforget' to forget startup definitions. prob. not useful.
\ 88/12/14  added new structure and structure array definitions
\ 89/02/04  some new help definitions
\ 89/02/13  added some directory search definitions 
\ 89/03/24  added array-to-stack definitions: asmove, rasmove, fasmove, rfasmove
\ 89/03/31  added new table-to-spline function definitions
\ 89/04/27  added directory-stack definitions
\ 89/06/10  added function-graphing def, modified directory functions
\ 89/06/11  added cs!, cs+! 
\ 89/06/25  fixed rmove,frmove,.... !!!!
\ 89/06/27  fixed _dirdo, moved Stevens to startup
\ 91/08/23  fixed +noffset, +soffset, added +coffset
\ 06/04/02  new 3d matrix code

" Solamente Mirella" progname strcpy
: whoamiwith cr cr ." **** " progname ct ." , amore vecchio ****" cr ;


10 constant \n
13 constant \r
9 constant  \t
7 constant Bell
8 constant \b
12 constant \f
27 constant ESC
32 constant blnk

ascii ; constant ';'

\ math aliases
alias ln log
alias lg log10

\ power function & 1/x jeg0312 FIXME

alias pow x^y
: 1/x 1. fswap f/ ;

\ numerical constants

2.302585093     fconstant Log10
6.283185308     fconstant 2pi
12.56637061     fconstant 4pi
3.141592654     fconstant Pi    
1.570796327     fconstant Pi/2
0.785398164     fconstant Pi/4
2.506628275     fconstant Sqrt2pi
57.29577951     fconstant Deg/rad
Deg/rad 60. f*  fconstant Min/rad
Deg/rad 3600. f* fconstant Arcsec/rad
Arcsec/rad      fconstant Sec/rad

create ffstring 2 allot \f ffstring c! 0 ffstring 1+ c!

: #define create bl cword atoi , does> @ ;  \ constant word for CPP construct
alias /* \                                  \ for C comments

load dotokens.h                             \ hah!

alias clear erase 
alias defines is  \ ' old defines new: old is existing word, new is deferred 

: 2over ( i j k l --- i j k l i j )
	3 pick 3 pick ;
	
: 2fover ( i.f j.f k.f l.f --- i.f j.f k.f l.f i.f j.f )
        3 fpick 3 fpick ;
        
: 2fswap ( i.f j.f k.f l.f --- k.f l.f i.f j.f )
        3 froll 3 froll ;       

: 3dup  ( i j k --- i j k i j k )
    2 pick 2 pick 2 pick
;

: 4dup ( i j k l --- i j k l i j k l )
    3 pick 3 pick 3 pick 3 pick
;
    

2 constant /s
: c+! ( n addr -- ) dup c@  rot + swap c! ;
: !=  = not ;
\ since the arg order of the forth strcpy and strncpy is opposite to C,
\ these are probably preferable to use. 

: cs! strcpy ;
: ncs! strncpy ;
: cs+! swap strcat ;

\ given a C string with space allocated for its length byte, set the length
: cstr2fstr ( str.st --- fstr.st )
   dup dup strlen swap 1- c! ;

: 2swap rot >r rot r> ;

2 constant Ssize    \ sizeof(short int)
4 constant Isize    \ sizeof(int)
4 constant Fsize    \ sizeof(float)
alias Psize SizeofN \ sizeof(void *)
alias Nsize SizeofN \ sizeof(normal)
1 constant Csize    \ sizeof(char) 

\ jeg9605  fixed for placekeeper change in mirellio.c and move here from
\ util.m; it uses lots of strictly mirella stuff.
\ this is broken for filenames with embedded '.'--problem is that
\ basename strips what follows the '.' . Need, I think, just to
\ export the C word appendm to mirella and use it...I think it has
\ all the necessary logic

: reload  ( i:name -- )  \ fixed jeg9807
    lvar lname  \ full pathname (or minus .m) for handing to _load
    lvar mname  \ basename.m, dictionary name

    bl word dup lname ! fbasename 
    1+ spush dup 1- mname ! ["] .m" strcat 2 mname @ c+! 
    mname @ canonical
    current @ vfind 0= if
        drop lname @ "load
    else
        (forget lname @ "load
    then
;

\ this word loads a file if needed (ie not present, does nothing if not)

: ?load  ( i:name -- )  \  jeg0112
    lvar lname  \ full pathname (or minus .m) for handing to _load
    lvar mname  \ basename.m, dictionary name

    bl word dup lname ! fbasename 
    1+ spush dup 1- mname ! ["] .m" strcat 2 mname @ c+! 
    mname @ canonical
    current @ vfind 0= if
        drop lname @ "load
    else
        drop
    then
;

: erret 
    dup 10 < if
        drop ["] " _erret
    else 
        _erret
    then
;

: .version version ct ;

: ct.r  ( str len --- )  \ writes a string in a field of length len, filling
                         \ with blanks; if too long, overruns
    >r   
    dup strlen
    swap ct 
    r> swap - dup 0> if spaces else drop then    
;

\ dictionary clearing, allocation, moving
: clallot dup here swap clear allot ;    \ like allot, but clears
: sallot 2* clallot ;
: nallot n* clallot ;
: nclear n* clear ;
: sclear 2* clear ;

: nmove n* move ;
: fmove 4* move ;
: smove 2* move ;   \ all nondestructive

\ float utils
: fa+ Fsize * + ;
: -frot frot frot ;
: 2fdup fover fover ;
: 2fdrop fdrop fdrop ;
: f0<  0. f< ;
alias f.r df.r
: fbetween fover fover f< not if fswap then frot fdup frot f< f< and ;
\ (fa fb fc -- i ) i = true iff fa is strictly between fb and fc 
\ note that this is different behavior from between, which is
\ inclusive. Write words for both ends inclusive and bottom inclusive????
\ inclusive, but beware roundoff
: fibetween fover fover f< not if fswap then frot fdup frot f> not f> not and ;
\ inclusive below, exclusive above, but beware roundoff
: flbetween fover fover f< not if fswap then frot fdup frot f< f> not and ;


: f0= 0. f= ;
: ft@  tsp@ f@ ;  \ nodestructive
: ft. ft@ f. ;    \ ditto
: f*! dup f@ f* f! ;
: f\! dup f@ fswap f/ f! ;
: f-! dup f@ fswap f- f! ;


: ivar create ,  ;  \ 0 ivar x creates variable x, initializes to zero 
: fvar create f, ;  \ 0. fvar x creates variable x, initializes to 0.0

: f? f@ f. ;
: s? s@ . ;
: c? c@ . ;    \ jeg 0406: Mirella normally sign-extends; this is seldom
               \ what you want with c@. 
               \ we need to decide what to do.
: us. 65535 and . ;
: us? s@ us. ;
: uc. 127 and . ;



: uc? c@ 255 and . ; \ prints u_char value
: uc@ c@ 255 and ;   \ fetches u_char value, no sign extension
               
: ++ 1 swap +! ;  ( addr --- , increments in place )
: -- -1 swap +! ;   ( addr --- , decrements in place )
: n++ 4 swap +! ;   ( addr --- increments pointer by one normal in place )
: n-- -4 swap +! ;  ( addr --- decrements pointer by one normal in place )
: @+ dup n+ swap @ ;  ( addr --- addr+sizeof<normal> *[addr] )
: -@ n- dup @ ;   ( addr -> addr-1 *[addr-sizeof<normal>] ) 
: t@ tsp@ @ ;     ( --- top_of_temp_stack )  \ nondestructive
: tdrop t>p drop ;
: t. t@ . ;      \ nondestructive                            
: ndup ( n -- list ) \ dups 1st n stack entries
    dup 1 - p>t 
    0 do 
        t@ pick 
    loop 
; 


    \ Conditional compilation stuff--like C, must be first thing on line

: #if ( flg --- ) ( turns on and off compilation )
    ccnest ++     \ always increment the level 
    if 
        0 nocompile ! 
    else 
        -1 nocompile ! 
        ccnest @ ccnoff !
    then
;   immediate

: #else 
    -1 nocompile ! 
    ccnest @ ccnoff !
;   immediate
    \ the compiler sees this only if nocompile
    \ is zero, and compilation wants turning off.

: #endif ccnest -- ;   immediate
    \ the compiler sees this only if nocompile is zero; nothing wants done,
    \ but the level always wants decrementing


    \ terminal lists from the stack to memory and vice versa

: rmove dup >r na+ r> 0 do dup i na- na1- rot swap ! loop drop ; 
    ( list addr n rmove puts list in addr, top last )
: frmove dup >r na+ r> 0 do dup i na- na1- f! loop drop ;
    ( same for floating list from floating stack )
: srmove dup >r sa+ r> 0 do dup i sa- sa1- rot swap s! loop drop ;
    ( same for short list )
: crmove dup >r + r> 0 do dup i - 1- rot swap c! loop drop ;
    ( same for short list )

: asmove   ( addr n --- contents_of_array )
    \ moves n normal-sized elements from  addr to the fstack, contents of
    \ LAST address on ftos
    0 do 
        dup i na+ @ swap
    loop
    drop
;

: fasmove   ( addr n --- contents_of_float_array )
    \ moves n normal-sized elements from  addr to the stack, contents of
    \ LAST address on tos
    0 do 
        dup i na+ f@ 
    loop
    drop
;

: rasmove ( addr n --- reversed_contents_of_array )
    \ moves n normal-sized elements from addr to the stack, contents of
    \ FIRST address on tos
    dup >r 1- na+ r> 0 do 
        dup i na- @ swap
    loop
    drop
;

: rfasmove ( addr n --- reversed_contents_of_float_array )
    \ moves n normal-sized elements from addr to the fstack, contents of
    \ FIRST address on ftos
    dup >r 1- na+ r> 0 do 
        dup i na- f@ 
    loop
    drop
;


    \ 1-d arrays: CAREFUL. tokens must agree with those in mirellin.c
: narray create DONAR here body> ! dup , nallot ;
    \ creates normal array name
( comp: size in:name ---) ( ex: n --- addr)
: sarray create DOSAR here body> ! dup , sallot ;
    \ creates short arr. name
: carray create DOCAR here body> ! dup , clallot ;
    \ creates char arr. name

    \ These words define arrays pointed to by minstalled C pointers 
    \ (NOT !!!) explicitly declared arrays, which minstall handles
    \ automatically
: ncparray ( comp: clink-addr nelt i:name ---) ( ex: index --- addr )
    create DONCPAR here body> ! , ,  ;
    \ usage: p' +blat nelt ncparray blat     for an minstalled qty 
    \ v +blat blat  p
: scparray ( comp: clink-addr nelt i:name ---) ( ex: index --- addr )
    create DOSCPAR here body> ! , ,  ;
: ccparray ( comp: clink-addr nelt i:name ---) ( ex: index --- addr )
    create DOCCPAR here body> ! , ,  ;

    \ C interface stuff
: p' ' na1+ ;    \ pushes pfa; this is the address at which Forth constants
                 \ are stored, and where the C-addresses of minstalled qtys
                 \ are stored.
: (p' ( -- value of skipped token)
    r> dup @ swap /token + >r 
;
                \ compiling version; compiles only addresses in dict, note
: [p'] compile (p' ' /token + , ; immediate

: cp' ' na1+ @ ;  \ pushes quan pointed to by pfa; for minstalled qtys, this
                  \ is an address in C-space, and must be treated with due
                  \ caution.
: (cp' ( -- contents of value of skipped token)
    r> dup @@ swap /token + >r
;
    \ [cp'] is the compilation version of cp' . The indirection is done at
    \ EXECUTION time, note, and is thus OK.
: [cp'] compile (cp' ' /token + , ; immediate

    \ for a cconstant  ccon installed with the c option of minstall,
    \ the value can be changed (but almost never wants to be)
    \ by  value cp' ccon  !,   or   value [cp'] ccon !   in a colon defn.

: tocstr ( forth_str_addr c_str_addr -- )  \ copies forth str to c str 
    dup p>t ( c addr ) swap dup c@ dup p>t p>t ( count)
    1+ swap t>p cmove t>p t>p + 0 swap c! ( terminate ) 
;

    \ words for handling structures in Forth. First set are obsolete.
: offset create DOOFFSET here body> ! ,  ;
: noffset n* offset ;
    \ n offset memname (compiling)
    \ memname adds offset in bytes or normals to stack (ex)
        
    \ use these
: begstruct 0 0 ;    \ begin a series of +offset memname memsize with this
: +offset + dup offset ;
: +coffset + dup offset 1 ;
: +soffset + dup offset 2 ;
: +noffset + dup offset 4 ;
: +foffset + dup offset Fsize ;
    \ like above but keeps running tally; thus
    \ begstruct +offset mem1 size1 +soffset mem2 +noffset mem3, etc.
    \ note that size refers to LAST member, not the next one. Remember to
    \ drop the running tally at the end (endstruct) or store it away someplace;
    \ it is the element size)

: +caroff create + dup , does> @ rot + + ;            \ char arrays
: +saroff create + dup , does> @ rot + swap sa+ ;     \ short int arrays
: +naroff create + dup , does> @ rot + swap na+ ;     \ normal arrays
: +starof create -rot + dup , swap , does> dup >r @ rot + swap r> na1+ @ * + ;
: +faroff create + dup , does> @ rot + swap fa+ ;
    \ these words are like the +offset words but define ARRAYS; they are safe,
    \ slow, and not quite legitimate Mirella arrays; they have no size entry.
    \ the usage is (running offset on stack) +saroff name Ssize Nmem * , etc.;
    \ or  Elsize +staroff name Elsize Nmem *, which allows structures to be 
    \ elements of structures

: endstruct 2drop ;   \ end with this, or
: endstchk  ( tally offset Csize --- )
    -rot + 2dup = not if 
        cr ." Mirella size=" . ."  different from C size="  . 
        1 erret
    else
        2drop
    then
;   \ checker, usage  Csize endstchk at end of structure offset declarations
\ or
: endstset ( tally offset i:name --- )
    create + , does> @
;   \ size definer, usage endstset Msizename at end of struct offset decl.
    \ These words are all designed to be used with the +offset words in a 
    \ particular mnemonic fashion; the declaration looks like
    \   begstruct
    \       +offset memname memsize
    \       .....
    \       +offset memname memsize
    \   endstruct  ( or  endstset Msizename  or  Csize endstchk )

    \ structure arrays defined in C as EXPLICIT arrays:

: struccarray   ( comp: clink_addr<pfa> elsize number in:name --- )   
                ( ex: n --- addr of base ) 
    create DOSTRUCCAR here body> !   \ create entry and place do-token
    , , na1+ na1+ @ ,     \ place number of elt, elt size,
                          \ content addr  (*((p' name) + 2)) -- MKE
;
    \ p' cname size nelt struccarray strname (compiling)
    \ n strname pushes base address of nth element of array
    \ n strname memname pushes address of member of nth element of array (ex)
    \ this cannot be done by minstall, since there is no way of knowing at 
    \ minstall time what the element size is. (Yes there is. -- MKE)
    \ N.B.!!! the minstallation should be  v +sar +sar n for c arrays
    \ explicitly declared as such... ie struct blat sar[nel]

    \ BUT IF THE STRUCTURE ARRAY IS POINTED TO BY A POINTER, AND THE
    \ POINTER IS THE MINSTALLED QUANTITY, YOU MUST USE

    \ Structure arrays in C defined by pointers :
    
: strucparray   ( comp: clink_addr<pfa> elsize number in:name --- )   
                ( ex: n --- addr of base ) 
    create DOSTRUCMAR here body> !   \ create entry and place do-token
    , , ,     \ place number of elt, elt size, clink addr  (p' name)
;

    \ structure arrays allocated in the dictionary:
: strucarray    ( comp: elsize number in:name ---) 
                ( ex:   n --- addr of base ) 
    create DOSTRUCAR here body> !    \ create entry and place do-token
    2dup , ,                         \ place number of elt, elt size
    * clallot                        \ allocate space
;

    \ prompt string stuff: pstring is an fstring, note
: prompt" " 1- pstring @ dup 30 blank "copy ; \ ( i:string ---)
: prompt ( -- )			\ word called whenever prompt is issued;
				\ executes (prompt) and flushes graphics
	f["] (prompt)" find if
		execute
	else
		drop
	then
	
	[ " gflush" 1- find \ if gflush is defined, execute it
#if
	] literal execute
#else
	drop ]
#endif
	;

: m  f["] M>" pstring @ "copy ;
: mi f["] Mirella>" pstring @ "copy ;

    \ memory dump utilities; chars,shorts,ints,floats

0 ivar dumpend  0 ivar dabsflg

: _cdump 2dup 1- + dumpend !  base @ p>t   
    0 do 
        cr dabsflg @ if dup i + else i then 7 .r  hex
        20 0 do 
            dup i + j + dup dumpend @ > if drop leave then c@ 0xff and 3 .r 
        loop  cr 7 spaces  t@ base !
        20 0 do 
            dup 2 spaces i + j + dup dumpend @ > if drop leave then c@
            dup isprint if emit else drop space then 
        loop 
        pause
    20 +loop drop t>p base ! 
    flushit
;

\ dumps as hex bytes and characters 

: _bdump 2dup 1- + dumpend !  
    0 do 
        dabsflg @ if dup i + else i then cr 10 u.r  
        15 0 do 
            dup i + j + dup dumpend @ > if drop leave then c@ 4 .r 
        loop 
        pause 
    15 +loop drop 
    flushit
;  \ dumps as bytes in any base

: _sdump 2dup 1- sa+ dumpend ! 
    0 do 
        dabsflg @ if dup i sa+ else i then cr 9 u.r  
        10 0 do 
            dup i j + sa+ dup dumpend @ > if drop leave then w@  7 .r 
        loop 
        pause
    10 +loop drop  
    flushit
; \ dumps short integers.

: _dump  2dup 1- na+ dumpend ! cr
    0 do 
        dabsflg @ if dup i na+ else i then cr 10 u.r  
        5 0 do 
            dup i j + na+ dup dumpend @ > if drop leave then @ 12 .r 
        loop 
        pause 
    5 +loop drop
    flushit
; \ dumps as integers 

: _udump  2dup 1- na+ dumpend ! cr
    0 do 
        dabsflg @ if dup i na+ else i then cr 10 u.r  
        5 0 do 
            dup i j + na+ dup dumpend @ > if drop leave then @ 12 u.r 
        loop 
        pause 
    5 +loop drop
    flushit
; \ dumps as unsigned 

: _fdump 2dup 1- na+ dumpend ! cr
    0 do 
        dabsflg @ if dup i + else i then cr 10 u.r     
        5 0 do 
            dup i j + na+ dup dumpend @ > if drop leave then f@ 12 -3 df.r 
        loop 
        pause 
    5 +loop drop
    flushit
; \ dumps as floats

: cdump 0 dabsflg ! _cdump ;   : cadump -1 dabsflg ! _cdump ;
: bdump 0 dabsflg ! _bdump ;   : badump -1 dabsflg ! _bdump ;
: sdump 0 dabsflg ! _sdump ;   : sadump -1 dabsflg ! _sdump ;
: dump  0 dabsflg ! _dump  ;   : adump  -1 dabsflg ! _dump  ;
: udump 0 dabsflg ! _udump ;   : uadump -1 dabsflg ! _udump ;
: fdump 0 dabsflg ! _fdump ;   : fadump -1 dabsflg ! _fdump ;
\ the adumps list absolute addresses, the others indices 

: ddump cr  dup >link  dup 7 .r @ 12 .r 
    cr dup  >name dup 7 .r 5 spaces count dup . ascii : emit type 
    20 adump flushit ;
\ dumps dictionary entry--takes cfa. e.g. ' blogs ddump 

\ Output Formatting                                   27Sep83map
: >type   (s adr len -- )
   tuck pad swap cmove   pad swap type  ;
variable lmargin    0 lmargin !
variable rmargin   60 rmargin !
: ?line   (s n -- )
   #out @ +  rmargin @ > if  cr  lmargin @ spaces  then   ;
: ?cr   (s -- )   0 ?line  ;
: to-column (s column -- )
   #out @  -  1 max spaces
;

\ Dr. Charles Eaker's case statement
\ Example of use:
\ : foo ( selector -- )
\   case
\     0  of  ." It was 0"   endof
\     1  of  ." It was 1"   endof
\     2  of  ." It was 2"   endof
\     ( selector) ." **** It was " dup u.
\   endcase
\ ;
\ The default clause is optional.
\ When an of clause is executed, the selector is NOT on the stack
\ When a default clause is executed, the selector IS on the stack.
\ The default clause may use the selector, but must not remove it
\ from the stack (it will be automatically removed just before the endcase)

\ At run time, (of tests the top of the stack against the selector.
\ If they are the same, the selector is dropped and the following
\ forth code is executed.  If they are not the same, execution continues
\ at the point just following the the matching ENDOF

\ code (of  ( selector test -- [ selector ] )
\    sp )+ d0 move  sp ) d0 cmp
\    0= if   2 ip addq  sp )+ d0 move  next
\    else    word ip ) ip adda  next
\    then
\ c;
: of      ( [ addresses ] 4 -- 5 )
  4 ?pairs
  compile over compile =  compile ?branch  \ for totally high level version
  >mark compile drop        \ for totally high level version
\   compile (of    >mark       \ for version using machine-code (of
  5
; immediate

: case    ( -- 4 )
  ?comp  csp @ !csp  4
; immediate

: endof   ( [ addresses ] 5 -- [ one more address ] 4 )
  5 ?pairs
  compile  branch     >mark
  swap  >resolve
  4
; immediate

: endcase ( [ addresses ] 4 -- )
  4 ?pairs
  compile drop
  begin sp@  csp @ <>
  while >resolve
  repeat
  csp !
; immediate

\ yesno word

: y/n  ( i:key --- flag )
     begin
        key case
            ascii y of -1 -1 ascii y emit endof  
                \ 1st -1 is return, 2nd for until
            ascii n of  0 -1 ascii n emit endof
            3       of  0 -1 ." ^C" 1 interrupt ! endof
            4       of  0 -1 ." ^D" 1 interrupt ! endof
            cr ." please enter 'y' or 'n' " 0 swap
        endcase
        flushit
    until
;


\ help stuff; see mirhelp.c
\
\ Call info; with an argument find the proper node
\
: info ( [i:word] --- )
	\n cword
	dup c@ 0= if
		["] info mirage"
	else
		dup ["] mirella" strcmp 0= if
			["] info mirella"
		else
			["] info mirage word %s" sprintf
		then
	then
	system
;

: addhelpfile ( in:helpfilename in: listfilename --- )
    bl cword bl cword _addhelpfile 
;  
    \ addhelpfile adds a helpfile name to the named listfile; the exts
    \ must be .hlp and .lis, respectively
    \ one must run makeindex ( in: listfilename) before the installation
    \ is complete. makeindex makes an index file with the same basename as
    \ the listfile, but with ext .inx
: makeindex ( in:listfilename) bl cword _makeindex ;
: mhelp bl cword ["] mhelp.inx" _ghelp ;  \ individual entry
: mhelpall bl cword ["] mhelp.inx" _ghelpall ;  
            \ displays all entries incl aliases
: chelp bl cword ["] chelp.inx" _ghelp ;  \ individual entry
: chelpall bl cword ["] chelp.inx" _ghelpall ;  
            \ displays all entries incl aliases
: ahelp bl cword ["] ahelp.inx" _ghelp ;  \ individual entry
: ahelpall bl cword ["] ahelp.inx" _ghelpall ;  
            \ displays all entries incl aliases
: achelp bl cword ["] achelp.inx" _ghelp ;  \ individual entry
: achelpall bl cword ["] achelp.inx" _ghelpall ;  
            \ displays all entries incl aliases

: browse bl cword _browse ;
: help ["] helpmsg.hlp" _browse ;
: hhelp ["] help.hlp" _browse ;
  

\ file handling; see mirelf.c, 'system'.m 

: fopen   bl cword swap _fopen ;
: fopenr   0 fopen ;
: fopenw   1 fopen ;
: fopenrw  2 fopen ;
\ n.b. all of these words push the channel number on the stack.
: faccess bl cword _faccess ;  \ n.b. returns TRUE if file exists
: pseek 0 1 seek ;   \ flushes buffers if relevant; resets
                     \ instantaneous read/write flag    
: fclose    bl cword _fclose ;   \ takes filename on input stream;
                                \ word which takes chan # is close
: fchannel  bl cword _fchannel ; \ takes filename on input stream;
                                \ word which takes chan # is channel
: fend      0 2 seek ;
: seekend   0 2 seek ;
: fwrite    Fsize * write ;  \ fix rest
: nwrite    n* write ;
: nread     n* read  ;
: swrite    2* write ;
: sread     2* read  ;
: aclose achan @ close ;        \ close the active channel

: puch pushchan ;
: poch popchan ;

\
\ set name MIRNAME to map to operating system name OPSNAME
\ (e.g. " *exabyte" " /dev/nrxb0" _set-special)
\
: _set-special ( mirname.st opsname.st --- )
   lvar opsname opsname !
   lvar mirname mirname !
   true if
   -1
   begin
      1+ dup special_names _mfname c@ 0= not
   while
      dup special_names _mfname opsname @ strcmp 0= if
	 mirname @ swap special_names _opsname cs!
	 exit
      then
   repeat
   then
;

\ tape words

: skip 1 mskip ;
: usertape 1 tstrict ! ;
: mytape 0 tstrict ! ;
: jump cr 0 do skip i 1+ . 0cr loop ;
: back cr 1+ 0 do 1 rskip i 1+ . 0cr loop ;

\ session logging  

\ changes jeg9603; logon would not work before
\ This is a little subtle and probably not good. `logfilename' is
\ a high-level word, which indicates the logfile in the wings which
\ is activated by 'logon', which is also high-level. 'logoff',
\ whis is low-level, does not null logfilename. But there IS a 
\ low-level string, logfname, which is populated by 'logfile str'
\ and nulled by 'logoff'. This needs at some point to be fixed
\ at low level. We have also thought about a logfile stack, and
\ this needs to be thought about in this context.

create logfilename 100 allot
: logfile   bl cword dup logfilename cs! _logfile ; 
\ takes logfile name on input; logfile <cr> 
: logoff ["] " _logfile ;       \ closes logfile
: logon logfilename _logfile ;     \ reopens last logfile
: newlog logfilename unlink logfilename _logfile ; \ deletes and reopens 

0 warning !
: _logfile dup logfilename cs! _logfile ;

\  this word emits a cr if there is no logfile, otherwise just writes a
\  newline to the screen; avoids blank lines at the beginning of logfiles,
\  which can be a problem if they are machine-read.
: nologcr 
    logfname c@ 0= if 
        cr
    else
        \n scrput        
    then
;

: nlcr nologcr ;

\ this word emits a cr and a backslash-space combination -- for comment lines
: ccr cr ." \ " ;

\ this word emits a cr and a '#'-space combination -- for unix comment lines
: pcr cr ." # " ; 

: nlccr
    logfname c@ 0= if
        ccr
    else
        \n scrput ["]  \" scrputs
    then
;
                            

1 warning !


\ *******  MEMORY ALLOCATION; SEE MIRELMEM.C

: memsptr  ( size i:name -- msptr )
    bl cword swap _memalloc ;  \ compiles nothing into dictionary; ??????
            \ leaves str. pointer on stack; name is 'name'. Useless.

alias _memsptr _memalloc \ for consistency (?)

: msalloc  ( size -- msptr )
    0 swap _memalloc ;  \ compiles nothing into dictionary;
            \ leaves str. pointer on stack; name is 'tn'
                         
: memfree  ( i:name -- ) \ frees 'name'; to free by str ptr, 'ptr chfree'
    bl cword _memfree ;

: scralloc ( size -- msptr )
    ["] " swap _memalloc ;       \ allocate with null name; def to "tn" 

: crealloc  ( size in:name --- msptr )    \ allocates size bytes to name
    create here na1- >name   \ retrieve name: forth string !!, so
    fspush swap _memalloc  
;  \ leaves mem structure pointer on stack !!

: memalloc  ( comp: size in:name --- ) ( ex: --- addr )   
    crealloc ,        \ compile memstrptr
    does> @@          \ pushes allocated ptr at execution
;         

\ CAREFUL-- do-tokens here must agree with definitions in mirellin, dotokens.h
: nmarray  ( comp: nelt i:name --- ) ( ex: index --- addr )
    /n * dup crealloc DONMAR here body> ! swap , , 
;    \ creates normal arrays in heap

: smarray  ( comp: nelt in:name --- ) ( ex: index --- addr )
    /s * dup crealloc DOSMAR here body> ! swap , ,
;    \ creates short arrays in heap

: cmarray  ( comp: nelt in:name --- ) ( ex: index --- addr )
    dup crealloc DOCMAR here body> ! swap , ,
;    \ creates byte arrays in heap

: strucmarray ( comp: elsize nelt in:name --- ) ( ex: index --- base_addr )
    2dup * crealloc DOSTRUCCAR here body> !  \ allocate, place do-token
    -rot , , ,   \ place nelt, elsize, pointer
;    \ creates structure arrays in heap

\ ***************** MATRIX STUFF ******************************     

\ 2-d matrix size stuff; matrix area has row ptr array at beginning

: nmatneed ( nrow ncol --- nbyte) 
    over * n* swap na+ ( howmany do we need for a normal matrix ? )
;

: smatneed ( nrow ncol --- nbyte)
    over * 2* swap na+  ( howmany do we need for a short matrix ?)
;

: cmatneed ( nrow ncol --- nbyte)
    over * swap na+     ( howmany do we need for a char matrix ?)
;

\ 3-d matrix allocation stuff. matrix area has matptr array at beginning, 
\ followed by nmat arrays of nrow ptrs each, followed by data

: n3matneed ( nmat nrow ncol --- nbyte )
    2 pick 2 pick * * n* p>t            \ data bytes sizeof(normal) is data size
    over *                              \ # row pointers 
    swap +                              \ plus # matrix pointers
    n*                                  \ bytes
    t>p +                               \ plus data
;

: s3matneed ( nmat nrow ncol --- nbyte )
    2 pick 2 pick * * 2* p>t            \ data bytes 2 is data size
    over *                              \ # row pointers 
    swap +                              \ plus # matrix pointers
    n*                                  \ bytes
    t>p +                               \ plus data
;

: c3matneed ( nmat nrow ncol --- nbyte )
    2 pick 2 pick * * p>t               \ data bytes
    over *                              \ # row pointers 
    swap +                              \ plus # matrix pointers
    n*                                  \ bytes
    t>p +                               \ plus data
;

\ 2-d matrix pointer array setup

: nmatptr ( nrow ncol addr --- nrow)    \ creates set of row pointers at addr
    2 pick 0 do                         \ make ptrs for each row 
        2dup swap i  * na+ 3 pick na+   \ pointer=addr + i*ncol*4 + nrow*4
        over i na+ !                    \ addr of pointer  
    loop 
    2drop                               \ leaves nrow on stack !!!
;

: smatptr ( nrow ncol addr --- nrow)    \ creates set of row pointers at addr
    2 pick 0 do                         \ make ptrs for each row 
        2dup swap i  * sa+ 3 pick na+   \ pointer=addr + i*ncol*2 + nrow*4
        over i na+ !                    \ addr of pointer  
    loop 
    2drop                               \ leaves nrow on stack !!!
;

: cmatptr ( nrow ncol addr --- nrow)    \ creates set of row pointers at addr
    2 pick 0 do                         \ make ptrs for each row 
        2dup swap i  * + 3 pick na+     \ pointer=addr + i*ncol + nrow*4
        over i na+ !                    \ addr of pointer  
    loop 
    2drop                               \ leaves nrow on stack !!!
;


\ 3-d pointer array setup  
\ this should be hierarchible.....
\ With the current (060403 definition of the memstruct array, these
\ words should leave sizn, sizn-1 .... siz1 on the stack; siz1 is
\ stored first, and then recursively down.

: n3matptr ( nmat nrow ncol addr --- nmat )
    lvar addr addr !
    lvar ncol ncol !
    lvar nrow nrow !
    lvar nmat nmat !
    lvar raddr nmat @ n* addr @ + raddr !  \ start of row pointer arrays
    lvar daddr nmat @ nrow @ * n* raddr @ + daddr !  \ data offset
    nmat @ 0 do
        \ first do matrix pointers
        i nrow @ * n* raddr @  +    \ addr of ith row ptr array
        addr @ i na+ !              \ put it in matrix ptr array
        \ now do row pointers
        nrow @ 0 do
            j nrow @ ncol @ * *     \ offset to jth matrix
            i ncol @ * +            \ add'l offset to ith row
            n* daddr @ +            \ 4 is data size
            addr @ j na+ @ i na+ !  \ addr of j,ith row
        loop
    loop
    nrow @ nmat @                   \ leaves nrow nmat on stack     
;

: s3matptr ( nmat nrow ncol addr --- nmat )
    lvar addr addr !
    lvar ncol ncol !
    lvar nrow nrow !
    lvar nmat nmat !
    lvar raddr nmat @ n* addr @ + raddr !  \ start of row pointer arrays
    lvar daddr nmat @ nrow @ * n* raddr @ + daddr !  \ data offset
    nmat @ 0 do
        \ first do matrix pointers
        i nrow @ * n* raddr @  +    \ addr of ith row ptr array
        addr @ i na+ !              \ put it in matrix ptr array
        \ now do row pointers
        nrow @ 0 do
            j nrow @ ncol @ * *     \ offset to jth matrix
            i ncol @ * +            \ add'l offset to ith row
            2* daddr @ +            \ 2 is data size
            addr @ j na+ @ i na+ !  \ addr of j,ith row
        loop
    loop
    nrow @ nmat @                   \ leaves nrow nmat on stack     
;

: c3matptr ( nmat nrow ncol addr --- nmat )
    lvar addr addr !
    lvar ncol ncol !
    lvar nrow nrow !
    lvar nmat nmat !
    lvar raddr nmat @ n* addr @ + raddr !  \ start of row pointer arrays
    lvar daddr nmat @ nrow @ * n* raddr @ + daddr !  \ data offset
    nmat @ 0 do
        \ first do matrix pointers
        i nrow @ * n* raddr @  +    \ addr of ith row ptr array
        addr @ i na+ !              \ put it in matrix ptr array
        \ now do row pointers
        nrow @ 0 do
            j nrow @ ncol @ * *     \ offset to jth matrix
            i ncol @ * +            \ add'l offset to ith row
            daddr @ +               \ data size is 1 byte, so no mul
            addr @ j na+ @ i na+ !  \ addr of j,ith row
        loop
    loop
    nrow @ nmat @                   \ leaves nrow nmat on stack
;

         
\ code for 2-d matrices in heap       

\ normal matrices in heap

: nmmatrix ( comp: nrow ncol i:name ---) ( exe: row col --- addr )
    2dup nmatneed crealloc      \ create entry; memstrc ptr on stack
    DONMMAT here body> !        \ compile token
    >r 2dup , , r>              \ compile nrows, ncols
    dup dup p>t ,               \ compile pointer, save copy on tstack
    @ nmatptr                   \ set up pointer array; nrows on stack
    4 t>p 2 na+ dup -rot w!     \ store elsize in memstr, addr of elsize on stk
    sa1+ w!                     \ store nrows, next short addr in memstruct 
;  

    \ short matrices in heap
: smmatrix ( comp: nrow ncol i:name ---) ( exe: row col --- addr )
    2dup smatneed crealloc      \ create entry; memstrc ptr on stack
    DOSMMAT here body> !        \ compile token
    >r 2dup , , r>              \ compile ncols, nrows
    dup dup p>t ,               \ compile pointer
    @ smatptr                   \ set up pointer array, nrows on stk
    2 t>p 2 na+ dup -rot w!     \ store elsize in memstr, addr of elsize on stk
    sa1+ w!                     \ store nrows, next short addr in memstruct 
; 

    \ u_char matrices in heap
: cmmatrix ( comp: nrow ncol i:name ---) ( exe: row col --- addr )
    2dup cmatneed crealloc      \ create entry; memstrc ptr on stack
    DOCMMAT here body> !        \ compile token
    >r 2dup , , r>              \ compile ncols, nrows
    dup dup p>t ,               \ compile pointer
    @ cmatptr                   \ set up pointer array, nrows on stk
    1 t>p 2 na+ dup -rot w!     \ store elsize in memstr, addr of elsize on stk
    sa1+ w!                     \ store nrows, next short addr in memstruct 
;  

\ code for 3-d matrices in heap  \ jeg060402

: n3mmatrix ( comp: nmat nrow ncol i:name --- ) ( exe: mat row col --- addr )
    3dup n3matneed crealloc     \ create entry; memstruc ptr on stack
    DON3MMAT here body> !       \ compile token
    >r 3dup , , , r>            \ compile nmat, ncol, nrow
    dup dup p>t ,               \ compile pointer, copy on stack
    @ n3matptr                  \ set up pointer array, nrows nmat on stk
    4 t>p 2 na+ dup -rot w!     \ store elsize in memstr, addr of elsiz on stk
    sa1+ dup -rot w!            \ store nmat im memstr, addr of nmat on stk
    sa1+ w!                     \ store nrow in memstr
;

: s3mmatrix ( comp: nmat nrow ncol i:name --- ) ( exe: mat row col --- addr )
    3dup s3matneed crealloc     \ create entry; memstruc ptr on stack
    DOS3MMAT here body> !       \ compile token
    >r 3dup , , , r>            \ compile nmat, ncol, nrow
    dup dup p>t ,               \ compile pointer, copy on stack
    @ s3matptr                  \ set up pointer array, nrows nmat on stk
    2 t>p 2 na+ dup -rot w!     \ store elsize in memstr, addr of elsiz on stk
    sa1+ dup -rot w!            \ store nmat im memstr, addr of nmat on stk
    sa1+ w!                     \ store nrow in memstr
;

: c3mmatrix ( comp: nmat nrow ncol i:name --- ) ( exe: mat row col --- addr )
    3dup c3matneed crealloc     \ create entry; memstruc ptr on stack
    DOC3MMAT here body> !       \ compile token
    >r 3dup , , , r>            \ compile nmat, ncol, nrow
    dup dup p>t ,               \ compile pointer, copy on stack
    @ c3matptr                  \ set up pointer array, nrows nmat on stk
    1 t>p 2 na+ dup -rot w!     \ store elsize in memstr, addr of elsiz on stk
    sa1+ dup -rot w!            \ store nmat im memstr, addr of nmat on stk
    sa1+ w!                     \ store nrow in memstr
;


\ TODO----ADD OTHER KINDS OF 3D MATRICES, AND ADD CODE FOR 4D MATRICES
\ ADD CODE IN MIRELMEM.C FOR 3 AND 4D MATRICES AND IN MINSTALL


    \ The following three words are links to matrices dynamically allocated
    \ in C code; if there is already a dictionary entry of the requested
    \ type of the given name, they modify it to the current call and attempt
    \ to free any associated memory; if there is not one, they create one; 
    \ if the name is in use for somthing else, they belly up. They
    \ are useful for functions which create matrices. The forth code
    \ produced is the same as for Minstalled matrices, but can be produced
    \ dynamically

    \ normal matrices in heap from already allocated pointer in C prog.
: tnmmatrix ( comp: sptr nrow ncol i:name ---) ( exe: row col --- addr )
    bl word current @ vfind if      \ is there an entry 'name'
        dup @ DONMMAT = not if      \ is name an nmatrix entry ?
            2drop 2drop  
            ["]   Not an nmatrix" erret
        else                        \ it is, so
            dup 3 na+ @ chfree      \ free its existing memsptr
            na1+ tuck ! na1+ tuck ! na1+ !   
                                    \ place ncol, nrow, ptr
        then
    else                            \ there isn't, so make one
        "create                     \ create entry
        DONMMAT here body> !        \ compile token
        , , ,                       \ compile ncols, nrows, pointer
    then
;  

    \ short matrices in heap from already allocated pointer in C
: tsmmatrix ( comp: sptr nrow ncol i:name ---) ( exe: row col --- addr )
    bl word current @ vfind if      \ is there an entry 'name'
        dup @ DOSMMAT = not if      \ is name an smatrix entry ?
            2drop 2drop  
            ["] not an smatrix" erret
        else                        \ it is, so
            dup 3 na+ @ chfree      \ free its existing memsptr
            na1+ tuck ! na1+ tuck ! na1+ !   
                                    \ place ncol, nrow, ptr
        then
    else                            \ there isn't, so make one
        "create                     \ create entry
        DOSMMAT here body> !        \ compile token
        , , ,                       \ compile ncols, nrows, pointer
    then
;  

    \ char matrices in heap from already allocated pointer in C
: tcmmatrix ( comp: sptr nrow ncol i:name ---) ( exe: row col --- addr )
    bl word current @ vfind if      \ is there an entry 'name'
        dup @ DOCMMAT = not if      \ is name a cmatrix entry ?
            2drop 2drop  
            ["] not a cmatrix" erret
        else                        \ it is, so
            dup 3 na+ @ chfree      \ free its existing memsptr
            na1+ tuck ! na1+ tuck ! na1+ !   
                                    \ place ncol, nrow, ptr
        then
    else                            \ there isn't, so make one
        "create                     \ create entry
        DOCMMAT here body> !        \ compile token
        , , ,                       \ compile ncols, nrows, pointer
    then
;  

    \ normal matrices in dictionary

: nmatrix ( comp: nrow ncol i:name ---) ( exe: row col --- addr )
    2dup create                 \ create entry
    DONMAT here body> !         \ compile token
    2dup , ,                    \ compile ncols, nrows
    here p>t                    \ save here
    nmatneed clallot            \ allocate & clr space
    t>p nmatptr drop            \ create pointer arrays
;  

    \ short matrices in dictionary
: smatrix ( comp: nrow ncol i:name ---) ( exe: row col --- addr )
    2dup create                 \ create entry
    DOSMAT here body> !         \ compile token
    2dup , ,                    \ compile ncols, nrows
    here p>t                    \ save here
    smatneed clallot            \ allocate & clr space
    t>p smatptr drop            \ create pointer arrays
;  

    \ char matrices in dictionary
: cmatrix ( comp: nrow ncol i:name ---) ( exe: row col --- addr )
    2dup create                 \ create entry
    DOCMAT here body> !         \ compile token
    2dup , ,                    \ compile ncols, nrows
    here p>t                    \ save here
    cmatneed clallot            \ allocate & clr space
    t>p cmatptr drop            \ create pointer arrays
;  


    \ words below return address of base of pointr array for any kind of
    \ matrix.  The 3 / is pretty stupid, IMHO, just to get rid of some
    \ cases...but it works. NB!! do not really need a' words--could just add
    \ code here for all arrays of any dimensionality or type.
    
DONMAT   3 / constant DOM3     \ 2d matrices
DONCMAT  3 / constant DOCM3
DONMMAT  3 / constant DOMM3
DON3MAT  3 / constant DO3M3    \ 3d matrices
DON3CMAT 3 / constant DO3CM3
DON3MMAT 3 / constant DO3MM3

: _m' ( cfa -- addr of base of ptr array of matrix)
    dup @ 3 / case                    \ token/3
        DOM3   of 3 na+      endof    \ DONMAT/3
        DOCM3  of 3 na+ @@   endof    \ DONCMAT/3
        DOMM3  of 3 na+ @@   endof    \ DONMMAT/3
        DO3M3  of 4 na+      endof    \ DON3MAT/3
        DO3CM3 of 4 na+ @@   endof    \ DON3CMAT/3
        DO3MM3 of 4 na+ @@   endof    \ DON3MMAT/3
        dup cr 3 * . ." is not a matrix token" 1 erret
    endcase
;

: m'  ( in:name -- addr of base of ptr array of matrix)
    ' _m'
;

: (m' ( -- addr of base of ptr array; token to be compiled for [m'] )
    r> dup @ _m' swap /token + >r   \ this is executed at EXECUTION time; the
                                    \ only address in the dictionary is the 
                                    \ cfa; see [m'] below
;

: [m'] compile (m' ' ,  ;  immediate    \ compiling word

\ words below interrogate and set matrix dimensions; the user is responsible
\ for not exceeding the originally allocated dimensions..EITHER ONE !!!!

: @nrcol ( cfa --- nrow ncol ) dup 1 na+ @ swap 2 na+ @ ;
: !nrcol ( cfa nrow ncol --- ) >r over 2 na+ r> swap ! swap na1+ ! ;

\ routine to 2D dump matrices. it is your responsibility to be sure your screen
\ is big enough. fldwid and prec are the args for f.r, which this word
\ uses

: mdump ( fldwid.i prec.i cfa.p )
    lvar cfa cfa !
    lvar prc prc !
    lvar wid wid !
    lvar n cfa @ 1 na+ @ n !  \ # rows
    lvar m cfa @ 2 na+ @ m !  \ # columns
    cr
    n @ 0 do
        cr
        m @ 0 do 
            j i cfa @ execute f@ wid @ prc @ f.r
        loop
    loop
;

\ ************ ARRAY STUFF **********************************
\ words below return the address 0th element of an array; they do the
\ same thing as 0 arrayname less efficiently, but are added for symmetry 
\ with the matrix words

DONAR   3 / constant DOA3
DONCAR  3 / constant DOCA3
DONMAR  3 / constant DOMA3
DONCPAR 3 / constant DOCPA3

: _a' ( cfa -- addr of 0th element of array )
    dup @  3 / case               \ (token)/3
        DOA3   of 2 na+      endof   \ DONAR/3
        DOCA3  of 2 na+ @    endof   \ DONCAR/3   
        DOMA3  of 2 na+ @@   endof   \ DONMAR/3
        DOCPA3 of 2 na+ @@ @ endof   \ DONCPAR/3
        dup cr 3 * . ."  is not an array token" 1 erret
    endcase
;

: a' ( in:name -- addr of 0th elt of array ) ' _a' ;

: (a' ( -- addr of 0th elt of array; token to be compiled for [a'] )
    r> dup @ _a' swap /token + >r
;

: [a'] compile (a' ' ,  ;  immediate    \ compiling word



\ *****  STRING WORDS FOR 'PRINTING' TO STRINGS ***********

: nsct ( str1 str2 n -- ) \ cats at most n chars of str2 onto str1, and
                          \ fills rest with blanks
    2 pick dup strlen +   \ address of start of cat
    dup p>t               \ save a copy
    over 2dup blank + 0 swap c!    \ fill with blanks and terminate
    over strlen min t>p swap strncpy drop
                            \ cat the string, min of n chars or length 
;
        
: nct ( str n --- )     \ prints at most n chars of s, fills with blanks
    over strlen over - negate 0 max p>t   \ number of blanks  
    over strlen min    \ # of chars of str to print
    0 ?do 
        dup i + c@ emit 
    loop drop
    t>p spaces 
;

: sf.rc ( str fl wid dec --- )  \ cats the sf.r string onto str 
    sf.r strcat 
;

: s.rc  ( str int wid --- )  \ cats the s.r string onto str 
    s.r strcat
;

\ ********************* GRAPHICS STUFF ******************************
1 
#if
    \ various bits useful for graphics
: draw 1 ldraw ;
: penmove 0 ldraw ;
: gsfill g_symb @ 64 or 32 not and g_symb ! ;
: gsempty g_symb @ 64 not and 32 not and g_symb ! ;
: gsinvert g_symb @ 16 or g_symb ! ;
: gsupright g_symb @ 16 not and g_symb ! ;
: gsstar g_symb @ 32 or 64 not and g_symb ! ;
: gspoly g_symb @ 32 not and g_symb ! ;
: gssize g_ssize ! ;
: gspoints 15 and g_symb @ 15 not and or g_symb ! ;
: spenmove 0 msdraw ;
: sdraw 1 msdraw ;


\ : ingraph    bl cword swap _ingraph ;
\ : outgraph   bl cword _outgraph ;

: invector   bl cword swap _invector ;
: outvector  bl cword _outvector ;

: writeit g_angle f@ g_magn f@ gstring ;

: standardgraph 1 g_stdflg ! -1 dup g_xdec ! g_ydec ! ; 
    \ returns to standard plot mode

\ some words to manipulate graphics modes

 p' +mvgraph 6 5000 strucparray mvgraph
 
\ code to graph arbitrary (float) functions
\ deprecated, see newfngraph, fngraph below

: grafunc ( cfa npt xlo xhi nxdiv --- )
    lvar nxdiv nxdiv !
    lvar xhi xhi f!
    lvar xlo xlo f!
    lvar npt npt !
    lvar cfa cfa !
    lvar delx
    lvar buf
    lvar bufptr

    xhi f@ xlo f@ f- npt @ float f/ delx f!
    xlo f@ xhi f@ nxdiv @ xscale
    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    npt @ 0 do
        i float delx f@ f* xlo f@ f+ fdup   \ x
        buf @ npt @ i + na+ f!       
        cfa @ execute buf @ i na+ f!     \ y  
    loop
    buf @ npt @ famaxmin 2drop 0 yscale
    gframe
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
;

: rgrafunc ( cfa xlo xhi npt --- ) \ assumes you have set up frame 
                                   \ xlo and xhi are evaluation limits,
                                   \ not necessarily the same as plot lims. 
    lvar npt npt !                 \ # plot points
    lvar xhi xhi f!     
    lvar xlo xlo f!
    lvar cfa cfa !
    lvar delx
    lvar buf
    lvar bufptr

    xhi f@ xlo f@ f- npt @ float f/ delx f!
    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    npt @ 0 do
        i float delx f@ f* xlo f@ f+ fdup   \ x
        buf @ npt @ i + na+ f!       
        cfa @ execute buf @ i na+ f!     \ y  
    loop
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
;



\ *********  COLOR GRAPHICS STUFF **************

14 constant G_Ncolors ( 14 colors at the moment)
create g_ccolor 3 allot
create g_ocolor 3 allot
: setcolor g_ccolor g_ocolor 3 cmove g_ccolor 3 crmove g_ccolor ilinecolor ;
: oldcolor g_ocolor g_ccolor 3 cmove g_ccolor ilinecolor ;

: g_black    0 0 0           setcolor ;
: g_blue     0 0 0xff        setcolor ;
: g_green    0 0xff 0        setcolor ;
: g_red      0xff 0 0        setcolor ;
: g_yellow   0xff 0xff 0     setcolor ;
: g_yellow2  0xff 0xc0 0     setcolor ;
: g_yellow3  0xe0 0xa0 0     setcolor ;
: g_cyan     0 0xff 0xff     setcolor ;
: g_magenta  0xff 0 0xff     setcolor ;
: g_grey     0x7f 0x7f 0x7f  setcolor ;
: g_white    0xff 0xff 0xff  setcolor ;
: g_violet   0x7f 0 0xff     setcolor ;
: g_brown    0xaf 0x5f 0     setcolor ;
: g_orange   0xff 0x4f 0     setcolor ;
: g_lgrey    0xaf 0xaf 0xaf  setcolor ;
: g_dgrey    0x3f 0x3f 0x3f  setcolor ;

create g_bkcolor 3 allot 0 0 0 g_bkcolor 3 crmove

: setbackground ( color.p --- ) \ sets background to proferred array
    24 G_bitpix = if
        g_bkcolor 3 move
        G_xsize 3 * 0 do
            g_bkcolor 0 i mgraph 3 move
        3 +loop
        G_ysize 0 do
            0 0 mgraph i 0 mgraph G_xsize 3 * move
        loop
    else
        drop
        cr ." NOT a color graph. Cannot do this"
        0 erret
    then
;

: graphbackground ( --- ) \ sets background to current color
    24 G_bitpix = if
        g_ccolor g_bkcolor 3 move
        G_xsize 3 * 0 do
            g_bkcolor 0 i mgraph 3 move
        3 +loop
        G_ysize 0 do
            0 0 mgraph i 0 mgraph G_xsize 3 * move
        loop
    else
        drop
        cr ." NOT a color graph. Cannot do this"
        0 erret
    then
;

: whitebackground
    24 G_bitpix = if
        3 0 do 0xff g_bkcolor i + c! loop
        G_xsize 3 * 0 do
            0xff 0 i mgraph c!
        loop
        G_ysize 0 do
            0 0 mgraph i 0 mgraph G_xsize 3 * move
        loop
    else
        cr ." NOT a color graph. Cannot do this"
        0 erret
    then
;    

: blackbackground
    24 G_bitpix = if
        3 0 do 0 g_bkcolor i + c! loop
        G_xsize 3 * 0 do
            0 0 i mgraph c!
        loop
        G_ysize 0 do
            0 0 mgraph i 0 mgraph G_xsize 3 * move
        loop
    else
        cr ." NOT a color graph. Cannot do this"
        0 erret
    then
;    


: callocgraph 0 8 gallocgraph ;  \ allocates 24-bit bitmap graph
: colarray 3 swap strucarray ;
G_Ncolors colarray stdcolors
G_Ncolors colarray scrncolarr
G_Ncolors colarray papercolarr

\ OK

0 0 0  
0 0 0xff  
0 0xff 0  
0xff 0 0   
0xff 0xff 0   
0 0xff 0xff  
0xff 0 0xff
0x7f 0x7f 0x7f  
0xff 0xff 0xff  
0x7f 0 0xff   
0xaf 0x5f 0  
0xff 0x4f 0
0xaf 0xaf 0xaf  
0x3f 0x3f 0x3f
0 scrncolarr G_Ncolors 3 * crmove

0xff 0xff 0xff  
0 0 0x7f  
0 0x7f 0  
0x7f 0 0  
0x7f 0x7f 0  
0 0x7f 0x7f 
0x7f 0 0x7f   
0x7f 0x7f 0x7f  
0 0 0 
0x7f 0 0xff   
0xaf 0x5f 0  
0xff 0x4f 0
0xaf 0xaf 0xaf  
0x3f 0x3f 0x3f
0 papercolarr G_Ncolors 3 * crmove

create colornames 128 allot
create papercolnames 128 allot
create scrncolnames 128 allot

" 0:blk 1:blu 2:grn 3:red 4:yel 5:cya 6:mag 7:wht 8:vio 9:brn 10:org 11:lgy 12:dgy" 
scrncolnames cs!
" 0:wht 1:blu 2:grn 3:red 4:org 5:cya 6:mag 7:blk 8:vio 9:brn 10:org 11:lgy 12:dgy" 
papercolnames cs!
: .colors colornames ct ;


0 ivar papercolflg

: papercolors  
    0 papercolarr 0 stdcolors G_Ncolors 3 * move papercolnames colornames cs!
    1 papercolflg !
;

: scrncolors 
    0 scrncolarr  0 stdcolors G_Ncolors 3 *  move scrncolnames colornames cs!
    0 papercolflg !
;

scrncolors


: setscolor \ std_color_#.i
    stdcolors ilinecolor 
;

#endif

\ ************ FUNCTION GRAPHING ******************************

\ These are (should be) more convenient function graphing words

100 constant Sfgpts  \ NB!!! play with this -- too big results in 
                      \ jagged plots bcz of roundoff. 100 *should*
                       \ be enough.

\ NB!! this does an egraph, so background is black. We need to preserve 
\ background  color when we egraph?? or just when we do a frame. HAVE to 
\ when we do a frame or we cannot get any background except black.
\ There was some peculiar namespace problem with _frame, in mirage,
\ which is an minstalled name for that field in the FITS header. so
\ g_frame. Should change the C name, which is frame.

: frame ( xlo.f xhi.f ndivx ylo.f yhi.f ndivy --- )
    egraph 
    g_bkcolor setbackground
    g_frame 
; 



: fasgraph ( cfa.p xlo.f xhi.f --- )  \ function autoscale
    lvar xhi xhi f!
    lvar xlo xlo f!
    lvar cfa cfa !
    lvar npt Sfgpts 1+ npt !
    lvar delx
    lvar buf
    lvar bufptr
    
    Gchan -1 = if 
        cr ." There is no graph allocated or set."
        erret
    then 

    g_white
    egraph
    gbegin
    xhi f@ xlo f@ f- npt @ float f/ delx f!
    xlo f@ xhi f@ 0  xscale
    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    npt @ 0 do
        i float delx f@ f* xlo f@ f+ fdup   \ x
        buf @ npt @ i + na+ f!       
        cfa @ execute buf @ i na+ f!     \ y  
    loop
    buf @ npt @ famaxmin 2drop 0 yscale
    gframe
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
    gend
;

\ compact word to graph a function, fully specified

: gfgraph ( cfa.p xlo.f xhi.f nx.i ylo.f yhi.f ny.i )
    lvar ny ny !
    lvar yhi yhi f!
    lvar ylo ylo f!
    lvar nx nx !
    lvar xhi xhi f!
    lvar xlo xlo f!
    lvar cfa cfa !
    
    lvar npt Sfgpts 1+ npt !
    lvar delx
    lvar buf
    lvar bufptr
    
    Gchan -1 = if 
        cr ." There is no graph allocated or set."
        erret
    then 

    g_white
    egraph
    gbegin
    xhi f@ xlo f@ f- npt @ float f/ delx f!
    xlo f@ xhi f@ nx @  xscale
    ylo f@ yhi f@ ny @  yscale
    gframe
    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    npt @ 0 do
        i float delx f@ f* xlo f@ f+ fdup   \ x
        buf @ npt @ i + na+ f!       
        cfa @ execute buf @ i na+ f!     \ y  
    loop
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
    gend
;


\ NB!!! there is a C word fgraph which plots a float array vs its
\ index. This has been changed IN FORTH to figraph, with similar
\ entites nigraph and sigraph. Probably should be changed in C.

: fgraph ( xlo.f xhi.f cfa.p --- ) \ assumes you have set up frame 
                              \ xlo and xhi are evaluation limits,
                              \ not necessarily the same as plot lims. 
                              \ # plot points. Do a g_color if you
                              \ want a different color.
                              \ ALL this does is plot the function
    lvar cfa cfa !
    lvar xhi xhi f!
    lvar xlo xlo f!
    lvar npt Sfgpts 1+ npt !
    lvar delx
    lvar buf
    lvar bufptr

    Gchan -1 = if 
        cr ." There is no graph allocated or set."
        erret
    then 

    xhi f@ xlo f@ f- npt @ 1- float f/ delx f! \ npt -1 spaces, npt points
    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    npt @ 0 do
        i float delx f@ f* xlo f@ f+ fdup   \ x
        buf @ npt @ i + na+ f!       
        cfa @ execute buf @ i na+ f!     \ y  
    loop
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
;


: sfgraph ( cfa --- )         \ simple function grapher
                              \ assumes you have set up frame 
                              \ plots Sfgpts points. Do a g_color if you
                              \ want a different color.
                              \ ALL this does is plot the function
                              \ plots to plotting limits
    lvar cfa cfa !
    lvar xhi g_fxhi  f@ xhi f!
    lvar xlo g_fxorg f@ xlo f!
    lvar npt Sfgpts 1+ npt !
    lvar delx
    lvar buf
    lvar bufptr

    Gchan -1 = if 
        cr ." There is no graph allocated or set."
        erret
    then 

    xhi f@ xlo f@ f- npt @ float f/ delx f!
    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    npt @ 0 do
        i float delx f@ f* xlo f@ f+ fdup   \ x
        buf @ npt @ i + na+ f!       
        cfa @ execute buf @ i na+ f!     \ y  
    loop
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
;



: g_circle ( xc.f yc.f r.f --- ) \ draws a circle
    lvar r r f!
    lvar yc yc f!
    lvar xc xc f!
    lvar npt r f@ g_fxhi f@ g_fxorg f@ f- f/ 1000. f* round npt !
    lvar buf
    lvar bufptr
    lvar dthet
    lvar theta
    
    Gchan -1 = if 
        cr ." There is no graph allocated or set."
        erret
    then 

    npt @ 2* 4* msalloc dup bufptr ! ( msptr) @ buf ! ( memptr)
    2pi npt @ float f/ dthet f!
    npt @ 0 do
        i float dthet f@ f* theta f!
        theta f@ cos r f@ f* xc f@ f+  \ x
        buf @ npt @ i + na+ f!       
        theta f@ sin r f@ f* yc f@ f+
        buf @ i na+ f!     \ y  
    loop
    buf @ dup npt @ na+ swap npt @ ffgraph
    bufptr @ chfree
;



    
\ ************* SOME DIRECTORY STUFF *********************

\ we could add more things, but we need these

: cdmirella  ["] MIRELLADIR" _getenv _pushd ;
: mirelladir ["] MIRELLADIR" _getenv ;
: cdmirage   ["] MIRAGEDIR"  _getenv _pushd ;
: miragedir  ["] MIRAGEDIR"  _getenv ;
: cdvtra     ["] VTRADIR"    _getenv _pushd ;
: vtradir    ["] VTRADIR"    _getenv ;


: pushd bl cword _pushd showdirs ;
: cd bl cword _pushd showdirs ;
\ todo: recognize ~/ in _pushd; make _mpushd
: mkdir bl cword _mkdir ;
: rmdir bl cword _rmdir ;
: pwd space getcwd ct ;
: home home_dir _pushd showdirs ;
warning @ 0 warning !
: popd popd showdirs ;
: pushd pushd showdirs ;
: rolld rolld showdirs ;
warning !
alias dirs showdirs
alias pd pushd

\ *********** SOME TIME CODE ************************

begstruct     \ struct mir_tm; global pointers are m_ltime, m_altime
    +offset tm_sec	Isize
    +offset tm_min	Isize
    +offset tm_hour	Isize
    +offset tm_mday	Isize
    +offset tm_mon	Isize
    +offset tm_year	Isize
    +offset tm_wday	Isize
    +offset tm_yday	Isize
    +offset tm_isdst	Isize
    +offset tm_zone	Isize
    +offset tm_gmtoff	Isize
endstruct


: gettimes  getltime drop ;              \ just evaluates struct and m_ytime
: .ltime    getltime ct ;                \ prints time and date in Unix form
: getytime  getltime drop m_ytime @ ;    \ pushes deciseconds since year end
: .ms stopwatch . ;
: .sw stopwatch float 1000. f/ cr ." Time = " 5 2 f.r ."  seconds" ;
: sw  startwatch ;


\ time/datestring words

create ms_datestr 64 allot

: datestr \ local ( format yymmdd )
    lvar d   
    lvar m
    lvar y 

    getltime drop
    m_ltime tm_mon @ 1+ m !
    m_ltime tm_year @ 100 - y !
    m_ltime tm_mday @ d !
    y @ 2 zitoa ms_datestr cs!
    m @ 2 zitoa ms_datestr cs+!
    d @ 2 zitoa ms_datestr cs+!
    ms_datestr
;

: timestr \ local  ( format yymmdd_hhmmss )
    datestr drop
    ["] _" ms_datestr cs+!
    m_ltime tm_hour @ 2 zitoa ms_datestr cs+!
    m_ltime tm_min  @ 2 zitoa ms_datestr cs+!
    m_ltime tm_sec  @ 2 zitoa ms_datestr cs+!
    ms_datestr
;

: udatestr \ UTC ( format yymmdd )
    lvar d   
    lvar m
    lvar y 

    getutime drop
    m_ltime tm_mon @ 1+ m !
    m_ltime tm_year @ 100 - y !
    m_ltime tm_mday @ d !
    y @ 2 zitoa ms_datestr cs!
    m @ 2 zitoa ms_datestr cs+!
    d @ 2 zitoa ms_datestr cs+!
    ms_datestr
;

: utimestr \ UTC  ( format yymmdd_hhmmss )
    udatestr drop
    ["] _" ms_datestr cs+!
    m_ltime tm_hour @ 2 zitoa ms_datestr cs+!
    m_ltime tm_min  @ 2 zitoa ms_datestr cs+!
    m_ltime tm_sec  @ 2 zitoa ms_datestr cs+!
    ms_datestr
;

: date/str \ local  ( format with '\' )
    lvar d   
    lvar m
    lvar y 

    getltime drop
    m_ltime tm_mon @ 1+ m !
    m_ltime tm_year @ 100 - y !
    m_ltime tm_mday @ d !
    y @ 2 zitoa ms_datestr cs!
    ["] /"      ms_datestr cs+!
    m @ 2 zitoa ms_datestr cs+!
    ["] /"      ms_datestr cs+!
    d @ 2 zitoa ms_datestr cs+!
    ms_datestr
;

: udate/str  \ UTC
    lvar d   
    lvar m
    lvar y 

    getutime drop
    m_ltime tm_mon @ 1+ m !
    m_ltime tm_year @ 100 - y !
    m_ltime tm_mday @ d !
    y @ 2 zitoa ms_datestr cs!
    ["] /"      ms_datestr cs+!
    m @ 2 zitoa ms_datestr cs+!
    ["] /"      ms_datestr cs+!
    d @ 2 zitoa ms_datestr cs+!
    ms_datestr
;
                                           
\ ************ WORDS TO MAKE SPLINE FUNCTIONS, TABULAR FNS IN GENERAL *******

begstruct     \ struct ftab_t
    +offset ft_xptr     Isize
    +offset ft_yptr     Isize
    +offset ft_kptr     Isize
    +offset ft_n        Isize
    +offset ft_ord      Isize
    +offset ft_minx     Fsize
    +offset ft_maxx     Fsize
    +offset ft_ser      Isize
endstruct  \ ftab_t

: maketabfun ( comp:  ftab_addr i:name --- ) ( exec: farg --- fval )
    \ usage: file xcol ycol splinflg filefnarray maketabfun name
    create ,
does> 
    @ dotabfun
;

\ the parameter address field of a tabfunction contains the ADDRESS of
\ the ftab_t structure pertinent to the function.

alias mtfun maketabfun \ shorter, nearly always useful
: fmtfun filefnarray maketabfun ; 
: sfmtfun 1 filefnarray maketabfun ; \ splined
: lfmtfun 0 filefnarray maketabfun ; \ linear interp.

: fmtxmin ( cfa --- xmin )     na1+ @ ft_minx f@ ;
: fmtxmax ( cfa --- xmax )     na1+ @ ft_maxx f@ ;
: fmtn    ( cfa --- npt  )     na1+ @ ft_n    @ ;
: fmtx    ( cfa --- addr_of_x) na1+ @ ft_xptr @ ;
: fmty    ( cfa --- addr_of_y) na1+ @ ft_yptr @ ;
: fmtk    ( cfa --- addr_of_k) na1+ @ ft_kptr @ ;

\ TODO: handle the tables via the Mirella memory allocation, and figure
\ out some way to free it if need be.... I think we need to do the dictionary
\ entries via the function names, so it can be freed automatically when
\ reallocated. Write some associated graphics code a la SM.


\ ************** REGRESSION STRUCTURE *********************

begstruct     \ struct regress_t; regress(), sregress() returns result in 
               \ static regstat
    +offset regr_m      Fsize
    +offset regr_b      Fsize
    +offset regr_corr   Fsize
    +offset regr_sig    Fsize
    +offset regr_n      Isize
endstruct

\ histogram analysis structure

begstruct      \ struct hstat_t; stathist returns result in static hist_stat
    +offset h_mean      Fsize
    +offset h_sigma     Fsize
    +offset h_mode      Fsize
    +naroff h_qt        Fsize 3 *
endstruct    


: fclip ( arr.p npt.i sigclip.f --- mean.f sigma.f ngood.i )
    lvar ngood
    lvar mean
    lvar sigma
    mean sigma ngood 0 _mfclip
    mean f@ sigma f@ ngood @
;

\ ************ FROM HERE DOWN IS A *MESS*. RATIONALIZE, PLEAE **********

\ word to forget startup code

: sforget 
    ["] startup" 1-  current @ vfind 
    if 
        (forget 
    else 
        drop 
    then 
;

\ ******************** MIRELLA DIRECTORY STUFF ****************************

begstruct  \ struct dir_t
    +offset dir_path    64
    +offset dir_ftime   Isize
    +offset dir_fmode   Isize
    +offset dir_size    Isize
    +offset dir_att     12
    +offset dir_tstr    20
    +offset dir_dir     48
    +offset dir_ftype   Isize
    +offset dir_reclen  Isize
endstruct

create dirbuffer 200 allot

: getstat bl cword dirbuffer _getstat ;

: isdir bl cword _isdir ;
 
defer direxec

: _dirdo ( pattern cfa )   \ does cfa (which must take a struct dir_t * as 
                            \ input and leave nothing on the stack to each file
                            \ matching 'pattern'
    is direxec                          
    dscan
    -1 = if exit then
    begin
        nextfile dup if dup direxec then 
        pause
    0= until
;

: dirdo ';' cword swap _dirdo ;


: listit 
    dup cr 30 ct.r
    dup dir_size @ 7 .r 2 spaces
    dup dir_tstr ct  2 spaces
    dir_att ct
;

\ mirella `ls' -- this word is broken ???? No?????
: lsm 
    ';' cword 
    pad cs!

\    pad cr ct \ debug
    
    pad baseptr c@ 0= if pad ["] *" cs+! then
    pad ['] listit _dirdo 
;        


\ ************ MIRELLA BACKUP/RESTORE STUFF (DELETE??) *********************

: b_normal   \ restores normal backup flags
    0 b_backstream !    \ do not make stream files from vms files
    0 b_askflg !        \ do not ask about overwriting later versions
    0 b_forceflg !      \ do not force overwriting later versions
    0 b_listflg !       \ do not list only
    1 b_stoansi !       \ convert ascii stream files to ansi text files if vms
;
    

: backup ( i:pattern --- ) ['] dsbackup dirdo b_normal ;

: restore 1 ';' cword dsrestore b_normal ; \ does not overwrite later versions
: listback 1 b_listflg ! restore ;  \ list contents only
: ?restore 1 b_askflg ! restore ;   \ asks about overwriting later versions    
: frestore 1 b_forceflg ! restore ; \ forces restore regardless of file date


: endbackup  freeback end-file ;
: endrestore freeback ;


\ Mirella terminal stuff. the fterm structure came with rhl's editor,
\ and is not really useful in Forth, since it mostly contains a function
\ dispatch table. We need them both, but be careful to keep them
\ consistent, or, better,  merge them (ugh!!)

\ FTERMINAL description
begstruct
    +offset  t_fname     28     \ name of terminal
    Nsize Isize - +             \ pad to Normal alignment
    +offset  fdel_char   Psize  \ function to delete previous char  
    +offset  fdel_line   Psize  \ delete to the end of the current line  
    +offset  fforw_curs  Psize  \ move cursor forward  
    +offset  fback_curs  Psize  \ move cursor backward  
    +offset  fput_curs   Psize  \ direct cursor addr  
    +offset  fnum_kpd    Psize  \ put keypad in numeric mode  
    +offset  fapp_kpd    Psize  \ put keypad in 'appl' mode  
    +offset  fquery_curs Psize  \ query cursor position  
    +offset  fsav_curs   Psize  \ save cursor position +offseternally  
    +offset  fres_curs   Psize  \ restore cursor after last command  
    +offset  fclr_scr    Psize  \ clear screen  
    +offset  fscrnprt    Psize  \ writes a string to the screen at cur curs 
    +offset  fscrnput    Psize  \ writes a char "              "            
    +offset  fscrnflsh   Psize  \ flushes the buffers to the screen if appl
    +offset  t_ncols     Isize  \ how many cols
    +offset  t_nrows     Isize  \ how many rows
    +offset  t_curx      Isize  \ saved x coor
    +offset  t_cury      Isize  \ saved y coor
    +offset  t_kpstate   Isize  \ saved keypad state
    +offset  ansiflg     Isize  \ flag for functions implemented as ESC seq
    +offset  stdoflg     Isize  \ flag for OK to write on stdout
    +offset  t_init      Psize  \ term init routine if any   
    +offset  t_close     Psize  \ term close routine if any   
    swap nceil swap             \ pad to n*sizeof(normal) alignment
Sizfterm endstchk   \ m_fterm, struct FTERMINAL

\ TERMINAL description.
begstruct
    +noffset  mt_baud                     \ baud rate of terminal 
    +noffset  mt_dlin                     \ `dest.n line' if "ch" unavailable
    +noffset  mt_ncols                    \ number of columns 
    +noffset  mt_nrows                    \ and number of lines 
    +noffset  mt_pad                      \ character for padding 
    +offset   mt_name        28           \ name of term 
    +offset   mt_del_char    Termcssize   \ delete previous character 
    +offset   mt_del_line    Termcssize   \ delete to the end of the curr. line 
    +offset   mt_forw_curs   Termcssize   \ move cursor forward 
    +offset   mt_back_curs   Termcssize   \ move cursor backward 
    +offset   mt_put_curs    Termcssize   \ direct cursor addr 
    +offset   mt_num_kpd     Termcssize   \ put keypad in numeric mode 
    +offset   mt_app_kpd     Termcssize   \ put keypad in 'appl' mode 
    +offset   mt_query_curs  Termcssize   \ query cursor position 
    +offset   mt_sav_curs    Termcssize   \ save cursor position internally 
    +offset   mt_res_curs    Termcssize   \ restore cursor after last command 
    +offset   mt_set_keys    Termcssize   \ put terminal in "keyboard transmit" 
    +offset   mt_unset_keys  Termcssize   \ take it out of "keyboard transmit" 
    +offset   mt_clr_scr     Termcssize   \ clear screen 
Sizterm endstchk  \ m_term, struct TERMINAL

: setterminal bl cword _setterminal ;

: bell ( --- ) 1 bells ;

\ departing message, filewriter

0 warning !
: bye 
    ["] bye.m" dup _faccess if _load else drop then
    bye
;
1 warning !

\ ****************** MIRELLA FILE UTILITIES ***************************

create fline 260 allot

: _filetype
    0 _fopen channel 
    begin
        fline 256 fgets
        rdeof @ if aclose exit then
        fline ct cr
    0 until
;        

: filetype bl cword _filetype ;        

\ stevens.doc has 13 lines 
: smessage
    lvar nrow  
    m_fterm t_nrows i@ nrow !
    ["] stevens.doc" mcfname dup if 
        nrow @ 13 - 2/ 8 + 0 do cr loop        
        _filetype 
        nrow @ 13 - 2/ 7 - dup 0> if 
            0 ?do cr loop        
        else drop then
    else 
        drop 
    then 
;

\ this reads a file into a buffer 
\ it checks for overflow

: _fileread ( filename.p buffer.p bufsiz.i --- ) 
    lvar bufsiz bufsiz !
    lvar buf buf !
    lvar len 0 len ! 
   
    0 _fopen pushchan rewind
    0 buf @ c!
    begin
        fline 253 fgets eol fline cs+!
        fline strlen len +!
        len @ bufsiz @ 1 - < not if 
            ." file longer than your buffer. I quit" 
            popchan
            0 erret 
        then
        rdeof @ if popchan exit then
        fline buf @ cs+!
    0 until
    popchan
;        

: smessages
    lvar nrow  
    m_fterm t_nrows i@ nrow !
    cr
    ["] stevens.doc" mcfname dup if 
        _filetype 
    cr        
    then
;


\ **************  COMMON SYSTEM STUFF *************************************


: sysword pad strcpy pad ';' cword strcat pad system ;  
            \ terminated by newline or ';'
          

\ ************ HODGEPODGE OF TEMPORARY AFTERTHOUGHTS *********************

: qmemfree bl cword _qmemfree ;

: getenv bl cword _getenv ;

\ some fits things

: fitsint ( addr --- int.i )
    lvar int
    dup c@ int 3 + c!
    1+ dup c@ int 2 + c!
    1+ dup c@ int 1+ c!
    1+ c@ int c!
    int @
;

: fitsshort ( addr --- int.i )
    lvar int
    dup c@ int 1+ c!
    1+ c@ int c!
    int s@
;

: fitsfloat ( addr --- float.f )
    lvar flt
    
    dup c@ flt 3 + c!
    1+ dup c@ flt 2 + c!
    1+ dup c@ flt 1+ c!
    1+ c@ flt c!
    flt f@
;    

: fitsfdump 2dup 1- na+ dumpend ! cr
    0 do 
        i cr 10 u.r     
        5 0 do 
            dup i j + na+ dup dumpend @ > if drop leave then fitsfloat
            12 -3 df.r 
        loop 
        pause 
    5 +loop drop
    flushit
; \ dumps as floats

\ **********  SOME SIMPLE STRING UTILITIES: ABOUT TIME (JEG061214) **********

\ arrays of strings

alias strarray strucarray   \  comp:( length number --- ) exec:( index --- addr)
alias strmarray strucmarray \  comp:( length number --- ) exec:( index --- addr)

\ moves a list of strings on the stack into an array. You must
\ be responsible for the elements of the array being big enough
\ NB!!!! this is different in one important respect from the
\ other rmove words--they take an address; this one takes
\ a cfa. We could make it take an address IF one could be sure
\ that the length was stored in a specific place wrt the storage
\ beginning, but that is impossible in general. Note that
\ the construct " str1" .... " scrn" 0 arrayname n rmove is NOT
\ safe, because it puts string stack addresses into the array, and
\ they are volatile. This word copies them into the proferred string array.
\ Note also that since the cfa of the array word is used, the origin cannot
\ be offset; they go into element 0...n-1.
\ DONT FORGET THE ' !!  ' arrayname (cfa_of_array_word)
\ The length is encoded in the array definition at ' word 2 na+, but
\ I can't quite figure out how to use it.


: strrmove ( list_of_strings cfa_of_array_word n --- )
    lvar n n !
    lvar cfa cfa !
    n @ 0 do
        n @ 1- i - cfa @ execute cs!
    loop
;

\ so string1 ...... stringn ' arrayname nstring strrmove

\ alternatively, you can give the origin and length 
\ (The one which DEFINED the strarray!)

: strlrmove ( list_of_strings pointer.p length.bytes.i n.i --- )
    lvar n n !
    lvar len len !
    lvar org org !
    n @ 0 do
        n @ 1- i - len @ * org @ + cs!
    loop  
;   

\ so string1 ..... stringn 0 arrayname length nstring strlrmove
\ with this word you can clearly offset, and put them into any
\ space if you keep track of the length allotted.
  

\ compares beginning of string 1 with all of string 2 rets 0 if so, -1 if not
: strbcmp ( str1.p str2.p --- flg )
    dup strlen strncmp
;
                                        


\ **************** stuff from newstuff.m (dos) ****************************

create c_dstr 2 allot c_dstr 2 clear

: ccat ( add1 char --- address of cat ) \ cats char to str at add1 on ss,
                                          \ pushes toss
     c_dstr c! c_dstr scat
;

\ utils for user input with defaults.
create dummy 256 allot
256 narray lptr \ this can be used a useful global for parse

: getfloat  ( addr --- )
    dup ." [" f? ." ] "
    dummy 20 expect
    dummy c@ 0= not if dummy atof f! else drop then
;

: getfarray ( addr nmax --- nactual )
    lvar na
    lvar addr swap addr !
    dummy 80 expect
    dummy 0 lptr parse na !
    na @ min dup na ! 0 do
        i lptr @ atof addr @ i na+ f!
    loop
    na @
;

: getnarray ( addr nmax --- nactual )
    lvar na 
    lvar addr swap addr !
    dummy 80 expect
    dummy 0 lptr parse na !
    na @ min dup na ! 0 do 
        i lptr @ atoi addr @ i na+ !
    loop
    na @
;

: getint ( addr --- )
    dup ." [" ? ." ] "
    dummy 20 expect 
    dummy c@ 0= not if dummy atoi swap ! else drop then 
;

\ these should invoke the editor on the old string

: getstring ( addr len ---  )
    ." [" over ct ." ] " cr dummy swap expect 
    dummy c@ 0= not if dummy swap cs! else drop then
;

: getsstring ( addr len ---  )  \ no cr; for short strings
    ." [" over ct ." ] " dummy swap expect 
    dummy c@ 0= not if dummy swap cs! else drop then
;

: editstring ( addr len --- )  \ writes current contents of string for editing
    cr mexpect
;

: editsstring ( addr len --- ) \ no cr
    mexpect
;

\ *********** SOME FLOATING ARRAY UTILITIES **************************

0 
#if
\ test code for runmed
create tdata 100 allot 
create tmdata 100 allot 
: filldata 25 0 do i 12 - float fnegate tdata i na+ f! loop ;
filldata
#endif


\ this routine creates a running ntile in a float array data
\ in a supplied array medarray, same size as data, npt points
\ percentile is centile (integer) half-width of medianing is wid.
\ there are 2*wid+1 points ntiled over, centered on data point
\ for points at least wid points from ends, truncated asymmetrically
\ at ends for points less than wid points from ends.

-1 ivar stopi \ debug

: runmed ( data.p medar.p npt.i centile.i wid.i )
    lvar wid wid !
    lvar centile centile !
    lvar npt npt !
    lvar medar medar !
    lvar data data !
    lvar memptr
    lvar scrptr 
    lvar nsort
    lvar si 0 si !
    
    wid @ 2* 10 + Nsize * scralloc dup memptr ! \ scratch array memptr
    @ scrptr !  \ pointer
    
    npt @ 0 do
        i wid @ < if  \ near bottom; sort from 0 to i+wid
            i wid @ + 1+ nsort !
            data @ scrptr @ nsort @ Nsize * move  \ move data to scratch ar
        else
            i npt @ 1- wid @ -  > if \ near top; sort from i-wid to npt-1
                npt @ i -  wid @ + nsort !
                data @ i wid @ - na+ scrptr @ nsort @ Nsize * move
            else  \ in middle; sort from i-wid to i+wid
                wid @ 2* 1+ nsort !
                data @ i wid @ - na+ scrptr @ nsort @ Nsize * move
            then
        then
        \ cr i 3 .r nsort @ 0 do scrptr @ i na+ f@ 4 0 f.r loop \ debug OK
        scrptr @ nsort @ 4 fqsort
        \ cr i 3 .r nsort @ 0 do scrptr @ i na+ f@ 4 0 f.r loop \ debug OK
        scrptr @ nsort @ centile @ 100 */ na+ f@ medar @ i na+ f!
        i stopi @ = if leave then \ debug
        si ++
    loop
    memptr @ chfree    
;        

\ **************** MEANS code ********************************

\ 1164. 384. 307. 402. 304. 676. 328. 89.6 316. 1765. 553.

\ routines to take various means

100 narray val

\ takes all three (arith, harmonic, geometric) and reports them
\ todo: extend to do sd, logsd

3 narray means_array

: means ( x1.f ..... xn.f n --- )
    lvar n n !
    lvar suma 0. suma f!
    lvar sumh 0. sumh f!
    lvar prod 1. prod f!
    lvar en n @ float en f!

    n @ 0 do
        fdup suma f+!
        fdup 1/x sumh f+!
        prod f@ f* prod f!
    loop
    cr ." Arithmetic mean = " 
    suma f@ en f@ f/ fdup 0 means_array f! f.
    cr ." Harmonic mean   = "
    sumh f@ en f@ f/ 1/x fdup 1 means_array f.
    cr ." Geometric mean  = "
    prod f@ en f@ 1/x pow fdup 2 means_array f.
;

: amean ( x1.f ..... xn.f n ---  arith_mean.f )
    lvar n n !
    lvar suma 0. suma f!
    lvar v
    lvar en n @ float en f!

    n @ 0 do
        suma f+!
    loop
    suma f@ en f@ f/ 
;

: hmean ( x1.f ..... xn.f n --- harmonic_mean.f)
    lvar n n !
    lvar sumh 0. sumh f!
    lvar en n @ float en f!

    n @ 0 do
        1/x sumh f+!
    loop
    sumh f@ en f@ f/ 1/x 
;

: gmean ( x1.f ..... xn.f n --- geom_mean.f )
    lvar n n !
    lvar prod 1. prod f!
    lvar en n @ float en f!

    n @ 0 do
        prod f@ f* prod f!
    loop
    prod f@ en f@ 1/x pow
;

\ ******************* ANGLE UTILITIES ***************************

\ convert rad to deg and vv
: r>d Deg/rad f* ;
: d>r Deg/rad f/ ;
: r>m Min/rad f* ;
: m>r Min/rad f/ ;
: r>s Sec/rad f* ;
: s>r Sec/rad f/ ;

\ ******************* IMAGE UTILITIES *******************************
\ CHECK FOR CONFLICT WITH MIRAGE !!!!!!!!!!!!  XXXXXXXXXXXXXXXXXX

: h_bitpix  Pbuf _bitpix ;
: h_pbsize  Pbuf _pbsize ;
: h_picorig Pbuf _picorig ;
: h_pline   Pbuf _pline ;
: h_header  Pbuf _header ;
: h_pfname  Pbuf _pfname ;
: h_decorder Pbuf _decorder ;

\ ****************  CODE TO PARSE RANGE LISTS *************************

\ words take a pointer array address ( typically n lptr, terminated by
\ null address or address of a null string; either works) and an
\ array address, either float or int, in which to put the flags --
\ the word puts the `yes' flag in the array for the indicated indices; the
\ rest of the array is filled with the `no' flag. Then the size of the index 
\ array, and the indicated (`yes') and non-indicated values (`no') for 
\ that array.
\ it MIGHT be convenient to have a version of these words which push
\ the `yes' *indices* on the stack, terminated somehow, but handling that
\ case in code sounds like an error-prone nightmare. This is inelegant
\ but bascally foolproof.  
\ NB!!! SIGNS !!!! `no' is generally the DEFAULT, `yes' is a *change*
\ Be not ye confused.

create rantok 32 allot

\ integer flagx in indexarray

: irangelist ( str_array.p indexarray.p n.i yes.i no.i --- )
    lvar nof nof !
    lvar yesf yesf !
    lvar n n !   
    lvar indexar indexar !
    lvar strar strar !
    lvar cp
    lvar ilo
    lvar ihi
    lvar rflg

    n @ 0 do
        nof @ indexar @ i na+ !
    loop        
    100000 0 do
        strar @ i na+ @ dup 0= if drop leave then \ address of token, null addr
                                                  \ terminates string    
        rantok 32 strncpy
        rantok c@ 0= if leave then  \ or blank entry terminates--either is fine
        \ cr i . space rantok ct        
        pause
        
        0 rflg !

        rantok ascii - strchr cp ! 
        cp @ 0= not if  \ delimited range
            0 cp @ c!
            cp ++
            rantok atoi ilo !
            cp @ atoi 1+ ihi !
            ihi @ ilo @ do
                yesf @ indexar @ i na+ !
            loop
            -1 rflg !
        then
        rantok ascii + strchr cp !  
        cp @ 0= not if \ end range--here to end
            0 cp @ c!
            rantok atoi ilo !
            n @ ihi !
            ihi @ ilo @ do 
                yesf @ indexar @ i na+ ! 
            loop
            -1 rflg !
        then                        
        rflg @ 0= if  \ not range, just isolated value
            rantok atoi ilo !
            yesf @ indexar @ ilo @ na+ !
        then
    loop
;

\ same, except float flags in indexarray

: frangelist ( str_array.p indexarray.p n.i yes.f no.f --- )
    lvar nof nof f!
    lvar yesf yesf f!
    lvar n n !   
    lvar indexar indexar !
    lvar strar strar !
    lvar cp
    lvar ilo
    lvar ihi
    lvar rflg \ flag to indicate that current lptr designates a range

    n @ 0 do
        nof f@ indexar @ i na+ f!
    loop        
    100000 0 do
        strar @ i na+ @ dup 0= if drop leave then \ address of token, null addr
                                                  \ terminates string    
        rantok 32 strncpy
        rantok c@ 0= if leave then  \ or blank entry terminates--either is fine
        \ cr i . space rantok ct        
        pause
        
        0 rflg !

        rantok ascii - strchr cp ! 
        cp @ 0= not if  \ delimited range
            0 cp @ c!
            cp ++
            rantok atoi ilo !
            cp @ atoi 1+ ihi !
            ihi @ ilo @ do
                yesf f@ indexar @ i na+ f!
            loop
            -1 rflg !
        then
        rantok ascii + strchr cp !  
        cp @ 0= not if \ end range--here to end
            0 cp @ c!
            rantok atoi ilo !
            n @ ihi !
            ihi @ ilo @ do 
                yesf f@ indexar @ i na+ f! 
            loop
            -1 rflg !
        then                        
        rflg @ 0= if  \ not range, just isolated value
            rantok atoi ilo !
            yesf f@ indexar @ ilo @ na+ f!
        then
    loop
;
            
\ *********** END OF MODULE MIRELLA.M ******************************
