#ifndef Void
#  define DEF_Void
#  define Void char
#endif
#define INT int

#ifndef fopena
   FILE *fopena P_ARGS(( char *, char * ));
#endif
#ifndef fopenb
   FILE *fopenb P_ARGS(( char *, char * ));
#endif
# ifndef token_t
#   define token_t unsigned normal
#endif

FILE *mcfopen P_ARGS(( char *name, char **np ));
FILE *mcfopenb P_ARGS(( char *name, char **np ));
int dos_clr_scr P_ARGS(( void ));
int dos_del_line P_ARGS(( void ));
int dos_forw_curs P_ARGS(( int n ));
int dos_put_curs P_ARGS(( unsigned int, unsigned int ));
int dos_q_pcurs P_ARGS(( void ));
int dos_query_curs P_ARGS(( void ));
int dos_res_curs P_ARGS(( void ));
int dos_sav_curs P_ARGS(( void ));
int dos_scrline P_ARGS(( char *, int ));
int dos_scrnput P_ARGS(( int ));
int edit_outfun P_ARGS(( char *s ));
int kill_callback P_ARGS(( CALLBACK_FUNC_PTR func ));
int pop_callback P_ARGS(( void ));
void push_callback P_ARGS(( CALLBACK_FUNC_PTR func ));
int add_timer P_ARGS(( char *word, int t ));
int delete_timer P_ARGS(( int i ));
void print_timers P_ARGS(( void ));
void fgraph P_ARGS(( void ));
void forth_scanf P_ARGS(( void ));
INT get1char P_ARGS(( int c ));
#if !defined(SGI)
   int gettimeofday();		    /* (struct timeval *, struct timezone *) */
