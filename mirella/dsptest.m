\ x11 test stuff

: ecursor 0 erret ;

512open
  
512 512 0 pballoc
0 pbuf

: pramp
    xsize @ 0 do
        ysize @ 0 do
            i j + 512 mod j i pic s!
        loop
    loop
;

0 512 zz

pramp mtv            

