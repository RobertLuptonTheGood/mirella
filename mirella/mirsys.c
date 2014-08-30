#include "options.h"

#if defined(UNIX)
#  include "mirunix.c"
#endif

#if defined(DOS32)
#  include "mir386zt.c"
#endif
