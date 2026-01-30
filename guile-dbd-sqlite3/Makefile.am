# Makefile.am - Main automake makefile
# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
# Written by Maurizio Boriani <baux@member.fsf.org>
#
# This file is part of the guile-dbd-sqlite3.
#
# The guile-dbd-sqlite3 is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# The guile-dbd-sqlite3 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#                                               -*- Makefile -*-
# Process this file with autoconf to produce a configure script.

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = gnu
SUBDIRS = src

EXTRA_DIST= autogen.sh \
	test/test-guile-dbd-sqlite3.scm \
	test/valgrind.supp \
	test/valgrind-test

check-local:
	guile -s $(srcdir)/test/test-guile-dbd-sqlite3.scm
