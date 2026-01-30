#!/usr/bin/env guile
-s
!#
;;; test-guile-dbd-postgresql.scm - Unit tests for guile-dbd-postgresql
;;;
;;; Copyright (C) 2026 Free Software Foundation, Inc.
;;;
;;; This file is part of the guile-dbd-postgresql.
;;;
;;; The guile-dbd-postgresql is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; The guile-dbd-postgresql is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;;;
;;; To run this test, you need a PostgreSQL server running with a test database.
;;; Configure via environment variables:
;;;   DBI_TEST_PGUSER - PostgreSQL username (default: "test")
;;;   DBI_TEST_PGPASS - PostgreSQL password (default: "test")
;;;   DBI_TEST_PGDB   - PostgreSQL database (default: "test")
;;;   DBI_TEST_PGHOST - PostgreSQL host (default: "localhost")
;;;   DBI_TEST_PGPORT - PostgreSQL port (default: "5432")

(use-modules (dbi dbi))

(define (getenv-or var default)
  (or (getenv var) default))

(define pg-user (getenv-or "DBI_TEST_PGUSER" "test"))
(define pg-pass (getenv-or "DBI_TEST_PGPASS" "test"))
(define pg-db   (getenv-or "DBI_TEST_PGDB"   "test"))
(define pg-host (getenv-or "DBI_TEST_PGHOST" "localhost"))
(define pg-port (getenv-or "DBI_TEST_PGPORT" "5432"))

;; Connection string format: user:pass:db:tcp:host:port
(define conn-string
  (string-append pg-user ":" pg-pass ":" pg-db ":tcp:" pg-host ":" pg-port))

(define (check headline result expected)
  (display ";;; ")
  (display headline)
  (if (equal? result expected)
      (begin (display " PASSED") (newline))
      (begin (display " FAILED: ")
             (display (list "expected" expected "got" result))
             (newline))))

(define (check-ok headline db-obj)
  (check headline (car (dbi-get_status db-obj)) 0))

(define (check-status headline db-obj)
  (let ((status (dbi-get_status db-obj)))
    (display ";;; ")
    (display headline)
    (if (zero? (car status))
        (begin (display " PASSED") (newline) #t)
        (begin (display " FAILED: ") (display status) (newline) #f))))

;; Try to connect
(display ";;; Connecting to PostgreSQL: ")
(display conn-string)
(newline)

(define db-obj (dbi-open "postgresql" conn-string))
(define connected (zero? (car (dbi-get_status db-obj))))

(if (not connected)
    (begin
      (display ";;; Connection failed: ")
      (display (dbi-get_status db-obj))
      (newline)
      (display ";;; Skipping PostgreSQL tests (no database available)")
      (newline)
      (exit 0)))

(display ";;; Connected successfully")
(newline)

;; Drop test table if exists (ignore errors)
(dbi-query db-obj "DROP TABLE IF EXISTS dbi_test_table")

(check-status "create table"
  (begin
    (dbi-query db-obj "CREATE TABLE dbi_test_table (id INTEGER, name VARCHAR(50))")
    db-obj))

(check-status "insert row 1"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (33, 'testname1')")
    db-obj))

(check-status "insert row 2"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (34, 'testname1')")
    db-obj))

(check-status "insert row 3"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (44, 'testname2')")
    db-obj))

(check-status "select"
  (begin
    (dbi-query db-obj "SELECT * FROM dbi_test_table WHERE name='testname1' ORDER BY id")
    db-obj))

(define row1 (dbi-get_row db-obj))
(check "get first row" row1 '(("id" . 33) ("name" . "testname1")))

(define row2 (dbi-get_row db-obj))
(check "get second row" row2 '(("id" . 34) ("name" . "testname1")))

(check-status "select non-existing"
  (begin
    (dbi-query db-obj "SELECT * FROM dbi_test_table WHERE name='nonexistent'")
    db-obj))

(check "get non-existing row" (dbi-get_row db-obj) #f)

(check-status "count query"
  (begin
    (dbi-query db-obj "SELECT COUNT(id) as cnt FROM dbi_test_table")
    db-obj))

(define count-row (dbi-get_row db-obj))
(check "get count" (cdr (assoc "cnt" count-row)) 3)

;; Clean up
(dbi-query db-obj "DROP TABLE dbi_test_table")
(dbi-close db-obj)

(display ";;; All PostgreSQL tests completed")
(newline)
