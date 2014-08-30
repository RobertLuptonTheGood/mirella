/*
 * These functions are usually provided by an application (e.g. mirage.c).
 */
#include "mirella.h"
#include "images.h"

/*****************************************************************************/
/*
 * Initialise the application part of the header
 */
void
init_app_header()
{
   ;
}

/*****************************************************************************/
/*
 * Allocate or free the application dependent part of a pbuf
 */
void
alloc_app_pbuf(buf)
PBUF *buf;
{
   ;
}

/*****************************************************************************/
/*
 * initialise the application part of a new pbuf
 */
void
init_app_pbuf(buf)
PBUF *buf;
{
   ;
}

/*****************************************************************************/
/*
 * Free the application part of a header structure
 */
void
free_app_pbuf(buf)
PBUF *buf;
{
   if(buf->_header.app != NULL) {
      free(buf->_header.app); buf->_header.app = NULL;
   }
   if(buf->_scr != NULL) {
      free(buf->_scr); buf->_scr = NULL;
   }
}

/*****************************************************************************/
/*
 * Reset any flags used by words bound to cursor keys
 */
void
reset_curflags()
{
   ;
}

