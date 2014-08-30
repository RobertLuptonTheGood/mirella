#include "mirella.h"
#if !defined(HAVE_SM)
#include <stdio.h>
#include "tty.h"

#ifdef DEF_BAUDRATE
#  undef DEF_BAUDRATE
#endif
#define	DEF_BAUDRATE	9600

static char *tty_find_capability();
static int sm_getline(),
	   tty_binsearch(),
	   tty_extract_alias(),
	   tty_fetch_entry(),
	   tty_scan_env(),
	   tty_scan_termcap_file();
/*
 * TTYOPEN -- Scan the named TERMCAP style file for the entry for the named
 * device, and if found allocate a TTY descriptor structure, leaving the
 * termcap entry for the device in the descriptor.
 */
TTY *
ttyopen(termcap_file,ac,av,ttyload)
char termcap_file[];		/* termcap file to be scanned */
int ac;				/* number of arguments to DEVICE */
char *av[];			/* pointers to arguments (av[0] is device) */
int (*ttyload)();		/* fetches pre-compiled entries from a cache */
{
   char ldevice[100];			/* local storage for "av[0]" */
   int i,
       len;				/* length of av[i] */
   TTY *tty;
/*
 * Allocate and initialize the tty descriptor structure
 */
     if((tty = (TTY *)malloc(sizeof(TTY))) == NULL) {
	printf("Can't malloc tty in ttyopen\n");
	return(NULL);
     }
     tty->t_op = 0;			/* index in tty->t_caplist */
     tty->t_caplen = 0;

     strcpy(ldevice,av[0]);		/* gets overwritten in tc expansion */
     if(ttyload != NULL) {
	tty->t_caplen = (*ttyload)(termcap_file,ldevice,tty);
     }

     if(tty->t_caplen == 0) {		/* didn't find device in cache */
	 if((tty->t_caplist = malloc(T_MEMINC)) == NULL) {
	    printf("Can't malloc tty->t_caplist in ttyopen\n");
	    free((char *)tty);
	    tty = NULL;
	    return(NULL);
	 } else {
	    tty->t_len = T_MEMINC;
	 }

	 for(i = 1;i < ac;i++) {	/* look for graphcap entries in av[] */
	    if(av[i][0] == ':') {
	       len = strlen(av[i]);
	       if(len + tty->t_op > tty->t_len) {
		  tty->t_len += T_MEMINC;
		  if((tty->t_caplist = realloc(tty->t_caplist,
					      (unsigned)tty->t_len)) == NULL) {
		     printf("Can't reallocate tty->t_caplist\n");
		     free((char *)tty);
		     return(NULL);
		  }
	       }
	       (void)strcpy(&(tty->t_caplist[tty->t_op]),
			    &av[i][i == 1 ? 0 : 1]); /* don't need 2 :'s */
	       tty->t_op += len - (i == 1 ? 0 : 1);
	       tty->t_caplen = tty->t_op + 1; /* include '\0' */
	    }
	 }
	 if(tty_scan_termcap_file(tty, termcap_file,ldevice) < 0) {
	     free(tty->t_caplist);
	     free((char *)tty);
	     return(NULL);
	 }
/* 
* Call realloc to return any unused space in the descriptor
*/
	 if((tty->t_caplist = realloc(tty->t_caplist,(unsigned)tty->t_caplen))
								    == NULL) {
	    printf("Can't realloc tty->t_caplist\n");
	    free((char *)tty);
	    tty = NULL;
	    return(NULL);
	 }
      }
      tty->t_len = tty->t_caplen;

     return (tty);
}

/**********************************************************/
/*
 * TTY_SCAN_TERMCAP_FILE -- Open and scan the named TERMCAP format database
 * file for the named device.  Fetch termcap entry, expanding any and all
 * "tc" references by repeatedly rescanning file.
 */
#define NFILES 3			/* max. number of files in path */

