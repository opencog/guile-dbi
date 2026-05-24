;;; Copyright (C) 2026 Free Software Foundation, Inc.
;;; NalaGinrut <mulei@gnu.org>
;;;
;;; This file is part of the guile-dbi.
;;;
;;; The guile-dbi is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; The guile-dbi is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;;; test-postgresql-params.scm
;;;
;;; Test suite for the parameterized-query path (PQexecParams / is_params /
;;; getrow_for_params) introduced in guile-dbd-postgresql.
;;;
;;; Coverage goals
;;;   1. Basic parameterized SELECT → dbi-get_row dispatches via is_params
;;;   2. Type fidelity for every OID branch in getrow_for_params
;;;   3. NULL values returned as SCM unspecified
;;;   4. Integer arrays: normal, single-element, empty '{}'
;;;   5. PQexec path still works after a params query (is_params reset)
;;;   6. Error path: bad params count, then re-query succeeds
;;;
;;; Prerequisites
;;;   - PostgreSQL reachable at $PGHOST/$PGPORT (defaults: localhost/5432)
;;;   - Env vars PGUSER, PGPASSWORD, PGDATABASE set, or edit *db-conn-str* below
;;;   - guile-dbi and guile-dbd-postgresql installed
;;;
;;; Run
;;;   guile test-postgresql-params.scm
;;;
;;; Exit code: 0 on success, non-zero on any failure.

(use-modules (dbi dbi)
             (ice-9 format)
             (ice-9 match))

;;; ── connection string ────────────────────────────────────────────────────────
;;; Test prepare:
;;;
;;; sudo nano /etc/postgresql/15/main/pg_hba.conf
;;; # add line like this to allow local passwordless auth for "test" user:
;;; # local   all            test                                trust
;;; sudo systemctl restart postgresql
;;; # Change ownership of test database to "test" user
;;; sudo -u postgres psql -c "CREATE DATABASE test OWNER test;"

