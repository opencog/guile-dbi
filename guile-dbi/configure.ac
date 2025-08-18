# configure.ac - Configure script for the guile-dbi.
# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
# Written by Maurizio Boriani <baux@member.fsf.org>
#
# This file is part of the guile-dbi.
#
# The guile-dbi is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# The guile-dbi is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([guile-dbi],[2.1.9],[bug-guile@gnu.org])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_MACRO_DIR([m4])

maintainer_mode=${enable_maintainer_mode-no}

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AM_PROG_AS
AC_CHECK_TOOL([NM], [nm], :)
AC_CHECK_TOOL([GREG], [greg], :)
AC_PATH_PROG([GUILESNARF], [guile-snarf], :)

LT_PREREQ([2.4.6])
LT_INIT([dlopen])
LT_CONFIG_LTDL_DIR([libltdl])
LTDL_INIT

# Checks for libraries.
PKG_CHECK_MODULES([GUILE], [guile-3.0])
AC_MSG_NOTICE([GUILE_LIBS: $GUILE_LIBS])
AC_MSG_NOTICE([GUILE_LTLIBS: $GUILE_LTLIBS])

#--
# The following are commented out because they seem to no longer be
# defined. It seems they are no longer needed.
#
#    GUILE_PKG([3.0])
#    GUILE_FLAGS
#    GUILE_SITE_DIR

#--
# The following are commented out because they should not be needed.
# They are left here as a hack of last resort. If things are not working
# as expected, be sure to try setting the ACLOCAL_PATH first! Like so:
#
#    ACLOCAL_PATH=/usr/local/share/aclocal/ ./autogen.sh --no-configure
#
# If that doesn't fix things, then maybe uncommenting the lines below
# will solve something. Maybe. Maybe not.
#
# CFLAGS="${CFLAGS} ${LTDLINCL} `$GUILE_CONFIG compile`"
# LIBS="$LIBLTDL `$GUILE_CONFIG link`"
# GUILE_SITE=`$GUILE_CONFIG info sitedir`

# Check for makeinfo; avoid ugliness if not installed.
AC_CHECK_PROG(have_makeinfo,makeinfo,yes,no)
AM_CONDITIONAL(HAVE_MAKEINFO,test "x$have_makeinfo" = "xyes")

. $srcdir/DBI-VERSION
AC_SUBST(DBI_MAJOR_VERSION)
AC_SUBST(DBI_MINOR_VERSION)
AC_SUBST(DBI_MICRO_VERSION)
AC_SUBST(DBI_INTERFACE)
AC_SUBST(VERSION)

# Checks for typedefs, structures, and compiler characteristics.
AC_LANG([C])
AC_CANONICAL_HOST
AC_C_CONST
AC_C_VOLATILE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile
                 guile-dbi.pc
                 doc/Makefile
                 include/Makefile
                 include/guile-dbi/Makefile
                 src/Makefile
                 src/dbi/Makefile
                 src/test/Makefile])

AC_CONFIG_FILES([src/test/check-guile-dbi],
                [chmod ugo+x src/test/check-guile-dbi])

AC_OUTPUT