static int
tty_scan_termcap_file(tty, termcap_file, device)
TTY *tty;				/* tty descriptor structure */
char termcap_file[];			/* termcap format file to be scanned */
char device[];				/* termcap entry to be scanned for */
{
   char *ip, *op, *caplist,
   	files[NFILES][80 + 1],		/* list of termcap files */
   	*tptr;
   FILE *fil;
   int i,j,
       nfiles,				/* number of termcap files */
       ntc;

   tptr = termcap_file;
   for(i = 0;i < NFILES;i++) {
      for(j = 0;j < 80 && *tptr != '\0' && !isspace(*tptr);j++) {
	 files[i][j] = *tptr++;
      }
      files[i][j] = '\0';
      while(isspace(*tptr)) tptr++;
      if(*tptr == '\0') {
	 break;
      }
   }
   nfiles = (i == NFILES) ? NFILES : i + 1;

   if((fil = fopen(files[0],"r")) == NULL) {
      if(tty_scan_env(tty,termcap_file,device) == 0) {
	 return(0);
      } else {
	 printf("Can't open %s\n",files[0]);
	 return(-1);
      }
   }
   fclose(fil);
   
   ntc = 0;
   for(;;) {
      for(i = 0;i < nfiles;i++) {
	 if((fil = fopen(files[i],"r")) == NULL ||
	 			tty_fetch_entry(fil,device,tty) == -1) {
	    if(fil != NULL) fclose(fil);
	    if(i == nfiles - 1 && tty->t_op == 0) {
	       printf("Can't find entry for %s in%s",device,
					      nfiles == 1 ? "" : " any of");
	       for(i = 0;i < nfiles;i++) {
		  printf(" %s",files[i]);
	       }
	       printf("\n");
	       return(-1);
	    }
	 } else {
	    fclose(fil);		/* we found something */
/*
 * Back up to start of last field in entry
 */
	    caplist = tty->t_caplist;
	    ip = &caplist[tty->t_op - 2];
	    while (*ip != ':') ip--;
/*
 * If last field is "tc", backup op so that the tc field gets
 * overwritten with the referenced entry
 */
	    if(strncmp(ip + 1,"tc",2) == 0 || strncmp(ip + 1,"TC",2) == 0) {
	       int is_tc = !strncmp(ip + 1,"tc",2); /* not TC */

	       if(++ntc > MAX_TC_NESTING) {	/* Check for recursive tc */
		  printf("Too deep nesting of \"tc\" for %s\n",device);
		  continue;
	       }
	       
	       /* Set op to point to the ":" in ":tc=file". */
	       tty->t_op = ip - caplist;
/*
 * Get device name from tc field, and loop again to fetch new entry
 */
	       ip += strlen(":tc=");
	       for(op=device;*ip != '\0' && *ip != ':';) {
		  *op++ = *ip++;
	       }
	       *op = '\0';

	       if(is_tc) {
		  break;
	       } else {
		  continue;
	       }
	    }
	    return(0);
	 }
      }
   }
}

/******************************************************/
/*
 * TTY_FETCH_ENTRY -- Search the termcap file for the named entry, then read
 * the colon delimited capabilities list into the caplist field of the tty
 * descriptor.  If the caplist field fills up, allocate more space.
 */
#define LBUF_SIZE 201
#define SZ_FNAME 40

static int
tty_fetch_entry (fil,device,tty)
FILE *fil;
char device[];
TTY *tty;
{
   char	lastch,
	*ip, *op, *otop, lbuf[LBUF_SIZE],
	alias[SZ_FNAME];
   int	ch,
   	device_found;
/*
 * Locate entry.  First line of each termcap entry contains a list
 * of aliases for the device.  Only first lines and comment lines
 * are left justified.
 */
   do {
      /* Skip comment and continuation lines and blank lines. */
      device_found = 0;
      
      if ((ch = getc(fil)) == EOF)
	return(-1);
      
      if (ch == '\n') {
	 /* Skip a blank line. */
	 continue;
      } else if (ch == '#' || isspace(ch)) {
	 /* Discard the rest of the line and continue. */
	 if(sm_getline(lbuf,LBUF_SIZE,fil) == EOF)
	   return(-1);
	 continue;
      }
/*
 * Extract list of aliases.  The first occurrence of ':' marks
 * the end of the alias list and the beginning of the caplist.
 */
      lbuf[0] = ch;
      op = lbuf + 1;
      
      for(;(ch = getc(fil)) != ':' && op < lbuf + LBUF_SIZE - 1;op++){
	 if(ch == EOF) {
	    return(-1);
	 }
	 *op = ch;
      }
      *op++ = ':';
      *op = '\0';
      
      ip = lbuf;
      while (tty_extract_alias (ip, alias, SZ_FNAME) > 0) {
	 if (device[0] == '\0' || !strcmp(alias, device)) {
	    device_found = 1;
	    break;
	 }
	 ip += strlen(alias);
	 if(*ip == '|' || *ip == ':')
	   ip++;				/* skip delimiter */
      }
      
      /* Skip rest of line if no match. */
      if (!device_found) {
	 if(sm_getline(lbuf,LBUF_SIZE,fil) == EOF) {
	    return(-1);
	 }
      }
   } while (!device_found);
/*
 * Caplist begins at first ':'.  Each line has some whitespace at the
 * beginning which should be skipped.  Escaped newline implies
 * continuation.
 */
   op = &tty->t_caplist[tty->t_op];
   otop = &tty->t_caplist[tty->t_len];
   
   /* We are already positioned to the start of the caplist. */
   *op++ = ':';
   lastch = ':';
   
   /* Extract newline terminated caplist string. */
   while ((ch = getc(fil)) != EOF) {
      if (ch == '\\') {				/* escaped newline? */
	 if((ch = getc(fil)) == '\n') {
	    if((ch = getc(fil)) == '#') {
	       if(sm_getline(lbuf,LBUF_SIZE,fil) == EOF) {
		  return(-1);
	       }
	    } else {
	       ungetc(ch,fil);
	    }
	    while((ch = getc(fil)) != EOF && isspace(ch)) ;
	    
	    if(ch == EOF || ch == '\n')
	      return(-1);
	    if(ch == ':' && lastch == ':')/* Avoid null entries "::"*/
	      continue;
	    else
	      *op = ch;
	 } else {			/* no, keep both chars */
	    *op++ = '\\';
	    *op = ch;
	 }
      } else if (ch == '\n') {			/* normal exit */
	 *op = '\0';
	 tty->t_op = op - tty->t_caplist;
	 tty->t_caplen = tty->t_op + 1;		/* include '\0' */
	 return(0);
      } else
	*op = ch;
      
/*
 * Increase size of buffer if necessary.  Note that realloc may
 * move the buffer, so we must recalculate op and otop
 */
      lastch = ch;
      if (++op >= otop - 1) {	/* can have to op += 2 */
	 tty->t_op = op - tty->t_caplist;
	 tty->t_len += T_MEMINC;
	 if((tty->t_caplist = realloc(tty->t_caplist,(unsigned)tty->t_len))
								    == NULL) {
	    printf("Can't reallocate tty->t_caplist\n");
	    free((char *)tty);
	    tty = NULL;
	    return(-1);
	 }
	 op = tty->t_caplist + tty->t_op;
	 otop = tty->t_caplist + tty->t_len;
      }
   }
   return(0);
}

