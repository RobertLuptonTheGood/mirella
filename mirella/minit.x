P         not            ( NOT                   1  )
P         and            ( AND                   2  )
P         or             ( OR                    3  )
P         xor            ( XOR                   4  )
P         +              ( PLUS                  5  )
P         -              ( MINUS                 6  )
P         *              ( TIMES                 7  )
P         /              ( DIVIDE                8  )
P         mod            ( MOD                   9  )
P         shift          ( SHIFT                10  )
P         dup            ( DUP                  11  )
P         drop           ( DROP                 12  )
P         swap           ( SWAP                 13  )
P         over           ( OVER                 14  )
P         rot            ( ROT                  15  )
P         pick           ( PICK                 16  )
P         ?dup           ( Q_DUP                17  )
P         >r             ( TO_R                 18  )
P         r>             ( R_FROM               19  )
P         r@             ( R_FETCH              20  )
P         depth          ( DEPTH                21  )
P         <              ( LESS                 22  )
P         =              ( EQUAL                23  )
P         >              ( GREATER              24  )
P         0<             ( Z_LESS               25  )
P         0=             ( Z_EQUAL              26  )
P         0>             ( Z_GREAT              27  )
P         u<             ( U_LESS               28  )
P         1+             ( ONE_PLUS             29  )
P         2+             ( TWO_PLUS             30  )
P         2-             ( TWO_MINUS            31  )
P         um*            ( U_M_TIMES            32  )
P         2/             ( TWO_DIVIDE           33  )
P         max            ( IMAX                 34  )
P         min            ( IMIN                 35  )
P         abs            ( ABS                  36  )
P         negate         ( NEGATE               37  )
P         @              ( FETCH                38  )
P         c@             ( C_FETCH              39  )
P         w@             ( W_FETCH              40  )
P         !              ( STORE                41  )
P         c!             ( C_STORE              42  )
P         w!             ( W_STORE              43  )
P         +!             ( PLUS_STORE           44  )
P         cmove          ( CMOVE                45  )
P         cmove>         ( CMOVE_UP             46  )
P         fill           ( FILL                 47  )
P         count          ( COUNT                48  )
i         (              ( PAREN                49  )
i         \              ( BACKSLASH            50  )
P         i              ( I                    51  )
P         j              ( J                    52  )
P         branch         ( BRANCH               53  )
P         ?branch        ( Q_BRANCH             54  )
P         unnest         ( UNNEST               55  )
P         exit           ( EXIT                 56  )
P         execute        ( EXECUTE              57  )
P         key            ( KEY                  58  )
P         emit           ( EMIT                 59  )
P         cr             ( CR                   60  )
i         .(             ( DOT_PAREN            61  )
P         allot          ( ALLOT                62  )
P         find           ( FIND                 63  )
P         vfind          ( VFIND                64  )
P         abort          ( ABORT                65  )
P         quit           ( QUIT                 66  )
P         finished       ( FINISHED             67  )
P         here           ( HERE                 68  )
P         tib            ( TIB                  69  )
P         .              ( DOT                  70  )
P         u.             ( U_DOT                71  )
P         word           ( WORD                 72  )
P         ,              ( COMMA                73  )
i         ."             ( DOT_QUOTE            74  )
P         :              ( COLON                75  )
i         ;              ( SEMICOLON            76  )
i         abort"         ( ABT_QTE              77  )
i         compile        ( COMPILE              78  )
P         constant       ( CONSTANT             79  )
P         variable       ( VARIABLE             80  )
P         create         ( CREATE               81  )
P         "create        ( QT_CREATE            82  )
P         user_size      ( USER_SIZE            83  )
P         user           ( USER                 84  )
P         immediate      ( IMMEDIATE            85  )
i         does>          ( DOES                 86  )
i         if             ( IF                   87  )
i         else           ( ELSE                 88  )
i         then           ( THEN                 89  )
P         >resolve       ( FWD_RSLV             90  )
P         >mark          ( FWD_MRK              91  )
i         literal        ( LITERAL              92  )
i         do             ( DO                   93  )
i         ?do            ( Q_DO                 94  )
i         loop           ( LOOP                 95  )
i         +loop          ( PLUS_LOOP            96  )
P         ?leave         ( Q_LEAVE              97  )
P         leave          ( LEAVE                98  )
i         begin          ( BEGIN                99  )
P         <mark          ( BCK_MRK             100  )
P         <resolve       ( BCK_RSLV            101  )
i         while          ( WHILE               102  )
i         repeat         ( REPEAT              103  )
i         until          ( UNTIL               104  )
P         '              ( TICK                105  )
i         [']            ( BR_TICK             106  )
i         [compile]      ( BR_COMPILE          107  )
P         (?do           ( P_Q_DO              108  )
P         (do            ( P_DO                109  )
P         (loop          ( P_LOOP              110  )
P         (+loop         ( P_PL_LOOP           111  )
P         ('             ( P_TICK              112  )
P         (lit           ( P_LIT               113  )
P         (does          ( P_DOES              114  )
P         (."            ( P_DOT_QT            115  )
P         (abort"        ( P_ABT_QT            116  )
P         (compile       ( P_COMPILE           117  )
P         bye            ( BYE                 118  )
P         lose           ( LOSE                119  )
P         sp@            ( SPFETCH             120  )
P         sp!            ( SPSTORE             121  )
P         rp@            ( RPFETCH             122  )
P         rp!            ( RPSTORE             123  )
P         up@            ( UPFETCH             124  )
P         up!            ( UPSTORE             125  )
P         'word          ( TICK_WORD           126  )
P         /mod           ( DIV_MOD             127  )
P         digit          ( DIGIT               128  )
P         number?        ( NMBR_Q              129  )
P         hide           ( HIDE                130  )
P         reveal         ( REVEAL              131  )
P         "load          ( QT_LOAD             132  )
P         load           ( LOAD                133  )
P         ;s             ( SEMI_S              134  )
P         canonical      ( CANON               135  )
P         f+             ( FPLUS               136  )
P         f-             ( FMINUS              137  )
P         f*             ( FTIMES              138  )
P         f/             ( FDIVIDE             139  )
P         fdup           ( FDUP                140  )
P         fdrop          ( FDROP               141  )
P         fswap          ( FSWAP               142  )
P         fover          ( FOVER               143  )
P         frot           ( FROT                144  )
P         fpick          ( FPICK               145  )
P         fix            ( FIX                 146  )
P         float          ( FLOAT               147  )
P         f<             ( FLESS               148  )
P         f=             ( FEQUAL              149  )
P         f>             ( FGREATER            150  )
P         fabs           ( FABS                151  )
P         fnegate        ( FNEGATE             152  )
P         fmin           ( FMIN                153  )
P         fmax           ( FMAX                154  )
P         f,             ( FCOMMA              155  )
P         f+!            ( FPLUSSTORE          156  )
P         f!             ( FSTORE              157  )
P         f@             ( FFETCH              158  )
P         4+             ( FOUR_PLUS           159  )
P         4-             ( FOUR_MINUS          160  )
P         4*             ( FOUR_TIMES          161  )
P         4/             ( FOUR_DIVIDE         162  )
P         2*             ( TWO_TIMES           163  )
P         */             ( STAR_SLASH          164  )
P         f>p            ( FTOP                165  )
P         p>f            ( PTOF                166  )
P         f>t            ( FTOT                167  )
P         p>t            ( PTOT                168  )
P         t>f            ( TTOF                169  )
P         t>p            ( TTOP                170  )
P         fconstant      ( FCONSTANT           171  )
P         f.             ( FDOT                172  )
P         fsp@           ( FSPFETCH            173  )
P         tsp@           ( TSPFETCH            174  )
P         spush          ( SPUSH               175  )
P         sdown          ( SDOWN               176  )
P         sup            ( SUP                 177  )
P         ssp!           ( SSP_STORE           178  )
P         ssp@           ( SSP_FETCH           179  )
P         haddr          ( H_ADDR              180  )
P         (flit          ( P_FLIT              181  )
P         w+!            ( W_PLUS_ST           182  )
P         @@             ( DFETCH              183  )
P         putc           ( PUTC                184  )
P         getc           ( GETC                185  )
i         lvar           ( LVAR                186  )
P         (lvar          ( P_LVAR              187  )
i         "lcreate       ( QT_LCREATE          188  )
P         roll           ( ROLL                189  )
P         froll          ( FROLL               190  )
P         i@             ( IFETCH              191  )
P         i!             ( ISTORE              192  )
u         #user          ( V_NR_USER             0  )
u         span           ( V_SPAN                1  )
u         >in            ( V_TO_IN               2  )
u         base           ( V_BASE                3  )
u         blk            ( V_BLK                 4  )
u         #tib           ( V_NR_TIB              5  )
u         state          ( V_STATE               6  )
u         current        ( V_CURRENT             7  )
u         context        ( V_CONTEXT             8  ) 
u         con2           (                       9  )
u         con3           (                      10  )
u         con4           (                      11  ) 
u         con5           (                      12  )
u         delimiter      ( V_DELIMITER          13  ) 
u         sp0            ( V_SP0                14  )
u         rp0            ( V_RP0                15  )
u         up0            ( V_UP0                16  )
u         last           ( V_LASTP              17  )
u         #out           ( V_NR_OUT             18  )
u         #line          ( V_NR_LINE            19  )
u         voc-link       ( V_VOC_LINK           20  ) 
u         dpl            ( V_DPL                21  )
u         warning        ( V_WARNING            22  )
u         caps           ( V_CAPS               23  )
u         errno          ( V_ERRNO              24  )
u         tsp0           ( V_TSPZERO            25  )
u         fsp0           ( V_FSPZERO            26  )
u         ssp0           ( V_SSPZERO            27  )
u         ?mirella       ( V_MIRELLA            28  )
u         pstring        ( V_PSTRING            29  )
u         app0           ( V_APAREA             30  )
u         fpf            ( V_FPF                31  )
u         fp0            ( V_FORIG              32  )
u         fssize         ( V_FSSIZE             33  )
u         mp0            ( V_MORIG              34  )
u         mssize         ( V_MSSIZE             35  )
u         ital           ( V_ITAL               36  )
u         io             ( V_IO                 37  )
u         noecho         ( V_NOECHO             38  )
u         #linein        ( V_LN_IN              39  )
u         lastfile       ( V_LASTFILE           40  )
u         silentout      ( V_SILENTOUT          41  )
u         fb0            ( V_FBZERO             42  )
u         nocompile      ( V_NOCOMPILE          43  )
u         remin          ( V_REMIN              44  )
u         res_stk        ( V_RES_STK            45  )
u         interflg       ( V_INTERFLG           46  )
u         nlvar          ( V_NLVAR              47  )
u         localdic       ( V_LOCALDIC           48  )
u         ccnest         ( V_CCNEST             49  )
u         ccnoff         ( V_CCNOFF             50  )
