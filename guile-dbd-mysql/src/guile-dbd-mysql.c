/* guile-dbd-mysql.c - Main source file
 * Copyright (C) 2004, 2005, 2006, 2008, 2026 Free Software Foundation, Inc.
 * Written by Maurizio Boriani <baux@member.fsf.org>
 * Maintained by Linas Vepstas <linasvepstas@gmail.com>
 *
 * This file is part of the guile-dbd-mysql.
 *
 * The guile-dbd-mysql is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The guile-dbd-mysql is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "config.h"
#include <errno.h>
#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <mariadb/errmsg.h>
#include <mariadb/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This function doesn't exist on Solaris 10u9 */
#ifndef HAVE_STRNDUP
static char *strndup (const char *s, size_t n)
{
  char *p = malloc (n + 1);
  strncpy (p, s, n);
  p[n] = 0;
  return p;
}
#endif

/* functions prototypes and structures */
void __mysql_make_g_db_handle (gdbi_db_handle_t *dbh);
void __mysql_close_g_db_handle (gdbi_db_handle_t *dbh);
void __mysql_query_g_db_handle (gdbi_db_handle_t *dbh, char *query);
void __mysql_params_query_g_db_handle (gdbi_db_handle_t *dbh, const char *query,
                                       int argc, const SCM *argv);
SCM __mysql_getrow_g_db_handle (gdbi_db_handle_t *dbh);

typedef struct
{
  MYSQL *mysql;

  /* legacy path */
  MYSQL_RES *res;

  /* params-query path */
  MYSQL_RES *meta;
  MYSQL_STMT *stmt;
  MYSQL_BIND *result_bind;
  unsigned long *lengths;
  my_bool *is_null;
  unsigned int num_fields;

  int retn;
} gdbi_mysql_ds_t;

void __mysql_make_g_db_handle (gdbi_db_handle_t *dbh)
{
  char sep = ':';
  int items = 0;
  gdbi_mysql_ds_t *mysqlP = NULL;
  SCM cp_list = SCM_EOL;

  if (scm_equal_p (scm_string_p (dbh->constr), SCM_BOOL_F) == SCM_BOOL_T)
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (
      scm_from_int (1), scm_from_locale_string ("missing connection string"));
    return;
  }

  cp_list = scm_string_split (dbh->constr, SCM_MAKE_CHAR (sep));

  dbh->closed = SCM_BOOL_T;

  items = scm_to_int (scm_length (cp_list));
  if (items >= 5 && items < 8)
  {
    void *ret = 0;
    int port = 0;
    char *user
      = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (0)));
    char *pass
      = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (1)));
    char *db = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (2)));
    char *ctyp
      = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (3)));
    char *loc = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (4)));

    mysqlP = (gdbi_mysql_ds_t *)calloc (sizeof (gdbi_mysql_ds_t), 1);

    if (mysqlP == NULL)
    {
      dbh->status = scm_cons (scm_from_int (errno),
                              scm_from_locale_string (strerror (errno)));
      return;
    }

    mysqlP->mysql = mysql_init (NULL);
    mysqlP->res = NULL;

    if (strcmp (ctyp, "tcp") == 0)
    {
      char *sport
        = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (5)));
      port = atoi (sport);
      ret = mysql_real_connect (mysqlP->mysql, loc, user, pass, db, port, NULL,
                                0);
      if (items == 7)
      {
        char *sretn
          = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (6)));
        mysqlP->retn = atoi (sretn);
      }
    }
    else
    {
      ret = mysql_real_connect (mysqlP->mysql, NULL, user, pass, db, port, loc,
                                0);
      if (items == 6)
      {
        char *sretn
          = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (5)));
        mysqlP->retn = atoi (sretn);
      }
    }

    /* free resources */
    if (user)
    {
      free (user);
    }
    if (pass)
    {
      free (pass);
    }
    if (db)
    {
      free (db);
    }
    if (ctyp)
    {
      free (ctyp);
    }
    if (loc)
    {
      free (loc);
    }

    if (ret == 0)
    {
      dbh->status = scm_cons (
        scm_from_int (1), scm_from_locale_string (mysql_error (mysqlP->mysql)));
      mysql_close (mysqlP->mysql);
      mysqlP->mysql = NULL;
      free (mysqlP);
      dbh->db_info = NULL;
      return;
    }
    else
    {
      /* todo: error msg to be translated */
      dbh->status
        = scm_cons (scm_from_int (0), scm_from_locale_string ("db connected"));
      dbh->db_info = mysqlP;
      dbh->closed = SCM_BOOL_F;
      return;
    }
  }
  else
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (
      scm_from_int (1), scm_from_locale_string ("invalid connection string"));
    dbh->db_info = NULL;
    return;
  }

  return;
}