/**********************************************************/
/*
 * TTY_EXTRACT_ALIAS -- Extract a device alias string from the header of
 * a termcap entry.  The alias string is terminated by '|' or ':'.  Leave
 * ip pointing at the delimiter.  Return number of chars in alias string.
 */
static int
tty_extract_alias (ip, outstr, maxch)
char *ip;		/* on input, first char of alias */
char	outstr[];
int	maxch;
{
   char *op;
   
   op = outstr;
   while(*ip != '|' && *ip != ':' && *ip != '\0') {
      *op++ = *ip++;
      if(op >= outstr + maxch) {	/* no room in outstr, */
	 op--;				/* but read to end of alias anyway */
      }
   }
   *op = '\0';
   
   return(op - outstr);
}
/*
 * Read a line from fil into buff, return number of chars read
 */
static int
sm_getline(buff,size,fil)
char buff[];
int size;
FILE *fil;
{
   buff[size - 1] = '\0';			/* make sure it's terminated */
   if(fgets(buff,size,fil) == NULL) {
      return(EOF);
   }

   if(buff[size - 1] != '\0') {
      printf("Truncating line in capfile to %d chars in sm_getline\n",size - 1);
      buff[size - 1] = '\0';
      return(size - 1);
   } else {
      return(strlen(buff));
   }
}

static int
tty_scan_env(tty, termcap, device)
TTY *tty;				/* tty descriptor structure */
char *termcap;				/* termcap string to be scanned */
char *device;
{
   char	alias[SZ_FNAME],
   	*termcap_file;
   int device_found,
       len;

   termcap_file = termcap;
   if(termcap[0] == ':') {		/* no list of names, just caps */
      ;
   } else {
      device_found = 0;
      while((len = tty_extract_alias(termcap,alias,SZ_FNAME)) > 0) {
	 if(device[0] == '\0' || !strcmp(alias, device)) {
	    device_found = 1;
	    break;
	 }
	 termcap += len;
	 if(*termcap == '|') termcap++;		/* skip delimiter */
      }
      
      if(!device_found) {
	 printf("Can't find entry for %s in %s\n",device,termcap_file);
	 return(-1);
      }
   }
/*
 * We know how long the string is, so allocate the space,
 * then copy the termcap string into the tty descriptor
 */
   free(tty->t_caplist);
   len = strlen(termcap);
   if((tty->t_caplist = malloc(len + 1)) == NULL) {
      fprintf(stderr,"Can't allocate storage for tty->t_caplist\n");
      return(-1);
   }
   tty->t_caplen = len;
   (void)strcpy(tty->t_caplist,termcap);
   return(0);
}
/*
 * ttygetb -- Determine whether or not a capability exists for a device.
 * If there is any entry at all, the capability exists.
 */
int
ttygetb (tty, cap)
TTY *tty;			/* tty descriptor */
char cap[];			/* two character capability name */
{
   return (tty_find_capability (tty, cap) != NULL);
}

/*************************************************************/
/*
 * TTYGETI -- Get an integer valued capability.  If the capability is not
 * found for the device, or cannot be interpreted as an integer, zero is
 * returned.  Integer capabilities have the format ":xx#dd:".
 */
