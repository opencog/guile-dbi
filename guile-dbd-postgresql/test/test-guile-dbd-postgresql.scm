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

(check-status "create dbi_test_table"
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

;; Test 1: affected_rows after single INSERT
(check-status "insert single row for affected_rows"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (1, 'Alice')")
    db-obj))

(define affected-after-single-insert (dbi-affected-rows db-obj))
(if (not (= affected-after-single-insert 1))
    (begin
      (display ";;; affected_rows after single insert FAILED: ")
      (display "(expected 1 got ")
      (display affected-after-single-insert)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after single insert PASSED")
      (newline)))

;; Test 2: affected_rows after multiple INSERT
(check-status "insert multiple rows for affected_rows"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (2, 'Bob'), (3, 'Carol'), (4, 'Dave')")
    db-obj))

(define affected-after-multi-insert (dbi-affected-rows db-obj))
(if (not (= affected-after-multi-insert 3))
    (begin
      (display ";;; affected_rows after multiple inserts FAILED: ")
      (display "(expected 3 got ")
      (display affected-after-multi-insert)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after multiple inserts PASSED")
      (newline)))

;; Test 3: affected_rows after UPDATE
(check-status "update rows for affected_rows"
  (begin
    (dbi-query db-obj "UPDATE dbi_test_table SET name = 'Updated' WHERE id IN (2, 3)")
    db-obj))

(define affected-after-update (dbi-affected-rows db-obj))
(if (not (= affected-after-update 2))
    (begin
      (display ";;; affected_rows after update FAILED: ")
      (display "(expected 2 got ")
      (display affected-after-update)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after update PASSED")
      (newline)))

;; Test 4: affected_rows when UPDATE matches no rows
(check-status "update no matching rows for affected_rows"
  (begin
    (dbi-query db-obj "UPDATE dbi_test_table SET name = 'None' WHERE id = 999")
    db-obj))

(define affected-after-no-match (dbi-affected-rows db-obj))
(if (not (= affected-after-no-match 0))
    (begin
      (display ";;; affected_rows when update matches nothing FAILED: ")
      (display "(expected 0 got ")
      (display affected-after-no-match)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows when update matches nothing PASSED")
      (newline)))

;; Test 5: affected_rows after DELETE
(check-status "delete row for affected_rows"
  (begin
    (dbi-query db-obj "DELETE FROM dbi_test_table WHERE id = 1")
    db-obj))

(define affected-after-delete (dbi-affected-rows db-obj))
(if (not (= affected-after-delete 1))
    (begin
      (display ";;; affected_rows after delete FAILED: ")
      (display "(expected 1 got ")
      (display affected-after-delete)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after delete PASSED")
      (newline)))

;; Drop table to clean up before next tests
(dbi-query db-obj "DROP TABLE IF EXISTS dbi_test_table")
(dbi-query db-obj "CREATE TABLE dbi_test_table (id INTEGER, name VARCHAR(50))")

;; Test 6: affected_rows after SELECT (should be 0)
(check-status "select for affected_rows"
  (begin
    (dbi-query db-obj "SELECT * FROM dbi_test_table")
    db-obj))

(define affected-after-select (dbi-affected-rows db-obj))
(if (not (= affected-after-select 0))
    (begin
      (display ";;; affected_rows after select FAILED: ")
      (display "(expected 0 got ")
      (display affected-after-select)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after select PASSED")
      (newline)))

;; Test 7: affected_rows updates with each query
(check-status "first insert for sequence test"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (10, 'Test1')")
    db-obj))

(define first-affected (dbi-affected-rows db-obj))

(check-status "second insert for sequence test"
  (begin
    (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (11, 'Test2'), (12, 'Test3')")
    db-obj))

(define second-affected (dbi-affected-rows db-obj))

(if (not (and (= first-affected 1) (= second-affected 2)))
    (begin
      (display ";;; affected_rows updates after each query FAILED: ")
      (display "(expected first=1 second=2, got first=")
      (display first-affected)
      (display " second=")
      (display second-affected)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows updates after each query PASSED")
      (newline)))

;; Cleanup
(dbi-query db-obj "DROP TABLE IF EXISTS dbi_test_table")
(dbi-close db-obj)

(display ";;; All PostgreSQL tests completed")
(newline)
