#if defined(UNIX)
#  include "unix.h"
#endif

#if defined(DOS32)
#  if defined(ZORTECH)
#     include "o486zt.h"
#  else
#     include "arc386.h"
#  endif
#endif

