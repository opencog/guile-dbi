# configure.ac - Configure script for the guile-dbd-mysql.
# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
# Written by Maurizio Boriani <baux@member.fsf.org>
#
# This file is part of the guile-dbd-mysql.
# 
# The guile-dbd-mysql is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# The guile-dbd-mysql is distributed in the hope that it will be useful,
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

AC_PREREQ(2.57)
AC_INIT(guile-dbd-mysql,2.1.8,bug-guile@gnu.org)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

maintainer_mode=${enable_maintainer_mode-no}

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_AS
AC_PROG_LIBTOOL
AC_CHECK_TOOL([AR], [ar], :)
AC_CHECK_TOOL([NM], [nm], :)
AC_PATH_PROG([GUILECONFIG], [guile-config], :)
AC_PATH_PROG([MYSQLCONFIG], [mariadb_config], :)

# Checks for typedefs, structures, and compiler characteristics.
AC_CANONICAL_HOST
AC_C_CONST
AC_C_VOLATILE

AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strndup])
AC_CHECK_HEADERS([string.h]) 
AC_FUNC_MALLOC
AC_HEADER_STDC 

# Checks for libs
AC_CHECK_LIB(mariadbclient,mysql_init,,
        AC_MSG_ERROR([*** Can't find libmysql]))

CFLAGS=`guile-config compile`
LIBS=`guile-config link`

MYSQL_CFLAGS=`mariadb_config --cflags`
MYSQL_LIBS=`mariadb_config --libs_r`

CFLAGS="${CFLAGS} ${MYSQL_CFLAGS}"
LIBS="${LIBS} ${MYSQL_LIBS}"

. $srcdir/DBD-VERSION
AC_SUBST(DBD_MAJOR_VERSION)
AC_SUBST(DBD_MINOR_VERSION)
AC_SUBST(DBD_MICRO_VERSION)
AC_SUBST(DBD_INTERFACE)
AC_SUBST(VERSION)

AC_CONFIG_FILES([Makefile
		 src/Makefile])

AC_OUTPUT

dnl ====================================================================
echo "
$PACKAGE-$VERSION

	prefix:      ${prefix}
	compiler:    ${CC} ${CPPFLAGS} ${CFLAGS}
	link:        ${LIBS}
"

