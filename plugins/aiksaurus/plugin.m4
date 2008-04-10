
aiksaurus_pkgs="aiksaurus-1.0"
aiksaurus_gtk_pkgs="gaiksaurus-1.0"

AIKSAURUS_CFLAGS=
AIKSAURUS_LIBS=

if test "$enable_aiksaurus" == "yes"; then

if test "$enable_aiksaurus_builtin" == "yes"; then
AC_MSG_ERROR([static linking is not supported for the `aiksaurus' plugin])
fi

PKG_CHECK_MODULES(AIKSAURUS,[ $aiksaurus_pkgs ])

if test "$TOOLKIT" == "gtk"; then
	PKG_CHECK_MODULES(AIKSAURUS_GTK,[ $aiksaurus_gtk_pkgs ])
	AIKSAURUS_CFLAGS="$AIKSAURUS_CFLAGS $AIKSAURUS_GTK_CFLAGS"
	AIKSAURUS_LIBS="$AIKSAURUS_LIBS $AIKSAURUS_GTK_LIBS"
fi

AIKSAURUS_CFLAGS="$AIKSAURUS_CFLAGS "'${PLUGIN_CFLAGS}'
AIKSAURUS_LIBS="$AIKSAURUS_LIBS "'${PLUGIN_LIBS}'

fi

AC_SUBST([AIKSAURUS_CFLAGS])
AC_SUBST([AIKSAURUS_LIBS])

