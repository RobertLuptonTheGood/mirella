/*
 * This is code to parse expressions, (including a symbol table for variables)
 * and implement a version of the C-Pre-Processor (CPP) as a library.
 *
 * 	Robert Lupton July 1990 (rhl@astro.princeton.edu)
 *
 * The function calls to access the parser are:
 *	void alias();			alias one symbol to another
 * 	void define();			define a value
 * 	int eval_string();		evaluate a string as an expression
 *	int is_defined();		is a symbol defined?
 *	void undefine();		undefine a symbol
 *	int value();			return a value from the symbol table
 *
 * The grammar for expressions is:
 *
 * expr : lexpr
 *	| lexpr && lexpr
 *	| lexpr || lexpr
 *
 * lexpr : aexpr
 *	| aexpr == aexpr
 *	| aexpr != aexpr
 *	| aexpr < aexpr
 *	| aexpr <= aexpr
 *	| aexpr > aexpr
 *	| aexpr >= aexpr
 *
 * aexpr : term
 *	| term + term
 *	| term - term
 *
 * term : primary * primary
 *	| primary / primary
 *
 * primary : WORD
 *	| INTEGER
 *	| - expr
 *	| ! expr
 *	| ( expr )
 *	| defined(WORD)
 *
 * Undefined WORD's are taken to have the value 0; if the symbol
 * __verbose is >= 1 a message is printed.
 *
 *
 * The CPP stuff is accessed via the call
 *	char *parse_cpp_line();
 * which sets a flag which tells you whether you should skip the next line;
 * If you feed parse_cpp_line() each input line (or each line beginning with
 * a #) and obey the flag you will have an implementation of the CPP,
 * except that you must deal with #include lines yourself --- but
 * parse_cpp_line() will return the filenames that you must read. All
 * CPP keywords (except ANSI's #elif and #pragma) are supported, and also
 * #alias name name2 ("#alias rhl Robert" makes "rhl" equivalent to
 * "Robert" when the latter is defined).
 *
 * A simple implementation of the CPP follows:
 */
#if 0					/* Example CPP */
void
read_file(file)
char *file;
{
   char buff[100];
   FILE *fil;
   int skip,nest;

   if((fil = fopen(file,"r")) == NULL) return;

   skip = nest = 0;
   while(fgets(buff,100,fil) != NULL) {
      if((file = parse_cpp_line(line,&skip,&nest)) != NULL) {
	 read_file(file);
      } else if(buff[0] != '#' && !skip) {
	 process line yourself;
      }
   }
   fclose(fil);
}
#endif
/********************************************************************/
/*
 * Begin Code
 */
#include "mirella.h"
/*
 * Token definitions
 */
#define ERROR 256
#define INTEGER 257
#define WORD 258
#define EQ 259				/* == */
#define NE 260				/* != */
#define GT 261				/* > */
#define GE 262				/* >= */
#define LT 263				/* < */
#define LE 264				/* <= */
#define NOT 265				/* ! */
#define AND 266				/* && */
#define OR 267				/* || */
#define DEFINED 268
/*
 * Parser
 */
static int aexpr();			/* see grammar in header */
static int lexpr();			/* for definitions of these */
static int expr();
static int primary();
static int term();
static int print_error();		/* report an error */

static int integer;			/* repository for INTEGERs */
static char s_word[100];			/* repository for WORDs */
static int token;			/* current token */
/*
 * Lex analyser
 */
static void int_or_word();		/* is a token an INTEGER or a WORD? */
static void next_token();		/* get the next token */
static void print_token();		/* print current token */
/*
 * Symbol table
 */
#define NAMESIZE (4*10-1)		/* length of a name: 4*n - 1
					   for efficiency (flag is 1 char) */
#define TBLSIZE 23			/* size of hash table */
#define ALIAS 1				/* this node is an alias */
#define VALUE 2				/* this one has a value */
#define ALIASED 4			/* and this one is aliased to */

typedef struct symbol {
   char name[NAMESIZE];			/* name of node */
   char flag;				/* what type is it? (see above) */
   struct symbol *next;			/* next node in chain */
   union {				/* possible values: */
      union {				/* 	a real value: */
	 int value;			/* 		it's value */
	 int nalias;			/* 		number of aliases */
      } real;
      struct symbol *alias;		/* 	pointer to aliased node */
   } vals;
} SYMBOL;

