
PRESENTATION_CFLAGS=
PRESENTATION_LIBS=

if test "$enable_presentation" != ""; then

test "$enable_presentation" = "auto" && PLUGINS="$PLUGINS presentation"

PRESENTATION_CFLAGS="$PRESENTATION_CFLAGS "'${PLUGIN_CFLAGS}'
PRESENTATION_LIBS="$PRESENTATION_LIBS "'${PLUGIN_LIBS}'

if test "$enable_presentation_builtin" = "yes"; then
	PRESENTATION_CFLAGS="$PRESENTATION_CFLAGS -DABI_PLUGIN_BUILTIN"
fi

fi

AC_SUBST([PRESENTATION_CFLAGS])
AC_SUBST([PRESENTATION_LIBS])

