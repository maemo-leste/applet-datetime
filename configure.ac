AC_PREREQ([2.52])

AC_INIT([applet-datetime], patsubst(esyscmd([dpkg-parsechangelog | sed -n '/^Version: \(.*\)$/ {s//\1/;p}']), [
]), [], [applet-datetime])

LT_CURRENT=1
LT_REVISION=0
LT_AGE=0
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AM_GCONF_SOURCE_2

PKG_CHECK_MODULES(OSSO, libosso)
AC_SUBST(OSSO_CFLAGS)
AC_SUBST(OSSO_LIBS)

PKG_CHECK_MODULES(HILDON, hildon-1)
AC_SUBST(HILDON_CFLAGS)
AC_SUBST(HILDON_LIBS)

PKG_CHECK_MODULES(OSSO_AF_SETTINGS, osso-af-settings)
localedir=`pkg-config osso-af-settings --variable=localedir`
AC_SUBST(localedir)

PKG_CHECK_MODULES([CPL], [hildon-control-panel])
AC_SUBST([CPL_CFLAGS])
AC_SUBST([CPL_LIBS])

PKG_CHECK_MODULES([CITYINFO], [libcityinfo0-0])
AC_SUBST([CITYINFO_CFLAGS])
AC_SUBST([CITYINFO_LIBS])

PKG_CHECK_MODULES([TIME], [libtime])
AC_SUBST([TIME_CFLAGS])
AC_SUBST([TIME_LIBS])

PKG_CHECK_MODULES(CLOCKCORE, libclockcore0-0)
AC_SUBST(CLOCKCORE_CFLAGS)
AC_SUBST(CLOCKCORE_LIBS)

PKG_CHECK_MODULES(HILDON_TZ, hildon-time-zone-chooser)
AC_SUBST(HILDON_TZ_CFLAGS)
AC_SUBST(HILDON_TZ_LIBS)

dnl Installation directories
controlpanelpluginlibdir=`pkg-config hildon-control-panel --variable=pluginlibdir`
AC_SUBST([controlpanelpluginlibdir])

controlpanelplugindesktopentrydir=`pkg-config hildon-control-panel --variable=plugindesktopentrydir`
AC_SUBST([controlpanelplugindesktopentrydir])

AC_ARG_ENABLE(cast-checks,  [  --disable-cast-checks   compile with GLIB cast checks disabled],[cchecks=${enableval}],cchecks=yes)
if test "x$cchecks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
fi

AC_ARG_ENABLE(asserts,      [  --disable-asserts       compile with GLIB assertions disabled],[asserts=${enableval}],asserts=yes)
if test "x$asserts" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_ASSERTS"
fi

AC_ARG_ENABLE(checks,       [  --disable-checks        compile with GLIB checks disabled],[checks=${enableval}],checks=yes)
if test "x$checks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CHECKS"
fi

AC_ARG_ENABLE(debug,     [  --enable-debug          enable printing of debug messages],[ddebug=${enableval}],ddebug=no)
if test "x$ddebug" != "xyes"; then
    CFLAGS="$CFLAGS -DG_DEBUG_DISABLE"
fi

AC_OUTPUT
