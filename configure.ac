dnl Process this file with autoconf to create configure.

# header
AC_PREREQ([2.65])

dnl Version definitions.

m4_define([xpdf-poppler_version_major],[3])
m4_define([xpdf-poppler_version_minor],[3])
m4_define([xpdf-poppler_version_micro],[0])
m4_define([xpdf-poppler_version],[xpdf-poppler_version_major.xpdf-poppler_version_minor.xpdf-poppler_version_micro])

AC_INIT([xpdf-poppler],[xpdf-poppler_version],[https://github.com/rbrito/xpdf-poppler/issues])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 foreign dist-xz])
m4_define([AM_SILENT_RULES])
AC_LANG([C++])

dnl Defines for the config.h (FIXME: this should be done here not hardcoded in config.h)
dnl AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
PKG_CHECK_MODULES([PKG_CONFIG], [poppler >= 0.20 xt xpm x11 fontconfig])
m4_pattern_allow(PKG_CONFIG_PKG_ERRORS)

dnl Openmotif does not provide pkgconfig files.
AC_ARG_WITH(motif-prefix,
	[AS_HELP_STRING([--with-motif-prefix=PREFIX], [Use motif.])],
)
AS_IF([test "x$with_motif_prefix" != "xyes"], [
		CPPFLAGS="$CPPFLAGS -I$with_motif_prefix/include"
		LDFLAGS="$LDFLAGS -L$with_motif_prefix/lib"
	]
)
AC_CHECK_LIB([Xm], [XmStringFree],, AC_MSG_ERROR([Cannot find motif (Xm) library]))

# Combine libraries
LIBS="${LIBS} ${PKG_CONFIG_LIBS}"

# Checks for header files.
AC_CHECK_HEADERS([langinfo.h locale.h stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([floor memset nl_langinfo setlocale strcasecmp strchr strncasecmp strrchr strstr])

# trailer
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