void __mysql_close_g_db_handle (gdbi_db_handle_t *dbh)
{
  gdbi_mysql_ds_t *mysqlP = (gdbi_mysql_ds_t *)dbh->db_info;
  if (mysqlP == NULL)
  {
    if (dbh->in_free)
      return; /* don't scm anything if in GC */
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_locale_string ("dbd info not found"));
    return;
  }
  else if (mysqlP->mysql == NULL)
  {
    if (0 == dbh->in_free)
    {
      /* todo: error msg to be translated */
      dbh->status
        = scm_cons (scm_from_int (1),
                    scm_from_locale_string ("dbi connection already closed"));
    }
    free (dbh->db_info);
    dbh->db_info = NULL;
    return;
  }

  mysql_close (mysqlP->mysql);
  if (mysqlP->res)
  {
    mysql_free_result (mysqlP->res);
    mysqlP->res = NULL;
  }

  free (dbh->db_info);
  dbh->db_info = NULL;

  /* todo: error msg to be translated */
  dbh->closed = SCM_BOOL_T;

  if (dbh->in_free)
    return; /* don't scm anything if in GC */
  dbh->status
    = scm_cons (scm_from_int (0), scm_from_locale_string ("dbi closed"));
  return;
}

void __mysql_query_g_db_handle (gdbi_db_handle_t *dbh, char *query)
{
  gdbi_mysql_ds_t *mysqlP = NULL;
  int err, i;

  if (dbh->db_info == NULL)
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_locale_string ("invalid dbi connection"));
    return;
  }

  mysqlP = (gdbi_mysql_ds_t *)dbh->db_info;

  if (mysqlP->res)
  {
    mysql_free_result (mysqlP->res);
    mysqlP->res = NULL;
  }

  err = mysql_real_query (mysqlP->mysql, query, strlen (query));
  for (i = 0; i < mysqlP->retn && err != 0; i++)
  {
    mysql_ping (mysqlP->mysql);
    err = mysql_real_query (mysqlP->mysql, query, strlen (query));
  }

  if (err)
  {
    dbh->status
      = scm_cons (scm_from_int (mysqlP->mysql->net.last_errno),
                  scm_from_locale_string (mysql_error (mysqlP->mysql)));
    return;
  }

  mysqlP->res = mysql_use_result (mysqlP->mysql);
  if (mysqlP->res == NULL)
  {
    dbh->status = scm_cons (scm_from_int (0),
                            scm_from_locale_string ("query ok, no results"));
    return;
  }

  dbh->status = scm_cons (scm_from_int (0),
                          scm_from_locale_string ("query ok, got results"));

  return;
}

static void free_binds (MYSQL_BIND *binds, int cnt)
{
  int i;

  if (binds)
  {
    for (i = 0; i < cnt; i++)
    {
      if (binds[i].buffer)
      {
        free (binds[i].buffer);
      }
    }

    free (binds);
    binds = NULL;
  }
}

SCM static getrow_for_stmt (gdbi_db_handle_t *dbh)
{
  gdbi_mysql_ds_t *mysqlP = (gdbi_mysql_ds_t *)dbh->db_info;
  SCM retrow = SCM_EOL;
  int f;

  MYSQL_FIELD *fields = mysql_fetch_fields (mysqlP->meta);
  mysql_stmt_fetch (mysqlP->stmt);

  for (f = 0; f < mysqlP->num_fields; f++)
  {
    MYSQL_BIND *b = &mysqlP->result_bind[f];
    SCM value;

    /* 1. SQL NULL */
    if (mysqlP->is_null[f])
    {
      value = SCM_UNSPECIFIED;
    }
    else
    {
      switch (b->buffer_type)
      {
      case MYSQL_TYPE_LONG:
        value = scm_from_int (*(int *)b->buffer);
        break;

      case MYSQL_TYPE_LONGLONG:
        value = scm_from_int64 (*(long long *)b->buffer);
        break;

      case MYSQL_TYPE_DOUBLE:
        value = scm_from_double (*(double *)b->buffer);
        break;

      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
        value = scm_from_locale_stringn ((char *)b->buffer, *b->length);
        break;

      default:
        value = SCM_UNSPECIFIED;
        break;
      }
    }

    /* 3. (column-name . value) */
    retrow = scm_append (scm_list_2 (
      retrow,
      scm_list_1 (scm_cons (scm_from_locale_string (fields[f].name), value))));
  }

  dbh->status
    = scm_cons (scm_from_int (0), scm_from_locale_string ("row fetched"));

  free (mysqlP->is_null);
  mysqlP->is_null = NULL;

  free (mysqlP->lengths);
  mysqlP->lengths = NULL;

  free_binds (mysqlP->result_bind, mysqlP->num_fields);

  mysql_free_result (mysqlP->meta);
  mysqlP->meta = NULL;

  free (mysqlP->stmt);
  mysqlP->stmt = NULL;

  free (mysqlP->res);
  mysqlP->res = NULL;

  mysqlP->retn = 0;
  mysql_stmt_close (mysqlP->stmt);

  return retrow;
}

