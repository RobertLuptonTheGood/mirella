/*
 * This is a minimal Mirella main programme
 */
#include "mirella.h"

int
mir_main(argc,argv)
int argc;
char *argv[];
{
    mirelinit(argc,argv);
/*
 * application initialization code here
 */
    if(q_restart()){
/*
 * your chance to mess with the old dictionary image
 */
    }
    mirellam();
    return(0);
}

/*****************************************************************************/
/*
 * Code to get version numbers from RCS
 */
static char version_name[] = "$Symbols: M_7_3 $";
static char version_date[] = "$Date: 1993/04/19 23:21:43 $";

static void
fix_rcs_string(str)
char *str;
{
   char *ptr = str;
   
   while(*str != '\0' && *str != '$') *ptr++ = *str++;
   while(*str != '\0' && *str++ != ':') continue;
   while(isspace(*str)) str++;
   while(*str != '\0' && *str != '$') *ptr++ = *str++;
   ptr--;
   while(isspace(*ptr)) ptr--;
   *++ptr = '\0';
}

void
version()
{
   static char *version_string = NULL;

   if(version_string == NULL) {		/* massage version strings */
      int d,m,y;
      char *mon[] = { "???",
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
      char *ptr;
      char *template = "Mirella: version %s (%s)";

      fix_rcs_string(version_date);
      sscanf(version_date,"%d/%d/%d",&y,&m,&d);
      if(d < 0 || d > 12) d = 0;
      sprintf(version_date,"%d%s %s %d",
	      d,((d == 11 || d == 12 || d == 13) ? "th" :
		 d%10 == 1 ? "st" :
		 d%10 == 2 ? "nd" :
		 d%10 == 3 ? "rd" :
		 "th"),mon[m],y);

      fix_rcs_string(version_name);
      for(ptr = version_name;*ptr != '\0' && !isspace(*ptr);ptr++) continue;
      *ptr = '\0';

      d = (strlen(template) - 4) +
	strlen(version_name) + strlen(version_date) + 1;
      if((version_string = malloc(d)) == NULL) {
	 fprintf(stderr,"Can't allocate storage for version_string\n");
	 return;
      }
      sprintf(version_string,template,version_name,version_date);
   }
   cspush(version_string);
}
