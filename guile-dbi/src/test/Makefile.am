# Makefile.am - Main sources automake makefile
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
#                                               -*- Makefile -*-
# Process this file with autoconf to produce a configure script.


AUTOMAKE_OPTIONS = gnu
AM_CPPFLAGS = -I$(top_srcdir)/include -I/usr/include
AM_CFLAGS = $(GUILE_CFLAGS)
LDADD = $(GUILE_LIBS)

lib_LTLIBRARIES = libguile-dbd-test.la


libguile_dbd_test_la_SOURCES = guile-dbd-test.c
libguile_dbd_test_la_LIBADD = -ldl $(GUILE_LTLIBS)
libguile_dbd_test_la_LDFLAGS = -export-dynamic
libguile_dbd_test_la_DEPENDENCIES = $(LTLIBOBJS)

EXTRA_DIST=test-guile-dbi.scm

# test section
# Set library path so guile can find libguile-dbi and libguile-dbd-test

check-local:
	LD_LIBRARY_PATH=$(abs_builddir)/.libs:$(abs_builddir)/../.libs:$$LD_LIBRARY_PATH \
	GUILE_DBD_PATH=$(abs_builddir)/.libs \
	guile -s $(srcdir)/test-guile-dbi.scm
