#if !defined(MIRCINTF_H)
#  define MIRCINTF_H
#endif

#include <ctype.h>

/*
 * This file defines the C-Forth interface, as created in minstall and
 * read by mircintf.c
 */
typedef struct {
   char *fname;				/* name of word (forth string) */
   short type;				/* word's type */
   short flags;				/* flags for the type */
   int d1,d2;				/* dimensions if an array or matrix */
   int size;				/* size of element of array/matrix */
} MIRCINTF;
/*
 * enum for type
 */
enum {
   CIF_VARIABLE,			/* Variable; M passes address */
   CIF_CONSTANT,			/* Constant; M passes integer value */
   CIF_FCONSTANT,			/* Floating const;
					   M passes float value */
   CIF_STRING,				/* C string; M copies string onto
					   str stack and passes ss address */
   CIF_OFFSET,				/* Structure member offset; M adds to
					   an address on the stack */

   CIF_NARRAY,				/* Normal array; M takes index and
					   passes address */
   CIF_SARRAY,				/* Short array; M takes index and
					   passes address */
   CIF_CARRAY,				/* char array; M takes index and
					   passes address */
   CIF_STRUCARRAY,			/* struct array, size in MIRCINTF.size
					   M takes index and passes address */

   CIF_NMATRIX,				/* normal matrix; M takes (row,col)
					   indices and passes address */
   CIF_SMATRIX,				/* Short matrix;  M takes (row,col)
					   indices and passes address */
   CIF_CMATRIX,				/* Char matrix;  M takes (row,col)
					   indices and passes address */

   CIF_FUNCTION,			/* Function; M executes */
   CIF_PROCEDURE,			/* procedure; M executes */
/*
 * Two vocabulary words
 */
   CIF_VOCAB,				/* vocabulary; creates vocabulary
					   dictionary entry and places
					   subsequent entries in that
					   vocabulary (changes only current,
					   and routine restores current at
					   entry upon exit) */
   CIF_FORTH,				/* change current vocabulary back to
					   value at entry to init_cif()
					   (presumably forth) */
/*
 * and three for dictionary initialization:
 */
   CIF_PRIMITIVE,			/* primitive; index in mirellin
					   switch */
   CIF_IMMEDIATE,			/* immediate primitive; index
					   in mirellin switch */
   CIF_USER				/* user variable; sets up initial
					   user list */
};

/*****************************************************************************/
/*
 * And for flags
 */
#define CIF_DYNAMIC 01			/* dynamic loading */