#endif
void lgraph P_ARGS(( void ));
INT long makelong P_ARGS(( char * ));
void lsgraph P_ARGS(( void ));
INT main P_ARGS(( int argc, char *argv[] ));
int mir_main P_ARGS(( int argc, char *argv[] ));
void mar_create P_ARGS(( char *str ));
char * m_gets P_ARGS(( char *str ));
char * init_fbuf P_ARGS(( void ));
char * mfgets P_ARGS(( char *s, int n ));
int mfgetsnc P_ARGS(( char *s, int n ));
int mfgetsncb P_ARGS(( char *s, int n ));
char **matrix P_ARGS(( int nr, int nc, int elsize ));
Void ***matrix3 P_ARGS(( int nmat, int rows, int cols, int elsize ));
char *SFIRST P_ARGS(( char *, int ));
char *SNEXT P_ARGS(( void ));
char *_strncpy P_ARGS(( int d, int s, int n ));
char *basename P_ARGS(( char *str ));
char *baseptr P_ARGS(( char *s ));
char *dicalloc P_ARGS(( char *name, int n ));
char *dirname P_ARGS(( char *s ));
char *dsgetcwd P_ARGS(( char *, int ));
char *envalloc P_ARGS(( int s ));
void extchk P_ARGS(( char *fname, char *ext, char *filename ));
char *extptr P_ARGS(( char *s ));
char *fbasename P_ARGS(( char *fstr ));
char *fcanon P_ARGS(( char *s ));
char *findstr P_ARGS(( char *s1, char *s2, char **pnc ));
char *fltstr P_ARGS(( double arg, int wid, int dec ));
char *getltime P_ARGS(( void ));
void mfcopy P_ARGS(( char *src, char *dest ));
void getpbm P_ARGS(( char *fname, int n ));
void getpgm P_ARGS(( char *fname, int n ));
void getppm P_ARGS(( char *fname, int n ));
void getpnm P_ARGS(( char *fname, int n ));
char *getpnmhead P_ARGS(( char *file ));
void hello P_ARGS(( int i ));
char *init_malloc P_ARGS(( int needed ));
char *isdir P_ARGS(( char *dirname ));
char *makepbmhead P_ARGS(( int xsize, int ysize ));
char *makepnmhead P_ARGS(( int magicno, int xsize, int ysize, int maxgrey ));
Void *matralloc P_ARGS(( char *name, int rows, int cols, int elsize ));
Void *mat3alloc P_ARGS(( char *name, int nmat, int rows, int cols, int elsize ));
char *mcfname P_ARGS(( char *fname ));
char *memalloc P_ARGS(( char *name, int size ));
char *nextword P_ARGS(( char *cpp ));
char *parse_cpp_line P_ARGS(( char *line, int *skip, int *nest ));
void putpbm P_ARGS (( char *fname ));
void putpgm P_ARGS (( char *fname ));
void putppm P_ARGS (( char *fname ));
void putpnm P_ARGS (( char *fname ));
void putpgmimg P_ARGS (( int zlo, int zhi, char *fname ));
void putppming P_ARGS (( int zlo, int zhi, char *fname ));
char *puttogether P_ARGS(( char *s1, char *s2 ));
char *scat P_ARGS(( char *s1, char *s2 ));
char *screensize P_ARGS(( void ));
char *strtolower P_ARGS(( char *s ));
char *tryalloc P_ARGS(( int nbytes ));
char *zitoa P_ARGS(( int n, int fld ));
double _ceil P_ARGS(( int x ));
double _floor P_ARGS(( int x ));
double afamax P_ARGS(( float *a, int n, int *pj ));
double amoeba P_ARGS(( float **p, int ndim, double ftol, double (*funk)(), normal *piter ));
double anhist P_ARGS(( int *histarr, int ncell, float *qarr ));
double defint P_ARGS(( int i, int j, float *x, float *y, float *k ));
double dotabfun();			/* double z, struct ftab_t *ftptr */
void do_init_2 P_ARGS(( void ));
double exp10 P_ARGS(( double f ));
double fainterp P_ARGS(( float *buf, double findx ));
double farsum P_ARGS(( float *buf, int n ));
double fasolve P_ARGS(( float *buf, normal n, double value ));
double faverage P_ARGS(( float *buf, int n ));
void   fargsmth P_ARGS(( float *bufin, float *bufout, int n, double sigma ));
double findmode P_ARGS(( int *histarr, int ncell, float *derarr, int nder ));
double foldder P_ARGS(( int j, int *histarr, int ncell, float *derarr, int nder ));
double integral P_ARGS(( double a, double b, float *x, float *y, float *k, int n ));
double linterp P_ARGS(( double z, float *x, float *y, int n ));
double matinv P_ARGS(( float *pa[], float *pai[], int n ));
double nasolve P_ARGS(( normal *buf, normal n, normal value ));
double naverage P_ARGS(( int *buf, int n ));
double nrand P_ARGS(( void ));
double fpoly P_ARGS(( double z, float a[], int m ));
double power P_ARGS(( double farg, int iarg ));
void mtv P_ARGS(( void ));
double ran2 P_ARGS(( void ));
double ran3 P_ARGS(( void ));
double samoeba P_ARGS(( float *x, float *dx, double (*funk)(), int nd ));
double sasolve P_ARGS(( short *buf, normal n, int value ));
double saverage P_ARGS(( short *buf, int n ));
double spft P_ARGS(( double z, float *x, float *y, float *k, int n, float *pd, int key ));
double spline P_ARGS(( double z, float *x, float *y, float *k, int n ));
float **mtick P_ARGS(( normal *cfa ));
int _STOPW P_ARGS(( int f ));
int _fgets P_ARGS(( char *buf, int n, FILE *fp ));
int _mcompare P_ARGS(( Const Void *p1, Const Void *p2 ));
int _yesno P_ARGS(( void ));
int app_keypd P_ARGS(( void ));
int aticonfig P_ARGS(( void ));
int atiline P_ARGS(( int line ));
int bells P_ARGS(( int n ));
int bflush P_ARGS(( void ));
int call_callback_fns P_ARGS(( void ));
int cexec P_ARGS(( token_t tp ));
int cexpect P_ARGS(( char *addr, normal count ));
int mexpect P_ARGS(( char *addr, normal count ));
int expect P_ARGS(( char *addr, normal count ));
int chfree P_ARGS(( Void * ));
int cinterp P_ARGS(( char *cstr ));
int circe P_ARGS(( double x, float *t, int n ));
int clear_phy_scr P_ARGS(( void ));
int clear_scr P_ARGS(( void ));
int closeblk P_ARGS(( int des ));
int cprint P_ARGS(( char *str ));
int creata P_ARGS(( char *, int flags ));
#ifndef creatb
   int creatb P_ARGS(( char *, int ));
#endif
#ifndef creatblk
   int creatblk P_ARGS(( char *, int ));
#endif
int desi P_ARGS(( int i ));
int digit  P_ARGS(( int base, int c ));
int diropen P_ARGS(( char *file ));
int dopause P_ARGS(( void ));
int dos_getdisplay P_ARGS(( void ));
int dos_scrnprt P_ARGS(( char * ));
#ifdef VARARGS
   int mdprintf();
#else
   int mdprintf P_ARGS(( char *, ... ));
#endif
int draw_string P_ARGS(( char *str, int x, int y ));
int dscan P_ARGS(( char *s ));
int dschdir P_ARGS(( int dir ));
int edit_hist P_ARGS(( char *cmd_line, int pflg ));
int edit_line P_ARGS(( char *line, int pflg ));
int eprompt P_ARGS(( void ));
int erase_line P_ARGS(( void ));
char *get_edit_str P_ARGS(( char * ));
#if defined(HSIZE)
   EDIT *find_str P_ARGS(( char *, int, EDIT *, EDIT * ));
