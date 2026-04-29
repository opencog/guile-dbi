/* guile-dbd-postgresql.c - Main source file
 * Copyright (C) 2004, 2005, 2006, 2008, 2026 Free Software Foundation, Inc.
 * Written by Maurizio Boriani <baux@member.fsf.org>
 * Maintained by Linas Vepstas <linasvepstas@gmail.com>
 *
 * This file is part of the guile-dbd-postgresql.
 *
 * The guile-dbd-postgresql is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The guile-dbd-postgresql is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * */

#include <errno.h>
#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>

/* Function prototypes and structures */
void __postgresql_make_g_db_handle (gdbi_db_handle_t *dbh);
void __postgresql_close_g_db_handle (gdbi_db_handle_t *dbh);
void __postgresql_query_g_db_handle (gdbi_db_handle_t *dbh, char *query);
void __postgresql_params_query_g_db_handle (gdbi_db_handle_t *g_db_handle,
                                            char *query, int argc, SCM *argv);
SCM __postgresql_getrow_g_db_handle (gdbi_db_handle_t *dbh);

typedef struct
{
  PGconn *pgsql;
  PGresult *res;
  int lget;
  int retn;
  int is_params; /* 1 = came from PQexecParams, 0 = came from PQexec */
} gdbi_pgsql_ds_t;

static void dbi_set_error (gdbi_db_handle_t *dbh, const char *msg)
{
  dbh->status = scm_cons (scm_from_int (1), scm_from_utf8_string (msg));
}

void __postgresql_make_g_db_handle (gdbi_db_handle_t *dbh)
{
  char sep = ':';
  int items = 0;
  gdbi_pgsql_ds_t *pgsqlP = NULL;
  SCM cp_list = SCM_EOL;

  if (scm_equal_p (scm_string_p (dbh->constr), SCM_BOOL_F) == SCM_BOOL_T)
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("missing connection string"));
    return;
  }

  cp_list = scm_string_split (dbh->constr, SCM_MAKE_CHAR (sep));

  items = scm_to_int (scm_length (cp_list));
  if (items >= 5 && items < 8)
  {
    char *port = NULL;
    char *user, *pass, *db, *ctyp, *loc;

    pgsqlP = (gdbi_pgsql_ds_t *)malloc (sizeof (gdbi_pgsql_ds_t));
    if (pgsqlP == NULL)
    {
      dbh->status = scm_cons (scm_from_int (errno),
                              scm_from_locale_string (strerror (errno)));
      return;
    }
    pgsqlP->retn = 0;

    user = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (0)));
    pass = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (1)));
    db = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (2)));
    ctyp = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (3)));
    loc = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (4)));

    pgsqlP->pgsql = NULL;
    pgsqlP->res = NULL;

    if (strcmp (ctyp, "tcp") == 0)
    {
      port = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (5)));
      pgsqlP->pgsql
        = (PGconn *)PQsetdbLogin (loc, port, NULL, NULL, db, user, pass);
      if (items == 7)
      {
        char *sretn
          = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (6)));
        pgsqlP->retn = atoi (sretn);
        free (sretn);
      }
    }
    else
    {
      pgsqlP->pgsql
        = (PGconn *)PQsetdbLogin (loc, NULL, NULL, NULL, db, user, pass);
      if (items == 6)
      {
        char *sretn
          = scm_to_locale_string (scm_list_ref (cp_list, scm_from_int (5)));
        pgsqlP->retn = atoi (sretn);
        free (sretn);
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
    if (port)
    {
      free (port);
    }

    if (PQstatus (pgsqlP->pgsql) == CONNECTION_BAD)
    {
      dbh->status
        = scm_cons (scm_from_int (1),
                    scm_from_locale_string (PQerrorMessage (pgsqlP->pgsql)));
      PQfinish (pgsqlP->pgsql);
      pgsqlP->pgsql = NULL;
      free (pgsqlP);
      dbh->db_info = NULL;
      dbh->closed = SCM_BOOL_T;
      return;
    }
    else
    {
      /* todo: error msg to be translated */
      dbh->status
        = scm_cons (scm_from_int (0), scm_from_utf8_string ("db connected"));
      dbh->db_info = pgsqlP;
      dbh->closed = SCM_BOOL_F;
      return;
    }
  }
  else
  {
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("invalid connection string"));
    dbh->db_info = NULL;
    dbh->closed = SCM_BOOL_T;
    return;
  }

  return;
}

