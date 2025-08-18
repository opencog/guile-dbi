
Guile-DBI
=========
guile-dbi provides a simple, generic, easy-to-use scheme/guile interface
to SQL databases, such as Postgres, MySQL or SQLite3.

The system is split into two parts: the DBI (database independent) part,
which provides the scheme interfaces, and the DBD (database dependent)
plugins, which connect to the actual SQL server. Currently, there are
DBD backends for PostgreSQL, MySQL and SQLite3.

Please see the [master README file](../README.md) for more info.

Notices
-------
Copyright (C) 2005,2008,2010 Free Software Foundation, Inc.

Written by Maurizio Boriani <baux@member.fsf.org> 
and Linas Vepstas <linasvepstas@gmail.org>

Guile DBI License
-----------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
Boston, MA 02111-1307 USA