void alias();				/* alias one symbol to another */
void define();				/* define a value */
int is_defined();			/* is a symbol defined? */
static SYMBOL *lookup();		/* return symbol table entry */
void undefine();			/* undefine a symbol */
int value();		/* return a value from symbol table */

static SYMBOL *table[TBLSIZE];		/* the symbol table itself */
/*
 * I/O
 */
static int peek();			/* look at next character */
static int readch();			/* read a character */
static int unreadch();			/* push a character */

static char *str,*sptr;
/*
 * Support for `CPP' stuff
 */
char *parse_cpp_line();			/* parse a line a la cpp */
/*
 * Misc
 */
int eval_string();

/********************************************************************/

int
eval_string(line)
char *line;
{
   sptr = str = line;

   next_token();
   return(expr());
}

/********************************************************************/

static int
expr()
{
   int left = lexpr();

   for(;;) {
      switch (token) {
       case AND:
	 next_token();
	 left = lexpr() && left;
	 break;
       case OR:
	 next_token();
	 left = lexpr() || left;
	 break;
       case ERROR:
	 return(print_error());
       default:
	 return(left);
      }
   }
}

/********************************************************************/

static int
lexpr()
{
   int left = aexpr();

   for(;;) {
      switch (token) {
       case EQ:
	 next_token();
	 left = left == aexpr();
	 break;
       case NE:
	 next_token();
	 left = left != aexpr();
	 break;
       case GT:
	 next_token();
	 left = left > aexpr();
	 break;
       case GE:
	 next_token();
	 left = left >= aexpr();
	 break;
       case LT:
	 next_token();
	 left = left < aexpr();
	 break;
       case LE:
	 next_token();
	 left = left <= aexpr();
	 break;
       case ERROR:
	 return(print_error());
       default:
	 return(left);
      }
   }
}

/********************************************************************/

static int
aexpr()
{
   int left = term();

   for(;;) {
      switch (token) {
       case '+':
	 next_token();
	 left += term();
	 break;
       case '-':
	 next_token();
	 left -= term();
	 break;
       case ERROR:
	 return(print_error());
       default:
	 return(left);
      }
   }
}

/********************************************************************/

static int
term()
{
   int left = primary();

   for(;;) {
      switch (token) {
       case '*':
	 next_token();
	 left *= primary();
	 break;
       case '/':
	 next_token();
	 left /= primary();
	 break;
       case ERROR:
	 return(print_error());
       default:
	 return(left);
      }
   }
}

/********************************************************************/

static int
primary()
{
   int e;
   
   for(;;) {
      switch (token) {
       case INTEGER:
	 next_token();
	 return(integer);
       case WORD:
	 next_token();
	 return(value(s_word));
       case '-':			/* unary minus */
	 next_token();
	 return(-primary());
       case NOT:			/* unary ! */
	 next_token();
	 return(!primary());
       case DEFINED:
	 next_token();
	 if(token != '(') {
	    return(print_error());
	 }

	 next_token();
	 if(token != WORD) {
	    return(print_error());
	 }
	 e = is_defined(s_word);

	 next_token();
	 if(token != ')') {
	    return(print_error());
	 }
	 next_token();
	 return(e);
       case '(':
	 next_token();
	 e = expr();

	 if(token != ')') {
	    return(print_error());
	 }
	 next_token();
	 return(e);
       case ')':
	 fprintf(stderr,"Unbalanced right paren\n");
	 return(1);
       case EOF:
	 next_token();
	 return(1);
       case ERROR:
	 return(print_error());
       default:
	 fprintf(stderr,"Primary expected, not ");
	 print_token();
	 next_token();
      }
   }
}

/********************************************************************/

static int
print_error()
{
   fprintf(stderr,"Syntax error:\n");
   fprintf(stderr,"%s%*s^\n",str, (int)(sptr - str),"");
   next_token();
   return(0);
}

/********************************************************************/
/*
 * End of parser, now the Lex analyser
 */
/********************************************************************/