int
ttygeti (tty, cap)
TTY *tty;			/* tty descriptor */
char cap[];		/* two character capability name */
{
   char *ip;
   
   if ((ip = tty_find_capability(tty, cap)) == NULL)
     return (0);
   else if(*ip != '#') {
      return (0);
   } else {
      return(atoi(++ip));		/* skip the '#' */
   }
}

/**************************************************/
/*
 * TTYGETR -- Get a real valued capability.  If the capability is not
 * found for the device, or cannot be interpreted as a number, zero is
 * returned.  Real valued capabilities have the format ":xx#num:".
 */
float
ttygetr (tty, cap)
TTY *tty;			/* tty descriptor */
char	cap[];		/* two character capability name */
{
   char *ip;			/* pointer to graphcap string */
   
   if ((ip = tty_find_capability(tty, cap)) == NULL)
     return(0.0);
   else if(*ip != '#') 
     return(0.0);
   else {
      return(atof(++ip));
   }
}

/***********************************************************/
/*
 * TTYGETS -- Get the string value of a capability. Process all termcap escapes
 * These are:
 * 
 * 	\E		ascii esc (escape)
 * 	^X		control-X (i.e., ^C=03B, ^Z=032B, etc.)
 * 	\[nrtbf]	newline, return, tab, backspace, formfeed
 * 	\ddd		octal value of character
 * 	\^		the character ^
 * 	\\		the character \
 * 
 * The character ':' may not be placed directly in a capability string; it
 * should be given as \072 instead.  The null character is represented as \200;
 * all characters are masked to 7 bits upon output by TTYPUTS, hence \200
 * is sent to the terminal as NUL.
 */
int
ttygets(tty, cap, outstr, maxch)
TTY *tty;			/* tty descriptor */
char cap[],			/* two character capability name */
     outstr[];			/* receives cap string */
int maxch;			/* size of outstr */
{
   char	ch,
	*ip,
	*op;

   op = outstr;
   *op = '\0';			/* empty string if not present */

   if((ip = tty_find_capability (tty, cap)) != NULL) {
/* Skip the '=' which follows the two character capability name. */
      if (*ip == '=') ip++;
/* Extract the string, processing all escapes. */
      for(ch = *ip;ch != ':';ch = *ip) {
	 if(ch == '^') {
	    if(islower(ch)) ch = toupper(ch);
	    ch = *++ip - 'A' + 1;
	 } else if (ch == '\\') {
	    switch (*++ip) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7':
	        ch = *ip - '0';
	        if(isdigit(*(ip+1))) ch = 8*ch + (*++ip - '0');
		if(isdigit(*(ip+1))) ch = 8*ch + (*++ip - '0');
	        break;
	    case 'E':
	        ch = '\033';		/* Escape */
		break;
	    case 'b':
	        ch = '\b';
		break;
	    case 'f':
	        ch = '\f';
		break;
	    case 'n':
	        ch = '\n';
		break;
	    case 'r':
	        ch = '\r';
		break;
	    case 't':
	        ch = '\t';
		break;
	    case '^':
	    case ':':
	    case '\\':
	    default:
		ch = *ip;
		break;
	    }
	 }

	*op++ = ch;
	ip++;
	if (op - outstr >= maxch - 1)		/* keep space for a NUL */
	    break;
      }
      *op = '\0';
   }
   return(op-outstr);
}
#if 0
.nf _________________________________________________________________________
TTY_INDEX_CAPS -- Prepare an index into the caplist string, stored in
the tty descriptor.  Each two character capability name maps into a unique
integer code, called the capcode.  We prepare a list of capcodes, keeping
only the first such code encountered in the case of multiple entries.
The offset of the capability in the caplist string is associated with each
capcode.  When these lists have been prepared, they are sorted to permit
a binary search for capabilities at runtime.
#endif

void
tty_index_caps (tty, t_capcode, t_capindex)
TTY *tty;
int t_capcode[], t_capindex[];
{
   char *ip, *caplist;
   int i, swap, capcode, temp,
       ncaps;

   ip = caplist = tty->t_caplist;
/*
 * Scan the caplist and prepare the capcode and capindex lists.
 */
   for(ncaps=0;ncaps < MAX_CAPS;) {
/*
 * Advance to the next capability field.  Normal exit occurs when ':' is
 * followed immediately by EOS.
 */
	while (*ip != ':' && *ip != '\0') ip++;
	if(*(ip+1) == '\0' || *ip == '\0') {
		break;
	}
	ip++;			/* skip : */
	capcode = ENCODE(ip);
/*
 * Is the capcode already in the list?  If not found, add it to the list.
 */
	for(i=0;i < ncaps && tty->t_capcode[i] != capcode;i++) ;
	if(i >= ncaps) {				/* not found */
	   t_capcode[ncaps] = capcode;
	   t_capindex[ncaps++] = ip - caplist;
	}
    }

    if (ncaps >= MAX_CAPS) {
       printf("To many capabilities in graphcap entry\n");
       tty->t_ncaps = MAX_CAPS;
    }
    tty->t_ncaps = ncaps;
/*
 * A simple interchange sort is sufficient here, even though it would
 * not be hard to interface to qsort.  The longest termcap entries are
 * about 50 caps, and the time req'd to sort such a short list is
 * negligible compared to the time spent searching the termcap file.
 */
   if (ncaps > 1) {
      do {
	 swap = 0;
	 for(i = 0;i < ncaps-1;i++) {
	    if(t_capcode[i] > t_capcode[i+1]) {
	       temp = t_capcode[i];
	       t_capcode[i] = t_capcode[i+1];
	       t_capcode[i+1] = temp;
	       temp = t_capindex[i];
	       t_capindex[i] = t_capindex[i+1];
	       t_capindex[i+1] = temp;
	       swap = 1;
	    }
	}
      } while (swap);
   }
}