;;; Format: "user:pass:db:socket-or-tcp:host[:port]"
(define *db-conn-str*
  (let ((user (or (getenv "DBI_TEST_PGUSER")     "test"))
        (pass (or (getenv "DBI_TEST_PGPASS") ""))
        (db   (or (getenv "DBI_TEST_PGDB") "test"))
        (host (or (getenv "DBI_TEST_PGHOST")     "localhost"))
        (port (or (getenv "DBI_TEST_PGPORT")     "5432")))
    (format #f "~a:~a:~a:tcp:~a:~a" user pass db host port)))

;;; ── tiny test framework ──────────────────────────────────────────────────────
(define *pass* 0)(define *fail* 0)

(define (check! label got expected)
  (if (equal? got expected)
      (begin
        (set! *pass* (+ *pass* 1))
        (format #t "  PASS  ~a\n" label))
      (begin
        (set! *fail* (+ *fail* 1))
        (format #t "  FAIL  ~a\n         expected: ~s\n         got:      ~s\n"
                label expected got))))

(define (check-true! label val)
  (check! label (if val #t #f) #t))

(define (check-unspecified! label val)
  (check-true! label (eq? val (if #f #f))))   ; SCM_UNSPECIFIED == (if #f #f)

;;; ── helpers ──────────────────────────────────────────────────────────────────
(define (exec! db sql)
  (dbi-query db sql)
  (unless (= 0 (car (dbi-get_status db)))
    (error "query failed" sql (dbi-get_status db))))

(define (params-exec! db sql . params)
  (dbi-params-query db sql params)
  (unless (= 0 (car (dbi-get_status db)))
    (error "params-query failed" sql (dbi-get_status db))))

(define (fetch-all db)
  "Return list of all remaining rows."
  (let lp ((rows '()))
    (let ((row (dbi-get_row db)))
      (if row
          (lp (cons row rows))
          (reverse rows)))))

(define (fetch-one db)
  (dbi-get_row db))

(define (col row name)
  (and=> (assoc name row) cdr))

;;; ── setup ────────────────────────────────────────────────────────────────────
(format #t "\n=== PostgreSQL parameterized-query tests ===\n")

(define db (dbi-open "postgresql" *db-conn-str*))
(unless (= 0 (car (dbi-get_status db)))
  (error "Cannot connect" (dbi-get_status db)))

;; Create a scratch table covering all OID branches we care about.
(exec! db "DROP TABLE IF EXISTS _dbi_param_test")
(exec! db "
CREATE TABLE _dbi_param_test (
  id          SERIAL PRIMARY KEY,
  col_bool    BOOLEAN,
  col_int2    SMALLINT,
  col_int4    INTEGER,
  col_int8    BIGINT,
  col_float4  REAL,
  col_float8  DOUBLE PRECISION,
  col_numeric NUMERIC(12,4),
  col_money   MONEY,
  col_text    TEXT,
  col_bytea   BYTEA,
  col_int2arr SMALLINT[],
  col_int4arr INTEGER[],
  col_int8arr BIGINT[],
  col_json    JSON,
  col_jsonb   JSONB,
  col_uuid    UUID
)")

;; Row 1: fully populated
(exec! db "
INSERT INTO _dbi_param_test
  (col_bool, col_int2, col_int4, col_int8,
   col_float4, col_float8, col_numeric, col_money,
   col_text, col_bytea,
   col_int2arr, col_int4arr, col_int8arr,
   col_json, col_jsonb, col_uuid)
VALUES
  (TRUE, 32767, 2147483647, 9223372036854775807,
   3.14, 2.718281828, 12345.6789, 9.99,
   'hello params', '\\xDEADBEEF'::bytea,
   ARRAY[1::smallint, 2::smallint, 3::smallint],
   ARRAY[10, 20, 30],
   ARRAY[100::bigint, 200::bigint],
   '{\"key\":\"val\"}',
   '{\"k\":1}',
   'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11')")

;; Row 2: NULLs for every nullable column
(exec! db "
INSERT INTO _dbi_param_test (col_bool) VALUES (FALSE)")

;; Row 3: empty integer arrays
(exec! db "
INSERT INTO _dbi_param_test
  (col_int2arr, col_int4arr, col_int8arr, col_text)
VALUES
  (ARRAY[]::smallint[], ARRAY[]::integer[], ARRAY[]::bigint[], 'empty arrays')")

;;; ── Test 1: is_params dispatch ───────────────────────────────────────────────
(format #t "-- Test 1: is_params dispatch --\n")

(params-exec! db "SELECT id FROM _dbi_param_test WHERE col_text = $1" "hello params")
(let ((row (fetch-one db)))
  (check-true! "params query returns a row" row)
  (check-true! "id column present" (assoc "id" row)))

;;; ── Test 2: type fidelity — row 1 ───────────────────────────────────────────
(format #t "\n-- Test 2: type fidelity --\n")

(params-exec! db
  "SELECT * FROM _dbi_param_test WHERE col_text = $1 LIMIT 1"
  "hello params")
(let ((row (fetch-one db)))
  (check-true!        "row fetched"              row)
  (check!             "bool TRUE"                (col row "col_bool")    #t)
  (check!             "smallint"                 (col row "col_int2")    32767)
  (check!             "integer"                  (col row "col_int4")    2147483647)
  (check!             "bigint"                   (col row "col_int8")    9223372036854775807)
  (check-true!        "float4 is real"           (real? (col row "col_float4")))
  (check-true!        "float8 is real"           (real? (col row "col_float8")))
  ;; numeric → string in params path (mirrors MySQL NEWDECIMAL)
  (check-true!        "numeric is string"        (string? (col row "col_numeric")))
  ;; money → string
  (check-true!        "money is string"          (string? (col row "col_money")))
  (check!             "text"                     (col row "col_text")    "hello params")
  ;; bytea → string by length
  (check-true!        "bytea is string"          (string? (col row "col_bytea")))
  ;; integer arrays → lists
  (check!             "smallint array"           (col row "col_int2arr") '(1 2 3))
  (check!             "integer array"            (col row "col_int4arr") '(10 20 30))
  (check!             "bigint array"             (col row "col_int8arr") '(100 200))
  ;; json/jsonb/uuid → strings
  (check-true!        "json is string"           (string? (col row "col_json")))
  (check-true!        "jsonb is string"          (string? (col row "col_jsonb")))
  (check-true!        "uuid is string"           (string? (col row "col_uuid")))
  ;; Drain remaining rows
  (fetch-all db))

;;; ── Test 3: NULL handling ────────────────────────────────────────────────────
(format #t "\n-- Test 3: NULL handling --\n")

(params-exec! db
  "SELECT * FROM _dbi_param_test WHERE col_bool = $1 LIMIT 1"
  #f)   ; col_bool = FALSE, all other nullable cols are NULL
(let ((row (fetch-one db)))
  (check-true! "null row fetched"               row)
  (check!      "bool FALSE"                     (col row "col_bool")    #f)
  ;; All other columns are SQL NULL → SCM unspecified
  (check-unspecified! "null smallint"           (col row "col_int2"))
  (check-unspecified! "null integer"            (col row "col_int4"))
  (check-unspecified! "null bigint"             (col row "col_int8"))
  (check-unspecified! "null float4"             (col row "col_float4"))
  (check-unspecified! "null float8"             (col row "col_float8"))
  (check-unspecified! "null numeric"            (col row "col_numeric"))
  (check-unspecified! "null text"               (col row "col_text"))
  (check-unspecified! "null int4arr"            (col row "col_int4arr"))
  (fetch-all db))

;;; ── Test 4: empty integer arrays '{}' ───────────────────────────────────────
(format #t "\n-- Test 4: empty array '{}' --\n")

(params-exec! db
  "SELECT col_int2arr, col_int4arr, col_int8arr
   FROM _dbi_param_test WHERE col_text = $1 LIMIT 1"
  "empty arrays")
(let ((row (fetch-one db)))
  (check-true! "empty-array row fetched"        row)
  (check!      "empty smallint array is '()'"   (col row "col_int2arr") '())
  (check!      "empty integer array is '()'"    (col row "col_int4arr") '())
  (check!      "empty bigint  array is '()'"    (col row "col_int8arr") '())
  (fetch-all db))

;;; ── Test 5: multi-row fetch ──────────────────────────────────────────────────
(format #t "\n-- Test 5: multi-row params fetch --\n")

(params-exec! db
  "SELECT id FROM _dbi_param_test ORDER BY id")
(let ((rows (fetch-all db)))
  (check! "three rows fetched" (length rows) 3))

;;; ── Test 6: PQexec path still works after a params query ────────────────────
;;; (is_params must be reset to 0 so getrow_g_db_handle is used, not getrow_for_params)
(format #t "\n-- Test 6: PQexec path unaffected after params query --\n")

(params-exec! db "SELECT 1 AS x")          ; sets is_params = 1
(fetch-all db)

(exec! db "SELECT 42 AS answer")           ; must reset is_params = 0
(let ((row (fetch-one db)))
  (check-true! "PQexec row fetched after params query" row)
  ;; The PQexec path returns integers via scm_from_int/long_long
  (check-true! "answer is a number" (number? (col row "answer")))
  (fetch-all db))

;;; ── Test 7: integer param types ─────────────────────────────────────────────
(format #t "\n-- Test 7: integer and boolean param types --\n")

(params-exec! db
              "SELECT $1::integer + $2::integer AS total, $3::boolean AS flag"
              42 58 #t)
(let ((row (fetch-one db)))
  (check-true! "integer addition row" row)
  (check!      "42 + 58 = 100"        (col row "total") 100)
  (check!      "boolean param TRUE"   (col row "flag")  #t)
  (fetch-all db))

;;; ── Test 8: error path — wrong param count ───────────────────────────────────
(format #t "\n-- Test 8: error recovery --\n")

;; Expected error
(catch #t
  (lambda ()
    (params-exec! db "SELECT $1::integer AS x" 1 2) ; 2 params, 1 placeholder
    )
  (lambda _
    (let ((st (dbi-get_status db)))
      (check! "wrong param count yields error status" (car st) 1))
    ;; After an error, a fresh query must work (is_params = 0, res = NULL)
    (params-exec! db "SELECT $1::text AS msg" "recovered")
    (let ((row (fetch-one db)))
      (check! "recovery after error" (col row "msg") "recovered")
      (fetch-all db))))

;;; ── teardown ─────────────────────────────────────────────────────────────────
(exec! db "DROP TABLE IF EXISTS _dbi_param_test")
(dbi-close db)

;;; ── summary ──────────────────────────────────────────────────────────────────
(format #t "\n=== Results: ~a passed, ~a failed ===\n" *pass* *fail*)
(when (> *fail* 0)
  (exit 1))
