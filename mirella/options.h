/* -*- C -*-
 * These are the compiler and such like option files for Mirella. If your
 * machine is one of
 *	ATI486 DSI386 DSI386_P SGI SOLARIS SUN RS6000 LINUX
 * you may simply define it here, otherwise please go through and define
 * the flags that you want. If you'll be using the machine again you should
 * add your configuration name to the list above to the list following the
 * first ^L, and to the code at the bottom of this file.
 */
#define LINUX

#if !(defined(ATI486) || defined(DSI386) || \
      defined(DSI386_P) || defined(RS6000) || \
      defined(SOLARIS) || defined(SUN) || defined(SGI) || defined(LINUX))
/*
 * Are we running Unix?
#define UNIX
 */

/*
 * Are we running a System V based Unix?
#define SYS_V
 */

/*
 * Are we running a BSD based Unix?
#define BSD
 */
/*
 * Do we have the BSD Unix tape routines?
 */
#define BKTAPE

/*
 * There are three ways to support variable numbers of arguments to
 * functions; in order of decreasing desirability they are stdargs,
 * varargs, and a naive implementation based on assumptions about
 * calling conventions. In order, these are enabled by defining
 * STDARGS, VARARGS, or nothing.
 */
#define STDARGS

/*
 * signals
 */
#define SIGNALS

/* 
 * If you want to use the newer sigaction() instead of signal(), use this 
#define SIGACTION
 */
 
/*
 * Do we have the dynamic linker?
 */
#define DLD

/*
 * Do we want to use sbrk() to allocate the dictionary? (scary...)
#define MIRSBRK
 */
/*
 * Do we have SM linked in?
 */
#define HAVE_SM

/*
 * Does the compiler understand ANSI prototypes?
 */
#define ANSI_PROTO

/*
 * Does the compiler have real ANSI include files?
#define ANSI_INCLUDE
 */
#else

#  if defined(LINUX)
#     define UNIX
/* #     define SYS_V */
#     define SIGNALS
#if 0
#     define BKTAPE
#     define BKSCSITAPE
#endif
#     if !defined(_POSIX_SOURCE)
#          define _POSIX_SOURCE
#     endif
#     define HAVE_SOCKET_PROTO		/* used by gunnspec extension */
#     define HAVE_SELECT_PROTO		/* used by gunnspec extension */
#  endif

#  if defined(SUN)
#     define UNIX
#     define BSD
#     define SIGNALS
#     define STDARGS
#if 0
#     define DLD
#     define HAVE_SM
#endif
#     define BKTAPE
#     define BKSCSITAPE
#     if defined(__STDC__)
#        define ANSI_PROTO
#     endif
#     if !defined(SOLARIS)
#        define NEED_MEMMOVE
#     endif
#  endif

#  if defined(ATI486)
#     define MIRSBRK
#     define SIGNALS
#     define DOS32
#     if defined(__ZTC__)
#        define ZORTECH
#     endif
#  endif

#  if defined(RS6000)
#     define UNIX
#     define BSD
#     define SIGNALS
#     define STDARGS
#     define ANSI_PROTO
#     define _POSIX_SOURCE
#  endif

#  if defined(SGI)
#     if defined(IRIX_4)		/* needed for version 4? */
#        define __STDC__
#     endif
#     define _BSD_TYPES
#     define UNIX
#     define SYS_V
#     define SIGNALS
#     define STDARGS
#if 0
#     define DLD
#     define HAVE_SM
#endif
#     define DEFUCHAR
#     define BKTAPE
#     define ANSI_PROTO
#     define ANSI_INCLUDE
#     define _POSIX_SOURCE
#  endif

#  if defined(SOLARIS)
#     define UNIX
#     define SYS_V
#     define SIGNALS
#     define STDARGS
#     define BKTAPE
#     define BKSCSITAPE
#     define ANSI_PROTO
#     define ANSI_INCLUDE
#     define _POSIX_SOURCE
#     define HAVE_SOCKET_PROTO		/* used by gunnspec extension */
#     define HAVE_SELECT_PROTO		/* used by gunnspec extension */
#  endif

#  if defined(DSI386)
#     define MIRSBRK
#     define DSISIGNALS
#     define DOS32
#  endif

#  if defined(DSI386_P)
#     define MIRSBRK
#     define SIGNALS
#     define DOS32
#     define PLASMA
#     if defined(__ZTC__)
#        define ZORTECH
#     endif
#  endif
#endif

#if defined(__STDC__)
#  define Const const
#  define Signed signed
#  define Void void      /* (Void *) is a generic pointer */
#  define Volatile volatile
#  define ANSI_PROTO
#  define ANSI_INCLUDE
#  define STDARGS
#else
#  define Const
#  define Signed
#  define Void char
#  define Volatile
#endif

#if defined(_POSIX_SOURCE)
#  define POSIX_TTY 1
#endif