void __postgresql_close_g_db_handle (gdbi_db_handle_t *dbh)
{
  gdbi_pgsql_ds_t *pgsqlP = (gdbi_pgsql_ds_t *)dbh->db_info;
  if (pgsqlP == NULL)
  {
    if (dbh->in_free)
      return; /* don't scm anything if in GC */
    /* todo: error msg to be translated */
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("dbd info not found"));
    return;
  }

  /* Don't scm anything if we are in garbage collection */
  if (0 == dbh->in_free)
  {
    if (NULL == pgsqlP->pgsql)
    {
      /* todo: error msg to be translated */
      dbh->status
        = scm_cons (scm_from_int (1),
                    scm_from_utf8_string ("dbi connection already closed"));
    }
    else
    {
      dbh->status
        = scm_cons (scm_from_int (0), scm_from_utf8_string ("dbi closed"));
    }
  }

  PQfinish (pgsqlP->pgsql);
  if (pgsqlP->res)
  {
    PQclear (pgsqlP->res);
    pgsqlP->res = NULL;
  }

  free (dbh->db_info);
  dbh->db_info = NULL;

  dbh->closed = SCM_BOOL_T;
}

void __postgresql_query_g_db_handle (gdbi_db_handle_t *dbh, char *query)
{
  gdbi_pgsql_ds_t *pgsqlP = NULL;

  if (dbh->db_info == NULL)
  {
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("invalid dbi connection"));
    return;
  }

  if (query == NULL)
  {
    dbh->status
      = scm_cons (scm_from_int (1), scm_from_utf8_string ("invalid dbi query"));
    return;
  }

  pgsqlP = (gdbi_pgsql_ds_t *)dbh->db_info;

  /* clear any previous result */
  if (pgsqlP->res)
  {
    PQclear (pgsqlP->res);
    pgsqlP->res = NULL;
  }

  pgsqlP->res = PQexec (pgsqlP->pgsql, query);
  pgsqlP->lget = 0;
  pgsqlP->is_params = 0;

  if (NULL == pgsqlP->res)
  {
    dbi_set_error (dbh, PQerrorMessage (pgsqlP->pgsql));
    return;
  }

  ExecStatusType st = PQresultStatus (pgsqlP->res);
  if (st == PGRES_COMMAND_OK || st == PGRES_TUPLES_OK)
  {
    dbh->status
      = scm_cons (scm_from_int (0), scm_from_utf8_string ("query ok"));
  }
  else
  {
    dbi_set_error (dbh, PQresultErrorMessage (pgsqlP->res));
    PQclear (pgsqlP->res);
    pgsqlP->res = NULL;
  }
}

