/* guile-dbi.c - Main source file
 * Copyright (C) 2004, 2005, 2006, 2026 Free Software Foundation, Inc.
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

#include <errno.h>
#include <guile-dbi/guile-dbi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static scm_t_bits g_db_handle_tag;

#define DBI_SMOB_P(obj) \
  ((SCM_NIMP (obj)) && (SCM_TYP16 (obj) == g_db_handle_tag))

SCM_DEFINE (make_g_db_handle, "dbi-open", 2, 0, 0, (SCM bcknd, SCM conn_string),
            "Build db_handle smob.")
#define FUNC_NAME s_make_g_db_handle
{
  char *sodbd = NULL;
  size_t sodbd_len;
  gdbi_db_handle_t *g_db_handle = NULL;
  void (*connect) (gdbi_db_handle_t *);

  SCM_ASSERT (scm_is_string (bcknd), bcknd, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_string (conn_string), conn_string, SCM_ARG2, FUNC_NAME);

  g_db_handle = (gdbi_db_handle_t *)scm_gc_malloc (sizeof (gdbi_db_handle_t),
                                                   "g_db_handle");

  g_db_handle->bcknd = bcknd;
  g_db_handle->constr = conn_string;
  g_db_handle->handle = NULL;
  g_db_handle->closed = SCM_BOOL_T;
  g_db_handle->in_free = 0;
  g_db_handle->db_info = NULL;
  g_db_handle->bcknd_str = scm_to_locale_string (bcknd);
  g_db_handle->bcknd_strlen = strlen (g_db_handle->bcknd_str);

  sodbd_len = sizeof (char)
              * (strlen ("libguile-dbd-") + g_db_handle->bcknd_strlen + 1);
  sodbd = (char *)malloc (sodbd_len);
  if (sodbd == NULL)
  {
    g_db_handle->status = scm_cons (scm_from_int (errno),
                                    scm_from_locale_string (strerror (errno)));
    SCM_RETURN_NEWSMOB (g_db_handle_tag, g_db_handle);
  }

  snprintf (sodbd, sodbd_len, "libguile-dbd-%s", g_db_handle->bcknd_str);

  g_db_handle->handle = lt_dlopenext (sodbd);
  if (g_db_handle->handle == NULL)
  {
    free (sodbd);
    g_db_handle->status
      = scm_cons (scm_from_int (1), scm_from_locale_string (lt_dlerror ()));
    SCM_RETURN_NEWSMOB (g_db_handle_tag, g_db_handle);
  }

  __gdbi_dbd_wrap (g_db_handle, __FUNCTION__, (void **)&connect);
  if (scm_equal_p (SCM_CAR (g_db_handle->status), scm_from_int (0))
      == SCM_BOOL_F)
  {
    free (sodbd);
    SCM_RETURN_NEWSMOB (g_db_handle_tag, g_db_handle);
  }

  (*connect) (g_db_handle);

  if (sodbd != NULL)
  {
    free (sodbd);
  }

  SCM_RETURN_NEWSMOB (g_db_handle_tag, g_db_handle);
}
#undef FUNC_NAME

static SCM mark_db_handle (SCM g_db_handle_smob)
{
  gdbi_db_handle_t *g_db_handle
    = (gdbi_db_handle_t *)SCM_SMOB_DATA (g_db_handle_smob);

  scm_gc_mark (g_db_handle->bcknd);
  scm_gc_mark (g_db_handle->constr);
  scm_gc_mark (g_db_handle->status);
  scm_gc_mark (g_db_handle->closed);

  return SCM_UNSPECIFIED;
}

/* fix this using bcknd function to print */
static int print_db_handle (SCM g_db_handle_smob, SCM port,
                            scm_print_state *pstate)
{
  gdbi_db_handle_t *g_db_handle
    = (gdbi_db_handle_t *)SCM_SMOB_DATA (g_db_handle_smob);

  scm_puts ("#<guile-dbi ", port);
  if (scm_equal_p (g_db_handle->closed, SCM_BOOL_T) == SCM_BOOL_T)
  {
    scm_puts ("close ", port);
  }
  else
  {
    scm_puts ("open ", port);
  }
  scm_display (g_db_handle->bcknd, port);
  scm_puts (" ", port);
  scm_display (g_db_handle->constr, port);
  scm_puts (" ", port);
  scm_display (g_db_handle->status, port);
  scm_puts (">", port);
  return 1;
}

