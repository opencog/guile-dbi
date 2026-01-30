/* guile-dbd-test.c - Dummy driver used for dbi testing
 * Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.
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
 *
 * Process this file with autoconf to produce a configure script. */


#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>



/* functions prototypes and structures 
   the word 'test' in function name is exactly the db type use to call
   this driver */
void __test_make_g_db_handle(gdbi_db_handle_t* dbh);
void __test_close_g_db_handle(gdbi_db_handle_t* dbh);
void __test_query_g_db_handle(gdbi_db_handle_t* dbh, char* query);
SCM __test_getrow_g_db_handle(gdbi_db_handle_t* dbh);


typedef struct gdbi_test_ds
{
  int simple_data;
} gdbi_test_ds_t;



void
__test_make_g_db_handle(gdbi_db_handle_t* dbh)
{
  if(scm_is_true(scm_string_p(dbh->constr)) == 0)
    {
      dbh->status = (SCM) scm_cons(scm_from_int(1),
				   scm_from_locale_string("missing connection string"));
      dbh->closed = SCM_BOOL_T;
      return;
    }
  else
    {
      dbh->status = (SCM) scm_cons(scm_from_int(0),
				   scm_from_locale_string("test connect ok"));
      dbh->closed = SCM_BOOL_F;
    }
  return;
}



void 
__test_close_g_db_handle(gdbi_db_handle_t* dbh)
{
  dbh->db_info = NULL;
  dbh->status = (SCM) scm_cons(scm_from_int(0),
			       scm_from_locale_string("test close ok"));
  dbh->closed = SCM_BOOL_T;
  return;
}



void
__test_query_g_db_handle(gdbi_db_handle_t* dbh, char* query)
{
  if (dbh->closed == SCM_BOOL_T)
    {
      dbh->status = (SCM) scm_cons(scm_from_int(1),
				   scm_from_locale_string("connection closed"));
      return;
    }
  dbh->status = (SCM) scm_cons(scm_from_int(0),
			       scm_from_locale_string("test query ok"));
  return;
}



SCM
__test_getrow_g_db_handle(gdbi_db_handle_t* dbh)
{
  if (dbh->closed == SCM_BOOL_T)
    {
      dbh->status = (SCM) scm_cons(scm_from_int(1),
				   scm_from_locale_string("connection closed"));
      return SCM_BOOL_F;
    }
  dbh->status = (SCM) scm_cons(scm_from_int(0),
			       scm_from_locale_string("row retrieved"));
  return (SCM_BOOL_F);
}