static SCM getrow_for_params (gdbi_db_handle_t *dbh)
{
  gdbi_pgsql_ds_t *pgsqlP = (gdbi_pgsql_ds_t *)dbh->db_info;
  SCM retrow = SCM_EOL;
  int fnum, f;

  if (NULL == pgsqlP->res || pgsqlP->lget >= PQntuples (pgsqlP->res))
  {
    dbh->status = scm_cons (scm_from_int (0), scm_from_utf8_string ("row end"));
    return SCM_BOOL_F;
  }

  fnum = PQnfields (pgsqlP->res);

  for (f = 0; f < fnum; f++)
  {
    SCM value;
    const char *fname = PQfname (pgsqlP->res, f);
    Oid type = PQftype (pgsqlP->res, f);

    /* NULL — mirrors MySQL getrow_for_stmt is_null[f] check */
    if (PQgetisnull (pgsqlP->res, pgsqlP->lget, f))
    {
      value = SCM_UNSPECIFIED;
      retrow = scm_append (scm_list_2 (
        retrow, scm_list_1 (scm_cons (scm_from_locale_string (fname), value))));
      continue;
    }

    const char *vstr = PQgetvalue (pgsqlP->res, pgsqlP->lget, f);
    size_t vlen = PQgetlength (pgsqlP->res, pgsqlP->lget, f);

    switch (type)
    {
    /* bool → scm_from_bool, mirrors MYSQL_TYPE_TINY 1/0 */
    case 16:
      value = scm_from_bool (vstr[0] == 't');
      break;

    /* smallint → scm_from_int, mirrors MYSQL_TYPE_SHORT */
    case 21:
      value = scm_from_int (atoi (vstr));
      break;

    /* integer / regproc / oid → scm_from_int, mirrors MYSQL_TYPE_LONG */
    case 23:
    case 24:
    case 26:
      value = scm_from_int (atoi (vstr));
      break;

    /* bigint → scm_from_int64, mirrors MYSQL_TYPE_LONGLONG */
    case 20:
      value = scm_from_int64 (atoll (vstr));
      break;

    /* float4 → double, mirrors MYSQL_TYPE_FLOAT */
    case 700:
      value = scm_from_double (atof (vstr));
      break;

    /* float8 → double, mirrors MYSQL_TYPE_DOUBLE */
    case 701:
      value = scm_from_double (atof (vstr));
      break;

    /* numeric/decimal → string, mirrors MySQL stmt NEWDECIMAL as string */
    case 1700:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    /* money → string */
    case 790:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    /* integer arrays → SCM list, mirrors no MySQL equivalent but
       keep consistent with PQexec path above */
    case 1005: /* _int2 */
    case 1007: /* _int4 */
    case 1016: /* _int8 */
    {
      char *p = strdup (vstr + 1); /* skip '{' */
      char *end = strrchr (p, '}');
      if (end)
        *end = '\0';
      char *tok = strrchr (p, ',');
      value = SCM_EOL;
      while (tok)
      {
        value = scm_cons (scm_from_int64 (atoll (tok + 1)), value);
        *tok = '\0';
        tok = strrchr (p, ',');
      }
      value = scm_cons (scm_from_int64 (atoll (p)), value);
      free (p);
      break;
    }

    /* bytea → raw string by length, mirrors MySQL BLOB */
    case 17:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    /* text-like: char/name/text/bpchar/varchar →
       scm_from_locale_stringn, mirrors MySQL STRING/VAR_STRING */
    case 18:
    case 19:
    case 25:
    case 1042:
    case 1043:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    /* date/time/timestamp → string, mirrors MySQL stmt path which
       returns string (not strptime — that is only in MySQL legacy path) */
    case 702:
    case 1082:
    case 1083:
    case 1114:
    case 1184:
    case 1266:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    /* bit/varbit → string */
    case 1560:
    case 1562:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    /* uuid/json/jsonb → string */
    case 2950:
    case 114:
    case 3802:
      value = scm_from_locale_stringn (vstr, vlen);
      break;

    default:
    {
      char msg[101];
      snprintf (msg, 100, "unknown field type %d for %s", type, fname);
      dbh->status = scm_cons (scm_from_int (1), scm_from_utf8_string (msg));
      pgsqlP->lget++;
      return SCM_BOOL_F;
    }
    }

    retrow = scm_append (scm_list_2 (
      retrow, scm_list_1 (scm_cons (scm_from_locale_string (fname), value))));
  }

  pgsqlP->lget++;

  if (retrow == SCM_EOL)
  {
    dbh->status = scm_cons (scm_from_int (0), scm_from_utf8_string ("row end"));
    return SCM_BOOL_F;
  }

  dbh->status
    = scm_cons (scm_from_int (0), scm_from_utf8_string ("row fetched"));
  return retrow;
}