/* TTY_FIND_CAPABILITY -- Search the caplist for the named capability. */
/* If found, return a pointer to the first char of the value field, */
/* otherwise return NULL. If the first char in the capability string */
/* is '@', the capability "is not present". */

static char *
tty_find_capability(tty, cap)
TTY *tty;			/* tty descriptor */
char cap[];			/* two character name of capability */
{
   char *ptr;
   int	capcode, capnum;
   
   capcode = ENCODE(cap);
   capnum = tty_binsearch (capcode, tty->t_capcode,tty->t_ncaps);
   
   if (capnum >= 0) {
      /* Add 2 to skip the two capname chars. */
      ptr = &tty->t_caplist[tty->t_capindex[capnum] + 2];
      if(*ptr == '@') return (NULL);
      else return(ptr);
   }
   return (NULL);
}


/* TTY_BINSEARCH -- Perform a binary search of the capcode array for the */
/* indicated capability.  Return the array index of the capability if found, */
/* else zero. */

static int
tty_binsearch (capcode, t_capcode, ncaps)
int capcode,
    t_capcode[],
    ncaps;
{
   int	low, high, pos, ntrips;

	low = 0;
	high = ncaps - 1;
	/* Cut range of search in half until code is found, or until range */
	/* vanishes (high - low <= 1).  If neither high or low is the one, */
	/* code is not found in the list. */

	pos = 0;
	for(ntrips = 0;ntrips < ncaps;ntrips++){
	    pos = (high - low) / 2 + low;
	    if (t_capcode[low] == capcode)
		return (low);
	    else if (t_capcode[high] == capcode)
		return (high);
	    else if (pos == low)			/* (high-low)/2 == 0 */
		return (-1);				/* not found */
	    else if (t_capcode[pos] < capcode)
		low = pos;
	    else
		high = pos;
	}

/* Search cannot fail to converge unless there is a bug in somewhere. */

   printf("Search failed to converge in tty_binsearch\n");
   return(-1);
}

/************************************************************/
/*
 * This routine takes a line and writes it to stdout.
 * every NLIN newlines, it stops and waits for:
 *   A carriage return or linefeed      (for one more line),
 *   /string<CR>                        (to search for string),
 *   ^C                                 (as an interrupt),
 *   ^D                                 (for another half page),
 *   h                                  (for help),
 *   n                                  (for repeat previous search),
 *   q                                  (for quit),
 * Anything else gives another page.
 *
 * If line is NULL, then initialise the the number of lines printed to 0.
 * Note that q ("Quit") works by cleaning up and returning -1.
 * It is YOUR responsibility to ensure that interrupts are acted upon.
 */

#define LSIZE 80                /* maximum number of characters in a line */
#define NOTHING 50258 	        /* more has nothing interesting to report */
#define PSIZE 40                /* longest pattern to look for */

extern int interrupt,	        /* handle ^C interrupts */
	   nlines;		/* number of lines on screen */
static int num_printed = 0,     /* # of lines in page already printed */
           search = 0,          /* looking for a pattern */
	   wait_for_a_character();

int
more(line)
char *line;                     	/* line to print */
{
   char *lptr;				/* pointer to line */
   static char pattern[PSIZE] = "",     /* pattern to look for */
               *pptr;                   /* pointer to pattern */

   if(line == NULL) {        		/* reset line counter */
      num_printed = 0;
      search = 0;                       /* no searching */
      return(0);
   }

   if(interrupt) return(-1);

   if(search) {                         /* we are looking for a pattern */
      pptr = pattern;
      for(lptr = line;*lptr != '\0';lptr++) {
	 if(*lptr == *pptr) {
            if(*(pptr + 1) == '\0') {      /* we've matched the pattern */
               search = 0;
               num_printed = -2;
	       break;
	    } else {
	       pptr++;
	    }
	 } else {                          /* no, this isn't it */
	    pptr = pattern;
	 }
      }
   }

   if(!search) {
      (void)fputs(line,stdout);
      for(lptr = line;*lptr != '\0';) {
	 if(*lptr++ == '\n') {
	    if(++num_printed >= nlines) {
	       num_printed = 0;
	       return(wait_for_a_character(pattern));	/* what it says */
	    }
	 }
      }
   }
   
   return(0);
}

