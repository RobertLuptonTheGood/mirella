/*VERSION 88/06/02: Mirella 5.0                                      */

/***************************** MPRIMS.H  *****************************/

/* This file provides the definitions for the tokens for mirellin.c.   */
/* Any changes there must be made here and in minit.x.                 */
#define NOT           1
#define AND           2
#define OR            3
#define XOR           4
#define PLUS          5
#define MINUS         6
#define TIMES         7
#define DIVIDE        8
#define MOD           9
#define SHIFT         10
#define DUP           11
#define DROP          12
#define SWAP          13
#define OVER          14
#define ROT           15
#define PICK          16
#define Q_DUP         17
#define TO_R          18
#define R_FROM        19
#define R_FETCH       20
#define DEPTH         21
#define LESS          22
#define EQUAL         23
#define GREATER       24
#define Z_LESS        25
#define Z_EQUAL       26
#define Z_GREAT       27
#define U_LESS        28
#define ONE_PLUS      29
#define TWO_PLUS      30
#define TWO_MINUS     31
#define U_M_TIMES     32
#define TWO_DIVIDE    33
#define IMAX          34
#define IMIN          35
#define ABS           36
#define NEGATE        37
#define FETCH         38
#define C_FETCH       39
#define W_FETCH       40
#define STORE         41
#define C_STORE       42
#define W_STORE       43
#define PLUS_STORE    44
#define CMOVE         45
#define CMOVE_UP      46
#define FILL          47
#define COUNT         48
#define PAREN         49
#define BACKSLASH     50
#define I             51
#define J             52
#define BRANCH        53
#define Q_BRANCH      54
#define UNNEST        55
#define EXIT          56
#define EXECUTE       57
#define KEY           58
#define EMIT          59
#define CR            60
#define DOT_PAREN     61
#define ALLOT         62
#define FIND          63
#define VFIND         64
#define ABORT         65
#define QUIT          66
#define FINISHED      67
#define HERE          68
#define TIB           69
#define DOT           70
#define U_DOT         71
#define WORD          72
#define COMMA         73
#define DOT_QUOTE     74
#define COLON         75
#define SEMICOLON     76
#define ABT_QTE       77
#define COMPILE       78
#define CONSTANT      79
#define VARIABLE      80
#define CREATE        81
#define QT_CREATE     82
#define USER_SIZE     83
#define USER          84
#define IMMEDIATE     85
#define DOES          86
#define IF            87
#define ELSE          88
#define THEN          89
#define FWD_RSLV      90
#define FWD_MRK       91
#define LITERAL       92
#define DO            93
#define Q_DO          94
#define LOOP          95
#define PLUS_LOOP     96
#define Q_LEAVE       97
#define LEAVE         98
#define BEGIN         99
#define BCK_MRK       100
#define BCK_RSLV      101
#define WHILE         102
#define REPEAT        103
#define UNTIL         104
#define M_TICK        105
#define BR_TICK       106
#define BR_COMPILE    107
#define P_Q_DO        108
#define P_DO          109
#define P_LOOP        110
#define P_PL_LOOP     111
#define P_TICK        112
#define P_LIT         113
#define P_DOES        114
#define P_DOT_QT      115
#define P_ABT_QT      116
#define P_COMPILE     117
#define BYE           118
#define LOSE          119
#define SPFETCH       120
#define SPSTORE       121
#define RPFETCH       122
#define RPSTORE       123
#define UPFETCH       124
#define UPSTORE       125
#define TICK_WORD     126
#define DIV_MOD       127
#define DIGIT         128
#define NMBR_Q        129
#define HIDE          130
#define REVEAL        131
#define QT_LOAD       132
#define LOAD          133
#define SEMI_S        134
#define CANON         135
#define FPLUS         136
#define FMINUS        137
#define FTIMES        138
#define FDIVIDE       139
#define FDUP          140
#define FDROP         141
#define FSWAP         142
#define FOVER         143
#define FROT          144
#define FPICK         145
#define FIX           146
#define FLOAT         147
#define FLESS         148
#define FEQUAL        149
#define FGREATER      150
#define FABS          151
#define FNEGATE       152
#define FMIN          153
#define FMAX          154
#define FCOMMA        155
#define FPLUSSTORE    156
#define FSTORE        157
#define FFETCH        158
#define FOUR_PLUS     159
#define FOUR_MINUS    160
#define FOUR_TIMES    161
#define FOUR_DIVIDE   162
#define TWO_TIMES     163
#define STAR_SLASH    164
#define FTOP          165
#define PTOF          166
#define FTOT          167
#define PTOT          168
#define TTOF          169
#define TTOP          170
#define FCONSTANT     171
#define FDOT          172
#define FSPFETCH      173
#define TSPFETCH      174
#define SPUSH         175
#define SDOWN         176
#define SUP           177
#define SSP_STORE     178
#define SSP_FETCH     179
#define H_ADDR        180
#define P_FLIT        181
#define W_PLUS_ST     182
#define DFETCH        183
#define PUTC          184
#define GETC          185
#define LVAR          186
#define P_LVAR        187
#define QT_LCREATE    188
#define ROLL          189
#define FROLL         190
#define IFETCH        191
#define ISTORE        192
#define NEXT_PRIM     193               // must be last
#define MAXPRIM       300

/*secondary tokens -- do not muck with this line; is marker for makemprim */
/********************** end of module *************************************/