SCM __postgresql_getrow_g_db_handle (gdbi_db_handle_t *dbh)
{
  gdbi_pgsql_ds_t *pgsqlP = NULL;
  SCM retrow = SCM_EOL;
  int fnum, f;

  if (dbh->db_info == NULL)
  {
    dbh->status = scm_cons (scm_from_int (1),
                            scm_from_utf8_string ("invalid dbi connection"));
    return SCM_BOOL_F;
  }

  pgsqlP = (gdbi_pgsql_ds_t *)dbh->db_info;

  /* dispatch to params path */
  if (pgsqlP->is_params)
    return getrow_for_params (dbh);

  /* ---- PQexec path ---- */

  /* res is already populated by __postgresql_query_g_db_handle (PQexec).
     The old lazy PQgetResult call is removed — it was only needed for the
     old async PQsendQuery path which is now gone. */
  if (NULL == pgsqlP->res)
  {
    dbh->status = scm_cons (scm_from_int (0), scm_from_utf8_string ("row end"));
    return SCM_BOOL_F;
  }

  if (pgsqlP->lget >= PQntuples (pgsqlP->res))
  {
    pgsqlP->lget = 0;
    PQclear (pgsqlP->res);
    pgsqlP->res = NULL;
    dbh->status = scm_cons (scm_from_int (0), scm_from_utf8_string ("row end"));
    return SCM_BOOL_F;
  }

  switch (PQresultStatus (pgsqlP->res))
  {
  case PGRES_BAD_RESPONSE:
  case PGRES_NONFATAL_ERROR:
  case PGRES_FATAL_ERROR:
    dbh->status = scm_cons (
      scm_from_int (1),
      scm_from_locale_string (PQresStatus (PQresultStatus (pgsqlP->res))));
    return SCM_BOOL_F;

  case PGRES_EMPTY_QUERY:
  case PGRES_COMMAND_OK:
  case PGRES_TUPLES_OK:
  case PGRES_COPY_OUT:
  case PGRES_COPY_IN:
    dbh->status = scm_cons (
      scm_from_int (0),
      scm_from_locale_string (PQresStatus (PQresultStatus (pgsqlP->res))));
    break;

  default:
    dbh->status = scm_cons (
      scm_from_int (0), scm_from_utf8_string ("unknown return query status"));
    break;
  }

  fnum = PQnfields (pgsqlP->res);

  for (f = 0; f < fnum; f++)
  {
    SCM value;
    Oid type = PQftype (pgsqlP->res, f);
    const char *fname = PQfname (pgsqlP->res, f);

    if (PQgetisnull (pgsqlP->res, pgsqlP->lget, f))
    {
      value = SCM_UNSPECIFIED;
      retrow = scm_append (scm_list_2 (
        retrow, scm_list_1 (scm_cons (scm_from_locale_string (fname), value))));
      continue;
    }

    const char *vstr = PQgetvalue (pgsqlP->res, pgsqlP->lget, f);

    switch (type)
    {
    case 16:
      value = scm_from_bool (vstr[0] == 't');
      break;

    case 20:
      value = scm_from_int64 (atoll (vstr));
      break;

    case 21:
    case 23:
    case 24:
    case 26:
      value = scm_from_int (atoi (vstr));
      break;

    case 700:
    case 701:
      value = scm_c_locale_stringn_to_number (vstr, strlen (vstr), 10);
      if (scm_is_false (value))
        value = scm_from_double (0.0);
      break;

    case 790:
    case 1700:
      value = scm_from_locale_string (vstr);
      break;

    case 1005:
    case 1007:
    case 1016:
    {
      char *p = strdup (vstr + 1);
      char *end = strrchr (p, '}');
      if (end)
        *end = '\0';
      char *tok = strrchr (p, ',');
      value = SCM_EOL;
      while (tok)
      {
        value = scm_cons (scm_from_int64 (atoll (tok + 1)), value);
        *tok = '\0';
        tok = strrchr (p, ',');
      }
      value = scm_cons (scm_from_int64 (atoll (p)), value);
      free (p);
      break;
    }

    case 17:
    {
      size_t len = PQgetlength (pgsqlP->res, pgsqlP->lget, f);
      value = scm_from_locale_stringn (vstr, len);
      break;
    }

    case 18:
    case 19:
    case 25:
    case 1042:
    case 1043:
      value = scm_from_locale_string (vstr);
      break;

    case 702:
    case 1082:
    case 1083:
    case 1114:
    case 1184:
    case 1266:
    case 1560:
    case 1562:
      value = scm_from_locale_string (vstr);
      break;

    case 2950:
    case 114:
    case 3802:
      value = scm_from_locale_string (vstr);
      break;

    default:
    {
      char msg[101];
      snprintf (msg, 100, "unknown field type %d for %s", type, fname);
      dbh->status = scm_cons (scm_from_int (1), scm_from_utf8_string (msg));
      pgsqlP->lget++;
      return SCM_BOOL_F;
    }
    }

    retrow = scm_append (scm_list_2 (
      retrow, scm_list_1 (scm_cons (scm_from_locale_string (fname), value))));
  }

  pgsqlP->lget++;

  if (retrow == SCM_EOL)
  {
    dbh->status = scm_cons (scm_from_int (0), scm_from_utf8_string ("row end"));
    return SCM_BOOL_F;
  }

  dbh->status
    = scm_cons (scm_from_int (0), scm_from_utf8_string ("row fetched"));
  return retrow;
}