/************************************************************/
/*
 * wait for a character, and act upon it
 */
static int
wait_for_a_character(pattern)
char *pattern;                         /* string to search for */
{
   int i,j;

   if(interrupt) return(-1);
   
   (void)get1char(CTRL_A);			/* set get1char */

   (void)write(1,"...",3);
   i = get1char(0);
   printf("\r   \b\b\b");

   if(i == '\n' || i == '\r') {         /* one more line */
      num_printed = nlines;
   } else if(i == CTRL_D) {             /* half a page */
      num_printed = nlines/2;
   } else if(i == 'h' || i == '?') {    /* help */
      (void)get1char(EOF);
      printf("%s\n%s\n%s\n",
	  "Use q to quit, <CR> for another line, ^D for half a page,",
	  "h or ? for this message, /string<CR> to search (n or /<CR> repeats)",
	  "Anything else gives a full page.");
      return(wait_for_a_character(pattern));
   } else if(i == 'n') {                        /* repeat search */
      (void)putchar('\r');
      if(pattern[0] == '\0') {			/* no pattern specified */
	 (void)putchar(CTRL_G);
	 return(wait_for_a_character(pattern));
      } else {
	 search = 1;                               /* start searching */
      }
   } else if(i == 'q') {                        /* quit */
      return(-1);
   } else if(i == '/') {                        /* look for a pattern */
      (void)putchar('/');
      (void)fflush(stdout);
      for(i = 0;i < PSIZE;i++) {
         if((j = get1char(0)) == '\n' || j == '\r') {
            break;
         } else if(j == DEL) {                  /* delete character */
            if(i == 0) {                        /* abort search */
               printf("\b \b");                 /* remove / */
	       (void)fflush(stdout);
	       (void)get1char(EOF);
	       return(wait_for_a_character(pattern));
            } else {
               i -= 2;                          /* ready to be ++'ed */
               (void)printf("\b \b");
	       (void)fflush(stdout);
            }
         } else {
            (void)putchar(pattern[i] = j);
	    (void)fflush(stdout);
         }
      }
      if(i != 0) {                           /* null terminate a new pattern */
         pattern[i] = '\0';
      }
      (void)putchar('\r');
      search = 1;                               /* start searching */
   }

   (void)get1char(EOF);                      /* no more RAW mode */
   return(0);
}

/*
 * Return a match for a Regular Expression.
 *
 * A subset of the usual Unix regular expressions are understood:
 *
 *	.	Matches any character except \n
 *	[...]	Matches any of the characters listed in ... .
 *		  If the first character is a ^ it means use anything except
 *		the range specified (in any other position ^ isn't special)
 *		  A range may be specified with a - (e.g. [0-9]), and if a ]
 *		is to be part of the range, it must appear first (or after
 *		the leading ^, if specified). A - may appear as the [---].
 *	^	Matches only at the start of the string
 *	$	Matches only at the end of a string
 *	\t	Tab
 *	\n	Newline
 *	\.	(any other character) simply escapes the character (e.g. \^)
 *	*	Any number of the preceeding character (or range)
 *	?	One or more of the preceeding character or range
 *
 */
#define START '\001'			/* special characters */
#define END   '\002'
#define DOT   '\003'
#define STAR  '\004'
#define QUERY '\005'
#define RANGE '\006'

/* compiler complains about char C; try this */
#define MATCH(P,C,R)			/* Does P match C, for range R? */\
   (((P) == RANGE && (R)[(int)C]) ||					\
    ((P) == DOT && (C) != '\n') ||					\
    (P) == (C))
#define RANGE_SIZE 128			/* number of chars allowed in ranges */

static char cpattern[50],		/* compiled pattern */
	    *_match_pattern(),
	    *s_range = NULL;		/* storage for range of characters */
static int have_pattern = 0,		/* do we have a compiled pattern? */
  	   case_fold_search = 0,	/* case insensitive? */
	   compile_pattern();
static unsigned int nrange = -1;	/* number of range's allocated */

/******************************************************************/
/*
 * Match a pattern, the beginning of the match is returned, or NULL.
 * If the pattern is given as NULL the same pattern as last time will
 * be used.
 * If end is not NULL it serves two purposes:
 * 1/ On entry, it marks the end of the region to search (specifically,
 *    it marks the last point where a match can start; matches can
 *    extend past end).
 * 2/ On return it will point to the end of a succesfull match, or the
 *    start of the string if unsuccessful.
 *
 * Note that this code is NOT reentrant, and that only one call can be
 * active at a time.
 */
