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
#ifdef PERF_DEBUG
  struct timeval toti;
  struct timeval lati;
  int totcnt;
  int lastcnt;
#endif
} gdbi_sqlite3_ds_t;

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

  char *db_name = scm_to_locale_string (dbh->constr);
  gdbi_sqlite3_ds_t *db_info = malloc (sizeof (gdbi_sqlite3_ds_t));
  if (db_info == NULL)
  {
    dbh->status = status_cons (1, "out of memory");
    return;
  }

  char s = sqlite3_open (db_name, &(db_info->sqlite3_obj));
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
  gdbi_sqlite3_ds_t *sqliteP = (gdbi_sqlite3_ds_t *)dbh->db_info;
  sqlite3 *db = sqliteP->sqlite3_obj;
  sqlite3_stmt *stmt = NULL;
  int rc, i;
  SCM status_out = SCM_EOL;

  // Prepare statement
  rc = sqlite3_prepare_v2 (db, query, -1, &stmt, NULL);
  if (SQLITE_OK != rc)
  {

    dbh->status = status_cons (1, sqlite3_errmsg (db));
    return;
  }

  sqliteP->stmt = stmt;

  // Bind parameters
  for (i = 0; i < argc; i++)
  {
    int idx = i + 1; // SQLite3 parameters are 1-based
    if (scm_is_integer (argv[i]))
    {
      rc = sqlite3_bind_int64 (stmt, idx, scm_to_long_long (argv[i]));
    }
    else if (scm_is_bool (argv[i]))
    {
      rc = sqlite3_bind_int (stmt, idx, scm_is_true (argv[i]) ? 1 : 0);
    }
    else if (scm_is_string (argv[i]))
    {
      char *s = scm_to_locale_string (argv[i]);
      if (NULL == s)
      {
        dbh->status = status_cons (1, "string allocation failed");
        goto cleanup;
      }
      rc = sqlite3_bind_text (stmt, idx, s, -1, SQLITE_TRANSIENT);
      free (s);
    }
    else if (scm_is_null (argv[i]))
    {
      rc = sqlite3_bind_null (stmt, idx);
    }
    else
    {
      dbh->status = status_cons (1, "unsupported parameter type");
      goto cleanup;
    }

    if (SQLITE_OK != rc)
    {
      dbh->status = status_cons (1, sqlite3_errmsg (db));
      goto cleanup;
    }
  }

  // Success: stmt is ready for getrow
  dbh->status = status_cons (0, "query ok");
  return;

cleanup:
  sqlite3_finalize (stmt);
  sqliteP->stmt = NULL;
}

void __sqlite3_query_g_db_handle (gdbi_db_handle_t *dbh, char *query_str)
{
  if (dbh->db_info == NULL)
  {
    dbh->status = status_cons (1, "invalid dbi connection");
    return;
  }

#ifdef PERF_DEBUG
  struct timeval enter, leave, diff, sum;
  struct timezone foo;
  gettimeofday (&enter, &foo);
#endif // PERF_DEBUG

  gdbi_sqlite3_ds_t *db_info = dbh->db_info;
  sqlite3_finalize (db_info->stmt);
  db_info->stmt = NULL;
  sqlite3_stmt *stmt;
  char s
    = sqlite3_prepare_v2 (db_info->sqlite3_obj, query_str, -1, &stmt, NULL);
  if (s != SQLITE_OK)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    return;
  }

  /* test if sqlite3_step runs successful */
  s = sqlite3_step (stmt);
  if ((s != SQLITE_ROW) && (s != SQLITE_DONE) && (s != SQLITE_OK))
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db_info->sqlite3_obj));
    return;
  }

  sqlite3_reset (stmt);
  db_info->stmt = stmt;
  dbh->status = status_cons (0, "query ok");

#ifdef PERF_DEBUG
  gettimeofday (&leave, &foo);
  timersub (&leave, &enter, &diff);
  timeradd (&diff, &db_info->toti, &sum);
  db_info->toti = sum;
  db_info->totcnt++;

  if (0 == db_info->totcnt % 10000)
  {
    timersub (&leave, &db_info->lati, &diff);

    printf ("time in sqlite3=%d.%6d secs; tot elapsed time=%d.%6d secs\n",
            db_info->toti.tv_sec, db_info->toti.tv_usec, diff.tv_sec,
            diff.tv_usec);

    timerclear (&db_info->toti);
    db_info->lati = leave;
  }
#endif // PERF_DEBUG
}

SCM __sqlite3_getrow_g_db_handle (gdbi_db_handle_t *dbh)
{
  gdbi_sqlite3_ds_t *sqliteP = (gdbi_sqlite3_ds_t *)dbh->db_info;
  sqlite3 *db = sqliteP->sqlite3_obj;
  SCM row = SCM_EOL;

  if (NULL == dbh || NULL == sqliteP)
  {
    dbh->status = status_cons (1, "invalid dbi connection");
    return SCM_BOOL_F;
  }

  if (NULL == sqliteP->stmt)
  {
    dbh->status = status_cons (1, "missing query result");
    return SCM_BOOL_F;
  }

  // Step to the next row (once, not per-column)
  int rc = sqlite3_step (sqliteP->stmt);
  if (SQLITE_DONE == rc)
  {
    dbh->status = status_cons (0, "no more rows");
    sqlite3_finalize (sqliteP->stmt);
    sqliteP->stmt = NULL;
    return SCM_BOOL_F;
  }
  if (SQLITE_ROW != rc)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db));
    sqlite3_finalize (sqliteP->stmt);
    sqliteP->stmt = NULL;
    return SCM_BOOL_F;
  }

  int ncols = sqlite3_column_count (sqliteP->stmt);

  // Build the row in reverse so we can prepend efficiently
  for (int c = ncols - 1; c >= 0; c--)
  {
    SCM value;
    int col_type = sqlite3_column_type (sqliteP->stmt, c);

    switch (col_type)
      {
      case SQLITE_INTEGER:
        value = scm_from_long_long (sqlite3_column_int64 (sqliteP->stmt, c));
        break;

      case SQLITE_FLOAT:
        value = scm_from_double (sqlite3_column_double (sqliteP->stmt, c));
        break;

      case SQLITE_TEXT:
      {
        const unsigned char *text = sqlite3_column_text (sqliteP->stmt, c);
        int len = sqlite3_column_bytes (sqliteP->stmt, c);
        value = scm_from_locale_stringn ((const char *)text, len);
        break;
      }

      case SQLITE_BLOB:
      {
        const void *blob = sqlite3_column_blob (sqliteP->stmt, c);
        int len = sqlite3_column_bytes (sqliteP->stmt, c);
        value = scm_from_locale_stringn ((const char *)blob, len);
        break;
      }

      case SQLITE_NULL:
      default:
        value = SCM_BOOL_F;
        break;
      }

      // Always return (field . value)
      const char *col_name = sqlite3_column_name (sqliteP->stmt, c);
      SCM pair = scm_cons (scm_from_locale_string (col_name), value);

    row = scm_cons (pair, row);
  }

  dbh->status = status_cons (0, "row fetched");
  return row;
}