SCM_DEFINE (close_g_db_handle, "dbi-close", 1, 0, 0, (SCM db_handle),
            "Close db connection.")
#define FUNC_NAME s_close_g_db_handle
{
  gdbi_db_handle_t *g_db_handle = NULL;
  void (*dbd_close) (gdbi_db_handle_t *);

  /* Handle already-finalized smobs gracefully */
  if (!DBI_SMOB_P (db_handle))
    return SCM_UNSPECIFIED;

  g_db_handle = (gdbi_db_handle_t *)SCM_SMOB_DATA (db_handle);

  if (g_db_handle->closed == SCM_BOOL_T)
  {
    return SCM_UNSPECIFIED;
  }

  __gdbi_dbd_wrap (g_db_handle, __FUNCTION__, (void **)&dbd_close);
  if (scm_equal_p (SCM_CAR (g_db_handle->status), scm_from_int (0))
      == SCM_BOOL_F)
  {
    scm_remember_upto_here_1 (db_handle);
    return SCM_UNSPECIFIED;
  }

  (*dbd_close) (g_db_handle);
  if (g_db_handle->handle)
  {
    lt_dlclose (g_db_handle->handle);
    g_db_handle->handle = NULL;
  }

  scm_remember_upto_here_1 (db_handle);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

static size_t free_db_handle (SCM g_db_handle_smob)
{
  gdbi_db_handle_t *g_db_handle = NULL;

  g_db_handle = (gdbi_db_handle_t *)SCM_SMOB_DATA (g_db_handle_smob);
  if (g_db_handle->in_free)
    return 0;
  g_db_handle->in_free = 1;

  close_g_db_handle (g_db_handle_smob);

  if (g_db_handle != NULL)
  {
    free ((void *)g_db_handle->bcknd_str);
    scm_gc_free (g_db_handle, sizeof (gdbi_db_handle_t), "g_db_handle");
  }

  SCM_SETCDR (g_db_handle_smob, SCM_EOL);
  return 0;
}

SCM_DEFINE (query_g_db_handle, "dbi-query", 2, 0, 0, (SCM db_handle, SCM query),
            "Do a query and set status.")
#define FUNC_NAME s_query_g_db_handle
{
  gdbi_db_handle_t *g_db_handle = NULL;
  char *query_str = NULL;

  void (*dbi_query) (gdbi_db_handle_t *, char *);

  SCM_ASSERT (DBI_SMOB_P (db_handle), db_handle, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_string (query), query, SCM_ARG2, FUNC_NAME);

  g_db_handle = (gdbi_db_handle_t *)SCM_SMOB_DATA (db_handle);
  query_str = scm_to_locale_string (query);

  __gdbi_dbd_wrap (g_db_handle, __FUNCTION__, (void **)&dbi_query);
  if (scm_equal_p (SCM_CAR (g_db_handle->status), scm_from_int (0))
      == SCM_BOOL_T)
  {
    (*dbi_query) (g_db_handle, query_str);
  }

  free (query_str);
  scm_remember_upto_here_1 (db_handle);
  return (SCM_UNSPECIFIED);
}
#undef FUNC_NAME

SCM_DEFINE (params_query_g_db_handle, "dbi-params-query", 3, 0, 0,
            (SCM db_handle, SCM query, SCM params),
            "Do a parameterized query and set status.")
#define FUNC_NAME s_params_query_g_db_handle
{
  gdbi_db_handle_t *g_db_handle = NULL;
  char *query_str = NULL;

  void (*dbi_params_query) (gdbi_db_handle_t *, char *, int, SCM *);

  SCM_ASSERT (DBI_SMOB_P (db_handle), db_handle, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_string (query), query, SCM_ARG2, FUNC_NAME);
  SCM_ASSERT (scm_list_p (params), params, SCM_ARG3, FUNC_NAME);

  g_db_handle = (gdbi_db_handle_t *)SCM_SMOB_DATA (db_handle);
  query_str = scm_to_locale_string (query);

  __gdbi_dbd_wrap (g_db_handle, __FUNCTION__, (void **)&dbi_params_query);

  if (scm_equal_p (SCM_CAR (g_db_handle->status), scm_from_int (0))
      == SCM_BOOL_T)
  {
    int argc = scm_to_int (scm_length (params));
    SCM *argv = (SCM *)calloc (sizeof (SCM), argc);
    SCM lst = params;

    for (int i = 0; i < argc; i++)
    {
      argv[i] = SCM_CAR (lst);
      lst = SCM_CDR (lst);
    }

    (*dbi_params_query) (g_db_handle, query_str, argc, argv);
    free (argv);
  }

  free (query_str);
  scm_remember_upto_here_1 (db_handle);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (getrow_g_db_handle, "dbi-get_row", 1, 0, 0, (SCM db_handle),
            "Do a query and return a row in form of pair list or false")
#define FUNC_NAME s_getrow_g_db_handle
{
  gdbi_db_handle_t *g_db_handle = NULL;
  SCM retrow = SCM_EOL;
  SCM (*dbi_getrow) (gdbi_db_handle_t *);

  SCM_ASSERT (DBI_SMOB_P (db_handle), db_handle, SCM_ARG1, FUNC_NAME);
  g_db_handle = (gdbi_db_handle_t *)SCM_SMOB_DATA (db_handle);

  __gdbi_dbd_wrap (g_db_handle, __FUNCTION__, (void **)&dbi_getrow);
  if (scm_equal_p (SCM_CAR (g_db_handle->status), scm_from_int (0))
      == SCM_BOOL_F)
  {
    scm_remember_upto_here_1 (db_handle);
    return (retrow);
  }

  retrow = (*dbi_getrow) (g_db_handle);

  scm_remember_upto_here_1 (db_handle);
  return (retrow);
}
#undef FUNC_NAME

SCM_DEFINE (getstat_g_db_handle, "dbi-get_status", 1, 0, 0, (SCM db_handle),
            "Return status pair, code and msg, from dbi smob.")
#define FUNC_NAME s_getstat_g_db_handle
{
  gdbi_db_handle_t *g_db_handle = NULL;

  SCM_ASSERT (DBI_SMOB_P (db_handle), db_handle, SCM_ARG1, FUNC_NAME);

  g_db_handle = (gdbi_db_handle_t *)SCM_SMOB_DATA (db_handle);

  if (g_db_handle != NULL)
  {
    scm_remember_upto_here_1 (db_handle);
    return (g_db_handle->status);
  }

  return (SCM_BOOL_F);
}
#undef FUNC_NAME

/* module init function */
void init_db_handle_type (void)
{
  g_db_handle_tag
    = scm_make_smob_type ("g_db_handle", sizeof (gdbi_db_handle_t));
/* guile-2.0 and newer should not do this.
   See http://lists.gnu.org/archive/html/guile-user/2011-11/msg00074.html */
#if 0
  scm_set_smob_mark (g_db_handle_tag, mark_db_handle);
#endif
  scm_set_smob_free (g_db_handle_tag, free_db_handle);
  scm_set_smob_print (g_db_handle_tag, print_db_handle);
}

void init_dbi (void)
{
  static int is_inited = 0;
  if (is_inited)
    return;
  is_inited = 1;
  init_db_handle_type ();
  lt_dlinit ();
  lt_dlsetsearchpath (getenv ("GUILE_DBD_PATH"));

#ifndef SCM_MAGIC_SNARFER
#include "guile-dbi.x"
#endif
}

/* dbd handler */
void __gdbi_dbd_wrap (gdbi_db_handle_t *dbh, const char *function_name,
                      void **function_pointer)
{
  const char *ret = NULL;
  char *func = NULL;
  size_t func_len;

  func_len = sizeof (char) * (strlen (function_name) + dbh->bcknd_strlen + 10);
  func = malloc (func_len);
  if (NULL == func)
  {
    if (dbh->in_free)
      return; /* do not SCM anything while in GC */
    dbh->status = scm_cons (scm_from_int (errno),
                            scm_from_locale_string (strerror (errno)));
    return;
  }

  /* I assume this is correct for all OS'es */
  snprintf (func, func_len, "__%s_%s", dbh->bcknd_str, function_name);
  lt_dlerror (); /* Clear any existing error -- Solaris needs this */
  *function_pointer = lt_dlsym (dbh->handle, func);
  if ((ret = lt_dlerror ()) != NULL)
  {
    free (func);
    if (dbh->in_free)
      return; /* do not SCM anything while in GC */
    dbh->status = scm_cons (scm_from_int (1), scm_from_locale_string (ret));
    return;
  }

  free (func);

  if (dbh->in_free)
    return; /* do not SCM anything while in GC */
  /* todo: error msg to be translated */
  dbh->status
    = scm_cons (scm_from_int (0), scm_from_locale_string ("symbol loaded"));
}
