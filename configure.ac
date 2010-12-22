dnl Process this file with autoconf to create configure.

AC_PREREQ(2.65)

dnl Version definitions.

m4_define([xpdf_version_major],[3])
m4_define([xpdf_version_minor],[3])
m4_define([xpdf_version_micro],[0])
m4_define([xpdf_version],[xpdf_version_major.xpdf_version_minor.xpdf_version_micro])

AC_INIT([xpdf],[xpdf_version],[https://github.com/rbrito/xpdf-poppler/issues])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 foreign dist-xz])
m4_define([AM_SILENT_RULES])

dnl Defines for the config.h (FIXME: this should be done here not hardcoded in config.h)
dnl AM_CONFIG_HEADER(config.h)

AC_PROG_CXX

dnl Cflags should not require altering. FIXME at some point.
dnl Same goes for those defines, these should be removed first.
CPPFLAGS="$CPPFLAGS -Wno-write-strings -DHAVE_DIRENT_H -DHAVE_X11_XPM_H"

PKG_CHECK_MODULES(X11, x11 xpm)
PKG_CHECK_MODULES(FONTCONFIG, fontconfig)
PKG_CHECK_MODULES(POPPLER, poppler)

dnl Openmotif does not provide pkgconfig files.
AC_ARG_WITH(motif-prefix,
	[AS_HELP_STRING([--with-motif-prefix=PREFIX], [Use motif.])],
)
AS_IF([test "x$with_motif_prefix" != "xyes"], [
		CPPFLAGS="$CPPFLAGS -I$with_motif_prefix/include"
		LDFLAGS="$LDFLAGS -L$with_motif_prefix/lib"
	]
)
AC_CHECK_LIB([Xm], [XmStringFree], [], [AC_ERROR([motif libraries not found])])
MOTIF_LIBS=-lXm
AC_SUBST(MOTIF_LIBS)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
