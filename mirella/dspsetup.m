\ simple words to set up image display

: q cursor ;
: xopen ( xsize.i ysize.i --- )
    dup wysize ! d_yhigh !
    dup wxsize ! d_xhigh !
    0 d_xlow !
    0 d_ylow !
    200 set_ncol
    ["] -g " x11_arg_string cs!
    x11_arg_string
    wxsize @ 1 s.r scat ["] x" scat wysize @ 1 s.r scat
    ["] +100+100 " scat x11_arg_string cs!
    cr ." x11_arg_string=" x11_arg_string ct
    dspopen
;
    
: 512open 512 dup xopen ;   
: vga 640 480 xopen ;
: svga 800 600 xopen ;
: xga 1024 768 xopen ;
: sxga 1280 1024 xopen ;
: wsxga 1680 1050 xopen ;

