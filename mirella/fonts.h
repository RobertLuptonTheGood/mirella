/*
 * Definitions for fonts
 */
#define NCHAR 96			/* number of characters in a font */
/* #define NFONT 6			   number of fonts: defined below */
#define MAGSTEP 1.2			/* size change corresponding to \1 */
#define SUB_SUPER_EXPAND 0.6		/* factor to expand sub/superscripts */
#define SLANT 0.2			/* italic slant ratio */
#define TEX_SIZE 50		/* longest allowed definition name or value */

static struct {
   char ab1[2],ab2[3];			/* names of fonts */
} fonts[] = {				/* names of fonts, TeX mode only */
#define ROMAN 0
   { "r", "rm"},
#define GREEK 1
   { "g", "gr"},
#define SCRIPT 2
   { "s", "sc"},
#define TINY 3
   { "t", "ti"},
#define OLD_ENGLISH 4
   { "o", "oe"},
#define PRIVATE 5
   { "p", "pr"}
/*#define ITALIC ?
   { "i", "it"}	is special, and doesn't appear here */
/*#define BOLD ?
   { "b", "bf"}	is special, and doesn't appear here */
};
#define NFONT (sizeof(fonts)/sizeof(fonts[0])) /* number of fonts */
