
rsvg_pkgs="librsvg-2.0 >= 2.0"

RSVG_CFLAGS=
RSVG_LIBS=

if test "$enable_rsvg" == "yes"; then

if test "$enable_rsvg_builtin" == "yes"; then
AC_MSG_ERROR([static linking is not supported for the `rsvg' plugin])
fi

AC_MSG_CHECKING([for gtk toolkit])
if test "$TOOLKIT" == "gtk"; then
  AC_MSG_RESULT([ok])
else
  AC_MSG_ERROR([rsvg plugin: only supported with gtk])
fi

PKG_CHECK_MODULES(RSVG,[ $rsvg_pkgs ])

RSVG_CFLAGS="$RSVG_CFLAGS "'${PLUGIN_CFLAGS}'
RSVG_LIBS="$RSVG_LIBS "'${PLUGIN_LIBS}'

fi

AC_SUBST([RSVG_CFLAGS])
AC_SUBST([RSVG_LIBS])

