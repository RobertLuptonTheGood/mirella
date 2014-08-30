\ UNIX system words

: syswordbk pad strcpy pad ';' cword strcat pad ["]  &" strcat pad system ;  
            \ terminated by newline or ';'
          

: vi  ["] vi  " sysword res_scr ;
: edit ["] em39 -r " sysword res_scr ;
: _edit ["] em39 -r " swap scat system res_scr cr prompt ;
: _medit mcfname ["] em39 -r " swap scat system res_scr cr prompt ;
: medit bl cword _medit ;
: _mview mcfname ["] em39  -r -v " swap scat system res_scr cr prompt ;
: mview bl cword _mview ;
: view ["] em39  -r -v " sysword res_scr ;
: editor ["] em39 -o -r" system res_scr ;
\ the -r is to suppress the reverse video on the status line, which
\ does not work on some 8-bit displays
: ls  cr ["] ls -bCF " sysword ;
: lhead cr ["] lhead " sysword ;
: cls 27 ( ESC) ["] %c[2J" printf ;    \ clears the screen on ANSI terminals
: rm  ["] rm -i " sysword ;
: del ["] rm -f " sysword ;
: cp  ["] cp -ip " sysword ;
: mv  ["] mv -i " sysword ;
: touch ["] touch " sysword ;
: less cr ["] less " sysword ;
: more cr ["] less " sysword ;
: diff cr ["] diff " sysword ;
: _less ["] less  " swap scat system ;
: _mless mcfname ["] less " swap scat system ;
: mless bl cword _mless ;
: todo ["] em39 todo.doc " system res_scr ;
: fgrep cr ["] fgrep -n " sysword ;
: grep  cr ["] grep -n " sysword ;
: lpr ["] lpr " sysword ;
: head ["] head " sysword ;
: tail ["] tail " sysword ;
: savem ["] mirella.dic" _savedic ; 
: savet ["] temp.dic" _savedic ;
: ciao savet bye ;
: desmove  ["] desmove "  sysword ;
: desdel   ["] desdel "   sysword ;
: desclean ["] desclean " sysword ;
: dos2unix ["] dos2unix" sysword ;
: unix2dos ["] unix2dos" sysword ;
: gzip ["] jgzip " sysword ;
: _gzip ["] jgzip " swap scat system ; 
: gunzip ["] gunzip " sysword ;
: _gunzip ["] gunzip " swap scat system ; 
: xv ["] xv " syswordbk ;
: _xv ["] xv " swap scat ["]  & " scat system ;
: % cr ';' cword system ;
alias wait ciao

\ just use less instead of the dysfunctional mirella browse word 
0 warning !
: browse less ;
: _browse _less ;
1 warning !

\ utilities

\ Environment --- These are general

create username 128 allot
getenv USER username cs!

create homename 128 allot
getenv HOME homename cs!
create hostname 128 allot

getenv HOSTNAME dup 0= not 
#if 
    hostname cs! 
#else
    getenv HOSTNAM hostname cs!
#endif



create hname 128 allot
getenv HNAME hname cs!

\ there is a confusing parallel path for this stuff--it is now
\ in .mirella, and is read in mirella.c. FIXME
create distroname 64 allot
create distrotype 32 allot
getenv DISTRO distroname cs!

: finddistrotype
    distroname c@ 
    case
        ascii U of ["] Debian" distrotype cs! endof
        ascii F of ["] Redhat" distrotype cs! endof
        ascii C of ["] Redhat" distrotype cs! endof
        cr ." NOT A KNOWN DISTRIBUTION"
    endcase
;

finddistrotype
0 constant Redhat
0 constant Debian
0 constant Unkdistro

distrotype " Redhat" strcmp 0= 
#if
    -1 p' Redhat !
#endif
distrotype " Debian" strcmp 0= 
#if
    -1 p' Debian !
#endif
Debian Redhat or 0=
#if
-1 p' Unkdistro !
#endif

: mtodo homename ["] /mirella/todo.txt" scat _edit ;

\ ascii files same as normal
alias _fopena _fopen
alias fopenar fopenr
alias fopenaw fopenw
alias fopenrarw fopenrw

\ binary files same as normal 
alias _fopenb _fopen
alias fopenbr fopenr
alias fopenbw fopenw
alias fopenbrw fopenrw

\ directory lists

create dirf_str1  10 clallot   " ls -1 " dirf_str1 strcpy
create dirf_str2  20 clallot    "  >dirfile.lis "  dirf_str2 strcpy

: dirlist 
    cr ." listing in dirfile.lis in current directory"
    dirf_str1 pad strcpy pad bl cword strcat pad dirf_str2 strcat pad system  
;

create g_screensize 64 allot
0 ivar g_screenxsize
0 ivar g_screenysize

: gsysgraph ( graph# bitspix --- )
    lvar bp bp !
    lvar gc gc !
    lvar xs 1024 xs !
    lvar ys  768 ys !
    lvar py

    ["] SSIZE" _getenv dup if
        \ dup cr ct
        g_screensize 63 strncpy
        \ g_screensize ct
        g_screensize ascii x index dup if
            dup 1+ py !
            0 swap c!
            g_screensize atoi xs !
            py @ atoi ys !
            cr ." setting sysgraph to " xs ? ." x" ys ?
        else
            1024 xs ! 768 ys !
        then
    else
        drop
        1024 xs !
        768 ys !
    then
    xs @ g_screenxsize !
    ys @ g_screenysize !
    xs @ ys @ gc @ 0 bp @ gallocgraph 
;

: g43sysgraph ( graph# bitspix --- ) \ allocates a 4:3 graph
    lvar bp bp !
    lvar gc gc !
    lvar xs 1024 xs !
    lvar ys  768 ys !
    lvar py

    ["] SSIZE" _getenv dup if
        \ dup cr ct
        g_screensize 63 strncpy
        \ g_screensize ct
        g_screensize ascii x index dup if
            dup 1+ py !
            py @ atoi ys !
            ys @ 4 * 3 / xs !
            cr ." setting sysgraph to " xs ? ." x" ys ?
        else
            1024 xs ! 768 ys !
        then
    else
        drop
        1024 xs !
        768 ys !
    then
    xs @ g_screenxsize !
    ys @ g_screenysize !
    xs @ ys @ gc @ 0 bp @ gallocgraph 
;

: gsqsysgraph ( graph# bitspix --- ) \ allocates a 1:1 graph
    lvar bp bp !
    lvar gc gc !
    lvar xs 1024 xs !
    lvar ys  768 ys !
    lvar py

    ["] SSIZE" _getenv dup if
        \ dup cr ct
        g_screensize 63 strncpy
        \ g_screensize ct
        g_screensize ascii x index dup if
            dup 1+ py !
            py @ atoi ys !
            ys @ xs !
            cr ." setting sysgraph to " xs ? ." x" ys ?
        else
            1024 xs ! 768 ys !
        then
    else
        drop
        1024 xs !
        768 ys !
    then
    xs @ g_screenxsize !
    ys @ g_screenysize !
    xs @ ys @ gc @ 0 bp @ gallocgraph 
;

: sysgraph 1 gsysgraph ;
: sys43graph 1 g43sysgraph ;
: syssqgraph 24 gsqsysgraph ;
: syscgraph 24 gsysgraph ;
: sys43cgraph 24 g43sysgraph ;
: syssqcgraph 24 gsqsysgraph ;
: alloccgraph 0 24 gallocgraph ;

0 syscgraph

: vgagraph ( graph# --- )
    lvar gc gc !
    640 480 gc @ allocgraph
;

: svgagraph ( graph# --- )
    lvar gc gc !
    800 600 gc @ allocgraph
;

: xgagraph ( graph# --- )
    lvar gc gc !
    1024 768 gc @ allocgraph
;

: vgacgraph ( graph# --- )
    lvar gc gc !
    640 480 gc @ alloccgraph
;

: svgacgraph ( graph# --- )
    lvar gc gc !
    800 600 gc @ alloccgraph
;

: xgacgraph ( graph# --- )
    lvar gc gc !
    1024 768 gc @ alloccgraph
;


\ this word is for an external graphics display program, in this case xv
create displayprog 32 allot
\ " xv -maxpect " displayprog cs!
" xv  " displayprog cs!
create displayrootname 40 allot
" /tmp/mirgraph" displayrootname cs!

create displayfname 128 allot

0 ivar g_dispwait \ global flag for background behavior of gend. 0
                  \ puts xv in background, nonzero waits. Setting to
                  \ nonzero is temporary; must be set for each invocation
                  \ of m_display (gend)
                  
: g_wait -1 g_dispwait ! ;

: m_display
    displayrootname displayfname cs!
    Gchan 1 s.r displayfname cs+!   
    G_bitscol
    case      
        1  of 
            ["] .pbm" displayfname cs+! 
            displayfname _putpbm
        endof
        8  of 
            ["] .pgm" displayfname cs+! 
            displayfname _putpgm
        endof
        24 of 
            ["] .ppm" displayfname cs+! 
            displayfname _putppm
        endof
        cr ." I cannot yet deal with G_bitscol = "   dup . 
    endcase
    g_dispwait @ 0= if
        displayprog displayfname scat ["]  &" scat system
    else
        displayprog displayfname scat ["]  " scat system
        0 g_dispwait !  \ reset
    then
;

: showgraph m_display ;

: _savegraph ["] ppmtogif " displayfname scat ["]  > " swap scat system ; 
: savegraph bl cword _savegraph ;

43 ivar screen_lines
80 ivar screen_columns


0 
#if  \ old

create screenline 64 allot  \ these arrays get incorporated into the 
                            \ environment, which is *NOT* a good idea. 
create screencol  64 allot  \ do this with setenv instead of putenv

\ this routine uses the shell utility resize to read the screen size, and set 
\ (I hope) all the Mirella internals to agree. 


: screen
    ["] resize > /tmp/resize.dat" system
    ["] /tmp/resize.dat" 0 _fopen puch rewind 
    
    \ first line should say COLUMNS=CC; 
    screencol 63 fgets
    0 screencol ascii ; index c!  \ kill trailing ';'
    screencol _putenv
    cr screencol ct
    ["] COLUMNS" _getenv dup 0= not if atoi screen_columns ! then
   
    \ next line should say LINES=LL 
    screenline 63 fgets
    0 screenline ascii ; index c!
    screenline _putenv
    cr screenline ct
     ["] LINES"   _getenv dup 0= not if atoi screen_lines ! then
 
    popchan   
    screen_lines    @ dup m_fterm t_nrows !  m_term mt_nrows !
    screen_columns  @ dup m_fterm t_ncols !  m_term mt_ncols !
    reset_scr
;
#endif


0 #if  \ functionality replaced by C routine screensize() which returns a 
       \ pointer to a string of the form *(cols,rows)
       
create screenscr 64 allot

\ this routine uses the shell utility resize to read the screen size, and set 
\ (I hope) all the Mirella internals to agree. 


: screen
    lvar valptr
    ["] resize > /tmp/resize.dat" system
    ["] /tmp/resize.dat" 0 _fopen puch rewind 
    
    \ first line should say COLUMNS=CC; 
    screenscr 63 fgets
    0 screenscr ascii ; index c!  \ kill trailing ';'
    0 screenscr ascii = index dup 1+ valptr ! c! 
    screenscr ["] COLUMNS" strcmp 0= not if 
        cr ." Unrecognized output from 'resize'. I quit" 
        exit
    then
    screenscr valptr @ _setenv
    ["] COLUMNS" _getenv dup 0= not if 
        atoi screen_columns ! 
    else
        cr ." Cannot get value of COLUMNS from environment. I quit"
        exit        
    then
    cr ." COLUMNS=" screen_columns @ 2 .r
   
    \ next line should say LINES=LL 
    screenscr 63 fgets
    0 screenscr ascii ; index c! \ kill trailing ';'
    0 screenscr ascii = index dup 1+ valptr ! c!
    screenscr ["] LINES" strcmp 0= not if
        cr ." Unrecognized output from 'resize'. I quit" 
        exit
    then
    screenscr valptr @ _setenv
    ["] LINES"   _getenv dup 0= not if 
        atoi screen_lines ! 
    else
        cr ." Cannot get value of LINES from environment. I quit"
        exit
    then
    cr ." LINES=" screen_lines @ 2 .r flushit
    cr
    
    popchan   
    screen_lines    @ dup m_fterm t_nrows !  m_term mt_nrows !
    screen_columns  @ dup m_fterm t_ncols !  m_term mt_ncols !
    reset_scr
;

#endif
