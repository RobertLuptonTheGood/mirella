/* Version 88/06/14: Mirella 5.00 */

/* *********************** DOTOKENS.H **************************************/

/* secondary tokens for mirellin.c and C interface code */
/* watch the comments; this must be legitimate forth code -- remember that */
/* slash/star is aliased with backslash in mirella.m, so must have spaces */
/* between delimiters and beginning of any text */

#define DOCOLON             301
#define DOCON               302
#define DOVAR               303
#define DOUSER              304
#define DODEFER             305
#define DOCODE              306
#define DOVOC               307
#define DOFCON              308
#define DOCCON              309
#define DOFCCON             310
#define DOSCCON	            311
#define DOCVAR              312
#define DODUM1              313
#define DODUM2              314       /* placekeepers, so following 'N' words */
                                      /* are divisible by 3 */
#define DONAR               315
#define DOSAR               316
#define DOCAR               317
#define DONCAR              318
#define DOSCAR              319
#define DOCCAR              320
#define DONMAR              321
#define DOSMAR              322
#define DOCMAR              323
#define DONCPAR             324
#define DOSCPAR             325
#define DOCCPAR             326
#define DONMAT              327
#define DOSMAT              328
#define DOCMAT              329
#define DONCMAT             330
#define DOSCMAT             331
#define DOCCMAT             332
#define DONMMAT             333
#define DOSMMAT             334
#define DOCMMAT             335
#define DOOFFSET            336
#define DOSTRUCAR           337
#define DOSTRUCCAR          338
#define DOSTRUCMAR          339
#define DOCODE_FPTR	    340
#define DOCODE_PPTR	    341
/* divis by 3, note, again, so can use crazy m' words */
#define DON3MAT             342       
#define DOS3MAT             343
#define DOC3MAT             344
#define DON3CMAT            345
#define DOS3CMAT            346
#define DOC3CMAT            347
#define DON3MMAT            348
#define DOS3MMAT            349
#define DOC3MMAT            350
#define DON4MAT             351
#define DOS4MAT             352
#define DOC4MAT             353
#define D0N4CMAT            354
#define DOS4CMAT            355
#define DOC4CMAT            356
#define DON4MMAT            357
#define DOS4MMAT            358
#define DOC4MMAT            359

/* above table of do-tokens must be transferred whenever changed to */
/* mirella.m, but mirella.m now READS this file as forth code */
/* ************** end of module ************************************ */