SCM getrow_common (gdbi_db_handle_t *dbh)
{
  gdbi_mysql_ds_t *mysqlP = NULL;
  SCM retrow = SCM_EOL;
  long fnum = 0;
  long f = 0;
  unsigned long *les;
  MYSQL_ROW row;
  MYSQL_FIELD *fields;

  if (dbh->db_info == NULL)
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_locale_string ("invalid dbi connection"));
    return (SCM_BOOL_F);
  }

  mysqlP = (gdbi_mysql_ds_t *)dbh->db_info;

  if (!mysqlP->res)
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_locale_string ("missing query result"));
    return (SCM_BOOL_F);
  }

  if ((row = mysql_fetch_row (mysqlP->res)) == NULL)
  {
    /* todo: error msg to be translated */
    dbh->status
      = scm_cons (scm_from_int (0), scm_from_locale_string ("row end"));
    return (SCM_BOOL_F);
  }
  fnum = mysql_num_fields (mysqlP->res);
  fields = mysql_fetch_fields (mysqlP->res);
  les = mysql_fetch_lengths (mysqlP->res);
  for (f = 0; f < fnum; f++)
  {

    SCM value;
    char *value_str = NULL;

    switch (fields[f].type)
    {
    case FIELD_TYPE_NULL:
      value = SCM_BOOL_F;
      break;
    case FIELD_TYPE_TINY:
    case FIELD_TYPE_SHORT:
    case FIELD_TYPE_INT24:
    case FIELD_TYPE_LONG:
    case FIELD_TYPE_DECIMAL:
      value_str = strndup (row[f], les[f]);
      value = scm_from_long (atoi (value_str));
      break;
    case FIELD_TYPE_LONGLONG:
      value_str = strndup (row[f], les[f]);
      value = scm_from_long_long (atoll (value_str));
      break;
    case FIELD_TYPE_FLOAT:
    case FIELD_TYPE_DOUBLE:
    case FIELD_TYPE_NEWDECIMAL:
      value_str = strndup (row[f], les[f]);
      value = scm_from_double (atof (value_str));
      break;
    case FIELD_TYPE_STRING:
    case FIELD_TYPE_ENUM:
    case FIELD_TYPE_VAR_STRING:
    case FIELD_TYPE_SET:
    case FIELD_TYPE_BLOB:
    case FIELD_TYPE_DATE:
    case FIELD_TYPE_TIME:
    case FIELD_TYPE_YEAR:
      value_str = strndup (row[f], les[f]);
      value = scm_from_locale_string (value_str);
      break;
    case FIELD_TYPE_DATETIME:
    case FIELD_TYPE_TIMESTAMP:
      /* (car (mktime (car (strptime "%Y-%m-%d %H:%M:%S" "2013-03-20
         23:05:44")))) scm_mktime needs to use the default time zone because
         MySQL stores times in UTC.  */
      value_str = strndup (row[f], les[f]);
      value = scm_from_locale_string (value_str);
      value
        = scm_strptime (scm_from_locale_string ("%Y-%m-%d %H:%M:%S"), value);
      value = SCM_CAR (value);
      value = scm_mktime (value, scm_from_locale_string ("GMT+0"));
      value = SCM_CAR (value);
      break;
    default:
      /* todo: error msg to be translated */
      dbh->status = scm_cons (scm_from_int (1),
                              scm_from_locale_string ("unknown field type"));
      return SCM_EOL;
      break;
    }
    retrow = scm_append (scm_list_2 (
      retrow,
      scm_list_1 (scm_cons (scm_from_locale_string (fields[f].name), value))));
    if (value_str != NULL)
    {
      free (value_str);
    }
  }
  /* todo: error msg to be translated */
  dbh->status
    = scm_cons (scm_from_int (0), scm_from_locale_string ("row fetched"));
  return (retrow);
}

