/*
 * Support dynamic linking using Wilson Ho's `DLD'
 */
#include "mirella.h"
#ifdef DLD
#include "dld.h"

char full_name[100];			/* full name of programme */

void
setup_dld()
{
   int i;
   
   if((i = dld_init(full_name)) != 0) {
      fprintf(stderr,"\ndld_init failed: %s",dld_error(i));
   }
}

void
read_objfile(obj_file)
char *obj_file;				/* name of file to read */
{
   int i;
   
   if((i = dld_link(obj_file)) != 0) {
      fprintf(stderr,"\nFailed to link %s: %s",obj_file,dld_error(i));
      return;
   }
}

void
unread_objfile(obj_file)
char *obj_file;				/* name of file to read */
{
   int i;
   
   if((i = dld_unlink_by_file(obj_file)) != 0) {
      fprintf(stderr,"\nFailed to unlink %s: %s",obj_file,dld_error(i));
      return;
   }
}

Void *
get_function(func)
char *func;
{
   void (*addr)();
   
   if((addr = (void (*)())dld_get_func(func)) == NULL) {
      fprintf(stderr,"\nFailed to find %s",func);
      return(NULL);
   } else if(!dld_function_executable_p(func)) {
      fprintf(stderr,"\n%s() is not executable",func);
   }
   return(addr);
}

Void *
get_symbol(symb)
char *symb;
{
   Void *addr;
   
   if((addr = (Void *)dld_get_symbol(symb)) == NULL) {
      fprintf(stderr,"\nFailed to find %s",symb);
      return(NULL);
   }
   return(addr);
}
#else
int mirella_dynamic_c;
#endif