char *
match_pattern(str,pattern,end)
char *str,				/* string to search */
     *pattern,				/* pattern to look for */
     **end;				/* end of pattern, if matched */
{
   char *eend;

   if(pattern != NULL) {
      if(compile_pattern(pattern) < 0) {
	 return(NULL);
      }
   } else if(!have_pattern) {
      (void)fprintf(stderr,"There is no current pattern to match\n");
      return(NULL);
   }
   if(*str == '\0') {
      return("");			/* allow setting/checking of pattern */
   }

   if(end == NULL) {			/* provide storage for end if needed */
      end = &eend;
      *end = NULL;
   }

   if(cpattern[0] == START) {
      return(_match_pattern(str,&cpattern[1],(char *)NULL,end,1));
   } else {
      return(_match_pattern(str,cpattern,(char *)NULL,end,0));
   }
}

/*******************************************************/
/*
 * Return start of match (already compiled). This function IS reentrant,
 * unlike match_pattern
 */
static char *
_match_pattern(str,cpatt,range0,end,match_at_start)
char *str,				/* string to search */
     *cpatt,				/* compiled pattern */
     *range0,				/* current range */
     **end;				/* end of match */
int match_at_start;			/* match must be at start of string? */
{
   char c,
   	*mptr,				/* where the current match has got to*/
        *next_range = NULL,		/* range for next RANGE in pattern
					   (initialised to NULL to placate
					   flow-analysing compilers) */
   	*range,				/* current range */
        *range1,			/* a spare range */
        *pptr,				/* where we are in pattern */
        *sptr;				/* where the current match starts */

   for(sptr = str;*sptr != '\0' && (*end == NULL || sptr < *end);sptr++) {
      for(mptr = sptr, pptr = cpatt, range = range0;;mptr++, pptr++) {
	 if(*mptr == '\0') {
	    if(*pptr == END || *pptr == '\0') {
	       *end = mptr;
	       return(sptr);
	    } else {
	       *end = str;
	       return(NULL);
	    }
	 }

	 switch(*pptr) {
	  case '\0':			/* reached end of pattern -- Success */
	    if(sptr == mptr) {		/* trivial match */
	       goto no_match;
	    }
	    *end = mptr;
	    return(sptr);
	  case END:
	    goto no_match;
	  case QUERY:
	  case STAR:
	  case START:
	    (void)fprintf(stderr,"You can't get here! (In _match_pattern)\n");
	    break;
	  case RANGE:
	    if(range == NULL) {
	       range = (range0 == NULL) ? s_range : range;
	    } else {
	       range += RANGE_SIZE;
	    }				/* fall through to default */
	  case DOT:			/* fall through to default */
	  default:
	    if(*(pptr + 1) == QUERY || *(pptr + 1) == STAR) {
	       int nmatch;
	       
	       if(*pptr == RANGE) {	/* we may need start it again */
		  range1 = (range == s_range) ? NULL : range - RANGE_SIZE;
	       } else {
		  range1 = range;
	       }
	       if(*(pptr + 2) == RANGE) {
		  if(range == NULL) {
		     next_range = (range0 == NULL) ? s_range : range;
		  } else {
		     next_range = range + RANGE_SIZE;
		  }
	       }

	       for(nmatch = 0;*mptr != '\0';mptr++,nmatch++) {
		  c = (case_fold_search && isupper(*mptr)) ?
		    				      tolower(*mptr) : (*mptr);
		  if(!MATCH(*pptr,c,range)) {
		     break;
		  }
		  if(MATCH(*(pptr + 2),c,next_range)) {
		     char *end1 = NULL,*end2 = NULL,
		          *start1,*start2;
		     
		     start1 = _match_pattern(mptr + 1,pptr,range1,&end1,1);
		     start2 = _match_pattern(mptr,pptr + 2,range,&end2,1);
		     if(start1 == NULL) {
			if(start2 == NULL) {
			   goto no_match;
			} else {
			   if(*(pptr + 1) == QUERY && nmatch == 0)  {
			      goto no_match;
			   }
			   *end = end2;
			   return(sptr);
			}
		     } else if(start2 == NULL) {
			*end = end1;
			return(sptr);
		     } else {		/* neither failed */
			*end = end1 > end2 ? end1 : end2;
			return(sptr);
		     }
		  }
	       }
	       mptr--;
	       if(*++pptr == QUERY && nmatch == 0)  {
		  goto no_match;
	       }
	    } else {
	       c = (case_fold_search && isupper(*mptr)) ?
		    				      tolower(*mptr) : (*mptr);
	       if(!MATCH(*pptr,c,range)) {
		  goto no_match;
	       }
	    }
	    break;
	 }
      }
      no_match: ;
      if(match_at_start) break;
   }
   *end = str;
   return(NULL);
}

/******************************************************************/
/*
 * Convert a string into an internal format, ready for pattern matching
 */