SCM __mysql_getrow_g_db_handle (gdbi_db_handle_t *dbh)
{
  gdbi_mysql_ds_t *mysqlP = (gdbi_mysql_ds_t *)dbh->db_info;

  if (mysqlP->stmt)
    return getrow_for_stmt (dbh);
  else
    return getrow_common (dbh);
}

#define MAX_TMPVAR 128
void __mysql_params_query_g_db_handle (gdbi_db_handle_t *dbh, const char *query,
                                       int argc, const SCM *argv)
{
  gdbi_mysql_ds_t *mysqlP = (gdbi_mysql_ds_t *)dbh->db_info;
  int i = 0;

  if (MAX_TMPVAR < argc)
  {
    char buf[100] = {0};
    snprintf (buf, sizeof (buf), "Exceeds %d limit, too many parameters",
              MAX_TMPVAR);
    dbh->status = scm_cons (scm_from_int (1), scm_from_utf8_string (buf));
    return;
  }

  /* cleanup old state */
  if (mysqlP->res)
  {
    mysql_free_result (mysqlP->res);
    mysqlP->res = NULL;
  }

  if (mysqlP->meta)
  {
    mysql_free_result (mysqlP->meta);
    mysqlP->meta = NULL;
  }

  if (mysqlP->stmt)
  {
    mysql_stmt_close (mysqlP->stmt);
    mysqlP->stmt = NULL;
  }

  if (mysqlP->result_bind)
  {
    free_binds (mysqlP->result_bind, argc);
  }

  if (mysqlP->is_null)
  {
    free (mysqlP->is_null);
    mysqlP->is_null = NULL;
  }

  if (mysqlP->lengths)
  {
    free (mysqlP->lengths);
    mysqlP->lengths = NULL;
  }

  mysqlP->stmt = mysql_stmt_init (mysqlP->mysql);
  if (NULL == mysqlP->stmt)
  {
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("mysql stmt init failed"));
    return;
  }

  if (mysql_stmt_prepare (mysqlP->stmt, query, strlen (query)) != 0)
  {
    dbh->status = scm_cons (
      scm_from_int (1), scm_from_utf8_string (mysql_stmt_error (mysqlP->stmt)));
    goto cleanup_stmt;
  }

  if ((int)mysql_stmt_param_count (mysqlP->stmt) != argc)
  {
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("params count mismatch"));
    goto cleanup_stmt;
  }

  /* -------- bind params -------- */
  MYSQL_BIND *params_bind = calloc (argc, sizeof (MYSQL_BIND));
  if (!params_bind)
  {
    dbh->status = scm_cons (
      scm_from_int (1), scm_from_utf8_string ("out of memory for params_bind"));
    goto cleanup_stmt;
  }

  /* temp array to avoid malloc */
  long long nums[MAX_TMPVAR] = {0};
  /* free str easily after use */
  char *str[MAX_TMPVAR] = {0};

  for (i = 0; i < argc; i++)
  {
    if (scm_is_integer (argv[i]) || scm_is_bool (argv[i]))
    {
      nums[i] = scm_is_bool (argv[i]) ? (scm_is_true (argv[i]) ? 1 : 0)
                                      : scm_to_long_long (argv[i]);
      params_bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
      params_bind[i].buffer = &nums[i];
    }
    else if (scm_is_string (argv[i]))
    {
      char *s = scm_to_locale_string (argv[i]);
      if (NULL == s)
      {
        dbh->status
          = scm_cons (scm_from_int (1),
                      scm_from_utf8_string ("out of memory for string param"));
        goto cleanup_params;
      }
      str[i] = s; /* save for later free */
      params_bind[i].buffer_type = MYSQL_TYPE_STRING;
      params_bind[i].buffer = s;
      params_bind[i].buffer_length = strlen (s);
    }
    else if (scm_is_null (argv[i]))
    {
      params_bind[i].buffer_type = MYSQL_TYPE_NULL;
    }
    else
    {
      dbh->status = scm_cons (
        scm_from_int (1), scm_from_utf8_string ("unsupported parameter type"));
      goto cleanup_params;
    }
  }

  if (mysql_stmt_bind_param (mysqlP->stmt, params_bind) != 0)
  {
    dbh->status = scm_cons (
      scm_from_int (1), scm_from_utf8_string (mysql_stmt_error (mysqlP->stmt)));
    goto cleanup_params;
  }

  if (mysql_stmt_execute (mysqlP->stmt) != 0)
  {
    dbh->status = scm_cons (
      scm_from_int (1), scm_from_utf8_string (mysql_stmt_error (mysqlP->stmt)));
    goto cleanup_params;
  }

  /* -------- prepare result fetching -------- */

  mysqlP->num_fields = mysql_stmt_field_count (mysqlP->stmt);

  if (mysqlP->num_fields > 0)
  {
    mysqlP->result_bind = calloc (mysqlP->num_fields, sizeof (MYSQL_BIND));
    mysqlP->lengths = calloc (mysqlP->num_fields, sizeof (unsigned long));
    mysqlP->is_null = calloc (mysqlP->num_fields, sizeof (my_bool));

    if (NULL == mysqlP->result_bind || NULL == mysqlP->lengths
        || NULL == mysqlP->is_null)
    {
      dbh->status
        = scm_cons (scm_from_int (1),
                    scm_from_utf8_string ("result_bind start: out of memory"));
      free_binds (mysqlP->result_bind, 0);
      goto cleanup_stmt;
    }

    mysqlP->meta = mysql_stmt_result_metadata (mysqlP->stmt);
    if (NULL == mysqlP->meta)
    {
      dbh->status = scm_cons (scm_from_int (1),
                              scm_from_utf8_string ("result metadata error"));
      goto cleanup_stmt;
    }

    MYSQL_FIELD *fields = mysql_fetch_fields (mysqlP->meta);
    if (NULL == fields)
    {
      dbh->status = scm_cons (scm_from_int (1),
                              scm_from_utf8_string ("fetch fields error"));
      goto cleanup_meta;
    }

    for (i = 0; i < mysqlP->num_fields; i++)
    {
      MYSQL_FIELD *fld = &fields[i];
      MYSQL_BIND *b = &mysqlP->result_bind[i];

      b->is_null = &mysqlP->is_null[i];
      b->length = &mysqlP->lengths[i];

      switch (fld->type)
      {
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_INT24:
        b->buffer_type = MYSQL_TYPE_LONG;
        b->buffer = calloc (sizeof (long), 1);
        b->buffer_length = sizeof (long);
        break;

      case MYSQL_TYPE_LONGLONG:
        b->buffer = calloc (sizeof (long long), 1);
        b->buffer_type = MYSQL_TYPE_LONGLONG;
        b->buffer_length = sizeof (long long);
        break;

      case MYSQL_TYPE_FLOAT:
      case MYSQL_TYPE_DOUBLE:
        b->buffer = calloc (sizeof (double), 1);
        b->buffer_type = MYSQL_TYPE_DOUBLE;
        b->buffer_length = sizeof (double);
        break;

      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
      default:
        b->buffer_type = MYSQL_TYPE_STRING;
        b->buffer_length = params_bind[i].buffer_length + 1;
        b->buffer = calloc (b->buffer_length, 1);
        break;
      }

      if (NULL == b->buffer)
      {
        dbh->status
          = scm_cons (scm_from_int (1),
                      scm_from_utf8_string ("result bind: out of memory"));
        goto cleanup_result_bind;
      }
    }

    if (mysql_stmt_bind_result (mysqlP->stmt, mysqlP->result_bind) != 0)
    {
      dbh->status = scm_cons (scm_from_int (1),
                              scm_from_utf8_string ("result bind mismatch"));
      goto cleanup_stmt;
    }
  }

  mysqlP->retn = mysql_stmt_affected_rows (mysqlP->stmt);
  dbh->status = scm_cons (scm_from_int (0), scm_from_utf8_string ("query ok"));

cleanup_params:
  i = 0;
  while (str[i])
  {
    free (str[i]);
    i++;
  }
  free (params_bind);
  params_bind = NULL;

  return;

cleanup_result_bind:
  free_binds (mysqlP->result_bind, mysqlP->num_fields);
  mysqlP->result_bind = NULL;
  free (mysqlP->is_null);
  mysqlP->is_null = NULL;
  free (mysqlP->lengths);
  mysqlP->lengths = NULL;

cleanup_meta:
  if (mysqlP->meta)
  {
    mysql_free_result (mysqlP->meta);
    mysqlP->meta = NULL;
  }

close_stmt:
  if (mysqlP->stmt)
  {
    mysql_stmt_close (mysqlP->stmt);
  }

cleanup_stmt:
  free (mysqlP->stmt);
  mysqlP->stmt = NULL;

  mysqlP->retn = 0;

  return;
}