#endif
int more P_ARGS(( char * ));
int map_key P_ARGS(( void ));
int eval_string P_ARGS(( char *line ));
int find P_ARGS(( char **sp ));
int forw_curs P_ARGS(( int n ));
int free86 P_ARGS(( int addr ));
int g_isalloc P_ARGS(( int n ));
int genchar P_ARGS(( char *str, u_char **out, int xout, int scale ));
int get1char P_ARGS(( int c ));
int get_scur P_ARGS(( void ));
int getstat();				/* char *name, struct dir_t *sp */
char *get_def_value P_ARGS(( char *name ));
int gopen P_ARGS(( char *name, int mode, int it ));
int gstrlen P_ARGS(( char *string, double angle, double expand ));
int hand_lit P_ARGS(( char *str ));
int hasdir P_ARGS(( char *s ));
int hasext P_ARGS(( char *s ));
int home_scur P_ARGS(( void ));
int in_intp P_ARGS(( token_t *ip ));
int input P_ARGS(( int port ));
void interp_tokarr P_ARGS(( int nitem, char **lptr ));
void interp_cstr P_ARGS(( u_char *cstr ));
int intp_word P_ARGS(( char *str ));
int is_defined P_ARGS(( char *name ));
int isasciibuf P_ARGS(( char *buf, int len, char *eol, int it ));
int iran2 P_ARGS (( void ));
int iran3 P_ARGS (( void ));
int key_avail P_ARGS(( void ));
int lfind P_ARGS(( char **sp ));
#ifndef lseekblk
int lseekblk P_ARGS(( int des, int offs, int orig ));
#endif
void m_bye P_ARGS(( int exit_code ));
Void *m_sbrk P_ARGS(( int size ));
int main P_ARGS(( int ac, char *av[] ));
int malloc86 P_ARGS(( int n ));
int memfree P_ARGS(( char *name ));
int qmemfree P_ARGS(( char *name ));
#if defined(MIRCINTF_H)
void misetup P_ARGS(( int *nv, int *nf, char ***varar, void (***funcar)(), MIRCINTF *(**)(), void (**i0)(), void (**i1)(), void (**i2)(), void (**i3)() ));
void mesetup P_ARGS(( int *nv, int *nf, char ***varar, void (***funcar)(), MIRCINTF *(**)(), void (**i0)(), void (**i1)(), void (**i2)(), void (**i3)() ));
void m1setup P_ARGS(( int *nv, int *nf, char ***varar, void (***funcar)(), MIRCINTF *(**)(), void (**i0)(), void (**i1)(), void (**i2)(), void (**i3)() ));
void m2setup P_ARGS(( int *nv, int *nf, char ***varar, void (***funcar)(), MIRCINTF *(**)(), void (**i0)(), void (**i1)(), void (**i2)(), void (**i3)() ));
void m3setup P_ARGS(( int *nv, int *nf, char ***varar, void (***funcar)(), MIRCINTF *(**)(), void (**i0)(), void (**i1)(), void (**i2)(), void (**i3)() ));
#endif
int mfgetc P_ARGS(( void ));
int mfputs P_ARGS(( char *s ));
int mkey_avail P_ARGS(( void ));
char *mir_mktemp P_ARGS(( char * ));
int mopen P_ARGS(( char *name, int mode ));
#ifdef VARARGS
   int mprintf();
#else
   int mprintf P_ARGS(( char *, ... ));
#endif
int mread P_ARGS(( char *addr, int nby ));
int mtype P_ARGS(( void ));
char *chan_name P_ARGS(( void ));
int munread P_ARGS(( char *addr, int nby ));
int mv_cursor P_ARGS(( int, int ));
int mwrite P_ARGS(( char *addr, int nby ));
#ifdef FTERMINAL
   int newterminal P_ARGS(( FTERMINAL * p ));
#endif
void need_refresh P_ARGS(( void ));
int nicelims P_ARGS(( double *pllim, double *pulim ));
int num_keypd P_ARGS(( void ));
int number P_ARGS(( char *str, int *n ));
int numlabel P_ARGS(( double f, char *s, double mag, int logf, double maxq ));
int opena P_ARGS(( char *, int ));
#ifndef openb
   int openb P_ARGS(( char *, int ));
#endif
#ifndef openblk
   int openblk P_ARGS(( char *, int ));
