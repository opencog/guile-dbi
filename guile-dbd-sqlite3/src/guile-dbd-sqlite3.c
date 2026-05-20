/* guile-dbd-sqlite3.c - main source file
 * Copyright (C) 2009-2026 (jkal@posteo.eu, https://github.com/jkalbhenn)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>. */

#include <errno.h>
#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

void __sqlite3_make_g_db_handle (gdbi_db_handle_t *dbh);
void __sqlite3_close_g_db_handle (gdbi_db_handle_t *dbh);
void __sqlite3_query_g_db_handle (gdbi_db_handle_t *dbh, char *query_str);
void __sqlite3_params_query_g_db_handle (gdbi_db_handle_t *dbh, char *query,
                                         int argc, SCM *argv);
SCM __sqlite3_getrow_g_db_handle (gdbi_db_handle_t *dbh);
SCM status_cons (int code, const char *message);

// Define PERF_DEBUG to get perdiodic printing of time spent in this
// code and the total elapsed time, including application time.
// Useful for verifying expected performance.
// #define PERF_DEBUG 1

typedef struct
{
  sqlite3 *sqlite3_obj;
  sqlite3_stmt *stmt;
  SCM cached_row;
  int has_cached_row;
#ifdef PERF_DEBUG
  struct timeval toti;
  struct timeval lati;
  int totcnt;
  int lastcnt;
#endif
} gdbi_sqlite3_ds_t;

SCM convert_row (sqlite3_stmt *stmt)
{
  SCM row = SCM_EOL;
  int ncols = sqlite3_column_count (stmt);

  for (int i = 0; i < ncols; i++)
  {
    const char *fname = sqlite3_column_name (stmt, i);
    SCM value;

    int type = sqlite3_column_type (stmt, i);

    switch (type)
    {
    case SQLITE_NULL:
      value = SCM_UNSPECIFIED;
      break;

    case SQLITE_INTEGER:
      value = scm_from_int64 (sqlite3_column_int64 (stmt, i));
      break;

    case SQLITE_FLOAT:
      value = scm_from_double (sqlite3_column_double (stmt, i));
      break;

    case SQLITE_TEXT:
      value
        = scm_from_locale_stringn ((const char *)sqlite3_column_text (stmt, i),
                                   sqlite3_column_bytes (stmt, i));
      break;

    case SQLITE_BLOB:
      value = scm_from_locale_stringn (sqlite3_column_blob (stmt, i),
                                       sqlite3_column_bytes (stmt, i));
      break;

    default:
      value = SCM_BOOL_F;
      break;
    }

    row = scm_cons (scm_cons (scm_from_locale_string (fname), value), row);
  }

  return scm_reverse_x (row, SCM_EOL);
}

SCM status_cons (int code, const char *message)
{
  return (scm_cons (scm_from_int (code), scm_from_locale_string (message)));
}

void __sqlite3_make_g_db_handle (gdbi_db_handle_t *dbh)
{
  dbh->closed = SCM_BOOL_T;

  /* check presence of connection string */
  if (scm_equal_p (scm_string_p (dbh->constr), SCM_BOOL_F) == SCM_BOOL_T)
  {
    dbh->status = status_cons (1, "missing connection string");
    return;
  }

  gdbi_sqlite3_ds_t *db_info = malloc (sizeof (gdbi_sqlite3_ds_t));
  if (db_info == NULL)
  {
    dbh->status = status_cons (1, "out of memory");
    return;
  }

  char *db_name = scm_to_locale_string (dbh->constr);
  int s = sqlite3_open (db_name, &(db_info->sqlite3_obj));
  free (db_name);
  if (s != SQLITE_OK)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    free (db_info);
    dbh->db_info = NULL;
    return;
  }

#ifdef PERF_DEBUG
  db_info->toti.tv_sec = 0;
  db_info->toti.tv_usec = 0;
  db_info->totcnt = 0;

  db_info->lati.tv_sec = 0;
  db_info->lati.tv_usec = 0;
  db_info->lastcnt = 0;

  struct timezone foo;
  gettimeofday (&db_info->lati, &foo);
#endif // PERF_DEBUG

  db_info->cached_row = SCM_EOL;
  db_info->has_cached_row = 0;
  db_info->stmt = NULL;
  dbh->db_info = db_info;
  dbh->status = status_cons (0, "db connected");
  dbh->closed = SCM_BOOL_F;
}

void __sqlite3_close_g_db_handle (gdbi_db_handle_t *dbh)
{
  /* check presence of db object */
  if (dbh->db_info == NULL)
  {
    if (!dbh->in_free)
      dbh->status = status_cons (1, "dbd info not found");
    return;
  }

  if (!dbh->in_free)
  {
    gdbi_sqlite3_ds_t *db_info = dbh->db_info;
    sqlite3_finalize (db_info->stmt);
    sqlite3_close_v2 (db_info->sqlite3_obj);
    free (dbh->db_info);
    dbh->db_info = NULL;
    dbh->closed = SCM_BOOL_T;
    dbh->status = status_cons (0, "dbi closed");
  }
}

