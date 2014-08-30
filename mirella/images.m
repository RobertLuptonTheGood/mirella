\ this is a motley collection of image words--RATIONALIZE!!!
: Pbsize ( --- size.i )
   Pbuf _pbsize	@ ;
: picorig ( --- orig.*s )
   Pbuf _picorig @ ;
: phead ( --- header.p )
   Pbuf _header _app @ ;
: picorigs ( n.i --- orig.*s )
   pbufs @ _picorig @ ;
: plines ( n.i --- pline.**s )
   pbufs @ _pline @ ;
: pbsizes ( n.i --- pbsize.i )
   pbufs @ _pbsize @ ;
: pheads ( n.i --- header.p )
   pbufs @ _header _app @ ;
: pfname Pbuf _header _pfname ;
\
\ DLT stuff
\
p' Dltable 0 ccparray dltable

alias rmtv mtv			\ used by JEG

\ BUFFER WORDS
alias pbuf picbuffer
alias setbuf pbuf
alias showpbufs showpbuffers	\ be consistent with using pbuf not picbuffer
alias showpbuf showpbuffers
: .pbuf Nbuf . ;
alias .buf .pbuf    \ .pbuf or .buf displays buffer number. Nbuf is a 
                    \ c CONSTANT which contains it. DO NOT CHANGE IT!!!
                    

\ moved from mirage.m 121022