#endif
int parse P_ARGS(( char *str, char **argv ));
int pdopause P_ARGS(( void ));
int put_scur P_ARGS(( void ));
int putcur P_ARGS(( int x, int y ));
int q_key P_ARGS(( void ));
int q_restart P_ARGS(( void ));
int qdsp_setup P_ARGS(( void ));
int raw_out P_ARGS(( int c ));
#ifndef readblk
   int readblk P_ARGS(( int des, Void *buf, int n ));
#endif
int readc P_ARGS(( void ));
int res_scur P_ARGS(( void ));
void reset_kbd P_ARGS(( void ));
void reset_tib P_ARGS(( void ));
void reset_vocabs P_ARGS(( int force ));
int rewindblk P_ARGS(( int des ));
int rexpect P_ARGS(( char *addr, int count ));
int rget1char P_ARGS(( void ));
int round P_ARGS(( double f ));
int sav_scur P_ARGS(( void ));
#ifdef VARARGS
   int scrprintf();
#else
   int scrprintf P_ARGS(( char *, ... ));
#endif
void set_defs_file P_ARGS(( char *file ));
void set_grafcolor P_ARGS(( normal col ));
void set_kbd P_ARGS(( void ));
int setmask P_ARGS(( float *specarr, char *mask, double llim, double ulim, int npt ));
int starttimer P_ARGS(( void ));
int stdflsh P_ARGS(( void ));
int stdline P_ARGS(( int line ));
int stdprt P_ARGS(( char *str ));
int stdput P_ARGS(( int c ));
int suspend P_ARGS(( void ));
#ifndef tellblk
   int tellblk P_ARGS(( int des ));
#endif
void unreadc P_ARGS(( int c ));
int value P_ARGS(( char *name ));
#if defined(VOCAB_T_DEF)
   int vfind P_ARGS(( char **sp, vocab_t *voc ));
#endif
void vgagrinit P_ARGS(( int mode ));
void vgaiminit P_ARGS(( int mode ));
int vmsbf_chan P_ARGS(( int des ));
int wildmatch P_ARGS(( char *str, char *pat ));
int writeascii P_ARGS(( int des, char *buf, int n, int ftype, int reclen, char *eol, int endflg, int runflg, int stoansi ));
#ifndef writeblk
   int writeblk P_ARGS(( int des, Void *buf, int n ));
#endif
int xtod P_ARGS(( double x ));
int ytod P_ARGS(( double y ));
void mirelinit P_ARGS(( int argc, char *argv[] ));
void mirellam P_ARGS(( void ));
INT move_from P_ARGS(( int from, int to, int n ));
char * mrgets P_ARGS(( char *str ));
void mspoint P_ARGS(( void ));
normal *aligned P_ARGS(( Void *addr ));
normal *get_pdic P_ARGS(( char *filename ));
normal *naligned P_ARGS(( int addr ));
normal attach P_ARGS(( int pid ));
normal flims P_ARGS(( double x1, double x2, float *p1, float *p2 ));
normal key P_ARGS(( void ));
normal mseek P_ARGS(( int org, normal pos ));
normal mtell P_ARGS(( void ));
normal nainterp P_ARGS(( normal *buf, double findx ));
normal narsum P_ARGS(( normal *buf, normal n ));
normal ntimer P_ARGS(( int tp ));
normal sainterp P_ARGS(( short *buf, double findx ));
normal sarsum P_ARGS(( short *buf, normal n ));
void   sargausmth P_ARGS(( short* bufin, short* bufout, int n, double sigma));
void   fargausmth P_ARGS(( float* bufin, float* bufout, int n, double sigma));
void   smatgausmth P_ARGS(( short **mat, int xs, int ys, double sigma));
void   fmatgausmth P_ARGS(( float **mat, int xs, int ys, double sigma));
normal shellesc P_ARGS(( void ));
normal stopwatch P_ARGS(( void ));
normal vgetpid P_ARGS(( char *procname ));
/*INT pwrite P_ARGS(( int fdes, int buf, int n ));*/ /* unused, conf unistd.h*/
void sgraph P_ARGS(( void ));
INT spawn P_ARGS(( char *cmd, char *procname ));
void ssgraph P_ARGS(( void ));
struct dir_t *dirread P_ARGS(( void ));
struct dir_t *nextfile P_ARGS(( void ));
struct fib_des_t *vmsbf_fib P_ARGS(( int des ));
struct ftab_t *filefnarray P_ARGS(( char *file, int xcol, int ycol, int ord ));
token_t *name_from P_ARGS(( char *nfa ));
char *appendm P_ARGS(( char *name ));
char *blword P_ARGS(( void ));
char *canon P_ARGS(( char *fstr ));
char *dsgetenv P_ARGS(( u_char *str ));
char *fspush P_ARGS(( char *fstr ));
Void *gd_line P_ARGS(( int line ));
char *sdown P_ARGS(( void ));
char *spush P_ARGS(( char *str ));
char *sup P_ARGS(( void ));
char *to_cstr P_ARGS(( char *from ));
char *word P_ARGS(( int delim ));
unsigned int gtime P_ARGS(( void ));
void CFILE P_ARGS(( void ));
void _chfree P_ARGS(( void ));
void _faccess P_ARGS(( void ));
void _famaxmin P_ARGS(( void ));
void _fchannel P_ARGS(( void ));
void _logfile P_ARGS(( void ));
void logfile P_ARGS(( char * ));
void logoff P_ARGS(( void ));
void _matralloc P_ARGS(( void ));
void _mchannel P_ARGS(( void ));
void _mclose P_ARGS(( void ));
void _memalloc P_ARGS(( void ));
void _memfree P_ARGS(( void ));
void _mfclose P_ARGS(( void ));
void _mfopen P_ARGS(( void ));
void _mgopen P_ARGS(( void ));
void _mread P_ARGS(( void ));
void _mretension P_ARGS(( void ));
void _mrewind P_ARGS(( void ));
void _mrskip P_ARGS(( void ));
void _mseek P_ARGS(( void ));
void _mskip P_ARGS(( void ));
void _mtback P_ARGS(( void ));
void _mtell P_ARGS(( void ));
void _mtfwd P_ARGS(( void ));
void _mtgetfno P_ARGS(( void ));
void _mtposition P_ARGS(( void ));
void _munload P_ARGS(( void ));
void _munread P_ARGS(( void ));
void _mweof P_ARGS(( void ));
void _mwrite P_ARGS(( void ));
void _namaxmin P_ARGS(( void ));
void _pause P_ARGS(( void ));
void _pushd P_ARGS(( void ));
void _puttogether P_ARGS(( void ));
void _samaxmin P_ARGS(( void ));
void alias P_ARGS(( char *name, char *name2 ));
void align P_ARGS(( void ));
void alloc_ss P_ARGS(( void ));
void allocgraph P_ARGS(( int xs, int ys, int n ));
void aloc_dic P_ARGS(( void ));
void applinit P_ARGS(( void ));
void bdrawlist P_ARGS(( void ));
void bmtodisk P_ARGS(( int bmbase, int xs, int ys, int mag, int filename ));
void bmtowin P_ARGS(( void ));
#ifndef bufcpy
   void bufcpy P_ARGS(( char *d, char *s, int n ));
