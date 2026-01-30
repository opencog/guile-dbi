# Makefile.am - Main sources automake makefile
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#                                               -*- Makefile -*-
# Process this file with autoconf to produce a configure script.


AUTOMAKE_OPTIONS = gnu
AM_CPPFLAGS = $(CFLAGS) -I/usr/include

lib_LTLIBRARIES = libguile-dbd-mysql.la

libguile_dbd_mysql_la_SOURCES = guile-dbd-mysql.c
libguile_dbd_mysql_la_LIBADD = -ldl -lz $(LIBS) 
libguile_dbd_mysql_la_LDFLAGS = -export-dynamic \
	-version-info @DBD_INTERFACE@
libguile_dbd_mysql_la_DEPENDENCIES = $(LTLIBOBJS)

install-exec-hook:
	-ldconfig
