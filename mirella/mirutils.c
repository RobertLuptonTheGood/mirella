#include "mirella.h"
/*
 * get various directories and files needed by mirella
 */
#define SIZE 200

#if !defined(DOS32)
   char *defaults_file = ".mirella";	/* File for defaults */
#else
   char *defaults_file = "mirelrc";
#endif

void
set_defs_file(file)
char *file;
{
   defaults_file = file;
}

/*
 * routine to read values from the initialization file, .mirella or ~/.mirella
 * returns NULL if name is not found
 */
 
char *
get_def_value(what)
char what[];				/* desired property */
{
   char file[100],
	name[SIZE],			/* name of attribute */
	*ptr;
   static char in_line[SIZE];		/* line from file */
   FILE *fil;
   int i,j;

   if((fil = fopen(defaults_file,"r")) == NULL) {
#ifdef vms
      (void)sprintf(file,"sys$login:%s",defaults_file);
#else
      if((ptr = getenv("HOME")) != NULL) {
         if(ptr[strlen(ptr) - 1] == '/') {
            (void)sprintf(file,"%s%s",getenv("HOME"),defaults_file);
         } else {
            (void)sprintf(file,"%s/%s",getenv("HOME"),defaults_file);
         }
      }
#endif
      if(ptr == NULL || (fil = fopen(file,"r")) == NULL) { /* try home dir */
	 fprintf(stderr,"Can't find a file \"%s\"\n",defaults_file);
	 return(NULL);
      }
   }
   while(fgets(in_line,SIZE,fil) != NULL) {
      if(in_line[0] == '\\') {		/* a comment */
	 continue;
      }
      i = strlen(in_line) - 1;
      if(in_line[i] == '\n') in_line[i] = '\0';		/* remove \n */
      (void)sscanf(in_line,"%s",name);
      if(!strcmp(name,what)) {
	 for(i = 0;i < SIZE && in_line[i] != '\0' &&
	     		!isspace(in_line[i]);i++) ;	/* skip name */
	 for(;i < SIZE && isspace(in_line[i]);i++) ;	/* find value */
	 (void)fclose(fil);
	 for(j = i;in_line[j] != '\0';j++) {
	    if(in_line[j] == '\\') {
	       j--;
	       if(j > i) {
		  while(isspace(in_line[j])) j--;
	       }
	       in_line[++j] = '\0';
	       break;
	    } else if(in_line[j] == '\\') {
	       if(in_line[j+1] == '\\' || in_line[j+1] == '#') {
		  if(in_line[j + 2] != '\0') {
		     j++;		/* skip escaped characters */
		  }
	       }
	    }
	 }
	 if(in_line[i] == '\0' || i == SIZE || i == j) return("1");
	 else return(&in_line[i]);
      }
   }
   (void)fclose(fil);

   sprintf(name,"MIR%s",what);		/* try MIRWHAT */
   for(i = 3;name[i] != '\0';i++) {
      if(islower(name[i])) name[i] = toupper(name[i]);
   }
   return(getenv(name));
}

/****************************************************/
/*
 * Provide a sprintf-like procedure for Mirella
 */
static char *ss();

void
forth_printf()
{
   char str[255];			/* the string to write */
   char *fmt = cspop;			/* the format to use */
   char ffmt[80];			/* writable copy of fmt, with
					 escape sequences expanded */
   char *ptr;

   for(ptr = ffmt;(*ptr = *fmt++) != '\0';ptr++) {
      if(*ptr == '\\') {
	 if(*fmt == 'n') {
	    *ptr = '\n'; fmt++;
	 } else if(*fmt == 't') {
	    *ptr = '\t'; fmt++;
	 } else if(*fmt == '"') {
	    *ptr = '"'; fmt++;
	 } else if(*fmt == '\\') {
	    *ptr = '\\'; fmt++;
	 } 
      }
   }
   ptr = spush(ss(str,ffmt));		/* push string on str stack and */
   push((normal)ptr);			/* address on parameter stack */
}

static char *
ss(str,fmt)
char *str;				/* string to write to */
char *fmt;				/* format to use */
{
   char buf[256];			/* result of %? */
   char buf2[256];			/* used to switch buf and str */
   char *ptr;
   
   for(ptr = fmt;*ptr != '\0';) {
      if(*ptr++ == '%') {
	 while(*ptr == '*' || *ptr == '-' || *ptr == '+' || *ptr == '#') {
	    if(*ptr == '*') {		/* not supported */
	       scrprintf("\n%%* formats are not supported");
	       *ptr = '#';		/* safe, I trust */
	    }
	    ptr++;
	 }
	 while(isdigit(*ptr)) ptr++;
	 if(*ptr == '.') {
	    ptr++;
	    while(isdigit(*ptr)) ptr++;
	 }
	 ptr++;				/* skip type character */
	 str = ss(str,ptr);
	 *ptr-- = '\0';
	 switch(*ptr) {
	  case '%':
	    strcpy(buf,fmt);
	    break;
	  case 'c':
	  case 'd':
	  case 'o':
	  case 'x':
	    sprintf(buf,fmt,pop);
	    break;
	  case 'e':
	  case 'f':
	  case 'g':
	    sprintf(buf,fmt,fpop);
	    break;
	  case 's':
	    sprintf(buf,fmt,cspop);
	    break;
	  default:
	    fprintf(stderr,"Unknown format character `%c'\n",*ptr);
	    strcpy(buf,fmt);
	    break;
	 }
	 sprintf(buf2,"%s%s",buf,str);
	 return(strcpy(str,buf2));
      }
   }
   return(strcpy(str,fmt));
}

/**************** Provide GETENV, SETENV, PUTENV for Mirella *****************/

char nullstr[2] = "";

char * 
m_getenv(name)
char name[];
{
    char *err;
    
    err = getenv(name);
    if (err != NULL){
        return err;
    }else{
        mprintf("\n no such environment variable as %s",name);
        return nullstr ;  
    }
}

void
m_setenv(name,value)
char name[];
char value[];
{
    int setenv();
    int err = setenv(name,value,1);
    if(err != 0){
        mprintf("\nFAILED attempting to set environment value %s to %s",
            name,value);
        erret(NULL);
    }
}
                    
void m_putenv(string)
char string[];
{
    int putenv();
    int err = putenv(string);
    if(err != 0){
        mprintf("\nFAILED attempting to set environment value %s",
            string);
        erret(NULL);
    }
}

/*****************************************************************************/
/*
 * Some recent versions of libc call strcp_chk which aborts if s1 == s2
 */
void
strcpy_safe(char *restrict s1, const char *restrict s2)
{
   if (s1 != s2) {
      (void)strcpy(s1, s2);
   }
}