static void
next_token()
{
   int c;
   char *ptr = s_word;

   while(ptr < s_word + 100) {
      if((c = readch()) == EOF) {
	 if(ptr == s_word) {
	    token = EOF;
	    return;
	 }
	 *ptr = '\0';
	 int_or_word();
	 return;
      }

      switch (c) {
       case '\n':
       case ' ':
       case '\t':
       case '(':
       case ')':
       case '-':
       case '+':
       case '*':
       case '/':
       case '=':
       case '!':
       case '<':
       case '>':
       case '|':
       case '&':
 	 if(ptr > s_word) {		/* terminate previous token */
	    (void)unreadch(c);
	    *ptr = '\0';
	    int_or_word();
	 } else if(c == ' ' || c == '\t') {
	    break;
	 } else if(c == '=') {
	    if(peek() == '=') {
	       (void)readch();
	       token = EQ;
	    } else {
	       token = ERROR;
	    }
	 } else if(c == '!') {
	    if(peek() == '=') {
	       (void)readch();
	       token = NE;
	    } else {
	       token = NOT;
	    }
	 } else if(c == '<') {
	    if(peek() == '=') {
	       (void)readch();
	       token = LE;
	    } else {
	       token = LT;
	    }
	 } else if(c == '>') {
	    if(peek() == '=') {
	       (void)readch();
	       token = GE;
	    } else {
	       token = GT;
	    }
	 } else if(c == '&') {
	    if(peek() == '&') {
	       (void)readch();
	       token = AND;
	    } else {
	       token = ERROR;
	    }
	 } else if(c == '|') {
	    if(peek() == '|') {
	       (void)readch();
	       token = OR;
	    } else {
	       token = ERROR;
	    }
	 } else {
	    token = c;
	 }
#if 0
	 print_token();
#endif
	 return;
       default:
	 *ptr++ = c;
	 break;
      }
   }
}

/********************************************************************/

static void
int_or_word()
{
   int atoi();
   char *ptr = s_word;

   while(isdigit(*ptr)) ptr++;
   if(*ptr == '\0') {
      integer = atoi(s_word);
      token = INTEGER;
   } else {
      if(!strcmp(s_word,"defined")) {
	 token = DEFINED;
      } else {
	 token = WORD;
      }
   }
}

/********************************************************************/

static void
print_token()
{
   switch (token) {
    case ERROR:
      fprintf(stderr,"ERROR\n");
      break;
    case INTEGER:
      fprintf(stderr,"INTEGER: %d\n",integer);
      break;
    case WORD:
      fprintf(stderr,"WORD: %s\n",s_word);
      break;
    case EQ:
      fprintf(stderr,"==\n");
      break;
    case NE:
      fprintf(stderr,"!=\n");
      break;
    case GT:
      fprintf(stderr,">\n");
      break;
    case GE:
      fprintf(stderr,">=\n");
      break;
    case LT:
      fprintf(stderr,"<\n");
      break;
    case LE:
      fprintf(stderr,"<=\n");
      break;
    case NOT:
      fprintf(stderr,"!\n");
      break;
    case AND:
      fprintf(stderr,"&&\n");
      break;
    case OR:
      fprintf(stderr,"||\n");
      break;
    case '\n':
      fprintf(stderr,"\\n\n");
      break;
    default:
      fprintf(stderr,"%c\n",token);
      break;
   }
}

/********************************************************************/
/*
 * End of Lex analyser, now the symbol table
 */
#define DEFINE 1
#define FIND 2
#define UNDEFINE 3

/********************************************************************/
/*
 * Lookup a value in the symbol table
 */
