/*version 88/10/24:Mirella 5.10                                      */
/********************** MIRKERN.H ************************************/
/* Recent history (after 1 apr 88)
88/04/27:   put in new user variables V_RES_STK and V_INTERFLG
88/05/20:   put in new "     "        V_NLVAR
88/08/21:   made token_t unsigned for Convex's negative addresses
88/10/24:   fixed up allocation constants to reflect changes in
            dynamic allocation of break and code space
*/

#define MIRKERN_H             
                            /* must precede inclusion of mirella.h; some
                               quantities are multiply defined otherwise */
#ifdef VMS
#include mirella
#else
#include "mirella.h"         
#endif
                                /* defines processor and system; this must
                                follow definition of 'normal' here or some
                                quantities are multiply defined */

/* note that mirella.h now includes stdio */

/* It is assumed that the system used supports either as functions
 * or as system calls:
 *      creat(b),open(b),close,read,write  
 *      and has available an int variable errno
 */

/*
 * Both token_t and normal should be big enough to hold an absolute
 * address on your machine.
 */

/*#define normal long   this should be defined in mirella.h, to either
    int (on machines with 32-bit ints) or long (on 16-bit machines) */
    
#define token_t unsigned normal   

/*
 * What boundary to align a "normal" to when it is stored in memory.
 * On a 68000, this has to be at least 2; on a VAX or a 68020,
 * it can be 1 if you want to conserve memory at the expense of
 * speed (VAXen and 68020's will fetch unaligned data without complaint,
 * but it takes longer), or 4 if you want maximum speed.
 * If in doubt, use sizeof(normal).
 */
/*
 * Right now this has to be a multiple of sizeof(normal), because the
 * branch code in the kernel uses branch displacements measured in
 * units of 1 normal.
 */

#define ALIGN_BOUNDARY (4)  
                            /* Good number for 68000 */
#define MEMPAGE 2048        
                            /* should be at least as big as the page size
                                on your machine (bytes);*/

/***************** MIRELLA SIZE PARAMETERS *******************************/

/* enlarged 07/01/29 */

#define DICT_SIZE (0x200000)   
                            /* normals (4-byte integers) */
       /* 8 MB in 4-byte words; includes user area, file area,
       memory area, and pre-break dictionary (primaries and c-links).
       From 5.1 on, the break between the (primaries + clinks) and the
       user dictionary is dynamically allocated on the nearest 4K
       boundary; each clink takes about 36 bytes.
       Origin is up_4th, as set dynamically on a 64K boundary
       by init_malloc() */

#define USER_AREA (0x80)   
        /* normals */
       /* size of Mirella user area, 512 B, 128 user variables.
             Origin up_4th */

#define FILE_AREA (0x400)       
        /* normals */
        /* size of file structure table. 4096 bytes, 48 files plus some
        take 512 bytes; room to expand structure tables for buffering ?? */

#define MEM_AREA (0x2000)
        /* normals */
        /* size of memory allocation structures, 32k bytes, 1024 areas */

#define APP_AREA (0x1000)   
        /* normals */
       /* application "user" area, 16 kb. Origin USERORIGIN + 4*DICT_SIZE */

/* memory is allocated at start of Mirella from up_4th to              */
/* up_4th + 4*(DICT_SIZE + APP_AREA), or                               */
/* from 0x80000 to 0xA4000 with original parameters. There is          */
/* an error message when a dictionary image is read in if up_4th has   */
/* been moved since the image was made, or if dictb_4th has moved.     */
/* The dictionary saver saves from up_4th to the end of the            */
/* used dictionary, and the application area.                          */
/*                                                                     */ 
/***********************************************************************/

#define V_NR_USER        up_4th[0]
#define V_SPAN           up_4th[1]
#define V_TO_IN          up_4th[2]
#define V_BASE           up_4th[3]
#define V_BLK            up_4th[4]
#define V_NR_TIB         up_4th[5]
#define V_STATE          up_4th[6]
#define V_CURRENT        up_4th[7]
#define V_CONTEXT        up_4th[8]
   /* 9, 10, 11, 12 are part of context */