static int
compile_pattern(pattern)
char *pattern;
{
   char c1,c2,
        *cptr,				/* pointer to cpattern */
        *range = NULL;
   int i,
       range_value;			/* 0 or 1, for [^...] or [...] */

   for(cptr = cpattern;cptr - cpattern < sizeof(cpattern) - 1;
       							cptr++,pattern++) {
      switch(*pattern) {
       case '\0':
	 *cptr = '\0';
	 have_pattern = 1;
	 return(0);
       case '^':
	 if(cptr != cpattern) {
	    (void)fprintf(stderr,
			  "A ^ can only appear at the start of a pattern\n");
	    have_pattern = 0;
	    return(-1);
	 }
	 *cptr = START;
	 break;
       case '$':
	 if(*(pattern + 1) != '\0') {
	    (void)fprintf(stderr,
			  "A $ can only appear at the end of a pattern\n");
	    have_pattern = 0;
	    return(-1);
	 }
	 *cptr = END;
	 break;
       case '.':
	 *cptr = DOT;
	 break;
       case '*':
       case '?':
	 if(cptr == cpattern ||
			(cptr == cpattern + 1 && cpattern[0] == START)) {
	    (void)fprintf(stderr,
		"%c cannot be the first character in a pattern\n",*pattern);
	    have_pattern = 0;
	    return(-1);
	 }
	 *cptr = *pattern == '*' ? STAR : QUERY;
	 break;
       case '\\':
	 if(*++pattern == '\0') {
	    (void)fprintf(stderr,
			  "A \\ cannot be the last character in a pattern\n");
	    have_pattern = 0;
	    return(-1);
	 }
	 if(*pattern == 'n') {
	    *cptr = '\n';
	 } else if(*pattern == 't') {
	    *cptr = '\t';
	 } else {
	    *cptr = *pattern;
	 }
	 break;
       case '[':
	 if(s_range == NULL) {
	    nrange = 5;
	    if((s_range = malloc(RANGE_SIZE*nrange)) == NULL) {
	       (void)fprintf(stderr,"Can't allocate storage for range\n");
	       have_pattern = 0;
	       return(-1);
	    }
	    range = s_range;
	 } else {
	    if(range == NULL) {
	       range = s_range;
	    } else {
	       range += RANGE_SIZE;
	       if(range >= s_range + RANGE_SIZE*nrange) {
		  nrange *= 2;
		  if((s_range = realloc(s_range,RANGE_SIZE*nrange)) == NULL) {
		     (void)fprintf(stderr,
				   "Can't reallocate storage for s_range\n");
		     have_pattern = 0;
		     return(-1);
		  }
		  range = s_range + RANGE_SIZE*(nrange/2);
	       }
	    }
	 }
	 *cptr = RANGE;

	 if(*++pattern == '^') {
	    range_value = 0;
	    pattern++;
	 } else {
	    range_value = 1;
	 }
	 for(i = 0;i < RANGE_SIZE;i++) {
	    range[i] = !range_value;
	 }

	 if(*pattern == ']') {		/* special case: initial ']' */
	    range[']'] = range_value;
	    pattern++;
	 }

	 for(;;pattern++) {
	    if(*pattern == '\0') {
	       (void)fprintf(stderr,"Unclosed range in pattern\n");
	       have_pattern = 0;
	       return(-1);
	    } else if(*pattern == ']') {
	       break;
	    } else if(*pattern == '\\') {
	       if(*++pattern == 'n') {
		  c1 = '\n';
	       } else if(*pattern == 't') {
		  c1 = '\t';
	       } else {
		  c1 = *pattern;
	       }
	    } else {
	       c1 = *pattern;
	    }
	    if(*(pattern + 1) == '-') {
	       pattern += 2;
	       if(*pattern == '\0') {
		  (void)fprintf(stderr,"Unclosed range in pattern\n");
		  have_pattern = 0;
		  return(-1);
	       } else if(*pattern == '-' && *(pattern + 1) == '-') {
		  range[(int)c1] = range_value;
		  if(case_fold_search && isupper(c1)) {
		     range[tolower(c1)] = range_value;
		  }

		  range['-'] = range_value; /* [---] is special */
		  pattern++;
		  continue;
	       }

	       if(*pattern == '\\') {
		  if(*++pattern == 'n') {
		     c2 = '\n';
		  } else if(*pattern == 't') {
		     c2 = '\t';
		  } else {
		     c2 = *pattern;
		  }
	       } else {
		  c2 = *pattern;
	       }
	       for(i = c1;i <= c2;i++) {
		  range[i] = range_value;
		  if(case_fold_search && isupper(i)) {
		     range[tolower(i)] = range_value;
		  }
	       }
	    } else {
	       range[(int)c1] = range_value;
	       if(case_fold_search && isupper(c1)) {
		  range[tolower(c1)] = range_value;
	       }
	    }
	 }
	 break;
       default:
	 *cptr = *pattern;
	 break;
      }
   }
   have_pattern = 1;
   return(0);
}
#endif
