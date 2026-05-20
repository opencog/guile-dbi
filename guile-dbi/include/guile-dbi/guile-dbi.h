/* guile-dbi.h - Main include file
 * Copyright (C) 2004, 2005, 2026 Free Software Foundation, Inc.
 * Written by Maurizio Boriani <baux@member.fsf.org>
 *
 * This file is part of the guile-dbi.
 *
 * The guile-dbi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The guile-dbi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


#ifndef __GUILE_DBI_H__
#define __GUILE_DBI_H__

#include <libguile.h>
#include <ltdl.h>

/* guile smob struct */
typedef struct
{
  SCM bcknd;
  SCM constr;
  SCM status;  /* pair: car = error numeric code, cdr = status message */
  SCM closed;  /* boolean, TRUE if closed otherwise FALSE */
  lt_dlhandle handle;
  void* db_info;
  int in_free;  /* boolean, used to avoid alloc during garbage collection */
  const char * bcknd_str;
  size_t bcknd_strlen;
  long long affected_rows; /* number of rows affected by last query */
} gdbi_db_handle_t;
/* end guile smob struct */



/* data wrapper prototypes */
void init_db_handle_type(void);
void init_dbi(void);
/* end data wrapper prototypes */



/* dbd dynamic wrapper stuff */
void __gdbi_dbd_wrap(gdbi_db_handle_t* dbh, const char* function_name,
                     void** function_pointer);
/* end dbd dynamic wrapper stuff */
#endif
