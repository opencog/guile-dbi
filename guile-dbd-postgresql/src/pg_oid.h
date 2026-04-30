/* guile-dbd-postgresql.c - Main source file
 * Copyright (C) 2026 Free Software Foundation, Inc.
 * Written by NalaGinrut <mulei@gnu.org>
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

/* pg_oid.h — PostgreSQL OID constants for guile-dbd-postgresql
 *
 * catalog/pg_type.h cannot be included from client code: it is a
 * server-internal header that requires postgres.h for uint32/uint16 etc.
 * These values are copied verbatim from PostgreSQL's
 *   src/include/catalog/pg_type.dat
 * and have been part of the stable wire protocol since PostgreSQL 7.4.
 * They are safe to define here for any supported server version.
 *
 * Wrapped in #ifndef BOOLOID so the block is silently skipped if a future
 * libpq release ever exports these defines from a client-safe header.
 */

#ifndef PG_OID_H
#define PG_OID_H

#ifndef BOOLOID

/* ── numeric ──────────────────────────────────────────────────── */
#define BOOLOID          16   /* boolean                          */
#define BYTEAOID         17   /* bytea                            */
#define CHAROID          18   /* "char" (internal single-byte)    */
#define NAMEOID          19   /* name (internal 64-byte string)   */
#define INT8OID          20   /* bigint                           */
#define INT2OID          21   /* smallint                         */
#define INT4OID          23   /* integer                          */
#define TEXTOID          25   /* text                             */
#define JSONOID         114   /* json                             */
#define FLOAT4OID       700   /* real                             */
#define FLOAT8OID       701   /* double precision                 */
#define NUMERICOID     1700   /* numeric / decimal                */

/* ── money (no pg_type.h symbol) ─────────────────────────────── */
#define MONEYOID        790   /* money                            */

/* ── character ────────────────────────────────────────────────── */
#define BPCHAROID      1042   /* char(n) blank-padded             */
#define VARCHAROID     1043   /* varchar(n)                       */

/* ── date / time ─────────────────────────────────────────────── */
#define DATEOID        1082   /* date                             */
#define TIMEOID        1083   /* time without time zone           */
#define TIMESTAMPOID   1114   /* timestamp without time zone      */
#define TIMESTAMPTZOID 1184   /* timestamp with time zone         */

/* ── misc ─────────────────────────────────────────────────────── */
#define UUIDOID        2950   /* uuid                             */
#define JSONBOID       3802   /* jsonb                            */

/* ── integer array types ──────────────────────────────────────── */
#define INT2ARRAYOID   1005   /* smallint[]                       */
#define INT4ARRAYOID   1007   /* integer[]                        */
#define INT8ARRAYOID   1016   /* bigint[]                         */

#endif /* BOOLOID */

#endif /* PG_OID_H */
