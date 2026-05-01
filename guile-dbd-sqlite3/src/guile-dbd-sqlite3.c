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
#include <string.h>
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
#ifdef PERF_DEBUG
  struct timeval toti;
  struct timeval lati;
  int totcnt;
  int lastcnt;
#endif
} gdbi_sqlite3_ds_t;

/* NOTE: SQLite3 is very different from PostgreSQL and MySQL in that it does not
   have a separate prepare/execute API. Instead, sqlite3_step() both executes
   the statement and returns the first row of the result (if any). This means
   that we have to execute the statement immediately after preparing it, and
   then keep the stmt handle around for fetching rows. */
typedef enum
{
  SQL_SELECT = 0,
  SQL_DML,
  SQL_DDL,
  SQL_UNKNOWN
} sql_kind_t;

static sql_kind_t detect_sql_type(const char *sql)
{
  while (*sql == ' ' || *sql == '\n' || *sql == '\t')
    sql++;

  if (strncasecmp (sql, "select", 6) == 0)
    return SQL_SELECT;

  if (strncasecmp (sql, "insert", 6) == 0 || strncasecmp (sql, "update", 6) == 0
      || strncasecmp (sql, "delete", 6) == 0)
    return SQL_DML;

  if (strncasecmp (sql, "create", 6) == 0 || strncasecmp (sql, "drop", 4) == 0
      || strncasecmp (sql, "alter", 5) == 0)
    return SQL_DDL;

  return SQL_UNKNOWN;
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

  char *db_name = scm_to_locale_string (dbh->constr);
  gdbi_sqlite3_ds_t *db_info = malloc (sizeof (gdbi_sqlite3_ds_t));
  if (db_info == NULL)
  {
    dbh->status = status_cons (1, "out of memory");
    return;
  }

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

void __sqlite3_params_query_g_db_handle(gdbi_db_handle_t *dbh,
                                        char *query,
                                        int argc,
                                        SCM *argv)
{
  gdbi_sqlite3_ds_t *p = (gdbi_sqlite3_ds_t *)dbh->db_info;
  sqlite3 *db = p->sqlite3_obj;
  sqlite3_stmt *stmt = NULL;
  int rc;

  if (p->stmt)
  {
    sqlite3_finalize(p->stmt);
    p->stmt = NULL;
  }

  /* ---------------- detect sql type ---------------- */
  sql_kind_t kind = detect_sql_type(query);

  /* ---------------- prepare ---------------- */
  rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    dbh->status = status_cons(1, sqlite3_errmsg(db));
    return;
  }

  /* ---------------- bind ---------------- */
  if (sqlite3_bind_parameter_count(stmt) != argc)
  {
    dbh->status = status_cons(1, "params count mismatch");
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
      rc = sqlite3_bind_text (stmt, idx, s, -1, SQLITE_TRANSIENT);
      free (s);
    }
    /* TODO: Add dbi specific null type, then bind it here. */
    /* else if (scm_is_false(v)) */
    /*   rc = sqlite3_bind_null(stmt, idx); */
    else
    {
      dbh->status = status_cons (1, "unsupported param type");
      goto error;
    }

    if (rc != SQLITE_OK)
    {
      dbh->status = status_cons (1, sqlite3_errmsg (db));
      goto error;
    }
  }

  /* =========================================================
     SELECT → cursor model (NO STEP)
     ========================================================= */
  if (kind == SQL_SELECT)
  {
    p->stmt = stmt;
    dbh->status = status_cons(0, "query ok");
    return;
  }

  /* =========================================================
     DML / DDL → execute immediately
     ========================================================= */
  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (db));
    sqlite3_finalize (stmt);
    p->stmt = NULL;
    return;
  }

  sqlite3_finalize (stmt);
  p->stmt = NULL;
  dbh->status = status_cons (0, "query ok");
  return;

error:
  if (stmt)
    sqlite3_finalize (stmt);
  p->stmt = NULL;
}

void __sqlite3_query_g_db_handle (gdbi_db_handle_t *dbh, char *query_str)
{
  gdbi_sqlite3_ds_t *db_info = dbh->db_info;
  sqlite3_stmt *stmt = NULL;

  if (!db_info)
  {
    dbh->status = status_cons (1, "invalid dbi connection");
    return;
  }

  if (db_info->stmt)
  {
    sqlite3_finalize (db_info->stmt);
    db_info->stmt = NULL;
  }

  int rc
    = sqlite3_prepare_v2 (db_info->sqlite3_obj, query_str, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    dbh->status = status_cons(1, sqlite3_errmsg(db_info->sqlite3_obj));
    return;
  }

  /* ---------------- EXECUTE ---------------- */
  rc = sqlite3_step(stmt);

  /* ================= DML ================= */
  if (rc == SQLITE_DONE)
  {
    sqlite3_finalize(stmt);
    db_info->stmt = NULL;

    dbh->status = status_cons(0, "query ok");
    return;
  }

  /* ================= SELECT ================= */
  if (rc == SQLITE_ROW)
  {
    db_info->stmt = stmt;
    dbh->status = status_cons(0, "query ok");
    return;
  }

  /* ================= ERROR ================= */
  dbh->status = status_cons(1, sqlite3_errmsg(db_info->sqlite3_obj));

  if (stmt)
    sqlite3_finalize(stmt);
}

SCM __sqlite3_getrow_g_db_handle (gdbi_db_handle_t *dbh)
{
  if (NULL == dbh || NULL == dbh->db_info)
  {
    dbh->status = status_cons (1, "invalid dbi connection");
    return SCM_BOOL_F;
  }

  gdbi_sqlite3_ds_t *p = (gdbi_sqlite3_ds_t *)dbh->db_info;
  sqlite3_stmt *stmt = p->stmt;

  if (NULL == stmt)
  {
    dbh->status = status_cons(1, "missing query result");
    return SCM_BOOL_F;
  }

  int rc = sqlite3_step(stmt);

  /* ================= END ================= */
  if (rc == SQLITE_DONE)
  {
    sqlite3_finalize(stmt);
    p->stmt = NULL;

    dbh->status = status_cons(0, "row end");
    return SCM_BOOL_F;
  }

  /* ================= ERROR ================= */
  if (rc != SQLITE_ROW)
  {
    dbh->status = status_cons (1, sqlite3_errmsg (p->sqlite3_obj));

    sqlite3_finalize (stmt);
    p->stmt = NULL;

    return SCM_BOOL_F;
  }

  /* ================= ROW ================= */
  SCM row = SCM_EOL;
  int ncols = sqlite3_column_count (stmt);

  for (int i = 0; i < ncols; i++)
  {
    SCM value;
    const char *fname = sqlite3_column_name (stmt, i);
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
      value
        = scm_from_locale_stringn ((const void *)sqlite3_column_blob (stmt, i),
                                   sqlite3_column_bytes (stmt, i));
      break;

    default:
      value = SCM_BOOL_F;
      break;
    }

    row = scm_append (scm_list_2 (
      row, scm_list_1 (scm_cons (scm_from_locale_string (fname), value))));
  }

  dbh->status = status_cons (0, "row fetched");
  return row;
}
