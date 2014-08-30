\ startup file

screensize ct  \ get rows and columns from UNIX environment

smessage
mi
cr ." /home/jeg/mirella/mirella/startup.m"
                     
" keymap" get_def_value dup 0=
#if
	drop
#else
	_read_map
#endif
reload bindings.m