#endif
void caprt P_ARGS(( char *ia, int n, char *titl ));
void cleardirs P_ARGS(( void ));
#ifndef clearn
   void clearn P_ARGS(( int n, char *buf ));
#endif
void clrhist P_ARGS(( int *histarr, int ncell ));
void clrint P_ARGS(( void ));
void cmove P_ARGS(( char *from, char *to, normal length ));
void cmove_up P_ARGS(( char *from, char *to, unsigned length ));
void comma_string P_ARGS(( char *str ));
void crcchk P_ARGS(( int *ibuf, int nibuf, int *pcksum, int *pcrc ));
void cspush P_ARGS(( char *str ));
void ctoscreen P_ARGS(( void ));
void dash_trlng P_ARGS(( void ));
void dblankn P_ARGS(( int, short * ));
void define P_ARGS(( char *name, int val ));
void delete_last_history P_ARGS(( void ));
void delgwin P_ARGS(( int nw ));
void dfdotr P_ARGS(( double flt, int dec, int width ));
#ifdef DLD
void setup_dld P_ARGS(( void ));
void read_objfile P_ARGS(( char *obj_file ));
void unread_objfile P_ARGS(( char *obj_file ));
#endif
void dirclose P_ARGS(( void ));
void dot_pop P_ARGS(( void ));
void dot_push P_ARGS(( int x ));
void draw P_ARGS(( int x, int y, int pen ));
void drawlist P_ARGS(( void ));
void dsbackup();			/* struct dir_t *sptr */
void dsfilln P_ARGS(( int n, int buf, int chr ));
void dsrestore P_ARGS(( int dirflg, char *pat ));
void dvztrap P_ARGS(( void ));
void e4010 P_ARGS(( void ));
void ebells P_ARGS(( int n ));
void egraph P_ARGS(( void ));
void emit P_ARGS(( int c ));
void erret P_ARGS(( char *arg ));
void error P_ARGS(( char *str ));
void erswin P_ARGS(( void ));
void exec_one P_ARGS(( token_t token ));
void exgraph P_ARGS(( int x0, int y0, int xsz, int ysz, int n ));
void famaxmin P_ARGS(( float *buf, normal n, float *pmin, float *pmax, normal *pmindx, normal *pmaxdx ));
double famax P_ARGS(( float*buf, normal n ));
double famin P_ARGS(( float*buf, normal n ));
normal namax P_ARGS(( normal *buf, normal n ));
normal samax P_ARGS(( short *buf, normal n ));
void faprt P_ARGS(( float *x, int n, char *titl ));
void faradd P_ARGS(( float *src, float *dest, int n ));
void farcon P_ARGS(( float *buf, normal n, double c ));
void farfaccum P_ARGS(( float *src, float *dest, normal n, double fac ));
void farfmul P_ARGS(( float *buf, normal n, double fac ));
void fdot P_ARGS(( double flt ));
void fdt_pop P_ARGS(( void ));
void fdt_push P_ARGS(( double x ));
void ferret P_ARGS(( int n ));
void ffgraph P_ARGS(( void ));
void ffill P_ARGS(( char *to, unsigned length, int with ));
void filln P_ARGS(( int n, char *buf, int chr ));
void set_dirpath P_ARGS(( void ));
void add_dirpath P_ARGS(( char *path ));
void flabel P_ARGS(( void ));
void flgtrap P_ARGS(( void ));
void flushit P_ARGS(( void ));
void fmattopgm P_ARGS(( double fxl, double fyl, double fxh, double fyh, double black, double white, int nx, int ny, float **mat ));
void fputrap P_ARGS(( void ));
void fqsort P_ARGS(( void ));
void frame P_ARGS(( double sx1, double sx2, int nx, double sy1, double sy2, int ny ));
void frchk P_ARGS(( double f, double f1, double f2, char *msg ));
void freeall P_ARGS(( void ));
void freeback P_ARGS(( void ));
void freegraph P_ARGS(( int n ));
void ftimes P_ARGS(( void ));
void g4010file P_ARGS(( char *filename ));
void g_dox P_ARGS(( void ));
void g_doy P_ARGS(( void ));
void gallocgraph P_ARGS(( int xs, int ys, int n, int vecflg, int bitscol ));
void gbegin P_ARGS(( void ));
void gbox P_ARGS(( int x1, int y1, int lx, int ly ));
void gd_init P_ARGS(( void ));
void gd_todis P_ARGS(( Void *from, Void *to, int nby ));
void gend P_ARGS(( void ));
void gers P_ARGS(( void ));
void get_dic P_ARGS(( char *filename ));
int image_to_buf P_ARGS(( int *x, int *y));
void get_rdic P_ARGS(( void ));
void gframe P_ARGS(( void ));
void gmode P_ARGS(( int mode ));
void gpoint P_ARGS(( int x, int y ));
void g_setpoint P_ARGS(( int x, int y ));
void graph P_ARGS(( float xa[], float ya[], int n ));
void gregress();			/* float *x, float *y, char *mask,
					   int n, struct regress *pstat */