void __sqlite3_params_query_g_db_handle (gdbi_db_handle_t *dbh, char *query,
                                         int argc, SCM *argv)
{
  gdbi_sqlite3_ds_t *db_info = dbh->db_info;
  if (!db_info)
  {
    dbh->status = status_cons (1, "invalid dbi connection");
    return;
  }

  /* cleanup old stmt */
  if (db_info->stmt)
  {
    sqlite3_finalize (db_info->stmt);
    db_info->stmt = NULL;
  }

  db_info->has_cached_row = 0;
  db_info->cached_row = SCM_UNDEFINED;

  /* prepare statement */
  sqlite3_stmt *stmt = NULL;
  int rc = sqlite3_prepare_v2 (db_info->sqlite3_obj, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    return;
  }

  /* bind parameters */
  if (sqlite3_bind_parameter_count (stmt) != argc)
  {
    dbh->status = status_cons (1, "params count mismatch");
    goto error;
  }

  for (int i = 0; i < argc; i++)
  {
    int idx = i + 1;
    SCM v = argv[i];
    if (scm_is_bool (v))
      rc = sqlite3_bind_int (stmt, idx, scm_is_true (v) ? 1 : 0);
    else if (scm_is_integer (v))
      rc = sqlite3_bind_int64 (stmt, idx, scm_to_long_long (v));
    else if (scm_is_number (v))
      rc = sqlite3_bind_double (stmt, idx, scm_to_double (v));
    else if (scm_is_string (v))
    {
      char *s = scm_to_locale_string (v);
      if (!s)
        goto error;
      rc = sqlite3_bind_text (stmt, idx, s, -1, SQLITE_TRANSIENT);
      free (s);
    }
    else if (scm_is_null (v))
      rc = sqlite3_bind_null (stmt, idx);
    else
    {
      dbh->status = status_cons (1, "unsupported parameter type");
      goto error;
    }

    if (rc != SQLITE_OK)
    {
      dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
      goto error;
    }
  }

  /* execute */
  rc = sqlite3_step (stmt);

  int col_count = sqlite3_column_count (stmt);

  if (col_count == 0)
  {
    /* DML */
    if (rc != SQLITE_DONE)
    {
      dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    }
    else
    {
      dbh->status = status_cons (0, "query ok");
      dbh->affected_rows = sqlite3_changes (db_info->sqlite3_obj);
    }
    sqlite3_finalize (stmt);
    db_info->stmt = NULL;
    return;
  }

  /* SELECT query */
  if (rc == SQLITE_ROW)
  {
    db_info->cached_row = convert_row (stmt);
    db_info->has_cached_row = 1;
    db_info->stmt = stmt;
    dbh->status = status_cons (0, "query ok");
    dbh->affected_rows = 0;
    return;
  }

  /* empty SELECT */
  if (rc == SQLITE_DONE)
  {
    dbh->status = status_cons (0, "query ok");
    dbh->affected_rows = 0;
    sqlite3_finalize (stmt);
    db_info->stmt = NULL;
    return;
  }

  /* error */
  dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));

error:
  if (stmt)
    sqlite3_finalize (stmt);
  db_info->stmt = NULL;
}

void __sqlite3_query_g_db_handle (gdbi_db_handle_t *dbh, char *query_str)
{
  gdbi_sqlite3_ds_t *db_info = dbh->db_info;
  if (!db_info)
  {
    dbh->status = status_cons (1, "invalid dbi connection");
    return;
  }

  /* cleanup old stmt */
  if (db_info->stmt)
  {
    sqlite3_finalize (db_info->stmt);
    db_info->stmt = NULL;
  }

  /* prepare statement */
  sqlite3_stmt *stmt = NULL;
  int rc
    = sqlite3_prepare_v2 (db_info->sqlite3_obj, query_str, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    return;
  }

  int col_count = sqlite3_column_count (stmt);

  if (col_count == 0)
  {
    /* DML or no-column statement */
    rc = sqlite3_step (stmt);
    if (rc != SQLITE_DONE)
    {
      dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    }
    else
    {
      dbh->status = status_cons (0, "query ok");
      dbh->affected_rows = sqlite3_changes (db_info->sqlite3_obj);
    }
    sqlite3_finalize (stmt);
    db_info->stmt = NULL;
    return;
  }

  /* SELECT query */
  rc = sqlite3_step (stmt);
  if (rc == SQLITE_ROW)
  {
    db_info->cached_row = convert_row (stmt);
    db_info->has_cached_row = 1;
    db_info->stmt = stmt;
    dbh->status = status_cons (0, "query ok");
    dbh->affected_rows = 0;
    return;
  }

  /* empty result set */
  if (rc == SQLITE_DONE)
  {
    dbh->status = status_cons (0, "query ok");
    dbh->affected_rows = 0;
    sqlite3_finalize (stmt);
    db_info->stmt = NULL;
    return;
  }

  /* error */
  dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
  sqlite3_finalize (stmt);
  db_info->stmt = NULL;
}

SCM __sqlite3_getrow_g_db_handle (gdbi_db_handle_t *dbh)
{
  gdbi_sqlite3_ds_t *p = dbh->db_info;
  sqlite3_stmt *stmt = p->stmt;

  if (p->has_cached_row)
  {
    p->has_cached_row = 0;
    SCM ret = p->cached_row;
    p->cached_row = SCM_EOL;
    return ret;
  }

  if (NULL == stmt)
  {
    dbh->status = status_cons (1, "missing stmt");
    return SCM_BOOL_F;
  }

  int rc = sqlite3_step (stmt);

  /* END */
  if (SQLITE_DONE == rc)
  {
    dbh->status = status_cons (0, "row end");
    sqlite3_finalize (stmt);
    p->stmt = NULL;
    return SCM_BOOL_F;
  }

  /* ERROR */
  if (SQLITE_ROW != rc)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (p->sqlite3_obj));
    sqlite3_finalize (stmt);
    p->stmt = NULL;
    return SCM_BOOL_F;
  }

  /* ROW */
  return convert_row (stmt);
}