void __postgresql_params_query_g_db_handle (gdbi_db_handle_t *g_db_handle,
                                            char *query, int argc, SCM *argv)
{
  gdbi_pgsql_ds_t *pgsqlP = (gdbi_pgsql_ds_t *)g_db_handle->db_info;

  char *pg_sql = query;
  const char **values = NULL;
  int *lengths = NULL;
  int *formats = NULL;
  PGresult *res = NULL;
  char errbuf[1024] = {0};

  values = calloc (argc, sizeof (char *));
  lengths = calloc (argc, sizeof (int));
  formats = calloc (argc, sizeof (int));

  if (NULL == pg_sql || NULL == values || NULL == lengths || NULL == formats)
  {
    snprintf (errbuf, sizeof (errbuf),
              "PostgreSQL parameterized query failed: out of memory");
    dbi_set_error (g_db_handle, errbuf);
    goto cleanup;
  }

  for (int i = 0; i < argc; i++)
  {
    SCM v = argv[i];
    char *s = NULL;

    if (scm_is_null (v))
    {
      s = NULL; /* SQL NULL */
    }
    else if (scm_is_string (v))
    {
      s = scm_to_locale_string (v);
    }
    else if (scm_is_number (v))
    {
      SCM str = scm_number_to_string (v, scm_from_int (10));
      s = scm_to_locale_string (str);
    }
    else if (scm_is_bool (v))
    {
      s = scm_is_true (v) ? strdup ("true") : strdup ("false");
    }
    else
    {
      snprintf (errbuf, sizeof (errbuf),
                "PostgreSQL parameterized query failed: unsupported parameter");
      dbi_set_error (g_db_handle, errbuf);
      goto cleanup;
    }

    values[i] = s;
    lengths[i] = s ? (int)strlen (s) : 0;
    formats[i] = 0; /* text */
  }

  res = PQexecParams (pgsqlP->pgsql, pg_sql, argc,
                      NULL, /* let PostgreSQL infer types */
                      values, lengths, formats, 0); /* text results */

cleanup:
  for (int i = 0; i < argc; i++)
    if (values[i])
      free ((char *)values[i]);

  free (values);
  free (lengths);
  free (formats);

  if (NULL == res)
  {
    snprintf (errbuf, sizeof (errbuf), "PostgreSQL execution failed: %s",
              PQerrorMessage (pgsqlP->pgsql));
    dbi_set_error (g_db_handle, errbuf);
    pgsqlP->is_params = 0;
    pgsqlP->res = NULL;
    return;
  }

  ExecStatusType st = PQresultStatus (res);
  if (st == PGRES_COMMAND_OK || st == PGRES_TUPLES_OK)
  {
    g_db_handle->status
      = scm_cons (scm_from_int (0), scm_from_utf8_string ("query ok"));
    pgsqlP->lget = 0;
    pgsqlP->is_params = 1;
    /* clear previous result before assigning new one */
    if (pgsqlP->res)
    {
      PQclear (pgsqlP->res);
    }
    pgsqlP->res = res;
  }
  else
  {
    dbi_set_error (g_db_handle, PQresultErrorMessage (res));
    PQclear (res);
    pgsqlP->res = NULL;
    pgsqlP->lget = 0;
    pgsqlP->is_params = 0;
  }
}