: ppsetup ysize ! xsize ! plptr ["] test" pfname strcpy ;
: psetup ppsetup pclear ; 
   \ both mark the buffer as 'contented'; psetup clears the buffer, ppsetup not
   
\ likewise for byte, float, and normal pictures  
: bpsetup ysize ! xsize ! bplptr ["] test" pfname strcpy ;   
: fpsetup ysize ! xsize ! fplptr ["] test" pfname strcpy ;   
: npsetup ysize ! xsize ! nplptr ["] test" pfname strcpy ;   

: newbuffer \ creates new buffer like the current one--same SIZE, note;
            \ xsize and ysize should be set upon accessing it first
    lvar sflg 0 sflg !
    lvar newbuf
    Maxpbuf 0 do
        i plines 0= if    \ mod jeg9210
            Pbsize float sqrt round 1+ dup i pballoc
            sflg ++
            i newbuf !
            leave
        then
    loop
    sflg @ 0= if 
        cr ." Cannot create new buffer; all active" 
    else
        cr ." Buffer " newbuf ? ."  created. NO xsize, ysize set"
    then
;
            
\ destination buffer words
: picorigd ( --- orig.*s )
	Pbufd _picorig @ ;
: Pbsized ( n.i --- pbsize.i )
	Pbufd _pbsize @ ;

\
\ cursor words
\
: !cur c_ys ! c_xs ! ;
: @cur c_xs @ c_ys @ ;
: .cur @cur swap 2dup . . 2 spaces pic s? ; \ jeg0306

: c del1 cursor ;			\ put up cursor, not on history

im_display " x11" strcmp 0=
#if
  alias acursor toggle-name-window
  alias bcursor toggle-blinkflag
  alias ccursor toggle-cursor-window
  alias rbcursor qq  
\  alias rbcursor scroll-lookup
  -1 constant isX 
  : openx x11_arg_string cs! _dspopen ;
  : xbuffersize ( wxsize.i wysize.i --- ) wysize ! wxsize ! ;
#else
  0 constant isX  
#endif
im_display " vga" strcmp 0=
#if
  alias bcursor toggle-blinkflag
  alias ocursor toggle-overlay
#endif
\ alias Bcursor toggle-boxflag \ jeg0304 ccode in mirage
alias ecursor egraph
: qcursor 0 cursor_key ! ;
\ alias cbcursor qcursor  \ nononever
\ alias Fcursor toggle-fileflag    \ jeg0304 ccode in mirage
alias lbcursor makemark gflush   \ this is incompatible with lb->focus
\ : .cursor makemark gflush ;
: mcursor ["] m" c@ c_xs @ c_ys @ find_extreme c_ys ! c_xs ! ;  \ min?
: Mcursor ["] M" c@ c_xs @ c_ys @ find_extreme c_ys ! c_xs ! ;  \ max ?
\ alias rcursor toggle-regionflag  \ jeg0304 ccode in mirage 
0
#if
alias pcursor toggle-point-cursor  \ FIXME!!!!
#endif

: Pcursor \ pan
     ers
     c_xs @ c_ys @ xyc
     mtv
;

alias cbcursor Pcursor ;

0 #if                        \ jeg0304 ccode in mirage
alias zcursor -zoom  
alias Zcursor +zoom
alias -cursor -zoom
alias +cursor +zoom
#endif
: 0cursor c_xs @ c_ys @ 0 dspzoom ;
: 1cursor c_xs @ c_ys @ 1 dspzoom ;
: 2cursor c_xs @ c_ys @ 2 dspzoom ;
: 3cursor c_xs @ c_ys @ 3 dspzoom ;
: 4cursor c_xs @ c_ys @ 4 dspzoom ;
: 5cursor c_xs @ c_ys @ 5 dspzoom ;
: 6cursor c_xs @ c_ys @ 6 dspzoom ;
: 7cursor c_xs @ c_ys @ 7 dspzoom ;
: 8cursor c_xs @ c_ys @ 8 dspzoom ;
: 9cursor c_xs @ c_ys @ 16 dspzoom ;
\
\ the current display struct
\

: d_xoff _display _xoff ;
: d_yoff _display _yoff ;
: d_xlow _display _dxlow ;
: d_xhigh _display _dxhigh ;
: d_ylow _display _dylow ;
: d_yhigh _display _dyhigh ;
: d_wxcen _display _wxcen ;
: d_wycen _display _wycen ;
: d_wzoom _display _wzoom ;
: d_dmode _display _dmode ;
: d_zoom _display _cnum ;
: d_cnum _display _cnum ;
: d_cden _display _cden ;
: d_wrapflg _display _wrapflg ;
: d_zorg _display _zorg ;
: d_zrng _display _zrng ;
: D_bufno _display _bufno ;
\
\ Read a simple short image; 2 ints give x and y size, then the (short) data
\
: _fillpbuf ( file.st -- )
   Nbuf 0< if
      ["] \nYou must select a pbuf" printf abort
   then
   dup
   0 _fopen pushchan
   xsize 4 read ysize 4 read
   xsize @ Pbuf _xsize @ > ysize @ Pbuf _ysize @ > or if
      ["] \nbuffer is too small" printf abort
   then
   ysize @ 0 do
      i 0 pic xsize @ 2* read
   loop
   popchan
   Pbuf _pfname cs!
   ;
: fillpbuf ( i:file.st -- )
   bl cword _fillpbuf ;


\ xv display words
0 ivar pgmno
0 ivar ppmno  \ serial numbers for files in /tmp/--makes little circular buffers

\ displays image in current buffer via xv as a .pgm file

create pgmtfname 64 allot

: display ( zlo.i zhi.i --- )
    ["] /tmp/mirimage" pgmtfname cs!
    pgmno @ itoa pgmtfname cs+!
    ["] .pgm" pgmtfname cs+!
    pgmtfname _putpgmimg
    pgmtfname _xv
    pgmno ++
    pgmno @ 10 mod pgmno !
;

create ppmtfname 64 allot

\ displays image in current buffer via xv as a color-coded .ppm file
: cdisplay ( zlo.i zhi.i )
    ["] /tmp/mirimage" ppmtfname cs!
    ppmno @ itoa ppmtfname cs+!
    ["] .ppm" ppmtfname cs+!
    ppmtfname _putppmimg
    ppmtfname _xv
    ppmno ++
    ppmno @ 10 mod ppmno !
;
    