static SYMBOL *
lookup(name,flag)
char *name;
int flag;				/* a new node? */
{
   SYMBOL *entry;
   static int first = 1;
   int i;
   int ind;				/* index into hash table */
   SYMBOL *prev;
   char *ptr;

   if(first) {
      first = 0;
      for(i = 0;i < TBLSIZE;i++) {
	 table[i] = NULL;
      }
   }
   
   ind = 0; ptr = name;			/* find hash index */
   while(*ptr != '\0') {
      ind = (ind << 1) ^ *ptr++;
   }
   if(ind < 0) ind = -ind;
   ind %= TBLSIZE;

   prev = NULL;
   for(entry = table[ind];entry != NULL;prev = entry, entry = entry->next) {
      if(!strcmp(entry->name,name)) {
	 if(flag == UNDEFINE) {
	    if(entry->flag & ALIASED && entry->vals.real.nalias > 0) {
	       entry->vals.real.nalias--;
	       entry->flag &= ~VALUE;
	       return(entry);
	    }
	    if(prev == NULL) {
	       table[ind] = NULL;
	    } else {
	       prev->next = entry->next;
	    }
	    free((char *)entry);
	    return(NULL);
	 } else {
	    return(entry);
	 }
      }
   }
   if(flag == DEFINE) {			/* make a node */
      if((entry = (SYMBOL *)malloc(sizeof(SYMBOL))) == NULL) {
	 fprintf(stderr,"Can't allocate storage for %s in symbol table\n",
		 							name);
	 return(NULL);
      }
      strncpy(entry->name,name,NAMESIZE);
      entry->next = table[ind];
      entry->flag = 0;
      entry->vals.real.nalias = 0;
      table[ind] = entry;
   }
   
   return(entry);
}

/********************************************************************/

int
value(name)
char *name;
{
   SYMBOL *entry;

   if((entry = lookup(name,FIND)) == NULL ||
			      ((entry->flag & (VALUE | ALIAS)) == 0)) {
      if((entry = lookup("__verbose",FIND)) != NULL &&
	 entry->vals.real.value != 0) {
	 fprintf(stderr,"%s is not #define'd -- setting to 0\n",name);
      }
      return(0);
   } else {
      if(entry->flag & VALUE) {
	 return(entry->vals.real.value);
      } else if(entry->flag & ALIAS) {
	 return(value(entry->vals.alias->name));
      } else {
	 fprintf(stderr,"A node must be a VALUE or an ALIAS\n");
	 return(0);
      }
   }
}

/********************************************************************/

int
is_defined(name)
char *name;
{
   SYMBOL *entry;

   entry = lookup(name,FIND);
   return(entry != NULL && (entry->flag & (VALUE | ALIAS)));
}

/********************************************************************/

void
alias(name,name2)
char *name;
char *name2;
{
   SYMBOL *entry;
   SYMBOL *entry2;

   if((entry = lookup(name,DEFINE)) != NULL) {
      if(entry->flag & VALUE) {
	 fprintf(stderr,"%s is already defined and thus can't be aliased\n",
		 name);
	 return;
      }
   }
   
   if((entry2 = lookup(name2,DEFINE)) == NULL) {
      undefine(name);			/* can't get the aliased node */
   } else {
      entry->vals.alias = entry2;
      entry->flag |= ALIAS;
      entry2->vals.real.nalias++;
      entry2->flag |= ALIASED;
   }
}

/********************************************************************/

void
define(name,val)
char *name;
int val;
{
   SYMBOL *entry;

   if((entry = lookup(name,DEFINE)) != NULL) {
      entry->vals.real.value = val;
      entry->flag |= VALUE;
   }
}

/********************************************************************/

void
undefine(name)
char *name;
{
   (void)lookup(name,UNDEFINE);
}

/********************************************************************/
/*
 * End of symbol table, now I/O (read from a string)
 */
/********************************************************************/

static int
readch()
{
   int c;

   if((c = *sptr++) == '\0') {
      sptr--;
      return(EOF);
   } else {
      return(c);
   }
}

/********************************************************************/

static int
unreadch(c)
int c;
{
   if(sptr > str) {
      *--sptr = c;
   }
   return(c);
}

/********************************************************************/

static int
peek()
{
   return(*sptr == '\0' ? EOF : *sptr);
}

/********************************************************************/
/*
 * Here is the code to deal with `cpp' command lines:
 * #include file
 * #define name val
 * #undef name
 * #if expr
 * #ifdef name
 * #ifndef name
 * #else
 * #endif
 *
 * Specifically:
 * #define and #undef are completely dealt with.
 *
 * #include returns the name of the include file with the trailing
 * " or > stripped; the initial one is kept.
 *
 * #if..., #else, and #endif lines are parsed and the appropriate
 * value of skip is returned
 */
#define DEPTH 20			/* max depth of nested #if's */
#define iswhite(C) ((C) == ' ' || (C) == '\t')

