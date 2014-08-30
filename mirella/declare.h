/*
 * Note that this file is explicitly excluded from all Makefiles
 */
/*
 * If ANSI_PROTO is defined to the preprocessor, provide prototypes for all
 * functions, other wise simply declare their return values.
 */
#if !defined(P_ARGS)
#  if defined(__STDC__) || defined(ANSI_PROTO)
#     define P_ARGS(A) A
#  else
#     define P_ARGS(A) ()
#  endif
#endif
#if !defined(FILE)
#  define DEF_FILE
#  define FILE char
#endif
#ifndef Void
#  define DEF_Void
#  define Void char
#endif
#define INT int
/*
 * Various system routines, mostly to pacify lint (or gcc -Wall)
 */
#if defined(ANSI_INCLUDE)
#  include <stdlib.h>
#  include <string.h>
#  include <time.h>
#else
   int _flsbuf P_ARGS(( unsigned char, FILE * ));
   int _filbuf P_ARGS(( FILE * ));
   void abort P_ARGS(( void ));
   int abs P_ARGS(( int ));
   double atof P_ARGS(( const char * ));
   int atoi P_ARGS(( char * ));
   char *ctime P_ARGS(( const long * ));
   void exit P_ARGS(( int ));
   int fclose P_ARGS(( FILE * ));
   int fflush P_ARGS(( FILE * ));
   int fgetc P_ARGS(( FILE * ));
   char *fgets P_ARGS(( char *, int, FILE * ));
   int fprintf P_ARGS(( FILE *, const char *, ... ));
   int fputs P_ARGS(( char *, FILE * ));
   int fscanf P_ARGS(( FILE *, const char *, ... ));
   int fseek P_ARGS(( FILE *, long, int ));
   int fwrite P_ARGS(( const char *, int, int, FILE * ));
   void free P_ARGS(( void * ));
   char *getenv P_ARGS(( char * ));
   Void *malloc P_ARGS(( int ));
   Void *memcpy P_ARGS(( Void *, const Void *, unsigned long ));
   char *memmove P_ARGS(( Void *, const Void *, int ));
   int memset P_ARGS(( Void *, int, int ));
   void perror P_ARGS(( const char * ));
   int printf P_ARGS(( const char *, ... ));
   int puts P_ARGS(( char * ));
   void qsort P_ARGS(( void *base, int number, int size, \
		      int (*compare)(const void *, const void *) ));
   int rand P_ARGS(( void ));
   int random P_ARGS(( void ));
   Void *realloc P_ARGS(( Void *, int ));
   int rename P_ARGS(( const char *old , const char *new ));
   int scanf P_ARGS(( const char *, ... ));
#if defined(sun) && defined(__STDC__)	/* arghh! */
   char *sprintf();			/* this is a disaster! */
#else
#  if defined(__STDC__) || defined(VMS) || defined(SYS_V)
      int sprintf P_ARGS(( char *, const char *, ... ));
#  else
      char *sprintf P_ARGS(( char *, const char *, ... ));
#  endif
#endif
   int sscanf P_ARGS(( char *, const char *, ... ));
   char *strcat P_ARGS(( char *, const char * ));
   char *strncat P_ARGS(( char *, const char *, unsigned long ));
   char *strcpy P_ARGS(( char *, const char * ));
   char *strncpy P_ARGS(( char *, const char *, unsigned long ));
   int strcmp P_ARGS(( const char *, const char * ));
   int strncmp P_ARGS(( const char *, const char *, unsigned long ));
   char *strchr P_ARGS(( const char *, int ));
   char *strrchr P_ARGS(( const char *, int ));
   int strlen P_ARGS(( const char * ));
   long time P_ARGS(( long * ));
#  if !defined(tolower)
      int tolower P_ARGS(( int ));
#  endif
#  if !defined(toupper)
      int toupper P_ARGS(( int ));
#  endif
   int ungetc P_ARGS(( int, FILE * ));
   int vsprintf();			/* real arg list needs an #include  */
#endif
/*
 * These are not in ANSI C (but will mostly be POSIX)
 */
#if defined(_POSIX_SOURCE)
#  if defined(LINUX)
      /* see if this works */
#     if !(defined(__USE_MISC))
#       define __USE_MISC			/* get e.g. sbrk() */
#     endif
#  endif
#  include <unistd.h>
#  include <fcntl.h>
#else
int access P_ARGS (( char *, int ));
int bcopy P_ARGS(( char *, char *, int ));
int bzero P_ARGS(( char *, int ));
int chdir P_ARGS(( char * ));
int close P_ARGS(( int ));
int creat P_ARGS(( const char *, unsigned short ));
char *ecvt P_ARGS(( double, int, int *, int * ));
int ftruncate P_ARGS(( int, int ));
int fork P_ARGS(( void ));
int getpid P_ARGS(( void ));
#if !(defined(SUN) && defined(X11_C))	/* workaround sunos omitting unistd.h */
   int getuid P_ARGS(( void ));
#endif
char *getcwd P_ARGS(( char *, int ));
int isatty P_ARGS(( int ));
int kill P_ARGS(( int, int ));
long lseek P_ARGS(( int, long, int ));
#if defined(sun) && defined(DEBUG)
   void malloc_debug P_ARGS(( int ));
#endif
int mkdir P_ARGS(( const char *name, unsigned short mode ));
#if 0
   int open P_ARGS(( char *, int, ... ));
#else
   int open();
#endif
int read P_ARGS(( int, char *, int ));
int rmdir P_ARGS(( char *name ));
#if !(defined(SUN) && defined(X11_C))	/* workaround sunos omitting unistd.h */
   void sleep P_ARGS(( long ));
#endif
int stat();				/* really char *, struct stat * */
int system P_ARGS(( const char * ));
long tell P_ARGS(( int fd ));
int unlink P_ARGS(( const char * ));
int write P_ARGS(( int, char *, int ));
#endif
/*
 * These are not in POSIX, but are common unix functions
 */
char *getwd P_ARGS(( char *name ));
#if !defined(SGI) && !defined(LINUX)
   int ioctl P_ARGS(( int, int, char * ));
#endif
char *mktemp P_ARGS(( char * ));
#if !defined(LINUX) || (defined(__DARWIN_C_LEVEL) && __DARWIN_C_LEVEL < 199506L)
   Void *sbrk P_ARGS(( int ));
#endif
#if defined(SOLARIS)
#  include <sys/select.h>
#else
   int select();
#endif
void swab P_ARGS (( const void *from, void *to, ssize_t));

#ifdef DEF_FILE
#  undef FILE
#  undef DEF_FILE
#endif
#ifdef DEF_Void
#  undef DEF_Void
#  undef Void
#endif
#undef INT
