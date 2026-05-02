#!/usr/bin/env guile
-s
!#
;;; test-guile-dbd-sqlite3-params-query.scm - Unit tests for guile-dbd-sqlite3 params API
;;;
;;; Copyright (C) 2026 Free Software Foundation, Inc.
;;;
;;; This file is part of the guile-dbd-sqlite3.
;;;
;;; The guile-dbd-sqlite3 is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; The guile-dbd-sqlite3 is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;;;
;;; To run this test, you need Guile DBI SQLite3 driver installed.
;;; Configure via environment variables:
;;;   DBI_TEST_SQLITE_DB - SQLite database file (default: "test-params.db")

(use-modules (dbi dbi))

;; ---------------- env ----------------

(define (getenv-or var default)
  (or (getenv var) default))

(define sqlite-db
  (getenv-or "DBI_TEST_SQLITE_DB" "test-params.db"))

;; ---------------- helpers ----------------

(define (check headline result expected)
  (display ";;; ")
  (display headline)
  (if (equal? result expected)
      (begin (display " PASSED") (newline))
      (begin (display " FAILED: ")
             (display (list "expected" expected "got" result))
             (newline))))

(define (check-status headline db-obj)
  (let ((status (dbi-get_status db-obj)))
    (display ";;; ")
    (display headline)
    (if (zero? (car status))
        (begin (display " PASSED") (newline) #t)
        (begin (display " FAILED: ") (display status) (newline) #f))))

;; ---------------- connect ----------------

(display ";;; Connecting to SQLite3: ")
(display sqlite-db)
(newline)

(define db-obj (dbi-open "sqlite3" sqlite-db))

(define connected (zero? (car (dbi-get_status db-obj))))

(if (not connected)
    (begin
      (display ";;; Connection failed\n")
      (display (dbi-get_status db-obj))
      (newline)
      (exit 1)))

(display ";;; Connected successfully\n")

;; ---------------- cleanup ----------------

(dbi-query db-obj "DROP TABLE IF EXISTS dbi_params_test")

;; ---------------- create table ----------------

(check-status "create table"
  (begin
    (dbi-query db-obj
      "CREATE TABLE dbi_params_test (
         id INTEGER,
         name TEXT,
         flag INTEGER
       )")
    db-obj))

;; =========================================================
;; INSERT (params-query)
;; =========================================================

(check-status "insert row 1"
  (begin
    (dbi-params-query
      db-obj
      "INSERT INTO dbi_params_test (id, name, flag) VALUES (?, ?, ?)"
      '(1 "alice" 1))
    db-obj))

(check-status "insert row 2"
  (begin
    (dbi-params-query
      db-obj
      "INSERT INTO dbi_params_test (id, name, flag) VALUES (?, ?, ?)"
      '(2 "bob" 0))
    db-obj))

(check-status "insert row 3"
  (begin
    (dbi-params-query
      db-obj
      "INSERT INTO dbi_params_test (id, name, flag) VALUES (?, ?, ?)"
      '(3 "carol" 1))
    db-obj))

;; =========================================================
;; SELECT (params-query)
;; =========================================================

(check-status "select query"
  (begin
    (dbi-params-query
      db-obj
      "SELECT id, name, flag FROM dbi_params_test WHERE flag = ? ORDER BY id"
      '(1))
    db-obj))

;; ---------------- fetch rows ----------------

(define r1 (dbi-get_row db-obj))

(check "first row"
       r1
       '(("id" . 1) ("name" . "alice") ("flag" . 1)))

(define r2 (dbi-get_row db-obj))

(check "second row"
       r2
       '(("id" . 3) ("name" . "carol") ("flag" . 1)))

(check "end of rows"
       (dbi-get_row db-obj)
       #f)

;; =========================================================
;; NULL + type test
;; =========================================================

(check-status "insert boolean false"
              (begin
                (dbi-params-query
                 db-obj
                 "INSERT INTO dbi_params_test (id, name, flag) VALUES (?, ?, ?)"
                 '(4 #f 0))
                db-obj))

(check-status "select boolean false"
              (begin
                (dbi-params-query
                 db-obj
                 "SELECT id, name FROM dbi_params_test WHERE id = ?"
                 '(4))
                db-obj))

(define r3 (dbi-get_row db-obj))

;; NOTE: Boolean false will be inexplicitly converted to 0, for SQLite3 has no
;;       Boolean type. This test ensures that the conversion works as expected.
(let ((v (cdr (assoc "name" r3))))
  (check "boolean false semantic"
         v
         "0"))

;; ---------------- cleanup ----------------

(dbi-query db-obj "DROP TABLE dbi_params_test")
(dbi-close db-obj)

(display ";;; All SQLite3 params tests completed\n")