#define V_DELIMITER      up_4th[13]
#define V_SPZERO         up_4th[14]
#define V_RPZERO         up_4th[15]
#define V_UPZERO         up_4th[16]
/* LASTP needed for use as an lvalue, where the cast doesn't work */
#define V_LAST           ((dic_en_t *)up_4th[17])
#define V_LASTP          up_4th[17]
#define V_NR_OUT         up_4th[18]
#define V_NR_LINE        up_4th[19]
#define V_VOC_LINK       up_4th[20]
#define V_DPL            up_4th[21]
#define V_WARNING        up_4th[22]
#define V_CAPS           up_4th[23]
#define V_ERRNO          up_4th[24]
#define V_TSPZERO        up_4th[25]
#define V_FSPZERO        up_4th[26]
#define V_SSPZERO        up_4th[27]
#define V_MIRELLA        up_4th[28]
#define V_PSTRING        up_4th[29]
#define V_APAREA         up_4th[30]
#define V_FPF            up_4th[31]
#define V_FORIG          up_4th[32]
#define V_FSSIZE         up_4th[33]
#define V_MORIG          up_4th[34]
#define V_MSSIZE         up_4th[35]
#define V_ITAL           up_4th[36]
#define V_IO             up_4th[37]
#define V_NOECHO         up_4th[38]
#define V_LN_IN          up_4th[39]
#define V_LASTFILE       up_4th[40]
#define V_SILENTOUT      up_4th[41]
#define V_FBZERO         up_4th[42]
#define V_NOCOMPILE      up_4th[43]
#define V_REMIN          up_4th[44]
#define V_RES_STK        up_4th[45]
#define V_INTERFLG       up_4th[46]
#define V_NLVAR          up_4th[47]
#define V_LOCALDIC       up_4th[48]
#define V_CCNEST         up_4th[49]
#define V_CCNOFF         up_4th[50]
#define NEXT_VAR            (51)

#define MAXUSER 100

#define NVOCS 5     
        /* number of vocabularies in search order at a time */

#define INTERPRETING 0
#define COMPILING 1
#define FEOF -1

#define TIBSIZE     254
#define PSSIZE     1024
#define FSSIZE      200
#define TSSIZE      200
/* SSNSTRING must be a power of 2, so SSNSTRING-1 is a mask */
#define SSNSTRING  256
/*256*/ /* 16*/
#define SSSIZE    65824
        /* 65824 */
        /* was 4360 */
        /* 65824 = 256*(SSNSTRING + 1) + 32*/
        /* have 256 256-byte strings with alignment & overflow */
#define RSSIZE      100
#define XRSSIZE      20
#define MAXVARS     100
#define CBUFSIZE    2
#define MAXSTRING   255

#define compile(token) (*dp_4th++ = (normal)token)

#define ps_top   (&par_stk[PSSIZE-10])   
                                        /* Top of parameter stack */
#define fs_top   (&fl_stk[FSSIZE-10])    
                                        /* Top of floating stack */
#define ts_top   (&tem_stk[TSSIZE-10])   
                                        /* Top of temporary stack */
#define ss_top   (&str_stk[0])   
                                        /* 'Top' (origin) of string stack */

struct d_e_t {
    struct d_e_t  *link;
    u_char flags;
    char name;
};
#define dic_en_t struct d_e_t

/* number of hashing threads. Must be power of 2 */
#define NTHREADS 8  
/* NTHREADS-1 */
#define NTM1 7

typedef struct voc_t {
    dic_en_t     *last_word[NTHREADS];   /* hashing */
    normal       *voc_link;
} vocab_t;
#define VOCAB_T_DEF

/* following definition for hash is very (maybe too) simple. */
   
#define hash(voc,str) (&((voc)->last_word[(*((str)+1) + *((str)+2)) & NTM1] ))

/* global variables in mirella.c */
/* memory map and dictionary */
extern normal   *up_4th ;     /* user origin , also V_UPZERO, 'up0' */
extern normal   *fp_4th;      /* file area origin, also V_FORIG, 'fp0' */
extern normal   *mp_4th;      /* memalloc area origin, also V_MORIG, 'mp0' */
extern normal   *origin ;     /* for init; will be set to dict_orig. 
                                pushed by 'origin' */
extern normal   *dict_orig;   /* dict origin, primaries and clinks. */
extern normal   *dictb_4th;   /* origin of extern normal forth dictionary. 
                                pushed  by <*> */
extern normal   *dp_4th;      /* dictionary pointer */
extern normal   *dict_end;    /* end */
extern normal   *ap_env;      /* origin of application environment area,
                                also V_APAREA, 'app0' */
/* stacks and stack pointers */
extern normal   par_stk[];
extern normal   *xsp_4th;
extern token_t  *rtn_stk[];    /* rudimentary external return stack. Real
                                return stack is auto variable in inner 
                                interpreter. */
extern token_t **xrp_4th;
extern normal tem_stk[];
extern normal *xtsp_4th;
extern float fl_stk[];
extern float *xfsp_4th;
extern char *ss_bot;
extern char *str_stk;
extern char *xssp_4th;
/* miscellany */
extern token_t  comp_buf[];
extern char     digits[];
#ifndef vms
#  include <errno.h>
#endif
extern int      insane;
extern normal   interrupt;
extern char     nmbr_buf[];
extern char     pstring[];
extern char     tibbuf_4th[];
extern normal   variable[];
extern char     wordbuf[];
extern int      dict_image;
extern char     out_string[];

/* global variables in mirellio.c */

extern char   *in_fl_name;

/* global variables in mircintf.c */

extern char *ss_bot;








