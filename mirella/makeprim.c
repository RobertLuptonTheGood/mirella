/*
 * This file writes the init function for mircintf, given a list
 * of primitive words (typically in minit.x)
 */
#include "mircintf.h"
#include "mirella.h"

#define SIZE 100

int
main(ac,av)
int ac;
char **av;
{
   FILE *infil;				/* input file */
   char line[SIZE];			/* input line */
   char eword[SIZE],*eptr;		/* expanded word and pointer thereto */
   char *ptr;				/* pointer to input line (mostly) */
   int lnum;				/* number of input line */
   FILE *outfil;			/* output file */
   int type;				/* type of word */
   char *word;				/* the word in question */
   
   if(ac <= 2) {
      fprintf(stderr,"Usage: makeprim init.x primit.c\n");
      exit(1);
   }

   if((infil = fopen(av[1],"r")) == NULL) {
      fprintf(stderr,"Can't open %s\n",av[1]);
      exit(1);
   }
   if((outfil = fopen(av[2],"w")) == NULL) {
      fprintf(stderr,"Can't open %s\n",av[2]);
      fclose(infil);
      exit(1);
   }
/*
 * Write header
 */
   fprintf(outfil,"/*\n");
   fprintf(outfil," * This file was produced from %s using makeprim\n",av[1]);
   fprintf(outfil," * Under no circumstances should you edit it by hand\n");
   fprintf(outfil," * (or any other way)\n");
   fprintf(outfil," */\n");
   fprintf(outfil,"#include <stdio.h>\n");
   fprintf(outfil,"#include \"mircintf.h\"\n");
   fprintf(outfil,"\n");
   fprintf(outfil,"static MIRCINTF cwords[] = {\n");
/*
 * Now the declarations
 */
   for(lnum = 1;(ptr = fgets(line,SIZE,infil)) != NULL;lnum++) {
      switch(type = *ptr++) {
       case 'P':
	 type = CIF_PRIMITIVE; break;
       case 'i':
	 type = CIF_IMMEDIATE; break;
       case 'u':
	 type = CIF_USER; break;
       default:
	 fprintf(stderr,"Unknown type %c at line %d\n",type,lnum);
      }
      while(isspace(*ptr)) ptr++;
      word = ptr;
      while(*ptr != '\0' && !isspace(*ptr)) ptr++;
      *ptr = '\0';
      for(ptr = word,eptr=eword;*ptr != '\0';) { /* expand " and \\ */
	 if(*ptr == '"' || *ptr == '\\') {
	    *eptr++ = '\\';
	 }
	 *eptr++ = *ptr++;
      }
      *eptr = '\0';
      fprintf(outfil,"   { \"\\%03o%s\", %d, 0, 0, 0, 0 },\n",
	      ptr - word,eword,type);
   }
   fclose(infil);
/*
 * And finally the tail
 */
   fprintf(outfil,"   { \"\", -1, 0, 0, 0, 0 },\n");
   fprintf(outfil,"};\n");
   fprintf(outfil,"\n");
   fprintf(outfil,"MIRCINTF *\n");
   fprintf(outfil,"init_prims()\n");
   fprintf(outfil,"{\n");
   fprintf(outfil,"   static MIRCINTF *ptr = cwords;\n");
   fprintf(outfil,"   if(*ptr->fname == '\\0') return(NULL);\n");
   fprintf(outfil,"   return(ptr++);\n");
   fprintf(outfil,"}\n");

   fclose(outfil);
   return(0);
}


