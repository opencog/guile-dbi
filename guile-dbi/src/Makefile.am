# Makefile.am - sources dir main makefile
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
SUBDIRS = dbi test
AM_CPPFLAGS = -I$(top_srcdir)/include -I/usr/include
AM_CFLAGS = $(GUILE_CFLAGS)
# LDADD = $(GUILE_LIBS) $(LIBLTDL)

BUILT_SOURCES = guile-dbi.x

SUFFIXES = .x
.c.x:
	$(GUILESNARF) $(DEFS) $(AM_CPPFLAGS) $(AM_CFLAGS) $(CPPFLAGS) $(CFLAGS) \
	$< > $@ || { rm $@; false; }

lib_LTLIBRARIES = libguile-dbi.la

libguile_dbi_la_SOURCES = guile-dbi.c guile-dbi.x
libguile_dbi_la_LIBADD = $(GUILE_LIBS) $(LIBLTDL)
libguile_dbi_la_LDFLAGS = -export-dynamic -version-info @DBI_INTERFACE@
libguile_dbi_la_DEPENDENCIES = $(LTLIBOBJS)

CLEANFILES = *.x