static int skipping[DEPTH] = { 0 };

char *
parse_cpp_line(line,skip,nest)
char *line;
int *skip;				/* if true, we are skipping lines */
int *nest;				/* nesting level of #if's */
{
   char *cp;
   char *name;

   if(*line ++ != '#') {
      return(NULL);
   }
   while(iswhite(*line)) line++;
   
   if(!strncmp(line,"endif",5) && isspace(line[5])) {
      if(--(*nest) < 0) {
	 fprintf(stderr,"Too many #endifs: %s",line);
	 *skip = 0;
	 *nest = 0;
      } else {
	 *skip = skipping[*nest];
      }
   } else if(!strncmp(line,"else",4) && isspace(line[4])) {
      if(*nest == 0) {
	 fprintf(stderr,"Unmatched #else: %s",line);
      } else {
	 *skip = skipping[*nest] = !skipping[*nest];
      }
   } else if(!strncmp(line,"define",6) && iswhite(line[6])) {
      if(*skip) {
	 return(NULL);
      }
      name = line + 7;
      while(iswhite(*name)) name++;
      cp = name;
      while(*cp != '\0' && !isspace(*cp)) cp++;
      if(cp != '\0') {
	 *cp++ = '\0';
      }
      while(isspace(*cp)) cp++;
      if(*cp == '\0') {
	 define(name,1);
      } else {
	 define(name,eval_string(cp));
      }
   } else if(!strncmp(line,"alias",5) && iswhite(line[5])) {
      char *name2;
      
      if(*skip) {
	 return(NULL);
      }
      name = line + 6;
      while(iswhite(*name)) name++;
      name2 = name;
      while(*name2 != '\0' && !isspace(*name2)) name2++;
      if(name2 != '\0') {
	 *name2++ = '\0';
      }
      cp = name2;
      while(*cp != '\0' && !isspace(*cp)) cp++;
      if(cp != '\0') {
	 *cp++ = '\0';
      }
      alias(name,name2);
   } else if(!strncmp(line,"undef",5) && iswhite(line[5])) {
      if(*skip) {
	 return(NULL);
      }
      name = line + 6;
      while(iswhite(*name)) name++;
      cp = name;
      while(*cp != '\0' && !isspace(*cp)) cp++;
      if(cp != '\0') {
	 *cp = '\0';
      }
      undefine(name);
   } else if((!strncmp(line,"if",2) && iswhite(line[2])) ||
	     (!strncmp(line,"ifdef",5) && iswhite(line[5])) ||
	     (!strncmp(line,"ifndef",6) && iswhite(line[6]))) {
      if((*nest)++ >= DEPTH) {
	 fprintf(stderr,"Too deep nesting of #if's at %s",line);
	 (*nest)--;
	 return(NULL);
      }
      
      if(*skip) {
	 skipping[*nest] = *skip;
      } else {
	 if(line[2] == ' ') {
	    *skip = skipping[*nest] = !eval_string(line + 2);
	 } else {
	    name = line + (line[2] == 'd' ? 6 : 7);
	    while(iswhite(*name)) name++;
	    cp = name;
	    while(*cp != '\0' && !isspace(*cp)) cp++;
	    if(cp != '\0') {
	       *cp = '\0';
	    }

	    *skip = skipping[*nest] = (line[2] == 'd' ?
				       !is_defined(name) : is_defined(name));
	 }
      }
   } else if(!strncmp(line,"include",7) && iswhite(line[7])) {
      if(*skip) {
	 return(NULL);
      }

      name = line + 8;
      while(iswhite(*name)) name++;
      cp = name;
      while(*cp != '\0' && !isspace(*cp)) cp++;
      if(cp != '\0') {
	 *cp = '\0';
      }

      cp--;
      if((*name == '"' && *cp == '"') || (*name == '<' && *cp == '>')) {
	 *cp = '\0';
      } else {
	 fprintf(stderr,"No closing \" or > for %s\n",name);
      }
      return(name + 1);			/* so name[-1] is initial " or < */
   } else {
      if(!*skip) {
	 fprintf(stderr,"Unknown #directive: %s",line);
      }
   }
   return(NULL);
}
