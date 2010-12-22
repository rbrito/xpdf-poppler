dnl Process this file with autoconf to create configure.

AC_PREREQ(2.65)

dnl Version definitions.

m4_define([xpdf_version_major],[3])
m4_define([xpdf_version_minor],[3])
m4_define([xpdf_version_micro],[0])
m4_define([xpdf_version],[xpdf_version_major.xpdf_version_minor.xpdf_version_micro])

AC_INIT([xpdf],[xpdf_version],[https://github.com/rbrito/xpdf-poppler/issues])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

dnl Defines for the config.h (FIXME: this should be done here not hardcoded in config.h)
dnl AM_CONFIG_HEADER(config.h)

AC_PROG_CXX
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

dnl Cflags should not require altering. FIXME at some point.
dnl Same goes for those defines, these should be removed first.
CPPFLAGS="$CPPFLAGS -Wno-write-strings -DHAVE_DIRENT_H -DHAVE_X11_XPM_H"

PKG_CHECK_MODULES(X11, x11 xpm)
PKG_CHECK_MODULES(FONTCONFIG, fontconfig)
PKG_CHECK_MODULES(POPPLER, poppler)

dnl Openmotif does not provide pkgconfig files.
AC_ARG_WITH(motif,
	[AS_HELP_STRING([--with-motif=PREFIX], [Use motif.])],
	[motif="$withval"],
	[motif="yes"]
)
AS_IF([test "x$motif" != "xyes"], [
		CPPFLAGS="$CPPFLAGS -I$motif/include"
		LDFLAGS="$LDFLAGS -L$motif/lib"
	]
)
AC_CHECK_LIB([Xm], [XmStringFree], [], [AC_ERROR([motif libraries not found])])
MOTIF_LIBS=-lXm
AC_SUBST(MOTIF_LIBS)

dnl Get user supplied path for system config
AC_ARG_WITH([config],
	[AS_HELP_STRING([--with-config], [File where global system config is going to be stored.])],
	[config=$withval],
	[config="/etc/xpdfrc"]
)
AC_DEFINE_UNQUOTED([SYSTEM_XPDFRC], ["$config"])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