void gsplinit P_ARGS(( float *x, float *y, float *k, int n, double q2b, double q2e ));
void gstring P_ARGS(( char *string, double angle, double expand ));
void gtoscreen P_ARGS(( int x0, int y0 ));
void gwindow P_ARGS(( void ));
void gwpgraph P_ARGS(( char *filename ));
void hdl_sig P_ARGS(( int ));
void hdl_int P_ARGS(( int ));
void hdl_quit P_ARGS(( int ));
void hide P_ARGS(( void ));
void histogram P_ARGS(( void ));
void history_list P_ARGS(( int dir ));
void hsdraw P_ARGS(( double xf, double yf, int pen ));
void htick P_ARGS(( normal y, normal lyflg ));
void ilinecolor P_ARGS(( char * color));
void illtrap P_ARGS(( void ));
void ingraph P_ARGS(( char *fname, int n ));
#if defined(MIRCINTF_H)
void init_cif P_ARGS(( MIRCINTF *(*)(), normal *vparr, void (*fparr[])() ));
MIRCINTF *init_prims P_ARGS(( void ));
#endif
void init_dic P_ARGS(( void ));
void init_ed P_ARGS(( void ));
void init_file P_ARGS(( void ));
void init_io P_ARGS(( void ));
void init_mem P_ARGS(( void ));
void init_mgs P_ARGS(( void ));
void init_scr P_ARGS(( void ));
void init_user P_ARGS(( void ));
void init_var P_ARGS(( void ));
void initmask P_ARGS(( char *mask, int npt ));
void insgraph P_ARGS(( int x0, int y0, int n ));
void invector P_ARGS(( char *fname, int n ));
void iqsort P_ARGS(( void ));
void irchk P_ARGS(( int n, int n1, int n2, char *msg ));
void kwait P_ARGS(( void ));
void l_create P_ARGS(( char *str, token_t cf ));
void label P_ARGS(( void ));
void ldraw P_ARGS(( int sx, int sy, int pen ));
void lineconnect P_ARGS(( void ));
void linsolv P_ARGS(( float *pa[], float b[], float x[], int n ));
void load P_ARGS(( char *filename ));
void lsdraw P_ARGS(( double xf, double yf, int pen ));
void lwait P_ARGS(( void ));
void madd P_ARGS(( float *pa[], float *pb[], float *pc[], int n, int m ));
void makeder P_ARGS(( double sigma, float *derarr, int nder ));
void makeimmediate P_ARGS(( void ));
void mchannel P_ARGS(( int i ));
void mclose P_ARGS(( int i ));
void mcmul P_ARGS(( float *pa[], double c, float *pb[], int n, int m ));
void memchk P_ARGS(( int, char *, int ));
void mfputc P_ARGS(( int c ));
void mfungetc P_ARGS(( int c ));
void mirelcmd P_ARGS(( int argc, char **argv ));
void mkorpol P_ARGS(( float *w, float *x, int n, int m ));
void mmul P_ARGS(( float *pa[], float *pb[], float *pc[], int n, int m, int l ));
void movescreen P_ARGS(( int xf, int yf ));
void mprt P_ARGS(( float *px[], int n, int m, char titl[] ));
void mqsort P_ARGS(( void ));
void msdraw P_ARGS(( void ));
void mvmul P_ARGS(( float *pa[], float *vec, float *pvec, int n, int m ));
void namaxmin P_ARGS(( normal *buf, normal n, normal *pmin, normal *pmax, normal *pmindx, normal *pmaxdx ));
void naprt P_ARGS(( int *ia, int n, char *titl ));
void naradd P_ARGS(( int *src, int *dest, int n ));
void narbswp P_ARGS(( normal *buf, int n ));
void narcon P_ARGS(( normal *buf, normal n, normal c ));
void narfaccum P_ARGS(( normal *src, normal *dest, normal n, double fac ));
void narfmul P_ARGS(( normal *buf, normal n, double fac ));
void ndot P_ARGS(( int n ));
void norm_input P_ARGS(( void ));
void nulldraw P_ARGS(( int x, int y, int pen ));
void ogmode P_ARGS(( void ));
void oldd_init P_ARGS(( void ));
void orpfit P_ARGS(( float y[], float ay[], int m ));
void out_intp P_ARGS(( void ));
void outgraph P_ARGS(( char *fname ));
void output P_ARGS(( int port, int val ));
void outvector P_ARGS(( char *fname ));
void plotbox P_ARGS(( int xl, int yl, int xh, int yh ));
void plotinall P_ARGS(( void ));
void plotinframe P_ARGS(( void ));
void pointdraw P_ARGS(( double xf, double yf, int pen ));
void points P_ARGS(( void ));
void pop_scr P_ARGS(( int i ));
void popchan P_ARGS(( void ));
void popd P_ARGS(( void ));
void popgwin P_ARGS(( int nw ));
void ppower P_ARGS(( void ));
void print_string P_ARGS(( char *str ));
int define_key P_ARGS(( char *, int ));
#ifdef INC_TTY_H
TTY *ttyopen P_ARGS(( char [], int, char *[], int (*)() ));
void tty_index_caps P_ARGS(( TTY *, int [], int [] ));
int ttygets P_ARGS(( TTY *, char [], char [], int ));
int ttygeti  P_ARGS(( TTY *, char [] ));
#endif
int define_key P_ARGS(( char *, int ));
void erase_str P_ARGS(( char *, int ));
char *match_pattern P_ARGS(( char *, char *, char ** ));
void prompt P_ARGS(( void ));
void push_scr P_ARGS(( void ));
void pushchan P_ARGS(( int n ));
void pushgwin P_ARGS(( void ));
void puteol P_ARGS(( void ));
void puthist P_ARGS(( double q, int *histarr, double cell, double origin, int ncell ));
void qt_create P_ARGS(( char *str, token_t cf ));
void r_fqsort P_ARGS(( void ));
void r_iqsort P_ARGS(( void ));
void r_sqsort P_ARGS(( void ));
void read_file P_ARGS(( int file ));
void read_hist P_ARGS(( void ));
void regress P_ARGS(( float *x, float *y, char *mask, int n ));
void reopenr P_ARGS(( void ));
void reopenw P_ARGS(( void ));
void res_scr P_ARGS(( void ));
void reset_scr P_ARGS(( void ));
int res_help P_ARGS(( void ));
void reveal P_ARGS(( void ));
void rm_draw P_ARGS(( int sx, int sy, int pen ));
void rollchan P_ARGS(( int i ));
void rolld P_ARGS(( void ));
void rtoscreen P_ARGS(( void ));
void samaxmin P_ARGS(( short *buf, normal n, normal *pmin, normal *pmax, normal *pmindx, normal *pmaxdx ));
void saprt P_ARGS(( short *ia, int n, char *titl ));
void saradd P_ARGS(( short *src, short *dest, int n ));
void sarbswp P_ARGS(( short *buf, int n ));
void sarcon P_ARGS(( short *buf, normal n, int c ));
void sarfaccum P_ARGS(( short *src, short *dest, normal n, double fac ));
void sarfmul P_ARGS(( short *buf, normal n, double fac ));
void save_dic P_ARGS(( char *file ));
void scompile P_ARGS(( Void *str ));
void scrput P_ARGS(( int i ));
void scrputs P_ARGS(( char * s ));
void sdotr P_ARGS(( void ));
void sdraw P_ARGS(( double xf, double yf, int pen ));
void seed2 P_ARGS(( int i ));
void seed3 P_ARGS(( int i ));
void segtrap P_ARGS(( void ));
void sel_scr P_ARGS(( int i ));
void semi_s P_ARGS(( void ));
void seq1 P_ARGS(( int n, int wid ));
void sequence P_ARGS(( int n, int wid ));
void setdosattrib P_ARGS(( int bkg, int fg ));
void setfont P_ARGS(( int nfont ));
void setgraph P_ARGS(( int n ));
void setgwin P_ARGS(( int nw ));
void setsigdfl P_ARGS(( int sig ));
void setterminal P_ARGS(( char *str ));
int set_term_type P_ARGS(( char *term_type, int nl ));
void sfdotr P_ARGS(( void ));
void show_scr P_ARGS(( int i ));
void showalloc P_ARGS(( void ));
void showblk P_ARGS(( void ));
void showchan P_ARGS(( void ));
void showdirs P_ARGS(( void ));
void showgraphs P_ARGS(( void ));
void showspec P_ARGS(( void ));
void splinit P_ARGS(( float *x, float *y, float *k, int n ));
void spoint P_ARGS(( double sx, double sy, int size, int symb ));
void sqsort P_ARGS(( void ));
void ssinit P_ARGS(( void ));
void sspaces P_ARGS(( void ));
void stack_init P_ARGS(( void ));
void stacktrap P_ARGS(( void ));
void startwatch P_ARGS(( void ));
void stathist P_ARGS(( int *hist, int ncell ));
void swap_scr P_ARGS(( void ));
void swapchan P_ARGS(( void ));
void swapd P_ARGS(( void ));
void sys P_ARGS(( void ));
void test1 P_ARGS(( void ));
void test10 P_ARGS(( void ));
void test2 P_ARGS(( void ));
void test3 P_ARGS(( void ));
void test4 P_ARGS(( void ));
void test5 P_ARGS(( void ));
void test6 P_ARGS(( void ));
void test7 P_ARGS(( void ));
void test8 P_ARGS(( void ));
void test9 P_ARGS(( void ));
int  tputs P_ARGS(( char *str ));
void trapinit P_ARGS(( void ));
void trapreset P_ARGS(( int nex ));
void tridi P_ARGS(( float *a, float *b, float *c, float *f, float *x, int n ));
void twarn P_ARGS(( char *s ));
void uditrap P_ARGS(( void ));
void udot P_ARGS(( unsigned normal u ));
void undefine P_ARGS(( char *name ));
void version P_ARGS(( void ));
void vgaclose P_ARGS(( void ));
void vgaopen P_ARGS(( void ));
void vmmul P_ARGS(( float *vec, float *pa[], float *pvec, int n, int m ));
void vmsmsg P_ARGS(( int n ));
void vs_draw P_ARGS(( int x, int y, int pen ));
void vtick P_ARGS(( normal x, normal lxflg ));
void vwait P_ARGS(( void ));
void waitfor P_ARGS(( int t ));
void waituntil P_ARGS(( int next, int tp ));
void wesc P_ARGS(( void ));
void where P_ARGS(( void ));
void wintobm P_ARGS(( void ));
void wpgraph P_ARGS(( char *filename ));
void wpstring P_ARGS(( char *s ));
void write_hist P_ARGS(( void ));
void wscrbuf P_ARGS(( char *buf ));
void xlabel P_ARGS(( char *slx ));
void xscale P_ARGS(( void ));
void ylabel P_ARGS(( char *sly ));
void yscale P_ARGS(( void ));

#ifdef DEF_FILE
#  undef FILE
#  undef DEF_FILE
#endif
#ifdef DEF_Void
#  undef DEF_Void
#  undef Void
#endif

