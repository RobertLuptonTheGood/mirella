/*
 * This is an ansi function that not all compilers support.
 * It is in a separate file so that if it isn't needed, the loader
 * won't pull it out of the archive
 */
#include "options.h"
#if defined(NEED_MEMMOVE)		/* actually, !__STDC_ should suffice,
					   but some compilers declare __STDC__
					   without having an ansi library */
#define BSIZE 512

char *
memmove(dest,src,n)
char *dest;
const char *src;
int n;
{
   char buf[BSIZE];
   char *ptr = dest;

   if(n%BSIZE != 0) {
      memcpy(buf,src,n%BSIZE);
      memcpy(ptr,buf,n%BSIZE);
      ptr += n%BSIZE;
      src += n%BSIZE;
      n -= n%BSIZE;
   }
      
   while(n > 0) {
      memcpy(buf,src,BSIZE);
      memcpy(ptr,buf,BSIZE);
      ptr += BSIZE;
      src += BSIZE;
      n -= BSIZE;
   }
   return(dest);
}
/* #else
static void foo() {;} */			/* fodder for picky linkers */
#endif

