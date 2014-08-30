load util.m
load mirella.m
m_opsys " bkunix" strcmp 0=
m_opsys " linux"  strcmp 0= or
m_opsys " osX"    strcmp 0= or
m_opsys " sysV"   strcmp 0= or
#if
load unix.m
#endif
m_opsys " vms" strcmp 0=
#if
load vms.m
#endif
m_opsys " dsi32dos" strcmp 0=
m_opsys " dsi68dos" strcmp 0= or
m_opsys " pl386dos" strcmp 0= or
#if
load dsidos.m
#endif
cr screensize ct
load images
load simfits
load decompile
\ cr .( Saving dictionary)
\ savem
cr




